
/*
 *@@sourcefile desktop.c:
 *      XFldDesktop implementation code. Note that more
 *      Desktop-related code resides in filesys\archives.c
 *      and filesys\shutdown.c.
 *
 *      This file is ALL new with V0.9.0.
 *
 *      Function prefix for this file:
 *      --  dtp*
 *
 *@@added V0.9.0 [umoeller]
 *@@header "filesys\desktop.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M�ller.
 *      This file is part of the XWorkplace source package.
 *      XWorkplace is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published
 *      by the Free Software Foundation, in version 2 as it comes in the
 *      "COPYING" file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#pragma strings(readonly)

/*
 *  Suggested #include order:
 *  1)  os2.h
 *  2)  C library headers
 *  3)  setup.h (code generation and debugging options)
 *  4)  headers in helpers\
 *  5)  at least one SOM implementation header (*.ih)
 *  6)  dlgids.h, headers in shared\ (as needed)
 *  7)  headers in implementation dirs (e.g. filesys\, as needed)
 *  8)  #pragma hdrstop and then more SOM headers which crash with precompiled headers
 */

#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSERRORS
#define INCL_DOSMISC

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINPOINTERS
#define INCL_WINMENUS
#define INCL_WINDIALOGS
#define INCL_WINSTATICS
#define INCL_WINBUTTONS
#define INCL_WINENTRYFIELDS
#define INCL_WINLISTBOXES
#define INCL_WINSTDCNR
#define INCL_WINSTDSPIN
#define INCL_WINSHELLDATA       // Prf* functions
#define INCL_WINSTDFILE
#define INCL_WINSYS

#define INCL_GPIBITMAPS
#include <os2.h>

// C library headers
#include <stdio.h>
#include <io.h>
#include <math.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\animate.h"            // icon and other animations
#include "helpers\comctl.h"             // common controls (window procs)
#include "helpers\dialog.h"             // dialog helpers
#include "helpers\gpih.h"               // GPI helper routines
#include "helpers\nls.h"                // National Language Support helpers
#include "helpers\prfh.h"               // INI file helper routines
#include "helpers\shapewin.h"           // shaped windows helper functions
#include "helpers\standards.h"          // some standard macros
#include "helpers\stringh.h"            // string helper routines
#include "helpers\syssound.h"           // system sound helper routines
#include "helpers\threads.h"            // thread helpers
#include "helpers\winh.h"               // PM helper routines
#include "helpers\wphandle.h"           // file-system object handles

// SOM headers which don't crash with prec. header files
#include "xfdesk.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling

#include "filesys\desktop.h"            // XFldDesktop implementation
#include "filesys\fdrmenus.h"           // shared folder menu logic
#include "filesys\xthreads.h"           // extra XWorkplace threads

#include "startshut\shutdown.h"         // XWorkplace eXtended Shutdown

// other SOM headers
#pragma hdrstop

/* ******************************************************************
 *
 *   Query setup strings
 *
 ********************************************************************/

/*
 *@@ dtpSetup:
 *      implementation of XFldDesktop::wpSetup.
 *
 *      This parses the XSHUTDOWNNOW setup string to
 *      start XShutdown now, if needed.
 *
 *@@added V0.9.7 (2001-01-25) [umoeller]
 *@@changed V0.9.14 (2001-07-28) [umoeller]: added SHOWRUNDLG
 */

BOOL dtpSetup(WPDesktop *somSelf,
              const char *pcszSetupString)
{
    BOOL brc = TRUE;

    CHAR szValue[500];
    ULONG cbValue = sizeof(szValue);
    if (_wpScanSetupString(somSelf,
                           (PSZ)pcszSetupString,
                           "XSHUTDOWNNOW",
                           szValue,
                           &cbValue))
    {
        // XSHUTDOWNNOW setup string present:
        // well, start shutdown now.
        // This is a bit tricky, because we want to support
        // overriding the default shutdown settings for this
        // one time... as a result, we fill a SHUTDOWNPARAMS
        // structure with the current settings and override
        // them when parsing szValue now.

            /* typedef struct _SHUTDOWNPARAMS
            {
                BOOL        optReboot,
                            optConfirm,
                            optDebug;
                ULONG       ulRestartWPS;
                    // changed V0.9.5 (2000-08-10) [umoeller]:
                    // restart Desktop flag, meaning:
                    // -- 0: no, do shutdown
                    // -- 1: yes, restart Desktop
                    // -- 2: yes, restart Desktop, but logoff also
                    //          (only if XWPSHELL is running)
                BOOL        optWPSCloseWindows,
                            optAutoCloseVIO,
                            optLog,
                            optAnimate,
                            optAPMPowerOff,
                            optAPMDelay,
                            optWPSReuseStartupFolder,
                            optEmptyTrashCan,
                            optWarpCenterFirst;
                CHAR        szRebootCommand[CCHMAXPATH];
            } SHUTDOWNPARAMS, *PSHUTDOWNPARAMS; */

        SHUTDOWNPARAMS xsd;
        PSZ pszToken;
        xsdQueryShutdownSettings(&xsd);

        // convert params to upper case
        nlsUpper(szValue, 0);

        pszToken = strtok(szValue, ", ");
        if (pszToken)
            do
            {
                if (!strcmp(pszToken, "HALT"))
                {
                    xsd.ulRestartWPS = 0;           // shutdown
                    xsd.optReboot = FALSE;
                    xsd.optAPMPowerOff = FALSE;
                }
                else if (!strcmp(pszToken, "REBOOT"))
                {
                    xsd.ulRestartWPS = 0;           // shutdown
                    xsd.optReboot = TRUE;
                    xsd.optAPMPowerOff = FALSE;
                }
                else if (!strncmp(pszToken, "USERREBOOT(", 11))
                {
                    PSZ p = strchr(pszToken, ')');
                    if (p)
                    {
                        PSZ pszCmd = strhSubstr(pszToken + 11,
                                                p);
                        if (pszCmd)
                        {
                            strhncpy0(xsd.szRebootCommand,
                                      pszCmd,
                                      sizeof(xsd.szRebootCommand));
                            free(pszCmd);
                        }
                    }
                    xsd.ulRestartWPS = 0;           // shutdown
                    xsd.optReboot = TRUE;
                    xsd.optAPMPowerOff = FALSE;
                }
                else if (!strcmp(pszToken, "POWEROFF"))
                {
                    xsd.ulRestartWPS = 0;           // shutdown
                    xsd.optReboot = FALSE;
                    xsd.optAPMPowerOff = TRUE;
                }
                else if (!strcmp(pszToken, "RESTARTWPS"))
                {
                    xsd.ulRestartWPS = 1;           // restart Desktop
                    xsd.optWPSCloseWindows = FALSE;
                    xsd.optWPSReuseStartupFolder = FALSE;
                }
                else if (!strcmp(pszToken, "FULLRESTARTWPS"))
                {
                    xsd.ulRestartWPS = 1;           // restart Desktop
                    xsd.optWPSCloseWindows = TRUE;
                    xsd.optWPSReuseStartupFolder = TRUE;
                }
                else if (!strcmp(pszToken, "NOAUTOCLOSEVIO"))
                    xsd.optAutoCloseVIO = FALSE;
                else if (!strcmp(pszToken, "AUTOCLOSEVIO"))
                    xsd.optAutoCloseVIO = TRUE;
                else if (!strcmp(pszToken, "NOLOG"))
                    xsd.optLog = FALSE;
                else if (!strcmp(pszToken, "LOG"))
                    xsd.optLog = TRUE;
                /* else if (!strcmp(pszToken, "NOANIMATE"))
                    xsd.optAnimate = FALSE;
                else if (!strcmp(pszToken, "ANIMATE"))
                    xsd.optAnimate = TRUE; */
                else if (!strcmp(pszToken, "NOCONFIRM"))
                    xsd.optConfirm = FALSE;
                else if (!strcmp(pszToken, "CONFIRM"))
                    xsd.optConfirm = TRUE;

            } while (pszToken = strtok(NULL, ", "));

        brc = xsdInitiateShutdownExt(&xsd);
    }

    // V0.9.14 (2001-07-28) [umoeller]
    cbValue = sizeof(szValue);
    if (_wpScanSetupString(somSelf,
                           (PSZ)pcszSetupString,
                           "SHOWRUNDLG",
                           szValue,
                           &cbValue))
    {
        PSZ pszStartup = NULL;
        if (strcmp(szValue, "DEFAULT"))         // boot drive
            pszStartup = szValue;
        brc = (cmnRunCommandLine(NULLHANDLE,           // active desktop
                                 pszStartup)
                    != NULLHANDLE);
    }

    if (_wpScanSetupString(somSelf,
                           (PSZ)pcszSetupString,
                           "TESTFILEDLG",
                           szValue,
                           &cbValue))
    {
        CHAR    szFullFile[CCHMAXPATH] = "";
        strcpy(szFullFile, szValue);
        if (cmnFileDlg(cmnQueryActiveDesktopHWND(),
                       szFullFile,
                       0,
                       NULLHANDLE,
                       NULL,
                       NULL))
            winhDebugBox(NULLHANDLE,
                         "Test file dlg",
                         szFullFile);
    }

    return (brc);
}

/*
 *@@ dtpQuerySetup:
 *      implementation of XFldDesktop::xwpQuerySetup2.
 *      See remarks there.
 *
 *      This returns the length of the XFldDesktop
 *      setup string part only.
 *
 *@@added V0.9.1 (2000-01-20) [umoeller]
 *@@todo warp4 setup strings
 */

BOOL dtpQuerySetup(WPDesktop *somSelf,
                   PVOID pstrSetup)
{
    // PSZ     pszTemp = NULL;
    // ULONG   ulValue = 0;
            // ulDefaultValue = 0;

    /*

      @@todo This is the complete list of all WPDesktop setup
      strings, as documented by WPSREF. However, method
      implementations only exist for Warp 4.

      We'd need to manually decode what all the settings
      in PM_Lockup in OS2.INI are good for.

    */


    // AUTOLOCKUP=YES/NO
    /* if (_wpQueryAutoLockup(somSelf))
        xstrcat(&pszTemp, "AUTOLOCKUP=YES");

    // LOCKUPAUTODIM=YES/NO
    if (_wpQueryLockupAutoDim(somSelf) == FALSE)
        xstrcat(&pszTemp, "LOCKUPAUTODIM=NO");

    // LOCKUPBACKGROUND

    // LOCKUPFULLSCREEN
    if (_wpQueryLockupFullScreen(somSelf) == FALSE)
        xstrcat(&pszTemp, "LOCKUPFULLSCREEN=NO");

    // LOCKUPONSTARTUP
    if (_wpQueryLockupOnStart(somSelf))
        xstrcat(&pszTemp, "LOCKUPONSTARTUP=YES");

    _wpQueryLockupBackground();

    // LOCKUPTIMEOUT
    ulValue = _wpQueryLockupTimeout(somSelf);
    if (ulValue != 3)
    {
        CHAR szTemp[300];
        sprintf(szTemp, "LOCKUPTIMEOUT=%d", ulValue);
        xstrcat(&pszTemp, szTemp);
    } */

    /*
     * append string
     *
     */

    /* if (pszTemp)
    {
        // return string if buffer is given
        if ( (pszSetupString) && (cbSetupString) )
            strhncpy0(pszSetupString,   // target
                      pszTemp,          // source
                      cbSetupString);   // buffer size

        // always return length of string
        ulReturn = strlen(pszTemp);
        free(pszTemp);
    } */

    return (TRUE);
}

/* ******************************************************************
 *
 *   Desktop menus
 *
 ********************************************************************/

/*
 *@@ dtpModifyPopupMenu:
 *      implementation for XFldDesktop::wpModifyPopupMenu.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.3 (2000-04-26) [umoeller]: changed shutdown menu IDs for launchpad
 *@@changed V0.9.7 (2000-12-13) [umoeller]: changed shutdown menu items
 *@@changed V0.9.7 (2000-12-13) [umoeller]: added "logoff network now"
 *@@changed V0.9.9 (2001-03-09) [umoeller]: "shutdown" wasn't always disabled if running
 */

VOID dtpModifyPopupMenu(WPDesktop *somSelf,
                        HWND hwndMenu)
{
    HWND            hwndMenuInsert = hwndMenu;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    PCKERNELGLOBALS  pKernelGlobals = krnQueryGlobals();
    // PNLSSTRINGS     pNLSStrings = cmnQueryNLSStrings();

    // position of original "Shutdown" menu item in context menu
    SHORT   sOrigShutdownPos = (SHORT)WinSendMsg(hwndMenu,
                                                 MM_ITEMPOSITIONFROMID,
                                                 MPFROM2SHORT(WPMENUID_SHUTDOWN, FALSE),
                                                 MPNULL);

    BOOL fShutdownRunning = xsdIsShutdownRunning();
    ULONG ulShutdownAttr = 0;

    if (fShutdownRunning)
        // disable all those menu items if XShutdown is currently running
        ulShutdownAttr = MIA_DISABLED;

#ifndef __NOXSHUTDOWN__
    if (    (cmnIsFeatureEnabled(XShutdown))    // XShutdown enabled?
         // && (!pGlobalSettings->NoWorkerThread)  // Worker thread enabled?
            // removed this setting V0.9.16 (2002-01-04) [umoeller]
       )
    {
        CHAR szShutdown[50];

        if (pGlobalSettings->fDTMShutdownMenu)
        {
            /*
             * create shutdown submenu (V0.9.0):
             *
             */

            // remove original shutdown item
            winhRemoveMenuItem(hwndMenu, WPMENUID_SHUTDOWN);

            strcpy(szShutdown, cmnGetString(ID_XSSI_XSHUTDOWN));
            if (pGlobalSettings->__flXShutdown & XSD_CONFIRM)
                strcat(szShutdown, "...");

            // create "Shutdown" submenu and use this for
            // subsequent menu items
            hwndMenuInsert
                = winhInsertSubmenu(hwndMenu,
                                    // submenu position: after existing "Shutdown" item
                                    sOrigShutdownPos + 1,
                                    pGlobalSettings->VarMenuOffset + ID_XFM_OFS_SHUTDOWNMENU,
                                    cmnGetString(ID_SDSI_SHUTDOWN),  // pszShutdown
                                    MIS_TEXT,
                                    // add "shutdown" menu item with original WPMENUID_SHUTDOWN;
                                    // this is intercepted in dtpMenuItemSelected to initiate
                                    // XShutdown:
                                    WPMENUID_SHUTDOWN,
                                    szShutdown,
                                    MIS_TEXT,
                                    // disable if Shutdown is currently running
                                    ulShutdownAttr);

            if (pGlobalSettings->fDTMShutdown)  // default shutdown menu item enabled?
                // yes: insert "default shutdown" before that
                winhInsertMenuItem(hwndMenuInsert,
                                   0,       // index
                                   pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_OS2_SHUTDOWN,
                                               // WPMENUID_SHUTDOWN,
                                               // changed V0.9.3 (2000-04-26) [umoeller]
                                   cmnGetString(ID_XSSI_DEFAULTSHUTDOWN),  // "Default OS/2 shutdown...", // pszDefaultShutdown
                                   MIS_TEXT,
                                   // disable if Shutdown is currently running
                                   ulShutdownAttr);

            // append "restart Desktop" to the end
            sOrigShutdownPos = MIT_END;
        } // end if (pGlobalSettings->DTMShutdownMenu)
        else
            // no submenu:
            // disable "shutdown" if shutdown is running
            // V0.9.9 (2001-03-07) [umoeller]
            if (fShutdownRunning)
                WinEnableMenuItem(hwndMenu,
                                  WPMENUID_SHUTDOWN,
                                  FALSE);

    } // end if (pGlobalSettings->XShutdown) ...

    if (cmnIsFeatureEnabled(RestartDesktop))
    {
        // insert "Restart Desktop"
        winhInsertMenuItem(hwndMenuInsert,  // either main menu or "Shutdown" submenu
                           sOrigShutdownPos,  // either MIT_END or position of "Shutdown" item
                           pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_RESTARTWPS,
                           cmnGetString(ID_SDSI_RESTARTWPS),  // pszRestartWPS
                           MIS_TEXT,
                           // disable if Shutdown is currently running
                           ulShutdownAttr);

        if ((pGlobalSettings->__flXShutdown & XSD_CONFIRM) == 0)
            // if XShutdown confirmations have been disabled,
            // remove "..." from "Restart Desktop" entry
            winhMenuRemoveEllipse(hwndMenuInsert,
                                  pGlobalSettings->VarMenuOffset
                                        + ID_XFMI_OFS_RESTARTWPS);
    }
#endif

    if (pKernelGlobals->pXWPShellShared)
    {
        // XWPShell running:
        // insert "logoff"
        winhInsertMenuItem(hwndMenuInsert,  // either main menu or "Shutdown" submenu
                           sOrigShutdownPos,  // either MIT_END or position of "Shutdown" item
                           pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_LOGOFF,
                           cmnGetString(ID_XSSI_XSD_LOGOFF),  // pszXSDLogoff
                           MIS_TEXT,
                           // disable if Shutdown is currently running
                           ulShutdownAttr);

        if ((pGlobalSettings->__flXShutdown & ID_XFMI_OFS_LOGOFF) == 0)
            // if XShutdown confirmations have been disabled,
            // remove "..." from "Logoff" entry
            winhMenuRemoveEllipse(hwndMenuInsert,
                                  pGlobalSettings->VarMenuOffset
                                        + ID_XFMI_OFS_RESTARTWPS);
    }

    // remove other default menu items?
    #ifndef WPMENUID_LOGOFF
        #define WPMENUID_LOGOFF 738  // 0x2E2
    #endif

    if (!pGlobalSettings->fDTMLockup)
        winhRemoveMenuItem(hwndMenu, WPMENUID_LOCKUP);
    if (!pGlobalSettings->fDTMSystemSetup)
        winhRemoveMenuItem(hwndMenu, WPMENUID_SYSTEMSETUP);
    if (!pGlobalSettings->fDTMLogoffNetwork)
        winhRemoveMenuItem(hwndMenu, WPMENUID_LOGOFF);

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        // if XWorkplace is compiled with
        // VAC++ debug memory funcs,
        // add a menu item for listing all memory objects
        winhInsertMenuSeparator(hwndMenu,
                                MIT_END,
                                (pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_SEPARATOR));
        winhInsertMenuItem(hwndMenu,
                           MIT_END,
                           DEBUG_MENUID_LISTHEAP,
                           "List VAC++ debug heap",
                           MIS_TEXT, 0);
        winhInsertMenuItem(hwndMenu,
                           MIT_END,
                           DEBUG_MENUID_RELEASEFREED,
                           "Discard logs for freed memory",
                           MIS_TEXT, 0);
    #endif

    #ifdef __DEBUG__
        // if we have a debug compile,
        // add "crash" items
        winhInsertMenuSeparator(hwndMenu,
                                MIT_END,
                                (pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_SEPARATOR));
        hwndMenuInsert = winhInsertSubmenu(hwndMenu,
                                           MIT_END,
                                           DEBUG_MENUID_CRASH_MENU,
                                           "Crash WPS",
                                           MIS_TEXT,
                                           // first item ID in "Shutdown" menu:
                                           // crash thread 1
                                           DEBUG_MENUID_CRASH_THR1,
                                           "Thread 1",
                                           MIS_TEXT, 0);
        winhInsertMenuItem(hwndMenuInsert,
                           MIT_END,
                           DEBUG_MENUID_CRASH_WORKER,
                           "Worker thread",
                           MIS_TEXT, 0);
        winhInsertMenuItem(hwndMenuInsert,
                           MIT_END,
                           DEBUG_MENUID_CRASH_QUICK,
                           "Speedy thread",
                           MIS_TEXT, 0);
        winhInsertMenuItem(hwndMenuInsert,
                           MIT_END,
                           DEBUG_MENUID_CRASH_FILE,
                           "File thread",
                           MIS_TEXT, 0);

        // add "Dump window list"
        winhInsertMenuItem(hwndMenu,
                           MIT_END,
                           DEBUG_MENUID_DUMPWINLIST,
                           "Dump window list",
                           MIS_TEXT, 0);
    #endif

    // krnUnlockGlobals();
}

/*
 *@@ dtpMenuItemSelected:
 *      implementation for XFldDesktop::wpMenuItemSelected.
 *
 *      This returns TRUE if one of the new items was processed
 *      or FALSE if the parent method should be called. We may
 *      change *pulMenuId and return FALSE.
 *
 *@@added V0.9.1 (99-12-04) [umoeller]
 *@@changed V0.9.3 (2000-04-26) [umoeller]: changed shutdown menu item IDs; changed prototype
 *@@changed V0.9.5 (2000-08-10) [umoeller]: added logoff support
 */

BOOL dtpMenuItemSelected(XFldDesktop *somSelf,
                         HWND hwndFrame,
                         PULONG pulMenuId) // in/out: menu item ID (can be changed)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // PCKERNELGLOBALS   pKernelGlobals = krnQueryGlobals();

    if (!xsdIsShutdownRunning())
    {
        ULONG ulMenuId2 = (*pulMenuId - (pGlobalSettings->VarMenuOffset));

        if (ulMenuId2 == ID_XFMI_OFS_RESTARTWPS)
        {
            xsdInitiateRestartWPS(FALSE);   // restart Desktop, no logoff
            return (TRUE);
        }
        else if (ulMenuId2 == ID_XFMI_OFS_LOGOFF)
        {
            xsdInitiateRestartWPS(TRUE);    // logoff
            return (TRUE);
        }
#ifndef __NOXSHUTDOWN__
        else if (    (cmnIsFeatureEnabled(XShutdown))
                 // &&  (pGlobalSettings->NoWorkerThread == 0)
                    // removed this setting V0.9.16 (2002-01-04) [umoeller]
                )
        {
            // shutdown enabled:
            if (*pulMenuId == WPMENUID_SHUTDOWN)
            {
                xsdInitiateShutdown();
                return (TRUE);
            }
            else if (ulMenuId2 == ID_XFMI_OFS_OS2_SHUTDOWN)
            {
                // default OS/2 shutdown (in submenu):
                // have parent method called with default shutdown menu item ID
                // to start OS/2 shutdown...
                *pulMenuId = WPMENUID_SHUTDOWN;
                return (FALSE);
            }
        }
#endif
    }

#ifdef __XWPLITE__
    if (*pulMenuId == 0x25D)          // product info
    {
        cmnShowProductInfo(NULLHANDLE,      // owner
                           MMSOUND_SYSTEMSTARTUP);
        return (TRUE);
    }
#endif

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        // if XWorkplace is compiled with
        // VAC++ debug memory funcs,
        // check the menu item for listing all memory objects
        if (*pulMenuId == DEBUG_MENUID_LISTHEAP)
        {
            memdCreateMemDebugWindow();
            return (TRUE);
        }
        else if (*pulMenuId == DEBUG_MENUID_RELEASEFREED)
        {
            HPOINTER hptrOld = winhSetWaitPointer();
            memdReleaseFreed();
            WinSetPointer(HWND_DESKTOP, hptrOld);
            return (TRUE);
        }
    #endif

    #ifdef __DEBUG__
        switch (*pulMenuId)
        {
            case DEBUG_MENUID_CRASH_THR1:
                krnPostThread1ObjectMsg(XM_CRASH, 0, 0);
            break;
            case DEBUG_MENUID_CRASH_WORKER:
                xthrPostWorkerMsg(XM_CRASH, 0, 0);
            break;
            case DEBUG_MENUID_CRASH_QUICK:
                xthrPostSpeedyMsg(XM_CRASH, 0, 0);
            break;
            case DEBUG_MENUID_CRASH_FILE:
                xthrPostFileMsg(XM_CRASH, 0, 0);
            break;

            case DEBUG_MENUID_DUMPWINLIST:
                winlCreateWinListWindow();
            break;
        }
    #endif

    return (FALSE);
}

/* ******************************************************************
 *
 *   XFldDesktop notebook settings pages callbacks (notebook.c)
 *
 ********************************************************************/

/*
 * fncbWPSDesktop1InitPage:
 *      notebook callback function (notebook.c) for the
 *      "Menu items" page in the Desktop's settings
 *      notebook.
 *      Sets the controls on the page according to the
 *      Global Settings.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.7 (2000-12-13) [umoeller]: changed shutdown menu items
 *@@changed V0.9.7 (2000-12-13) [umoeller]: added "logoff network now"
 */

VOID dtpMenuItemsInitPage(PCREATENOTEBOOKPAGE pcnbp,   // notebook info struct
                          ULONG flFlags)        // CBI_* flags (notebook.h)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

    if (flFlags & CBI_INIT)
    {
        if (pcnbp->pUser == NULL)
        {
            // first call: backup Global Settings for "Undo" button;
            // this memory will be freed automatically by the
            // common notebook window function (notebook.c) when
            // the notebook page is destroyed
            pcnbp->pUser = malloc(sizeof(GLOBALSETTINGS));
            memcpy(pcnbp->pUser, pGlobalSettings, sizeof(GLOBALSETTINGS));
        }
    }

    if (flFlags & CBI_SET)
    {
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_SORT,
                              pGlobalSettings->fDTMSort);
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_ARRANGE,
                              pGlobalSettings->fDTMArrange);
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_SYSTEMSETUP,
                              pGlobalSettings->fDTMSystemSetup);
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_LOCKUP  ,
                              pGlobalSettings->fDTMLockup);
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_LOGOFFNETWORKNOW,
                              pGlobalSettings->fDTMLogoffNetwork); // V0.9.7 (2000-12-13) [umoeller]

        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_SHUTDOWN,
                              pGlobalSettings->fDTMShutdown);
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_SHUTDOWNMENU,
                              pGlobalSettings->fDTMShutdownMenu);
    }

    if (flFlags & CBI_ENABLE)
    {
#ifndef __NOXSHUTDOWN__
        winhEnableDlgItem(pcnbp->hwndDlgPage, ID_XSDI_DTP_SHUTDOWNMENU,
                         (     (cmnIsFeatureEnabled(XShutdown))
                           // &&  (pGlobalSettings->fDTMShutdown)
                           // &&  (!pGlobalSettings->NoWorkerThread)
                            // removed this setting V0.9.16 (2002-01-04) [umoeller]
                         ));
#endif
    }
}

/*
 * dtpMenuItemsItemChanged:
 *      notebook callback function (notebook.c) for the
 *      "Menu items" page in the Desktop's settings
 *      notebook.
 *      Reacts to changes of any of the dialog controls.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.7 (2000-12-13) [umoeller]: changed shutdown menu items
 *@@changed V0.9.7 (2000-12-13) [umoeller]: added "logoff network now"
 *@@changed V0.9.9 (2001-04-07) [pr]: fixed Undo
 */

MRESULT dtpMenuItemsItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                ULONG ulItemID,
                                USHORT usNotifyCode,
                                ULONG ulExtra)      // for checkboxes: contains new state
{
    GLOBALSETTINGS *pGlobalSettings = cmnLockGlobalSettings(__FILE__, __LINE__, __FUNCTION__);
    ULONG ulChange = 1;

    // LONG lTemp;

    switch (ulItemID)
    {
        case ID_XSDI_DTP_SORT:
            pGlobalSettings->fDTMSort = ulExtra;
        break;

        case ID_XSDI_DTP_ARRANGE:
            pGlobalSettings->fDTMArrange = ulExtra;
        break;

        case ID_XSDI_DTP_SYSTEMSETUP:
            pGlobalSettings->fDTMSystemSetup = ulExtra;
        break;

        case ID_XSDI_DTP_LOCKUP:
            pGlobalSettings->fDTMLockup = ulExtra;
        break;

        case ID_XSDI_DTP_LOGOFFNETWORKNOW: // V0.9.7 (2000-12-13) [umoeller]
            pGlobalSettings->fDTMLogoffNetwork = ulExtra;
        break;

        case ID_XSDI_DTP_SHUTDOWN:
            pGlobalSettings->fDTMShutdown = ulExtra;
            // dtpMenuItemsInitPage(pcnbp, CBI_ENABLE);
        break;

        case ID_XSDI_DTP_SHUTDOWNMENU:
            pGlobalSettings->fDTMShutdownMenu = ulExtra;
        break;

        case DID_UNDO:
        {
            // "Undo" button: get pointer to backed-up Global Settings
            GLOBALSETTINGS *pGSBackup = (GLOBALSETTINGS*)(pcnbp->pUser);

            // and restore the settings for this page
            pGlobalSettings->fDTMSort = pGSBackup->fDTMSort;  // V0.9.9
            pGlobalSettings->fDTMArrange = pGSBackup->fDTMArrange;  // V0.9.9
            pGlobalSettings->fDTMSystemSetup = pGSBackup->fDTMSystemSetup;
            pGlobalSettings->fDTMLockup = pGSBackup->fDTMLockup;
            pGlobalSettings->fDTMLogoffNetwork = pGSBackup->fDTMLogoffNetwork;  // V0.9.9
            pGlobalSettings->fDTMShutdown = pGSBackup->fDTMShutdown;
            pGlobalSettings->fDTMShutdownMenu = pGSBackup->fDTMShutdownMenu;

            // update the display by calling the INIT callback
            pcnbp->pfncbInitPage(pcnbp, CBI_SET | CBI_ENABLE);
        break; }

        case DID_DEFAULT:
        {
            // set the default settings for this settings page
            // (this is in common.c because it's also used at
            // Desktop startup)
            cmnSetDefaultSettings(pcnbp->ulPageID);
            // update the display by calling the INIT callback
            pcnbp->pfncbInitPage(pcnbp, CBI_SET | CBI_ENABLE);
        break; }

        default:
            ulChange = 0;
    }

    cmnUnlockGlobalSettings();

    if (ulChange)
        // enable/disable items
        cmnStoreGlobalSettings();

    return ((MPARAM)0);
}

CONTROLDEF
#ifndef __NOBOOTLOGO__
    BootLogoGroup = CONTROLDEF_GROUP(
                            LOAD_STRING, // "Workplace Shell boot logo",
                            ID_XSDI_DTP_LOGOGROUP),
    BootLogoCB = CONTROLDEF_AUTOCHECKBOX(
                            LOAD_STRING, // "Enable ~boot logo"
                            ID_XSDI_DTP_BOOTLOGO,
                            -1,
                            -1),
    LogoStyleGroup = CONTROLDEF_GROUP(
                            LOAD_STRING, // "Boot logo style"
                            ID_XSDI_DTP_LOGOSTYLEGROUP),
    LogoTransparentRadio = CONTROLDEF_FIRST_AUTORADIO(
                            LOAD_STRING, // "~Transparent style"
                            ID_XSDI_DTP_LOGO_TRANSPARENT,
                            -1,
                            -1),
    LogoBlowUpRadio = CONTROLDEF_NEXT_AUTORADIO(
                            LOAD_STRING, // "B~low-up style"
                            ID_XSDI_DTP_LOGO_BLOWUP,
                            -1,
                            -1),
    LogoFrameGroup = CONTROLDEF_GROUP(
                            "",
                            ID_XSDI_DTP_LOGOFRAME),
    LogoBitmap =
        {
            WC_STATIC,
            "",
            SS_FGNDFRAME | WS_VISIBLE,
            ID_XSDI_DTP_LOGOBITMAP,
            CTL_COMMON_FONT,
            0,
            {133, 100},
            COMMON_SPACING
        },
    LogoFileText = CONTROLDEF_TEXT(
                            LOAD_STRING, // "OS/2 1.3 BMP ~file for boot logo:"
                            ID_XSDI_DTP_LOGOFILETXT,
                            -1,
                            -1),
    LogoFileEF = CONTROLDEF_ENTRYFIELD(
                            NULL,
                            ID_XSDI_DTP_LOGOFILE,
                            200,
                            -1),
    LogoFileBrowseButton = CONTROLDEF_PUSHBUTTON(
                            LOAD_STRING, // "Bro~wse..."
                            DID_BROWSE,
                            -1,
                            30),
    LogoFileTestButton = CONTROLDEF_PUSHBUTTON(
                            LOAD_STRING, // "T~est logo",
                            ID_XSDI_DTP_TESTLOGO,
                            -1,
                            30),
#endif
    WriteXWPStartLogCB = CONTROLDEF_AUTOCHECKBOX(
                            LOAD_STRING, // "Write XWP~START.LOG file",
                            ID_XSDI_DTP_WRITEXWPSTARTLOG,
                            -1,
                            -1),
#ifndef __NOBOOTUPSTATUS__
    BootupStatusCB = CONTROLDEF_AUTOCHECKBOX(
                            LOAD_STRING, // "Show W~PS class initialization"
                            ID_XSDI_DTP_BOOTUPSTATUS,
                            -1,
                            -1),
#endif
    CreateStartupFolderButton = CONTROLDEF_PUSHBUTTON(
                            LOAD_STRING, // "Create ~XWorkplace Startup folder",
                            ID_XSDI_DTP_CREATESTARTUPFLDR,
                            -1,
                            30),
    NumLockOnCB = CONTROLDEF_AUTOCHECKBOX(
                            LOAD_STRING, // "Enable ~NumLock at startup",
                            ID_XSDI_DTP_NUMLOCKON,
                            -1,
                            -1);

DLGHITEM dlgDesktopStartup[] =
    {
        START_TABLE,            // root table, required
#ifndef __NOBOOTLOGO__
            START_ROW(0),       // boot logo group
                START_GROUP_TABLE(&BootLogoGroup),
                    START_ROW(0),
                        CONTROL_DEF(&BootLogoCB),
                    START_ROW(ROW_VALIGN_CENTER),
                        CONTROL_DEF(&LogoFileText),
                    START_ROW(ROW_VALIGN_CENTER),
                        CONTROL_DEF(&LogoFileEF),
                    // START_ROW(0),
                        CONTROL_DEF(&LogoFileBrowseButton),
                    START_ROW(0),
                        START_GROUP_TABLE(&LogoFrameGroup),
                            START_ROW(0),
                                CONTROL_DEF(&LogoBitmap),
                        END_TABLE,      // logo frame group
                        START_TABLE,
                            START_ROW(0),
                                START_GROUP_TABLE(&LogoStyleGroup),
                                    START_ROW(0),
                                        CONTROL_DEF(&LogoTransparentRadio),
                                    START_ROW(0),
                                        CONTROL_DEF(&LogoBlowUpRadio),
                                END_TABLE,      // logo style group
                            START_ROW(0),
                                CONTROL_DEF(&LogoFileTestButton),
                        END_TABLE,
                END_TABLE,      // end of boot logo group
#endif
            START_ROW(0),
                CONTROL_DEF(&WriteXWPStartLogCB),
#ifndef __NOBOOTUPSTATUS__
            START_ROW(0),
                CONTROL_DEF(&BootupStatusCB),
#endif
            START_ROW(0),
                CONTROL_DEF(&NumLockOnCB),
            START_ROW(0),
                CONTROL_DEF(&CreateStartupFolderButton),
            START_ROW(0),       // notebook buttons (will be moved)
                CONTROL_DEF(&G_UndoButton),         // notebook.c
                CONTROL_DEF(&G_DefaultButton),      // notebook.c
                CONTROL_DEF(&G_HelpButton),         // notebook.c
        END_TABLE
    };

/*
 * dtpStartupInitPage:
 *      notebook callback function (notebook.c) for the
 *      "Startup" page in the Desktop's settings
 *      notebook.
 *      Sets the controls on the page according to the
 *      Global Settings.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.1 (2000-02-09) [umoeller]: added NumLock support to this page
 *@@changed V0.9.13 (2001-06-14) [umoeller]: fixed Undo for boot logo file
 *@@changed V0.9.14 (2001-08-21) [umoeller]: added "write startuplog" setting
 *@@changecd V0.9.16 (2001-09-29) [umoeller]: now using dialog formatter
 */

VOID dtpStartupInitPage(PCREATENOTEBOOKPAGE pcnbp,   // notebook info struct
                        ULONG flFlags)        // CBI_* flags (notebook.h)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

    if (flFlags & CBI_INIT)
    {
        if (pcnbp->pUser == NULL)
        {
            // first call: backup Global Settings for "Undo" button;
            // this memory will be freed automatically by the
            // common notebook window function (notebook.c) when
            // the notebook page is destroyed
            pcnbp->pUser = malloc(sizeof(GLOBALSETTINGS));
            memcpy(pcnbp->pUser, pGlobalSettings, sizeof(GLOBALSETTINGS));

            // insert the controls using the dialog formatter
            // V0.9.16 (2001-09-29) [umoeller]
            ntbFormatPage(pcnbp->hwndDlgPage,
                          dlgDesktopStartup,
                          ARRAYITEMCOUNT(dlgDesktopStartup));

#ifndef __NOBOOTLOGO__
            // backup old boot logo file
            pcnbp->pUser2 = cmnQueryBootLogoFile();     // malloc'ed
                    // fixed V0.9.13 (2001-06-14) [umoeller]

            // prepare the control to properly display
            // stretched bitmaps
            ctlPrepareStretchedBitmap(WinWindowFromID(pcnbp->hwndDlgPage,
                                                      ID_XSDI_DTP_LOGOBITMAP),
                                      TRUE);    // preserve proportions

            // set entry field limit
            winhSetEntryFieldLimit(WinWindowFromID(pcnbp->hwndDlgPage,
                                                   ID_XSDI_DTP_LOGOFILE),
                                   CCHMAXPATH);
#endif
        }
    }

    if (flFlags & CBI_SET)
    {
#ifndef __NOBOOTLOGO__
        USHORT      usRadioID;
        ULONG       ulError;
        HDC         hdcMem;
        HPS         hpsMem;
        HBITMAP     hbmBootLogo;

        HPOINTER hptrOld = winhSetWaitPointer();

        SIZEL       szlPage = {0, 0};
        PSZ         pszBootLogoFile = cmnQueryBootLogoFile();

        // "boot logo enabled"
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_BOOTLOGO,
                              cmnIsFeatureEnabled(BootLogo));

        // "boot logo style"
        if (pGlobalSettings->_bBootLogoStyle == 0)
            usRadioID = ID_XSDI_DTP_LOGO_TRANSPARENT;
        else
            usRadioID = ID_XSDI_DTP_LOGO_BLOWUP;
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, usRadioID,
                              BM_CHECKED);

        // set boot logo file entry field
        WinSetDlgItemText(pcnbp->hwndDlgPage,
                          ID_XSDI_DTP_LOGOFILE,
                          pszBootLogoFile);

        // attempt to display the boot logo
        if (gpihCreateMemPS(WinQueryAnchorBlock(pcnbp->hwndDlgPage),
                            &szlPage,
                            &hdcMem,
                            &hpsMem))
        {
            if (hbmBootLogo = gpihLoadBitmapFile(hpsMem,
                                                 pszBootLogoFile,
                                                 &ulError))
            {
                // and have the subclassed static control display the thing
                WinSendDlgItemMsg(pcnbp->hwndDlgPage, ID_XSDI_DTP_LOGOBITMAP,
                                  SM_SETHANDLE,
                                  (MPARAM)(hbmBootLogo),
                                  MPNULL);

                // delete the bitmap again
                // (the static control has made a private copy
                // of the bitmap, so this is safe)
                GpiDeleteBitmap(hbmBootLogo);
            }
            GpiDestroyPS(hpsMem);
            DevCloseDC(hdcMem);
        }
        free(pszBootLogoFile);

        WinSetPointer(HWND_DESKTOP, hptrOld);
#endif
        // startup log file
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_WRITEXWPSTARTLOG,
                              pGlobalSettings->fWriteXWPStartupLog);

#ifndef __NOBOOTUPSTATUS__
        // bootup status
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_BOOTUPSTATUS,
                              pGlobalSettings->_fShowBootupStatus);
#endif

        // numlock on
        winhSetDlgItemChecked(pcnbp->hwndDlgPage, ID_XSDI_DTP_NUMLOCKON,
                              pGlobalSettings->fNumLockStartup);
    }

    if (flFlags & CBI_ENABLE)
    {
#ifndef __NOBOOTLOGO__
        PSZ     pszBootLogoFile = cmnQueryBootLogoFile();
        BOOL    fBootLogoFileExists = (access(pszBootLogoFile, 0) == 0);
        free(pszBootLogoFile);

        winhEnableDlgItem(pcnbp->hwndDlgPage, ID_XSDI_DTP_LOGOBITMAP,
                         cmnIsFeatureEnabled(BootLogo));
        winhEnableDlgItem(pcnbp->hwndDlgPage, ID_XSDI_DTP_TESTLOGO, fBootLogoFileExists);
#endif

        if (WinQueryObject((PSZ)XFOLDER_STARTUPID))
            winhEnableDlgItem(pcnbp->hwndDlgPage, ID_XSDI_DTP_CREATESTARTUPFLDR, FALSE);
    }
}

#ifndef __NOBOOTLOGO__

/*
 *@@ SetBootLogoFile:
 *      changes the boot logo file. Shared between the
 *      entry field handler and "Undo".
 *
 *@@added V0.9.13 (2001-06-14) [umoeller]
 */

VOID SetBootLogoFile(PCREATENOTEBOOKPAGE pcnbp,
                     const char *pcszNewBootLogoFile,
                     BOOL fWrite)                   // in: if TRUE, write back to OS2.INI
{
    winhEnableDlgItem(pcnbp->hwndDlgPage, ID_XSDI_DTP_TESTLOGO,
                     (access(pcszNewBootLogoFile, 0) == 0));

    if (fWrite)
    {
        // query new file name from entry field
        PrfWriteProfileString(HINI_USER,
                              (PSZ)INIAPP_XWORKPLACE,
                              (PSZ)INIKEY_BOOTLOGOFILE,
                              (PSZ)pcszNewBootLogoFile);
        // update the display by calling the INIT callback
        pcnbp->pfncbInitPage(pcnbp, CBI_SET | CBI_ENABLE);
    }
}

#endif

/*
 * dtpStartupItemChanged:
 *      notebook callback function (notebook.c) for the
 *      "Startup" page in the Desktop's settings
 *      notebook.
 *      Reacts to changes of any of the dialog controls.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.1 (2000-02-09) [umoeller]: added NumLock support to this page
 *@@changed V0.9.3 (2000-04-11) [umoeller]: fixed major resource leak; the bootlogo bitmap was never freed
 *@@changed V0.9.9 (2001-04-07) [pr]: fixed Undo
 *@@changed V0.9.13 (2001-06-14) [umoeller]: fixed Undo for boot logo file
 *@@changed V0.9.14 (2001-08-21) [umoeller]: added "write startuplog" setting
 */

MRESULT dtpStartupItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                              ULONG ulItemID,
                              USHORT usNotifyCode,
                              ULONG ulExtra)      // for checkboxes: contains new state
{
    BOOL fProcessed = TRUE;

    ULONG ulChange = 1;

    {
        GLOBALSETTINGS *pGlobalSettings = cmnLockGlobalSettings(__FILE__, __LINE__, __FUNCTION__);

        switch (ulItemID)
        {
#ifndef __NOBOOTLOGO__
            case ID_XSDI_DTP_BOOTLOGO:
                pGlobalSettings->__fBootLogo = ulExtra;
                ulChange = 2;       // re-enable items
            break;

            case ID_XSDI_DTP_LOGO_TRANSPARENT:
                pGlobalSettings->_bBootLogoStyle = 0;
            break;

            case ID_XSDI_DTP_LOGO_BLOWUP:
                pGlobalSettings->_bBootLogoStyle = 1;
            break;
#endif

            case ID_XSDI_DTP_WRITEXWPSTARTLOG:
                pGlobalSettings->fWriteXWPStartupLog = ulExtra;
            break;

#ifndef __NOBOOTUPSTATUS__
            case ID_XSDI_DTP_BOOTUPSTATUS:
                pGlobalSettings->_fShowBootupStatus = ulExtra;
            break;
#endif

            case ID_XSDI_DTP_NUMLOCKON:
                pGlobalSettings->fNumLockStartup = ulExtra;
                winhSetNumLock(ulExtra);
            break;

            case DID_UNDO:
            {
                // "Undo" button: get pointer to backed-up Global Settings
                GLOBALSETTINGS *pGSBackup = (GLOBALSETTINGS*)(pcnbp->pUser);

                // and restore the settings for this page
                pGlobalSettings->fWriteXWPStartupLog = pGSBackup->fWriteXWPStartupLog;
#ifndef __NOBOOTUPSTATUS__
                pGlobalSettings->_fShowBootupStatus = pGSBackup->_fShowBootupStatus;
#endif
#ifndef __NOBOOTLOGO__
                pGlobalSettings->__fBootLogo = pGSBackup->__fBootLogo;
                pGlobalSettings->_bBootLogoStyle = pGSBackup->_bBootLogoStyle;
#endif
                pGlobalSettings->fNumLockStartup = pGSBackup->fNumLockStartup;  // V0.9.9

#ifndef __NOBOOTLOGO__
                SetBootLogoFile(pcnbp,
                                (const char *)pcnbp->pUser2,
                                TRUE);      // write
#endif

                // update the display by calling the INIT callback
                pcnbp->pfncbInitPage(pcnbp, CBI_SET | CBI_ENABLE);
            break; }

            case DID_DEFAULT:
            {
                // set the default settings for this settings page
                // (this is in common.c because it's also used at
                // Desktop startup)
                cmnSetDefaultSettings(pcnbp->ulPageID);
                // update the display by calling the INIT callback
                pcnbp->pfncbInitPage(pcnbp, CBI_SET | CBI_ENABLE);
            break; }

            default:
                ulChange = 0;
                fProcessed = FALSE;
        }

        cmnUnlockGlobalSettings();
    }

    if (ulChange)
    {
        cmnStoreGlobalSettings();
        if (ulChange == 2)
            // enable/disable items
            pcnbp->pfncbInitPage(pcnbp, CBI_ENABLE);
    }

    if (!fProcessed)
    {
        // not processed above:
        // second switch-case with non-global settings stuff
        PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

        switch (ulItemID)
        {
#ifndef __NOBOOTLOGO__
            /*
             * ID_XSDI_DTP_LOGOFILE:
             *      focus leaves "file" entry field:
             *      update OS2.INI
             */

            case ID_XSDI_DTP_LOGOFILE:
            {
                PSZ pszNewBootLogoFile = winhQueryWindowText(pcnbp->hwndControl);
                SetBootLogoFile(pcnbp,
                                pszNewBootLogoFile,
                                (usNotifyCode == EN_KILLFOCUS)); // write?
                if (pszNewBootLogoFile)
                    free(pszNewBootLogoFile);
            break; }

            /*
             * DID_BROWSE:
             *      "Browse" button: open file dialog
             */

            case DID_BROWSE:
            {
                // FILEDLG fd;
                CHAR szFile[CCHMAXPATH] = "*.BMP";
                PSZ pszNewBootLogoFile = winhQueryWindowText(WinWindowFromID(pcnbp->hwndDlgPage,
                                                                             ID_XSDI_DTP_LOGOFILE));

                /* memset(&fd, 0, sizeof(FILEDLG));
                fd.cbSize = sizeof(FILEDLG);
                fd.fl = FDS_OPEN_DIALOG
                          | FDS_CENTER;
                */

                if (pszNewBootLogoFile)
                {
                    // get last directory used
                    PSZ p = strrchr(pszNewBootLogoFile, '\\');
                    if (p)
                    {
                        // contains directory:
                        PSZ pszDir = strhSubstr(pszNewBootLogoFile, p + 1);
                        strcpy(szFile, pszDir);
                        free(pszDir);
                    }
                    free(pszNewBootLogoFile);
                }
                strcat(szFile, "*.bmp");

                /* if (    WinFileDlg(HWND_DESKTOP,    // parent
                                   pcnbp->hwndFrame, // owner
                                   &fd)
                    && (fd.lReturn == DID_OK)
                   ) */
                if (cmnFileDlg(pcnbp->hwndFrame,
                               szFile,
                               0, // WINH_FOD_INILOADDIR | WINH_FOD_INISAVEDIR,
                               0,
                               0,
                               0))
                {
                    // copy file from FOD to page
                    WinSetDlgItemText(pcnbp->hwndDlgPage,
                                      ID_XSDI_DTP_LOGOFILE,
                                      szFile);
                    PrfWriteProfileString(HINI_USER,
                                          (PSZ)INIAPP_XWORKPLACE,
                                          (PSZ)INIKEY_BOOTLOGOFILE,
                                          szFile);
                    // update the display by calling the INIT callback
                    pcnbp->pfncbInitPage(pcnbp, CBI_SET | CBI_ENABLE);
                }
            break; }

            /*
             * ID_XSDI_DTP_TESTLOGO:
             *
             */

            case ID_XSDI_DTP_TESTLOGO:
            {
                HDC         hdcMem;
                HPS         hpsMem;
                HBITMAP     hbmBootLogo;
                ULONG       ulError;
                SIZEL       szlPage = {0, 0};
                HPOINTER    hptrOld = winhSetWaitPointer();

                PSZ         pszBootLogoFile = cmnQueryBootLogoFile();

                // attempt to load the boot logo
                if (gpihCreateMemPS(WinQueryAnchorBlock(pcnbp->hwndDlgPage),
                                    &szlPage,
                                    &hdcMem,
                                    &hpsMem))
                {
                    if (hbmBootLogo = gpihLoadBitmapFile(hpsMem,
                                                         pszBootLogoFile,
                                                         &ulError))
                    {
                        if (pGlobalSettings->_bBootLogoStyle == 1)
                        {
                            // blow-up mode:
                            HPS     hpsScreen = WinGetScreenPS(HWND_DESKTOP);
                            anmBlowUpBitmap(hpsScreen,
                                            hbmBootLogo,
                                            1000);  // total animation time

                            DosSleep(2000);

                            WinReleasePS(hpsScreen);

                            // repaint all windows
                            winhRepaintWindows(HWND_DESKTOP);
                        }
                        else
                        {
                            // transparent mode:
                            SHAPEFRAME sf;
                            SWP     swpScreen;

                            sf.hab = WinQueryAnchorBlock(pcnbp->hwndDlgPage);
                            sf.hps = hpsMem;
                            sf.hbm = hbmBootLogo;
                            sf.bmi.cbFix = sizeof(sf.bmi);
                            GpiQueryBitmapInfoHeader(sf.hbm, &sf.bmi);

                            // set ptlLowerLeft so that the bitmap
                            // is centered on the screen
                            WinQueryWindowPos(HWND_DESKTOP, &swpScreen);
                            sf.ptlLowerLeft.x = (swpScreen.cx - sf.bmi.cx) / 2;
                            sf.ptlLowerLeft.y = (swpScreen.cy - sf.bmi.cy) / 2;

                            if (shpCreateWindows(&sf)) // this selects the bitmap into the HPS
                            {
                                DosSleep(2000);

                                GpiSetBitmap(sf.hps, NULLHANDLE); // V0.9.3 (2000-04-11) [umoeller]

                                WinDestroyWindow(sf.hwndShapeFrame) ;
                                WinDestroyWindow(sf.hwndShape);
                            }
                        }
                        // delete the bitmap again
                        if (!GpiDeleteBitmap(hbmBootLogo))
                            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                                "Unable to free bootlogo bitmap.");
                    }
                    GpiDestroyPS(hpsMem);
                    DevCloseDC(hdcMem);
                }
                free(pszBootLogoFile);

                WinSetPointer(HWND_DESKTOP, hptrOld);
            break; }
#endif

            /*
             *@@ ID_XSDI_DTP_CREATESTARTUPFLDR:
             *      "Create startup folder"
             */

            case ID_XSDI_DTP_CREATESTARTUPFLDR:
            {
                CHAR        szSetup[200];
                HOBJECT     hObj;
                sprintf(szSetup,
                    "DEFAULTVIEW=ICON;ICONVIEW=NONFLOWED,MINI;"
                    "OBJECTID=%s;",
                    XFOLDER_STARTUPID);
                if (hObj = WinCreateObject((PSZ)G_pcszXFldStartup,
                                           "XWorkplace Startup",
                                           szSetup,
                                           (PSZ)WPOBJID_DESKTOP, // "<WP_DESKTOP>",
                                           CO_UPDATEIFEXISTS))
                    winhEnableDlgItem(pcnbp->hwndDlgPage, ID_XSDI_DTP_CREATESTARTUPFLDR, FALSE);
                else
                    cmnMessageBoxMsg(pcnbp->hwndFrame,
                                     104, 105,
                                     MB_OK);
            break; }

        }
    } // end if (!fProcessed)

    return ((MPARAM)0);
}


