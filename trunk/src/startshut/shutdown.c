
/*
 *@@sourcefile shutdwn.c:
 *      this file contains all the XShutdown code, which
 *      was in xfdesk.c before V0.84.
 *
 *      With V0.9.0, this file has been renamed from xshutdwn.c
 *      because we want the files to start with "x" to contain
 *      SOM classes only.
 *
 *      All the functions in this file have the xsd* prefix.
 *
 *      XShutdown is started from xfdesk.c simply by creating
 *      the Shutdown thread, which will then take over.
 *      See fntShutdownThread for a detailed description.
 *
 *@@header "startshut\shutdown.h"
 */

/*
 *      Copyright (C) 1997-99 Ulrich M�ller.
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

/*
 *@@todo:
 *
 */

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

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h
#include <stdarg.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\animate.h"            // icon and other animations
#include "helpers\comctl.h"             // common controls (window procs)
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\except.h"             // exception handling
#include "helpers\linklist.h"           // linked list helper routines
#include "helpers\prfh.h"               // INI file helper routines
#include "helpers\procstat.h"           // DosQProcStat handling
#include "helpers\winh.h"               // PM helper routines

// SOM headers which don't crash with prec. header files
#pragma hdrstop                 // VAC++ keeps crashing otherwise
#include "xfldr.h"              // needed for shutdown folder

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling

#include "filesys\menus.h"              // common XFolder context menu logic
#include "filesys\xthreads.h"           // extra XWorkplace threads

#include "startshut\apm.h"              // APM power-off for XShutdown
#include "startshut\shutdown.h"         // XWorkplace eXtended Shutdown

// other SOM headers
#include <wpdesk.h>                     // WPDesktop
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)

/* ******************************************************************
 *                                                                  *
 *   Global variables                                               *
 *                                                                  *
 ********************************************************************/

HWND                hwndMain = NULLHANDLE,
                    hwndShutdownStatus = NULLHANDLE;
PFNWP               SysWndProc;
BOOL                fAllWindowsClosed = FALSE,
                    fClosingApps = FALSE,
                    fShutdownBegun = FALSE;

PID                 pidWPS, pidPM;
ULONG               sidWPS, sidPM;

ULONG               ulMaxItemCount,
                    ulLastItemCount;

// HFILE               hfShutdownLog = NULLHANDLE;      // changed V0.9.0
FILE*               fileShutdownLog = NULL;

// shutdown animation
SHUTDOWNANIM        sdAnim;

/*
 *  here come the necessary definitions and functions
 *  for maintaining pliShutdownFirst, the global list of
 *  windows which need to be closed; these lists are
 *  maintained according to the system Tasklist
 */

// this is the global list of items to be closed (SHUTLISTITEMs)
PLINKLIST           pllShutdown = NULL,
// and the list of items that are to be skipped
                    pllSkipped = NULL;
HMTX                hmtxShutdown = NULLHANDLE,
                    hmtxSkipped = NULLHANDLE;
HEV                 hevUpdated = NULLHANDLE;

// temporary storage for closing VIOs
HWND                hwndVioDlg = NULLHANDLE;
CHAR                szVioTitle[1000] = "";
SHUTLISTITEM        VioItem;

// global linked list of auto-close VIO windows (AUTOCLOSELISTITEM)
PLINKLIST           pllAutoClose = NULL;

WPDesktop           *pActiveDesktop = NULL;
HWND                hwndActiveDesktop = NULLHANDLE;

// forward declarations
MRESULT EXPENTRY fnwpShutdown(HWND hwndFrame, ULONG msg, MPARAM mp1, MPARAM mp2);
void _Optlink xsd_fntUpdateThread(PVOID ptiMyself);
VOID xsdFinishShutdown(VOID);
VOID xsdFinishStandardMessage(VOID);
VOID xsdFinishStandardReboot(VOID);
VOID xsdFinishUserReboot(VOID);
VOID xsdFinishAPMPowerOff(VOID);

/* ******************************************************************
 *                                                                  *
 *   Shutdown interface                                             *
 *                                                                  *
 ********************************************************************/

/*
 *@@ xsdInitiateShutdown:
 *      Shutdown entry point; checks pGlobalSettings->XShutdown
 *      for all the XSD_* flags (shutdown options).
 *      If compiled with XFLDR_DEBUG defined (in common.h),
 *      debug mode will also be turned on if the SHIFT key is
 *      pressed at call time (that is, when the menu item is
 *      selected).
 *
 *      This routine will display a confirmation box,
 *      if the settings want it, and then start the
 *      main shutdown thread (xsd_fntShutdownThread),
 *      which will keep running even after shutdown
 *      is complete, unless the user presses the
 *      "Cancel shutdown" button.
 *
 *      Although this method does return almost
 *      immediately (after the confirmation dlg is dismissed),
 *      the shutdown will proceed in the separate thread
 *      after this function returns.
 *
 *@@changed V0.9.0 [umoeller]: global SHUTDOWNPARAMS removed
 *@@changed V0.9.0 [umoeller]: this used to be an XFldDesktop instance method
 *@@changed V0.9.1 (99-12-10) [umoeller]: fixed KERNELGLOBALS locks
 *@@changed V0.9.1 (99-12-12) [umoeller]: fixed memory leak when shutdown was cancelled
 */

BOOL xsdInitiateShutdown(VOID)
{
    BOOL                fStartShutdown = TRUE;
    ULONG               ulSpooled = 0;
    PCGLOBALSETTINGS    pGlobalSettings = cmnQueryGlobalSettings();
    PSHUTDOWNPARAMS     psdp = (PSHUTDOWNPARAMS)malloc(sizeof(SHUTDOWNPARAMS));

    {
        PKERNELGLOBALS     pKernelGlobals = krnLockGlobals(5000);
        // lock shutdown menu items
        pKernelGlobals->fShutdownRunning = TRUE;

        if (thrQueryID(pKernelGlobals->ptiShutdownThread))
            // shutdown thread already running: return!
            fStartShutdown = FALSE;
        krnUnlockGlobals();
    }

    if (fStartShutdown)
    {
        psdp->optReboot = ((pGlobalSettings->ulXShutdownFlags & XSD_REBOOT) != 0);
        psdp->optRestartWPS = FALSE;
        psdp->optWPSCloseWindows = TRUE;
        psdp->optWPSReuseStartupFolder = psdp->optWPSCloseWindows;
        psdp->optConfirm = ((pGlobalSettings->ulXShutdownFlags & XSD_CONFIRM) != 0);
        psdp->optAutoCloseVIO = ((pGlobalSettings->ulXShutdownFlags & XSD_AUTOCLOSEVIO) != 0);
        psdp->optLog = ((pGlobalSettings->ulXShutdownFlags & XSD_LOG) != 0);
        psdp->optAnimate = ((pGlobalSettings->ulXShutdownFlags & XSD_ANIMATE) != 0);
        psdp->optAPMPowerOff = (  ((pGlobalSettings->ulXShutdownFlags & XSD_APMPOWEROFF) != 0)
                          && (apmPowerOffSupported())
                         );

        #ifdef DEBUG_SHUTDOWN
            psdp->optDebug = doshQueryShiftState();
        #else
            psdp->optDebug = FALSE;
        #endif

        psdp->szRebootCommand[0] = 0;

        if (psdp->optConfirm)
        {
            ULONG ulReturn = xsdConfirmShutdown(psdp);
            if (ulReturn != DID_OK)
                fStartShutdown = FALSE;
        }

        if (fStartShutdown)
        {
            // check for pending spool jobs
            ulSpooled = winhQueryPendingSpoolJobs();
            if (ulSpooled)
            {
                // if we have any, issue a warning message and
                // allow the user to abort shutdown
                CHAR szTemp[20];
                PSZ pTable[1];
                sprintf(szTemp, "%d", ulSpooled);
                pTable[0] = szTemp;
                if (cmnMessageBoxMsgExt(HWND_DESKTOP, 114, pTable, 1, 115,
                                        MB_YESNO | MB_DEFBUTTON2)
                            != MBID_YES)
                    fStartShutdown = FALSE;
            }
        }
    }

    {
        PKERNELGLOBALS     pKernelGlobals = krnLockGlobals(5000);
        if (fStartShutdown)
        {
            // everything OK: create shutdown thread,
            // which will handle the rest
            thrCreate(&(pKernelGlobals->ptiShutdownThread),
                        fntShutdownThread,
                        (ULONG)psdp);           // pass SHUTDOWNPARAMS to thread
            xthrPlaySystemSound(MMSOUND_XFLD_SHUTDOWN);
        }
        else
            free(psdp);     // fixed V0.9.1 (99-12-12)

        pKernelGlobals->fShutdownRunning = fStartShutdown;
        krnUnlockGlobals();
    }

    return (fStartShutdown);
}

/*
 *@@ xsdInitiateRestartWPS:
 *      pretty similar to xsdInitiateShutdown, i.e. this
 *      will also show a confirmation box and start the Shutdown
 *      thread, except that flags are set differently so that
 *      after closing all windows, no shutdown is performed, but
 *      only the WPS is restarted.
 *
 *@@changed V0.9.0 [umoeller]: global SHUTDOWNPARAMS removed
 *@@changed V0.9.0 [umoeller]: this used to be an XFldDesktop instance method
 *@@changed V0.9.1 (99-12-10) [umoeller]: fixed KERNELGLOBALS locks
 *@@changed V0.9.1 (99-12-12) [umoeller]: fixed memory leak when shutdown was cancelled
 */

BOOL xsdInitiateRestartWPS(VOID)
{
    BOOL                fStartShutdown = TRUE;
    PCGLOBALSETTINGS    pGlobalSettings = cmnQueryGlobalSettings();
    PSHUTDOWNPARAMS     psdp = (PSHUTDOWNPARAMS)malloc(sizeof(SHUTDOWNPARAMS));

    {
        PKERNELGLOBALS     pKernelGlobals = krnLockGlobals(5000);
        // lock shutdown menu items
        pKernelGlobals->fShutdownRunning = TRUE;

        if (thrQueryID(pKernelGlobals->ptiShutdownThread))
            // shutdown thread already running: return!
            fStartShutdown = FALSE;
        krnUnlockGlobals();
    }

    if (fStartShutdown)
    {
        psdp->optReboot =  FALSE;
        psdp->optRestartWPS = TRUE;
        psdp->optWPSCloseWindows = ((pGlobalSettings->ulXShutdownFlags & XSD_WPS_CLOSEWINDOWS) != 0);
        psdp->optWPSReuseStartupFolder = psdp->optWPSCloseWindows;
        psdp->optConfirm = ((pGlobalSettings->ulXShutdownFlags & XSD_CONFIRM) != 0);
        psdp->optAutoCloseVIO = ((pGlobalSettings->ulXShutdownFlags & XSD_AUTOCLOSEVIO) != 0);
        psdp->optLog =  ((pGlobalSettings->ulXShutdownFlags & XSD_LOG) != 0);
        #ifdef DEBUG_SHUTDOWN
            psdp->optDebug = doshQueryShiftState();
        #else
            psdp->optDebug = FALSE;
        #endif

        if (psdp->optConfirm)
        {
            ULONG ulReturn = xsdConfirmRestartWPS(psdp);
            if (ulReturn != DID_OK)
                fStartShutdown = FALSE;
        }
    }

    {
        PKERNELGLOBALS     pKernelGlobals = krnLockGlobals(5000);
        if (fStartShutdown)
        {
            // everything OK: create shutdown thread,
            // which will handle the rest
            thrCreate(&(pKernelGlobals->ptiShutdownThread),
                        fntShutdownThread,
                        (ULONG)psdp);           // pass SHUTDOWNPARAMS to thread
            xthrPlaySystemSound(MMSOUND_XFLD_SHUTDOWN);
        }
        else
            free(psdp);     // fixed V0.9.1 (99-12-12)

        pKernelGlobals->fShutdownRunning = fStartShutdown;
        krnUnlockGlobals();
    }

    return (fStartShutdown);
}

/*
 *@@ xsdInitiateShutdownExt:
 *      just like the XFldDesktop method, but this one
 *      allows setting all the shutdown parameters by
 *      using the SHUTDOWNPARAMS structure. This is used
 *      for calling XShutdown externally, which is done
 *      by sending T1M_EXTERNALSHUTDOWN to the XFolder
 *      object window (see xfobj.c).
 *
 *      NOTE: The memory block pointed to by psdp is
 *      not released by this function.
 */

BOOL xsdInitiateShutdownExt(PSHUTDOWNPARAMS psdpShared)
{
    PKERNELGLOBALS      pKernelGlobals = krnLockGlobals(5000);
    PSHUTDOWNPARAMS     psdpNew = (PSHUTDOWNPARAMS)malloc(sizeof(SHUTDOWNPARAMS));

    if (thrQueryID(pKernelGlobals->ptiShutdownThread))
    {
        // shutdown thread already running: return!
        krnUnlockGlobals();
        return FALSE;
    }

    if (psdpShared == NULL)
    {
        krnUnlockGlobals();
        return FALSE;
    }

    pKernelGlobals->fShutdownRunning = TRUE;

    psdpNew->optReboot = psdpShared->optReboot;
    psdpNew->optRestartWPS = psdpShared->optRestartWPS;
    psdpNew->optWPSCloseWindows = psdpShared->optWPSCloseWindows;
    psdpNew->optConfirm = psdpShared->optConfirm;
    psdpNew->optAutoCloseVIO = psdpShared->optAutoCloseVIO;
    psdpNew->optLog = psdpShared->optLog;
    psdpNew->optAnimate = psdpShared->optAnimate;

    psdpNew->optDebug = psdpShared->optDebug;

    strcpy(psdpNew->szRebootCommand, psdpShared->szRebootCommand);

    if (psdpNew->optConfirm)
    {
        ULONG ulReturn;
        if (psdpNew->optRestartWPS)
            ulReturn = xsdConfirmRestartWPS(psdpNew);
        else
            ulReturn = xsdConfirmShutdown(psdpNew);

        if (ulReturn != DID_OK)
        {
            pKernelGlobals->fShutdownRunning = FALSE;
            krnUnlockGlobals();
            return (FALSE);
        }
    }

    // everything OK: create shutdown thread,
    // which will handle the rest
    thrCreate(&(pKernelGlobals->ptiShutdownThread),
                fntShutdownThread,
                (ULONG)psdpNew);

    krnUnlockGlobals();

    xthrPlaySystemSound(MMSOUND_XFLD_SHUTDOWN);
    return (TRUE);
}

/*
 *@@ xsdLoadAutoCloseItems:
 *      this gets the list of VIO windows which
 *      are to be closed automatically from OS2.INI
 *      and appends AUTOCLOSELISTITEM's to the given
 *      list accordingly.
 *
 *      If hwndListbox != NULLHANDLE, the program
 *      titles are added to that listbox as well.
 *
 *      Returns the no. of items which were added.
 *
 *@@added V0.9.1 (99-12-10) [umoeller]
 */

USHORT xsdLoadAutoCloseItems(PLINKLIST pllItems,   // in: list of AUTOCLOSELISTITEM's to append to
                            HWND hwndListbox)     // in: listbox to add items to or NULLHANDLE if none
{
    USHORT      usItemCount = 0;
    ULONG       ulKeyLength;
    PSZ         p, pINI;

    // get existing items from INI
    if (PrfQueryProfileSize(HINI_USER,
                            INIAPP_XWORKPLACE, INIKEY_AUTOCLOSE,
                            &ulKeyLength))
    {
        // printf("Size: %d\n", ulKeyLength);
        // items exist: evaluate
        pINI = malloc(ulKeyLength);
        if (pINI)
        {
            PrfQueryProfileData(HINI_USER,
                                INIAPP_XWORKPLACE, INIKEY_AUTOCLOSE,
                                pINI,
                                &ulKeyLength);
            p = pINI;
            //printf("%s\n", p);
            while (strlen(p))
            {
                PAUTOCLOSELISTITEM pliNew = malloc(sizeof(AUTOCLOSELISTITEM));
                strcpy(pliNew->szItemName, p);
                lstAppendItem(pllItems, // pData->pllAutoClose,
                              pliNew);

                if (hwndListbox)
                    WinSendMsg(hwndListbox,
                               LM_INSERTITEM,
                               (MPARAM)LIT_END,
                               (MPARAM)p);

                p += (strlen(p)+1);

                if (strlen(p))
                {
                    pliNew->usAction = *((PUSHORT)p);
                    p += sizeof(USHORT);
                }

                usItemCount++;
            }
            free(pINI);
        }
    }

    return (usItemCount);
}

/*
 *@@ xsdWriteAutoCloseItems:
 *      reverse to xsdLoadAutoCloseItems, this writes the
 *      auto-close items back to OS2.INI.
 *
 *      This returns 0 only if no error occured. If something
 *      != 0 is returned, that's the index of the list item
 *      which was found to be invalid.
 *
 *@@added V0.9.1 (99-12-10) [umoeller]
 */

USHORT xsdWriteAutoCloseItems(PLINKLIST pllItems)
{
    USHORT  usInvalid = 0;
    PSZ     pINI, p;
    BOOL    fValid = TRUE;
    ULONG   ulItemCount = lstCountItems(pllItems);

    // store data in INI
    if (ulItemCount)
    {
        pINI = malloc(
                    sizeof(AUTOCLOSELISTITEM)
                  * ulItemCount);
        memset(pINI, 0,
                    sizeof(AUTOCLOSELISTITEM)
                  * ulItemCount);
        if (pINI)
        {
            PLISTNODE pNode = lstQueryFirstNode(pllItems);
            USHORT          usCurrent = 0;
            p = pINI;
            while (pNode)
            {
                PAUTOCLOSELISTITEM pli = pNode->pItemData;
                if (strlen(pli->szItemName) == 0)
                {
                    usInvalid = usCurrent;
                    break;
                }

                strcpy(p, pli->szItemName);
                p += (strlen(p)+1);
                *((PUSHORT)p) = pli->usAction;
                p += sizeof(USHORT);

                pNode = pNode->pNext;
                usCurrent++;
            }

            PrfWriteProfileData(HINI_USER,
                                INIAPP_XWORKPLACE, INIKEY_AUTOCLOSE,
                                pINI,
                                (p - pINI + 2));

            free (pINI);
        }
    } // end if (pData->pliAutoClose)
    else
        // no items: delete INI key
        PrfWriteProfileData(HINI_USER,
                            INIAPP_XWORKPLACE, INIKEY_AUTOCLOSE,
                            NULL, 0);

    return (usInvalid);
}

/* ******************************************************************
 *                                                                  *
 *   Shutdown settings pages                                        *
 *                                                                  *
 ********************************************************************/

/*
 * xsdShutdownInitPage:
 *      notebook callback function (notebook.c) for the
 *      "XShutdown" page in the Desktop's settings
 *      notebook.
 *      Sets the controls on the page according to the
 *      Global Settings.
 *
 *@@added V0.9.0 [umoeller]
 */

VOID xsdShutdownInitPage(PCREATENOTEBOOKPAGE pcnbp,   // notebook info struct
                         ULONG flFlags)        // CBI_* flags (notebook.h)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

    if (flFlags & CBI_INIT)
    {
        APIRET      arc = NO_ERROR;
        PEXECUTABLE pExec;
        CHAR    szAPMVersion[30];
        CHAR    szAPMSysFile[CCHMAXPATH];
        sprintf(szAPMVersion, "APM %s", apmQueryVersion());
        WinSetDlgItemText(pcnbp->hwndPage, ID_SDDI_APMVERSION, szAPMVersion);
        sprintf(szAPMSysFile,
                "%c:\\OS2\\BOOT\\APM.SYS",
                doshQueryBootDrive());
        #ifdef DEBUG_SHUTDOWN
            _Pmpf(("Opening %s", szAPMSysFile));
        #endif

        WinSetDlgItemText(pcnbp->hwndPage, ID_SDDI_APMSYS,
                          "Error");

        if ((arc = doshExecOpen(szAPMSysFile,
                                &pExec))
                    == NO_ERROR)
        {
            if ((arc = doshExecQueryBldLevel(pExec))
                            == NO_ERROR)
            {
                if (pExec->pszVersion)
                    WinSetDlgItemText(pcnbp->hwndPage, ID_SDDI_APMSYS,
                                      pExec->pszVersion);

            }

            doshExecClose(pExec);
        }
    }

    if (flFlags & CBI_SET)
    {
        /* winhSetDlgItemChecked(pcnbp->hwndPage, ID_SDDI_ENABLED,
            (pGlobalSettings->ulXShutdownFlags & XSD_ENABLED) != 0); */
        winhSetDlgItemChecked(pcnbp->hwndPage, ID_SDDI_REBOOT,
            (pGlobalSettings->ulXShutdownFlags & XSD_REBOOT) != 0);
        winhSetDlgItemChecked(pcnbp->hwndPage, ID_SDDI_ANIMATE,
            (pGlobalSettings->ulXShutdownFlags & XSD_ANIMATE) != 0);
        winhSetDlgItemChecked(pcnbp->hwndPage, ID_SDDI_APMPOWEROFF,
            (apmPowerOffSupported())
                ? ((pGlobalSettings->ulXShutdownFlags & XSD_APMPOWEROFF) != 0)
                : FALSE
            );
        winhSetDlgItemChecked(pcnbp->hwndPage, ID_SDDI_CONFIRM,
            (pGlobalSettings->ulXShutdownFlags & XSD_CONFIRM) != 0);
        winhSetDlgItemChecked(pcnbp->hwndPage, ID_SDDI_AUTOCLOSEVIO,
            (pGlobalSettings->ulXShutdownFlags & XSD_AUTOCLOSEVIO) != 0);
        winhSetDlgItemChecked(pcnbp->hwndPage, ID_SDDI_LOG,
            (pGlobalSettings->ulXShutdownFlags & XSD_LOG) != 0);
    }

    if (flFlags & CBI_ENABLE)
    {
        BOOL fXShutdownValid = (pGlobalSettings->NoWorkerThread == 0);
        BOOL fXShutdownEnabled =
                (   (fXShutdownValid)
                 && (pGlobalSettings->fXShutdown)
                );
        BOOL fXShutdownOrWPSValid =
                (   (   (pGlobalSettings->fXShutdown)
                     || (pGlobalSettings->fRestartWPS)
                    )
                 && (pGlobalSettings->NoWorkerThread == 0)
                );

        WinEnableControl(pcnbp->hwndPage, ID_SDDI_ENABLED, fXShutdownValid);
        WinEnableControl(pcnbp->hwndPage, ID_SDDI_REBOOT,  fXShutdownEnabled);
        WinEnableControl(pcnbp->hwndPage, ID_SDDI_REBOOTEXT, fXShutdownEnabled);
        WinEnableControl(pcnbp->hwndPage, ID_SDDI_ANIMATE, fXShutdownEnabled);
        WinEnableControl(pcnbp->hwndPage, ID_SDDI_APMPOWEROFF,
                    ( fXShutdownEnabled && (apmPowerOffSupported()) )
                );

        WinEnableControl(pcnbp->hwndPage, ID_SDDI_CONFIRM, fXShutdownOrWPSValid);
        WinEnableControl(pcnbp->hwndPage, ID_SDDI_AUTOCLOSEVIO, fXShutdownOrWPSValid);
        WinEnableControl(pcnbp->hwndPage, ID_SDDI_AUTOCLOSEDETAILS, fXShutdownOrWPSValid);
        WinEnableControl(pcnbp->hwndPage, ID_SDDI_LOG, fXShutdownOrWPSValid);

        if (WinQueryObject(XFOLDER_SHUTDOWNID))
            // shutdown folder exists already: disable button
            WinEnableControl(pcnbp->hwndPage, ID_SDDI_CREATESHUTDOWNFLDR, FALSE);
    }
}

/*
 * xsdShutdownItemChanged:
 *      notebook callback function (notebook.c) for the
 *      "XShutdown" page in the Desktop's settings
 *      notebook.
 *      Reacts to changes of any of the dialog controls.
 *
 *@@added V0.9.0 [umoeller]
 */

MRESULT xsdShutdownItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                USHORT usItemID,
                                USHORT usNotifyCode,
                                ULONG ulExtra)      // for checkboxes: contains new state
{
    ULONG ulChange = 1;
    ULONG ulFlag = -1;

    // LONG lTemp;

    switch (usItemID)
    {
        /* case ID_SDDI_ENABLED:
            ulFlag = XSD_ENABLED;
        break; */

        case ID_SDDI_REBOOT:
            ulFlag = XSD_REBOOT;
        break;

        case ID_SDDI_ANIMATE:
            ulFlag = XSD_ANIMATE;
        break;

        case ID_SDDI_APMPOWEROFF:
            ulFlag = XSD_APMPOWEROFF;
        break;

        case ID_SDDI_CONFIRM:
            ulFlag = XSD_CONFIRM;
        break;

        case ID_SDDI_AUTOCLOSEVIO:
            ulFlag = XSD_AUTOCLOSEVIO;
        break;

        case ID_SDDI_LOG:
            ulFlag = XSD_LOG;
        break;

        // Reboot Actions (Desktop page 1)
        case ID_SDDI_REBOOTEXT:
            cmnSetHelpPanel(ID_XFH_REBOOTEXT);
            WinDlgBox(HWND_DESKTOP,         // parent is desktop
                      pcnbp->hwndPage,                  // owner
                      (PFNWP)fnwpUserRebootOptions,     // dialog procedure
                      cmnQueryNLSModuleHandle(FALSE),
                      ID_XSD_REBOOTEXT,        // dialog resource id
                      (PVOID)NULL);            // no dialog parameters
            ulChange = 0;
        break;

        // Auto-close details (Desktop page 1)
        case ID_SDDI_AUTOCLOSEDETAILS:
            cmnSetHelpPanel(ID_XFH_AUTOCLOSEDETAILS);
            WinDlgBox(HWND_DESKTOP,         // parent is desktop
                      pcnbp->hwndPage,             // owner
                      (PFNWP)fnwpAutoCloseDetails,    // dialog procedure
                      cmnQueryNLSModuleHandle(FALSE),  // from resource file
                      ID_XSD_AUTOCLOSE,        // dialog resource id
                      (PVOID)NULL);            // no dialog parameters
            ulChange = 0;
        break;

        // "Create shutdown folder"
        case ID_SDDI_CREATESHUTDOWNFLDR:
        {
            CHAR    szSetup[500];
            HOBJECT hObj = 0;
            sprintf(szSetup,
                "DEFAULTVIEW=ICON;ICONVIEW=NONFLOWED,MINI;"
                "OBJECTID=%s;",
                XFOLDER_SHUTDOWNID);
            if (hObj = WinCreateObject("XFldShutdown", "XFolder Shutdown",
                                       szSetup,
                                       "<WP_DESKTOP>",
                                       CO_UPDATEIFEXISTS))
                WinEnableControl(pcnbp->hwndPage, ID_SDDI_CREATESHUTDOWNFLDR, FALSE);
            else
                cmnMessageBoxMsg(pcnbp->hwndPage, 104, 106, MB_OK);
            ulChange = 0;
        break; }

        default:
            ulChange = 0;
    }

    if (ulFlag != -1)
    {
        GLOBALSETTINGS *pGlobalSettings = cmnLockGlobalSettings(5000);
        if (ulExtra)
            pGlobalSettings->ulXShutdownFlags |= ulFlag;
        else
            pGlobalSettings->ulXShutdownFlags &= ~ulFlag;
        cmnUnlockGlobalSettings();
    }

    if (ulChange)
    {
        // enable/disable items
        xsdShutdownInitPage(pcnbp, CBI_ENABLE);
        cmnStoreGlobalSettings();
    }

    return ((MPARAM)0);
}

/* ******************************************************************
 *                                                                  *
 *   Shutdown helper functions                                      *
 *                                                                  *
 ********************************************************************/

/*
 *@@ xsdLog:
 *      common function for writing into the XSHUTDWN.LOG file.
 *
 *@@added V0.9.0 [umoeller]
 */

void xsdLog(const char* pcszFormatString,
            ...)
{
    if (fileShutdownLog)
    {
        va_list vargs;

        DATETIME dt;
        CHAR szTemp[2000];
        ULONG   cbWritten;
        DosGetDateTime(&dt);
        fprintf(fileShutdownLog, "Time: %02d:%02d:%02d.%02d ",
                dt.hours, dt.minutes, dt.seconds, dt.hundredths);

        // get the variable parameters
        va_start(vargs, pcszFormatString);
        vfprintf(fileShutdownLog, pcszFormatString, vargs);
        va_end(vargs);
        fflush(fileShutdownLog);
    }
}

/*
 *@@ xsdLoadAnimation:
 *      this loads the shutdown (traffic light) animation
 *      as an array of icons from the XFLDR.DLL module.
 */

VOID xsdLoadAnimation(PSHUTDOWNANIM psda)
{
    HMODULE hmod = cmnQueryMainModuleHandle();
    (psda->ahptr)[0] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM1);
    (psda->ahptr)[1] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM2);
    (psda->ahptr)[2] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM3);
    (psda->ahptr)[3] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM4);
    (psda->ahptr)[4] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM5);
    (psda->ahptr)[5] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM4);
    (psda->ahptr)[6] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM3);
    (psda->ahptr)[7] = WinLoadPointer(HWND_DESKTOP, hmod, ID_ICONSDANIM2);
}

/*
 *@@ xsdFreeAnimation:
 *      this frees the animation loaded by xsdLoadAnimation.
 */

VOID xsdFreeAnimation(PSHUTDOWNANIM psda)
{
    USHORT us;
    for (us = 0; us < XSD_ANIM_COUNT; us++)
    {
        WinDestroyPointer((psda->ahptr)[us]);
        (psda->ahptr)[us] = NULLHANDLE;
    }
}

/*
 *@@ xsdRestartWPS:
 *      terminated the WPS process, which will lead
 *      to a WPS restart.
 *
 *      Runs on the Shutdown thread.
 */

VOID xsdRestartWPS(VOID)
{
    ULONG ul;

    // wait a maximum of 2 seconds while there's still
    // a system sound playing
    for (ul = 0; ul < 20; ul++)
        if (xthrIsPlayingSystemSound())
            DosSleep(100);
        else
            break;

    // terminate the current process,
    // which is PMSHELL.EXE. We cannot use DosExit()
    // directly, because this might mess up the
    // C runtime library.
    exit(0);        // 0 == no error
}

/*
 *@@ xsdFlushWPS2INI:
 *      this forces the WPS to flush its internal buffers
 *      into OS2.INI/OS2SYS.INI. We call this function
 *      after we have closed all the WPS windows, before
 *      we actually save the INI files.
 *
 *      This undocumented semaphore was published in some
 *      newsgroup ages ago, I don't remember.
 *
 *      Returns APIRETs of event semaphore calls.
 *
 *@@added V0.9.0 (99-10-22) [umoeller]
 */

APIRET xsdFlushWPS2INI(VOID)
{
    APIRET arc  = 0;
    HEV hev = NULLHANDLE;

    arc = DosOpenEventSem("\\SEM32\\WORKPLAC\\LAZYWRIT.SEM", &hev);
    if (arc == NO_ERROR)
    {
        arc = DosPostEventSem(hev);
        DosCloseEventSem(hev);
    }

    return (arc);
}

/* ******************************************************************
 *                                                                  *
 *   XShutdown dialogs                                              *
 *                                                                  *
 ********************************************************************/

/*
 *@@ ReformatConfirmWindow:
 *      depending on fExtended, the shutdown confirmation
 *      dialog is extended to show the "reboot to" listbox
 *      or not.
 *
 *@@added V0.9.0 [umoeller]
 */

VOID ReformatConfirmWindow(HWND hwndDlg,        // in: confirmation dlg window
                           BOOL fExtended)      // in: if TRUE, the list box will be shown
{
    HWND    hwndBootMgrListbox = WinWindowFromID(hwndDlg, ID_SDDI_BOOTMGR);
    SWP     swpBootMgrListbox;
    SWP     swpDlg;

    // _Pmpf(("ReformatConfirmWindow: %d", fExtended));

    WinQueryWindowPos(hwndBootMgrListbox, &swpBootMgrListbox);
    WinQueryWindowPos(hwndDlg, &swpDlg);

    if (fExtended)
        swpDlg.cx += swpBootMgrListbox.cx;
    else
        swpDlg.cx -= swpBootMgrListbox.cx;

    WinShowWindow(hwndBootMgrListbox, fExtended);
    WinSetWindowPos(hwndDlg,
                    NULLHANDLE,
                    0, 0,
                    swpDlg.cx, swpDlg.cy,
                    SWP_SIZE);
}

BOOL    fConfirmWindowExtended = FALSE;

/*
 * fnwpConfirm:
 *      dlg proc for XShutdown confirmation windows.
 *
 *@@changed V0.9.0 [umoeller]: redesigned the whole confirmation window.
 */

MRESULT EXPENTRY fnwpConfirm(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = MPNULL;
    switch (msg)
    {
        case WM_CONTROL:
            /* _Pmpf(("WM_CONTROL %d: fConfirmWindowExtended: %d",
                    SHORT1FROMMP(mp1), fConfirmWindowExtended)); */

            if (    (SHORT2FROMMP(mp1) == BN_CLICKED)
                 || (SHORT2FROMMP(mp1) == BN_DBLCLICKED)
               )
                switch (SHORT1FROMMP(mp1)) // usItemID
                {
                    case ID_SDDI_SHUTDOWNONLY:
                    case ID_SDDI_STANDARDREBOOT:
                        if (fConfirmWindowExtended)
                        {
                            ReformatConfirmWindow(hwndDlg, FALSE);
                            fConfirmWindowExtended = FALSE;
                        }
                    break;

                    case ID_SDDI_REBOOTTO:
                        if (!fConfirmWindowExtended)
                        {
                            ReformatConfirmWindow(hwndDlg, TRUE);
                            fConfirmWindowExtended = TRUE;
                        }
                    break;
                }

            mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
        break;

        default:
            mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
    }
    return (mrc);
}

/*
 *@@ xsdConfirmShutdown:
 *      this displays the eXtended Shutdown (not Restart WPS)
 *      confirmation box. Returns MBID_YES/NO.
 *
 *@@changed V0.9.0 [umoeller]: redesigned the whole confirmation window.
 */

ULONG xsdConfirmShutdown(PSHUTDOWNPARAMS psdParms)
{
    ULONG       ulReturn;
    HWND        hwndConfirm;
    HMODULE     hmodResource = cmnQueryNLSModuleHandle(FALSE);
    // CHAR        szDefault[100];
    ULONG       ulKeyLength;
    PSZ         p, pINI = NULL;
    ULONG       ulCheckRadioButtonID = ID_SDDI_SHUTDOWNONLY;

    {
        PKERNELGLOBALS  pKernelGlobals = krnLockGlobals(5000);
        HPOINTER        hptrShutdown = WinLoadPointer(HWND_DESKTOP, hmodResource,
                                                 ID_SDICON);
        cmnSetHelpPanel(ID_XMH_XSHUTDOWN);
        fConfirmWindowExtended = TRUE;
        hwndConfirm = WinLoadDlg(HWND_DESKTOP, NULLHANDLE,
                                 fnwpConfirm,
                                 hmodResource,
                                 ID_SDD_CONFIRM,
                                 NULL);

        WinPostMsg(hwndConfirm,
                   WM_SETICON,
                   (MPARAM)hptrShutdown,
                    NULL);
        krnUnlockGlobals();
    }

    // prepare confirmation box items
    winhSetDlgItemChecked(hwndConfirm, ID_SDDI_MESSAGEAGAIN, psdParms->optConfirm);

    if (psdParms->optReboot)
        ulCheckRadioButtonID = ID_SDDI_STANDARDREBOOT;

    // insert ext reboot items into combo box;
    // check for reboot items in OS2.INI
    if (PrfQueryProfileSize(HINI_USER,
                            INIAPP_XWORKPLACE, INIKEY_BOOTMGR,
                            &ulKeyLength))
    {
        PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
        // items exist: evaluate
        pINI = malloc(ulKeyLength);
        if (pINI)
        {
            PrfQueryProfileData(HINI_USER,
                                INIAPP_XWORKPLACE, INIKEY_BOOTMGR,
                                pINI,
                                &ulKeyLength);
            p = pINI;
            // _Pmpf(( "%s", p ));
            while (strlen(p))
            {
                WinSendDlgItemMsg(hwndConfirm, ID_SDDI_BOOTMGR,
                                  LM_INSERTITEM,
                                  (MPARAM)LIT_END,
                                  (MPARAM)p);
                 // skip description string
                p += (strlen(p)+1);
                // skip reboot command
                p += (strlen(p)+1);
            }
        }

        // select reboot item from last time
        if (pGlobalSettings->usLastRebootExt != 0xFFFF)
        {
            WinSendDlgItemMsg(hwndConfirm, ID_SDDI_BOOTMGR,
                              LM_SELECTITEM,
                              (MPARAM)pGlobalSettings->usLastRebootExt, // item index
                              (MPARAM)TRUE); // select (not deselect)
            if (ulCheckRadioButtonID == ID_SDDI_STANDARDREBOOT)
                ulCheckRadioButtonID = ID_SDDI_REBOOTTO;
        }
    }
    else
        // no items found: disable
        WinEnableControl(hwndConfirm, ID_SDDI_REBOOTTO, FALSE);

    // check radio button
    winhSetDlgItemChecked(hwndConfirm, ulCheckRadioButtonID, TRUE);
    winhSetDlgItemFocus(hwndConfirm, ulCheckRadioButtonID);
    if (ulCheckRadioButtonID == ID_SDDI_REBOOTTO)
        ReformatConfirmWindow(hwndConfirm, TRUE);

    cmnSetControlsFont(hwndConfirm, 1, 5000);
    winhCenterWindow(hwndConfirm);

    xsdLoadAnimation(&sdAnim);
    ctlPrepareAnimation(WinWindowFromID(hwndConfirm, ID_SDDI_ICON),
                        XSD_ANIM_COUNT,
                        &(sdAnim.ahptr[0]),
                        150,    // delay
                        TRUE);  // start now

    // go!!
    ulReturn = WinProcessDlg(hwndConfirm);

    ctlStopAnimation(WinWindowFromID(hwndConfirm, ID_SDDI_ICON));
    xsdFreeAnimation(&sdAnim);

    if (ulReturn == DID_OK)
    {
        GLOBALSETTINGS *pGlobalSettings = cmnLockGlobalSettings(5000);

        // check "show this msg again"
        if (!(winhIsDlgItemChecked(hwndConfirm, ID_SDDI_MESSAGEAGAIN)))
            pGlobalSettings->ulXShutdownFlags &= ~XSD_CONFIRM;

        // check reboot options
        psdParms->optReboot = FALSE;
        if (winhIsDlgItemChecked(hwndConfirm, ID_SDDI_REBOOTTO))
        {
            USHORT usSelected = (USHORT)WinSendDlgItemMsg(hwndConfirm, ID_SDDI_BOOTMGR,
                                                          LM_QUERYSELECTION,
                                                          (MPARAM)LIT_CURSOR,
                                                          MPNULL);
            USHORT us;
            psdParms->optReboot = TRUE;

            p = pINI;
            for (us = 0; us < usSelected; us++)
            {
                // skip description string
                p += (strlen(p)+1);
                // skip reboot command
                p += (strlen(p)+1);
            }
            // skip description string to get to reboot command
            p += (strlen(p)+1);
            strcpy(psdParms->szRebootCommand, p);

            pGlobalSettings->ulXShutdownFlags |= XSD_REBOOT;
            pGlobalSettings->usLastRebootExt = usSelected;
        }
        else if (winhIsDlgItemChecked(hwndConfirm, ID_SDDI_STANDARDREBOOT))
        {
            psdParms->optReboot = TRUE;
            // szRebootCommand is a zero-byte only, which will lead to
            // the standard reboot in the Shutdown thread
            pGlobalSettings->ulXShutdownFlags |= XSD_REBOOT;
            pGlobalSettings->usLastRebootExt = 0xFFFF;
        }
        else
            // standard shutdown:
            pGlobalSettings->ulXShutdownFlags &= ~XSD_REBOOT;

        cmnUnlockGlobalSettings();
        cmnStoreGlobalSettings();
    }

    if (pINI)
        free(pINI);

    WinDestroyWindow(hwndConfirm);

    return (ulReturn);
}

/*
 *@@ xsdConfirmRestartWPS:
 *      this displays the WPS restart
 *      confirmation box. Returns MBID_YES/NO.
 */

ULONG xsdConfirmRestartWPS(PSHUTDOWNPARAMS psdParms)
{
    ULONG       ulReturn;
    HWND        hwndConfirm;
    HMODULE     hmodResource = cmnQueryNLSModuleHandle(FALSE);

    HPOINTER hptrShutdown = WinLoadPointer(HWND_DESKTOP, hmodResource,
                                      ID_SDICON);

    cmnSetHelpPanel(ID_XMH_RESTARTWPS);
    hwndConfirm = WinLoadDlg(HWND_DESKTOP, NULLHANDLE,
                             fnwpConfirm,
                             hmodResource,
                             ID_SDD_CONFIRMWPS,
                             NULL);

    WinPostMsg(hwndConfirm,
               WM_SETICON,
               (MPARAM)hptrShutdown,
                NULL);

    winhSetDlgItemChecked(hwndConfirm, ID_SDDI_WPS_CLOSEWINDOWS, psdParms->optWPSCloseWindows);
    winhSetDlgItemChecked(hwndConfirm, ID_SDDI_WPS_STARTUPFOLDER, psdParms->optWPSCloseWindows);
    winhSetDlgItemChecked(hwndConfirm, ID_SDDI_MESSAGEAGAIN, psdParms->optConfirm);
    winhCenterWindow(hwndConfirm);

    xsdLoadAnimation(&sdAnim);
    ctlPrepareAnimation(WinWindowFromID(hwndConfirm, ID_SDDI_ICON),
                        XSD_ANIM_COUNT,
                        &(sdAnim.ahptr[0]),
                        150,    // delay
                        TRUE);  // start now

    // *** go!
    ulReturn = WinProcessDlg(hwndConfirm);

    ctlStopAnimation(WinWindowFromID(hwndConfirm, ID_SDDI_ICON));
    xsdFreeAnimation(&sdAnim);

    if (ulReturn == DID_OK)
    {
        GLOBALSETTINGS *pGlobalSettings = cmnLockGlobalSettings(5000);
        psdParms->optWPSCloseWindows = winhIsDlgItemChecked(hwndConfirm,
                                                            ID_SDDI_WPS_CLOSEWINDOWS);
        if (psdParms->optWPSCloseWindows)
            pGlobalSettings->ulXShutdownFlags |= XSD_WPS_CLOSEWINDOWS;
        else
            pGlobalSettings->ulXShutdownFlags &= ~XSD_WPS_CLOSEWINDOWS;
        psdParms->optWPSReuseStartupFolder = winhIsDlgItemChecked(hwndConfirm,
                                                                  ID_SDDI_WPS_STARTUPFOLDER);
        if (!(winhIsDlgItemChecked(hwndConfirm,
                                   ID_SDDI_MESSAGEAGAIN)))
            pGlobalSettings->ulXShutdownFlags &= ~XSD_CONFIRM;
        cmnUnlockGlobalSettings();
        cmnStoreGlobalSettings();
    }

    WinDestroyWindow(hwndConfirm);

    return (ulReturn);
}

/*
 *@@ fnwpAutoCloseDetails:
 *      dlg func for "Auto-Close Details".
 *      This gets called from the notebook callbacks
 *      for the "XDesktop" notebook page
 *      (fncbDesktop1ItemChanged, xfdesk.c).
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 */

MRESULT EXPENTRY fnwpAutoCloseDetails(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = (MPARAM)NULL;

    switch (msg)
    {
        case WM_INITDLG:
        {
            // create window data in QWL_USER
            PAUTOCLOSEWINDATA pData = malloc(sizeof(AUTOCLOSEWINDATA));
            memset(pData, 0, sizeof(AUTOCLOSEWINDATA));
            pData->pllAutoClose = lstCreate(TRUE);  // auto-free items

            // set animation
            xsdLoadAnimation(&sdAnim);
            ctlPrepareAnimation(WinWindowFromID(hwndDlg,
                                                ID_SDDI_ICON),
                                XSD_ANIM_COUNT,
                                &(sdAnim.ahptr[0]),
                                150,    // delay
                                TRUE);  // start now

            pData->usItemCount = 0;

            pData->usItemCount = xsdLoadAutoCloseItems(pData->pllAutoClose,
                                                      WinWindowFromID(hwndDlg,
                                                                      ID_XSDI_XRB_LISTBOX));

            winhSetEntryFieldLimit(WinWindowFromID(hwndDlg, ID_XSDI_XRB_ITEMNAME),
                                   100-1);

            WinSetWindowULong(hwndDlg, QWL_USER, (ULONG)pData);

            WinPostMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
        break; }

        /*
         * WM_CONTROL:
         *
         */

        case WM_CONTROL:
        {
            switch (SHORT1FROMMP(mp1))
            {
                /*
                 * ID_XSDI_XRB_LISTBOX:
                 *      listbox was clicked on:
                 *      update other controls with new data
                 */

                case ID_XSDI_XRB_LISTBOX:
                {
                    if (SHORT2FROMMP(mp1) == LN_SELECT)
                        WinSendMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
                break; }

                /*
                 * ID_XSDI_XRB_ITEMNAME:
                 *      user changed item title: update data
                 */

                case ID_XSDI_XRB_ITEMNAME:
                {
                    if (SHORT2FROMMP(mp1) == EN_KILLFOCUS)
                    {
                        PAUTOCLOSEWINDATA pData =
                                (PAUTOCLOSEWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
                        if (pData)
                        {
                            if (pData->pliSelected)
                            {
                                WinQueryDlgItemText(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                                                    sizeof(pData->pliSelected->szItemName)-1,
                                                    pData->pliSelected->szItemName);
                                WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                                  LM_SETITEMTEXT,
                                                  (MPARAM)pData->sSelected,
                                                  (MPARAM)(pData->pliSelected->szItemName));
                            }
                        }
                    }
                break; }

                // radio buttons
                case ID_XSDI_ACL_WMCLOSE:
                case ID_XSDI_ACL_CTRL_C:
                case ID_XSDI_ACL_KILLSESSION:
                case ID_XSDI_ACL_SKIP:
                {
                    if (SHORT2FROMMP(mp1) == BN_CLICKED)
                    {
                        PAUTOCLOSEWINDATA pData =
                                (PAUTOCLOSEWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
                        if (pData)
                        {
                            if (pData->pliSelected)
                            {
                                pData->pliSelected->usAction =
                                    (SHORT1FROMMP(mp1) == ID_XSDI_ACL_WMCLOSE)
                                        ? ACL_WMCLOSE
                                    : (SHORT1FROMMP(mp1) == ID_XSDI_ACL_CTRL_C)
                                        ? ACL_CTRL_C
                                    : (SHORT1FROMMP(mp1) == ID_XSDI_ACL_KILLSESSION)
                                        ? ACL_KILLSESSION
                                    : ACL_SKIP;
                            }
                        }
                    }
                break; }

                default:
                    mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
            }
        break; }

        case WM_UPDATE:
        {
            // posted from various locations to wholly update
            // the dlg items
            PAUTOCLOSEWINDATA pData =
                    (PAUTOCLOSEWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
            //printf("WM_CONTROL ID_XSDI_XRB_LISTBOX LN_SELECT\n");
            if (pData)
            {
                pData->pliSelected = NULL;
                pData->sSelected = (USHORT)WinSendDlgItemMsg(hwndDlg,
                        ID_XSDI_XRB_LISTBOX,
                        LM_QUERYSELECTION,
                        (MPARAM)LIT_CURSOR,
                        MPNULL);
                //printf("  Selected: %d\n", pData->sSelected);
                if (pData->sSelected != LIT_NONE)
                {
                    pData->pliSelected = (PAUTOCLOSELISTITEM)lstItemFromIndex(
                                               pData->pllAutoClose,
                                               pData->sSelected);
                }

                if (pData->pliSelected)
                {
                    WinSetDlgItemText(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                            pData->pliSelected->szItemName);

                    switch (pData->pliSelected->usAction)
                    {
                        case ACL_WMCLOSE:
                            winhSetDlgItemChecked(hwndDlg,
                                ID_XSDI_ACL_WMCLOSE, 1); break;
                        case ACL_CTRL_C:
                            winhSetDlgItemChecked(hwndDlg,
                                ID_XSDI_ACL_CTRL_C, 1); break;
                        case ACL_KILLSESSION:
                            winhSetDlgItemChecked(hwndDlg,
                                ID_XSDI_ACL_KILLSESSION, 1); break;
                        case ACL_SKIP:
                            winhSetDlgItemChecked(hwndDlg,
                                ID_XSDI_ACL_SKIP, 1); break;
                    }
                }
                else
                    WinSetDlgItemText(hwndDlg,
                                      ID_XSDI_XRB_ITEMNAME,
                                      "");

                WinEnableControl(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                            (pData->pliSelected != NULL));

                WinEnableControl(hwndDlg, ID_XSDI_ACL_WMCLOSE,
                            (pData->pliSelected != NULL));
                WinEnableControl(hwndDlg, ID_XSDI_ACL_CTRL_C,
                            (pData->pliSelected != NULL));
                WinEnableControl(hwndDlg, ID_XSDI_ACL_KILLSESSION,
                            (pData->pliSelected != NULL));
                WinEnableControl(hwndDlg, ID_XSDI_ACL_SKIP,
                            (pData->pliSelected != NULL));

                WinEnableControl(hwndDlg,
                                  ID_XSDI_XRB_DELETE,
                                  (   (pData->usItemCount > 0)
                                   && (pData->pliSelected)
                                  ));
            }
        break; }

        case WM_COMMAND:
        {
            switch (SHORT1FROMMP(mp1))
            {

                /*
                 * ID_XSDI_XRB_NEW:
                 *      create new item
                 */

                case ID_XSDI_XRB_NEW:
                {
                    PAUTOCLOSEWINDATA pData =
                            (PAUTOCLOSEWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
                    PAUTOCLOSELISTITEM pliNew = malloc(sizeof(AUTOCLOSELISTITEM));
                    strcpy(pliNew->szItemName, "???");
                    pliNew->usAction = ACL_SKIP;
                    lstAppendItem(pData->pllAutoClose,
                                  pliNew);

                    pData->usItemCount++;
                    WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                      LM_INSERTITEM,
                                      (MPARAM)LIT_END,
                                      (MPARAM)pliNew->szItemName);
                    WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                      LM_SELECTITEM, // will cause WM_UPDATE
                                      (MPARAM)(lstCountItems(
                                              pData->pllAutoClose)),
                                      (MPARAM)TRUE);
                    winhSetDlgItemFocus(hwndDlg, ID_XSDI_XRB_ITEMNAME);
                    WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                                      EM_SETSEL,
                                      MPFROM2SHORT(0, 1000), // select all
                                      MPNULL);
                break; }

                /*
                 * ID_XSDI_XRB_DELETE:
                 *      delete selected item
                 */

                case ID_XSDI_XRB_DELETE:
                {
                    PAUTOCLOSEWINDATA pData =
                            (PAUTOCLOSEWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
                    //printf("WM_COMMAND ID_XSDI_XRB_DELETE BN_CLICKED\n");
                    if (pData)
                    {
                        if (pData->pliSelected)
                        {
                            lstRemoveItem(pData->pllAutoClose,
                                          pData->pliSelected);
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                              LM_DELETEITEM,
                                              (MPARAM)pData->sSelected,
                                              MPNULL);
                        }
                        WinPostMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
                    }
                    winhSetDlgItemFocus(hwndDlg, ID_XSDI_XRB_LISTBOX);
                break; }

                /*
                 * DID_OK:
                 *      store data in INI and dismiss dlg
                 */

                case DID_OK:
                {
                    PAUTOCLOSEWINDATA pData =
                        (PAUTOCLOSEWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);

                    USHORT usInvalid = xsdWriteAutoCloseItems(pData->pllAutoClose);
                    if (usInvalid)
                    {
                        WinAlarm(HWND_DESKTOP, WA_ERROR);
                        WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                          LM_SELECTITEM,
                                          (MPARAM)usInvalid,
                                          (MPARAM)TRUE);
                    }
                    else
                        // dismiss dlg
                        mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
                break; }

                default:  // includes DID_CANCEL
                    mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
                break;
            }
        break; }

        /*
         * WM_DESTROY:
         *      clean up allocated memory
         */

        case WM_DESTROY:
        {
            PAUTOCLOSEWINDATA pData = (PAUTOCLOSEWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
            ctlStopAnimation(WinWindowFromID(hwndDlg, ID_SDDI_ICON));
            xsdFreeAnimation(&sdAnim);
            lstFree(pData->pllAutoClose);
            free(pData);
            mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
        break; } // continue

        default:
            mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
        break;
    }
    return (mrc);
}

/*
 *@@ fnwpUserRebootOptions:
 *      dlg proc for the "Extended Reboot" options.
 *      This gets called from the notebook callbacks
 *      for the "XDesktop" notebook page
 *      (fncbDesktop1ItemChanged, xfdesk.c).
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 *@@changed V0.9.0 [umoeller]: renamed from fnwpRebootExt
 *@@changed V0.9.0 [umoeller]: added "Partitions" button
 */

MRESULT EXPENTRY fnwpUserRebootOptions(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = (MPARAM)NULL;

    switch (msg)
    {
        /*
         * WM_INITDLG:
         *
         */

        case WM_INITDLG:
        {
            ULONG       ulKeyLength;
            PSZ         p, pINI;

            // create window data in QWL_USER
            PREBOOTWINDATA pData = malloc(sizeof(REBOOTWINDATA));
            memset(pData, 0, sizeof(REBOOTWINDATA));
            pData->pllReboot = lstCreate(TRUE);

            // set animation
            xsdLoadAnimation(&sdAnim);
            ctlPrepareAnimation(WinWindowFromID(hwndDlg, ID_SDDI_ICON),
                                XSD_ANIM_COUNT,
                                &(sdAnim.ahptr[0]),
                                150,    // delay
                                TRUE);  // start now

            pData->usItemCount = 0;

            // get existing items from INI
            if (PrfQueryProfileSize(HINI_USER,
                        INIAPP_XWORKPLACE, INIKEY_BOOTMGR,
                        &ulKeyLength))
            {
                // _Pmpf(( "Size: %d", ulKeyLength ));
                // items exist: evaluate
                pINI = malloc(ulKeyLength);
                if (pINI)
                {
                    PrfQueryProfileData(HINI_USER,
                                INIAPP_XWORKPLACE, INIKEY_BOOTMGR,
                                pINI,
                                &ulKeyLength);
                    p = pINI;
                    // _Pmpf(( "%s", p ));
                    while (strlen(p))
                    {
                        PREBOOTLISTITEM pliNew = malloc(sizeof(REBOOTLISTITEM));
                        strcpy(pliNew->szItemName, p);
                        lstAppendItem(pData->pllReboot,
                                    pliNew);

                        WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                        LM_INSERTITEM,
                                        (MPARAM)LIT_END,
                                        (MPARAM)p);
                        p += (strlen(p)+1);

                        if (strlen(p)) {
                            strcpy(pliNew->szCommand, p);
                            p += (strlen(p)+1);
                        }
                        pData->usItemCount++;
                    }
                    free(pINI);
                }
            }

            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                EM_SETTEXTLIMIT, (MPARAM)(100-1), MPNULL);
            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_COMMAND,
                EM_SETTEXTLIMIT, (MPARAM)(CCHMAXPATH-1), MPNULL);

            WinSetWindowULong(hwndDlg, QWL_USER, (ULONG)pData);

            // create "menu button" for "Partitions..."
            ctlMakeMenuButton(WinWindowFromID(hwndDlg, ID_XSDI_XRB_PARTITIONS),
                              // set menu resource module and ID to
                              // 0; this will cause WM_COMMAND for
                              // querying the menu handle to be
                              // displayed
                              0, 0);

            WinPostMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
        break; }

        /*
         * WM_CONTROL:
         *
         */

        case WM_CONTROL:
        {
            switch (SHORT1FROMMP(mp1))
            {
                /*
                 * ID_XSDI_XRB_LISTBOX:
                 *      new reboot item selected.
                 */

                case ID_XSDI_XRB_LISTBOX:
                    if (SHORT2FROMMP(mp1) == LN_SELECT)
                        WinSendMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
                break;

                /*
                 * ID_XSDI_XRB_ITEMNAME:
                 *      reboot item name changed.
                 */

                case ID_XSDI_XRB_ITEMNAME:
                {
                    if (SHORT2FROMMP(mp1) == EN_KILLFOCUS)
                    {
                        PREBOOTWINDATA pData =
                                (PREBOOTWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
                        // _Pmpf(( "WM_CONTROL ID_XSDI_XRB_ITEMNAME EN_KILLFOCUS" ));
                        if (pData)
                        {
                            if (pData->pliSelected)
                            {
                                WinQueryDlgItemText(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                                                    sizeof(pData->pliSelected->szItemName)-1,
                                                    pData->pliSelected->szItemName);
                                WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                                  LM_SETITEMTEXT,
                                                  (MPARAM)pData->sSelected,
                                                  (MPARAM)(pData->pliSelected->szItemName));
                            }
                        }
                    }
                break; }

                /*
                 * ID_XSDI_XRB_COMMAND:
                 *      reboot item command changed.
                 */

                case ID_XSDI_XRB_COMMAND:
                {
                    if (SHORT2FROMMP(mp1) == EN_KILLFOCUS)
                    {
                        PREBOOTWINDATA pData =
                                (PREBOOTWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
                        // _Pmpf(( "WM_CONTROL ID_XSDI_XRB_COMMAND EN_KILLFOCUS" ));
                        if (pData)
                            if (pData->pliSelected)
                                WinQueryDlgItemText(hwndDlg, ID_XSDI_XRB_COMMAND,
                                                    sizeof(pData->pliSelected->szCommand)-1,
                                                    pData->pliSelected->szCommand);
                    }
                break; }

                default:
                    mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
            }
        break; }

        /*
         * WM_UPDATE:
         *      updates the controls according to the
         *      currently selected list box item.
         */

        case WM_UPDATE:
        {
            PREBOOTWINDATA pData =
                    (PREBOOTWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
            if (pData)
            {
                pData->pliSelected = NULL;
                pData->sSelected = (USHORT)WinSendDlgItemMsg(hwndDlg,
                                                             ID_XSDI_XRB_LISTBOX,
                                                             LM_QUERYSELECTION,
                                                             (MPARAM)LIT_CURSOR,
                                                             MPNULL);
                if (pData->sSelected != LIT_NONE)
                    pData->pliSelected = (PREBOOTLISTITEM)lstItemFromIndex(
                            pData->pllReboot,
                            pData->sSelected);

                if (pData->pliSelected)
                {
                    WinSetDlgItemText(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                            pData->pliSelected->szItemName);
                    WinSetDlgItemText(hwndDlg, ID_XSDI_XRB_COMMAND,
                            pData->pliSelected->szCommand);
                }
                else
                {
                    WinSetDlgItemText(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                            "");
                    WinSetDlgItemText(hwndDlg, ID_XSDI_XRB_COMMAND,
                            "");
                }
                WinEnableControl(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                            (pData->pliSelected != NULL));
                WinEnableControl(hwndDlg, ID_XSDI_XRB_COMMAND,
                            (pData->pliSelected != NULL));
                WinEnableControl(hwndDlg, ID_XSDI_XRB_PARTITIONS,
                            (pData->pliSelected != NULL));
                WinEnableControl(hwndDlg, ID_XSDI_XRB_UP,
                            (   (pData->pliSelected != NULL)
                             && (pData->usItemCount > 1)
                             && (pData->sSelected > 0)
                            ));
                WinEnableControl(hwndDlg, ID_XSDI_XRB_DOWN,
                            (   (pData->pliSelected != NULL)
                             && (pData->usItemCount > 1)
                             && (pData->sSelected < (pData->usItemCount-1))
                            ));

                WinEnableControl(hwndDlg, ID_XSDI_XRB_DELETE,
                            (   (pData->usItemCount > 0)
                             && (pData->pliSelected)
                            )
                        );
            }
        break; }

        /*
         * WM_COMMAND:
         *
         */

        case WM_COMMAND:
        {
            PREBOOTWINDATA pData =
                    (PREBOOTWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
            USHORT usItemID = SHORT1FROMMP(mp1);
            switch (usItemID)
            {
                /*
                 * ID_XSDI_XRB_NEW:
                 *      create new item
                 */

                case ID_XSDI_XRB_NEW:
                {
                    PREBOOTLISTITEM pliNew = malloc(sizeof(REBOOTLISTITEM));
                    // _Pmpf(( "WM_COMMAND ID_XSDI_XRB_NEW BN_CLICKED" ));
                    strcpy(pliNew->szItemName, "???");
                    strcpy(pliNew->szCommand, "???");
                    lstAppendItem(pData->pllReboot,
                                  pliNew);

                    pData->usItemCount++;
                    WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                    LM_INSERTITEM,
                                    (MPARAM)LIT_END,
                                    (MPARAM)pliNew->szItemName);
                    WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                    LM_SELECTITEM,
                                    (MPARAM)(lstCountItems(
                                            pData->pllReboot)
                                         - 1),
                                    (MPARAM)TRUE);
                    winhSetDlgItemFocus(hwndDlg, ID_XSDI_XRB_ITEMNAME);
                    WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_ITEMNAME,
                            EM_SETSEL,
                            MPFROM2SHORT(0, 1000), // select all
                            MPNULL);
                break; }

                /*
                 * ID_XSDI_XRB_DELETE:
                 *      delete delected item
                 */

                case ID_XSDI_XRB_DELETE:
                {
                    if (pData)
                    {
                        if (pData->pliSelected)
                        {
                            lstRemoveItem(pData->pllReboot,
                                    pData->pliSelected);
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                    LM_DELETEITEM,
                                    (MPARAM)pData->sSelected,
                                    MPNULL);
                        }
                        WinPostMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
                    }
                    winhSetDlgItemFocus(hwndDlg, ID_XSDI_XRB_LISTBOX);
                break; }

                /*
                 * ID_XSDI_XRB_UP:
                 *      move selected item up
                 */

                case ID_XSDI_XRB_UP:
                {
                    if (pData)
                    {
                        // _Pmpf(( "WM_COMMAND ID_XSDI_XRB_UP BN_CLICKED" ));
                        if (pData->pliSelected)
                        {
                            PREBOOTLISTITEM pliNew = malloc(sizeof(REBOOTLISTITEM));
                            *pliNew = *(pData->pliSelected);
                            // remove selected
                            lstRemoveItem(pData->pllReboot,
                                    pData->pliSelected);
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                    LM_DELETEITEM,
                                    (MPARAM)pData->sSelected,
                                    MPNULL);
                            // insert item again
                            lstInsertItemBefore(pData->pllReboot,
                                                pliNew,
                                                (pData->sSelected-1));
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                            LM_INSERTITEM,
                                            (MPARAM)(pData->sSelected-1),
                                            (MPARAM)pliNew->szItemName);
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                            LM_SELECTITEM,
                                            (MPARAM)(pData->sSelected-1),
                                            (MPARAM)TRUE); // select flag
                        }
                        WinPostMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
                    }
                    winhSetDlgItemFocus(hwndDlg, ID_XSDI_XRB_LISTBOX);
                break; }

                /*
                 * ID_XSDI_XRB_DOWN:
                 *      move selected item down
                 */

                case ID_XSDI_XRB_DOWN:
                {
                    if (pData)
                    {
                        // _Pmpf(( "WM_COMMAND ID_XSDI_XRB_DOWN BN_CLICKED" ));
                        if (pData->pliSelected)
                        {
                            PREBOOTLISTITEM pliNew = malloc(sizeof(REBOOTLISTITEM));
                            *pliNew = *(pData->pliSelected);
                            // remove selected
                            // _Pmpf(( "  Removing index %d", pData->sSelected ));
                            lstRemoveItem(pData->pllReboot,
                                          pData->pliSelected);
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                    LM_DELETEITEM,
                                    (MPARAM)pData->sSelected,
                                    MPNULL);
                            // insert item again
                            lstInsertItemBefore(pData->pllReboot,
                                                pliNew,
                                                (pData->sSelected+1));
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                            LM_INSERTITEM,
                                            (MPARAM)(pData->sSelected+1),
                                            (MPARAM)pliNew->szItemName);
                            WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                            LM_SELECTITEM,
                                            (MPARAM)(pData->sSelected+1),
                                            (MPARAM)TRUE); // select flag
                        }
                        WinPostMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);
                    }
                    winhSetDlgItemFocus(hwndDlg, ID_XSDI_XRB_LISTBOX);
                break; }

                /*
                 * ID_XSDI_XRB_PARTITIONS:
                 *      "Partitions" button.
                 *      Even though this is part of the WM_COMMAND
                 *      block, this is not really a command msg
                 *      like the other messages; instead, this
                 *      is sent (!) to us and expects a HWND for
                 *      the menu to be displayed on the button.
                 *
                 *      We create a menu containing bootable
                 *      partitions.
                 */

                case ID_XSDI_XRB_PARTITIONS:
                {
                    HPOINTER       hptrOld = winhSetWaitPointer();
                    HWND           hMenu = NULLHANDLE;

                    if (pData->ppi == NULL)
                    {
                        // first time:
                        USHORT         usContext = 0;
                        APIRET arc = doshGetPartitionsList(&pData->ppi,
                                                           &pData->usPartitions,
                                                           &usContext);
                        if (arc != NO_ERROR)
                            pData->ppi = NULL;
                    }

                    if (pData->ppi)
                    {
                        PPARTITIONINFO ppi = pData->ppi;
                        SHORT          sItemId = ID_XSDI_PARTITIONSFIRST;
                        hMenu = WinCreateMenu(HWND_DESKTOP,
                                              NULL); // no menu template
                        while (ppi)
                        {
                            if (ppi->fBootable)
                            {
                                CHAR szMenuItem[100];
                                sprintf(szMenuItem,
                                        "%s (Drive %d, %c:)",
                                        ppi->szBootName,
                                        ppi->bDisk,
                                        ppi->cLetter);
                                winhInsertMenuItem(hMenu,
                                                   MIT_END,
                                                   sItemId++,
                                                   szMenuItem,
                                                   MIS_TEXT,
                                                   0);
                            }
                            ppi = ppi->pNext;
                        }
                    }

                    WinSetPointer(HWND_DESKTOP, hptrOld);

                    mrc = (MRESULT)hMenu;
                break; }

                /*
                 * DID_OK:
                 *      store data in INI and dismiss dlg
                 */

                case DID_OK:
                {
                    PSZ     pINI, p;
                    BOOL    fValid = TRUE;
                    ULONG   ulItemCount = lstCountItems(pData->pllReboot);

                    // _Pmpf(( "WM_COMMAND DID_OK BN_CLICKED" ));
                    // store data in INI
                    if (ulItemCount)
                    {
                        pINI = malloc(
                                    sizeof(REBOOTLISTITEM)
                                  * ulItemCount);
                        memset(pINI, 0,
                                    sizeof(REBOOTLISTITEM)
                                  * ulItemCount);

                        if (pINI)
                        {
                            PLISTNODE       pNode = lstQueryFirstNode(pData->pllReboot);
                            USHORT          usCurrent = 0;
                            p = pINI;

                            while (pNode)
                            {
                                PREBOOTLISTITEM pli = pNode->pItemData;

                                if (    (strlen(pli->szItemName) == 0)
                                     || (strlen(pli->szCommand) == 0)
                                   )
                                {
                                    WinAlarm(HWND_DESKTOP, WA_ERROR);
                                    WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                                    LM_SELECTITEM,
                                                    (MPARAM)usCurrent,
                                                    (MPARAM)TRUE);
                                    fValid = FALSE;
                                    break;
                                }
                                strcpy(p, pli->szItemName);
                                p += (strlen(p)+1);
                                strcpy(p, pli->szCommand);
                                p += (strlen(p)+1);

                                pNode = pNode->pNext;
                                usCurrent++;
                            }

                            PrfWriteProfileData(HINI_USER,
                                        INIAPP_XWORKPLACE, INIKEY_BOOTMGR,
                                        pINI,
                                        (p - pINI + 2));

                            free (pINI);
                        }
                    } // end if (pData->pliReboot)
                    else
                        PrfWriteProfileData(HINI_USER,
                                    INIAPP_XWORKPLACE, INIKEY_BOOTMGR,
                                    NULL, 0);

                    // dismiss dlg
                    if (fValid)
                        mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
                break; }

                default: // includes DID_CANCEL
                    if (    (pData->ppi)
                         && (pData->usPartitions)
                       )
                    {
                        // partitions valid:
                        if (    (usItemID >= ID_XSDI_PARTITIONSFIRST)
                             && (usItemID < ID_XSDI_PARTITIONSFIRST + pData->usPartitions)
                             && (pData->pliSelected)
                           )
                        {
                            // partition item from "Partitions" menu button:
                            // search partitions list then
                            PPARTITIONINFO ppi = pData->ppi;
                            SHORT sItemIDCompare = ID_XSDI_PARTITIONSFIRST;
                            while (ppi)
                            {
                                if (ppi->fBootable)
                                {
                                    // bootable item:
                                    // then we have inserted the thing into
                                    // the menu
                                    if (sItemIDCompare == usItemID)
                                    {
                                        // found our one:
                                        // insert into entry field
                                        CHAR szItem[20];
                                        CHAR szCommand[100];
                                        ULONG ul = 0;

                                        // strip trailing spaces
                                        strcpy(szItem, ppi->szBootName);
                                        for (ul = strlen(szItem) - 1;
                                             ul > 0;
                                             ul--)
                                            if (szItem[ul] == ' ')
                                                szItem[ul] = 0;
                                            else
                                                break;

                                        // now set reboot item's data
                                        // according to the partition item
                                        strcpy(pData->pliSelected->szItemName,
                                               szItem);
                                        // compose new command
                                        sprintf(pData->pliSelected->szCommand,
                                                "setboot /iba:\"%s\"",
                                                szItem);

                                        // update list box item
                                        WinSendDlgItemMsg(hwndDlg, ID_XSDI_XRB_LISTBOX,
                                                          LM_SETITEMTEXT,
                                                          (MPARAM)pData->sSelected,
                                                          (MPARAM)(pData->pliSelected->szItemName));
                                        // update rest of dialog
                                        WinSendMsg(hwndDlg, WM_UPDATE, MPNULL, MPNULL);

                                        break; // while (ppi)
                                    }
                                    else
                                        // next item
                                        sItemIDCompare++;
                                }
                                ppi = ppi->pNext;
                            }

                            break;
                        }
                    }
                    mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
                break;
            }
        break; }

        /*
         * WM_DESTROY:
         *      clean up allocated memory
         */

        case WM_DESTROY:
        {
            PREBOOTWINDATA pData = (PREBOOTWINDATA)WinQueryWindowULong(hwndDlg, QWL_USER);
            ctlStopAnimation(WinWindowFromID(hwndDlg, ID_SDDI_ICON));
            xsdFreeAnimation(&sdAnim);
            lstFree(pData->pllReboot);
            if (pData->ppi != NULL)
                doshFreePartitionsList(pData->ppi);
            free(pData);
            mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
        break; }

        default:
            mrc = fnwpDlgGeneric(hwndDlg, msg, mp1, mp2);
        break;
    }
    return (mrc);
}

/* ******************************************************************
 *                                                                  *
 *   XShutdown data maintenance                                     *
 *                                                                  *
 ********************************************************************/

/*
 *@@ xsdItemFromPID:
 *      searches a given LINKLIST of SHUTLISTITEMs
 *      for a process ID.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 */

PSHUTLISTITEM xsdItemFromPID(PLINKLIST pList,
                             PID pid,
                             HMTX hmtx,
                             ULONG ulTimeout)
{
    PSHUTLISTITEM pItem = NULL;
    BOOL          fAccess = FALSE,
                  fSemOwned = FALSE;

    TRY_QUIET(excpt1, NULL)
    {
        if (hmtx)
        {
            fSemOwned = (DosRequestMutexSem(hmtx, ulTimeout) == NO_ERROR);
            fAccess = fSemOwned;
        } else
            fAccess = TRUE;

        if (fAccess)
        {
            PLISTNODE pNode = lstQueryFirstNode(pList);
            while (pNode)
            {
                pItem = pNode->pItemData;
                if (pItem->swctl.idProcess == pid)
                    break;

                pNode = pNode->pNext;
                pItem = 0;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fSemOwned) {
        DosReleaseMutexSem(hmtx);
        fSemOwned = FALSE;
    }

    return (pItem);
}

/*
 *@@ xsdItemFromSID:
 *      searches a given LINKLIST of SHUTLISTITEMs
 *      for a session ID.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 */

PSHUTLISTITEM xsdItemFromSID(PLINKLIST pList,
                             ULONG sid,
                             HMTX hmtx,
                             ULONG ulTimeout)
{
    PSHUTLISTITEM pItem = NULL;
    BOOL          fAccess = FALSE,
                  fSemOwned = FALSE;

    TRY_QUIET(excpt1, NULL)
    {
        if (hmtx) {
            fSemOwned = (DosRequestMutexSem(hmtx, ulTimeout) == NO_ERROR);
            fAccess = fSemOwned;
        } else
            fAccess = TRUE;

        if (fAccess)
        {
            PLISTNODE pNode = lstQueryFirstNode(pList);
            while (pNode)
            {
                pItem = pNode->pItemData;
                if (pItem->swctl.idSession == sid)
                    break;

                pNode = pNode->pNext;
                pItem = 0;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fSemOwned) {
        DosReleaseMutexSem(hmtx);
        fSemOwned = FALSE;
    }

    return (pItem);
}

/*
 *@@ xsdCountRemainingItems:
 *      counts the items left to be closed by counting
 *      the window list items and subtracting the items
 *      which were skipped by the user.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 */

ULONG xsdCountRemainingItems(VOID)
{
    ULONG   ulrc = 0;
    BOOL    fShutdownSemOwned = FALSE,
            fSkippedSemOwned = FALSE;

    TRY_QUIET(excpt1, NULL)
    {
        fShutdownSemOwned = (DosRequestMutexSem(hmtxShutdown, 4000) == NO_ERROR);
        fSkippedSemOwned = (DosRequestMutexSem(hmtxSkipped, 4000) == NO_ERROR);
        if ( (fShutdownSemOwned) && (fSkippedSemOwned) )
            ulrc = (
                        lstCountItems(pllShutdown)
                      - lstCountItems(pllSkipped)
                   );
    }
    CATCH(excpt1) { } END_CATCH();

    if (fShutdownSemOwned) {
        DosReleaseMutexSem(hmtxShutdown);
        fShutdownSemOwned = FALSE;
    }
    if (fSkippedSemOwned) {
        DosReleaseMutexSem(hmtxSkipped);
        fSkippedSemOwned = FALSE;
    }

    return (ulrc);
}

/*
 *@@ xsdLongTitle:
 *      creates a descriptive string in pszTitle from pItem
 *      (for the main (debug) window listbox).
 */

void xsdLongTitle(PSZ pszTitle, PSHUTLISTITEM pItem)
{
    sprintf(pszTitle, "%s%s",
            pItem->swctl.szSwtitle,
            (pItem->swctl.uchVisibility == SWL_VISIBLE)
                ? (", visible")
                : ("")
        );

    strcat(pszTitle, ", ");
    switch (pItem->swctl.bProgType)
    {
        case PROG_DEFAULT: strcat(pszTitle, "default"); break;
        case PROG_FULLSCREEN: strcat(pszTitle, "OS/2 FS"); break;
        case PROG_WINDOWABLEVIO: strcat(pszTitle, "OS/2 win"); break;
        case PROG_PM:
            strcat(pszTitle, "PM, class: ");
            strcat(pszTitle, pItem->szClass);
        break;
        case PROG_VDM: strcat(pszTitle, "VDM"); break;
        case PROG_WINDOWEDVDM: strcat(pszTitle, "VDM win"); break;
        default:
            sprintf(pszTitle+strlen(pszTitle), "? (%lX)");
        break;
    }
    sprintf(pszTitle+strlen(pszTitle),
            ", hwnd: 0x%lX, pid: 0x%lX, sid: 0x%lX, pObj: 0x%lX",
            (ULONG)pItem->swctl.hwnd,
            (ULONG)pItem->swctl.idProcess,
            (ULONG)pItem->swctl.idSession,
            (ULONG)pItem->pObject
        );
}

/*
 *@@ xsdQueryCurrentItem:
 *      returns the next PSHUTLISTITEM to be
 *      closed (skipping the items that were
 *      marked to be skipped).
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 */

PSHUTLISTITEM xsdQueryCurrentItem(VOID)
{
    CHAR            szShutItem[1000],
                    szSkipItem[1000];
    BOOL            fShutdownSemOwned = FALSE,
                    fSkippedSemOwned = FALSE;
    PSHUTLISTITEM   pliShutItem = 0;

    TRY_QUIET(excpt1, NULL)
    {
        fShutdownSemOwned = (DosRequestMutexSem(hmtxShutdown, 4000) == NO_ERROR);
        fSkippedSemOwned = (DosRequestMutexSem(hmtxSkipped, 4000) == NO_ERROR);

        if ((fShutdownSemOwned) && (fSkippedSemOwned))
        {
            PLISTNODE pShutNode = lstQueryFirstNode(pllShutdown);
            // pliShutItem = pliShutdownFirst;
            while (pShutNode)
            {
                PLISTNODE pSkipNode = lstQueryFirstNode(pllSkipped);
                pliShutItem = pShutNode->pItemData;

                while (pSkipNode)
                {
                    PSHUTLISTITEM pliSkipItem = pSkipNode->pItemData;
                    xsdLongTitle(szShutItem, pliShutItem);
                    xsdLongTitle(szSkipItem, pliSkipItem);
                    if (strcmp(szShutItem, szSkipItem) == 0)
                        /* current shut item is on skip list:
                           break (==> take next shut item */
                        break;

                    pSkipNode = pSkipNode->pNext;
                    pliSkipItem = 0;
                }

                if (pSkipNode == NULL)
                    // current item is not on the skip list:
                    // return this item
                    break;

                // current item was skipped: take next one
                pShutNode = pShutNode->pNext;
                pliShutItem = 0;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fShutdownSemOwned) {
        DosReleaseMutexSem(hmtxShutdown);
        fShutdownSemOwned = FALSE;
    }
    if (fSkippedSemOwned) {
        DosReleaseMutexSem(hmtxSkipped);
        fSkippedSemOwned = FALSE;
    }

    return (pliShutItem);
}

/*
 *@@ xsdAppendShutListItem:
 *      this appends a new PSHUTLISTITEM to the given list
 *      and returns the address of the new item; the list
 *      to append to must be specified in *ppFirst / *ppLast.
 *
 *      NOTE: It is entirely the job of the caller to serialize
 *      access to the list, using mutex semaphores.
 *      The item to add is to be specified by swctl and possibly
 *      *pObject (if swctl describes an open WPS object).
 *
 *      Since this gets called from xsdBuildShutList, this runs
 *      on both the Shutdown and Update threads.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 */

PSHUTLISTITEM xsdAppendShutListItem(PLINKLIST pList,   // in/out: linked list to work on
                                    SWCNTRL swctl,     // in: tasklist entry to add
                                    WPObject *pObject) // in: !=NULL: WPS object
{
    PSHUTLISTITEM  pNewItem = NULL;

    pNewItem = malloc(sizeof(SHUTLISTITEM));

    if (pNewItem)
    {
        pNewItem->pObject = pObject;
        pNewItem->swctl = swctl;

        strcpy(pNewItem->szClass, "unknown");

        if (pObject)
            // for WPS objects, store additional data
            if (wpshCheckObject(pObject))
            {
                strncpy(pNewItem->szClass, (PSZ)_somGetClassName(pObject), sizeof(pNewItem->szClass)-1);

                pNewItem->swctl.szSwtitle[0] = '\0';
                strncpy(pNewItem->swctl.szSwtitle, _wpQueryTitle(pObject), sizeof(pNewItem->swctl.szSwtitle)-1);

                // always set PID and SID to that of the WPS,
                // because the tasklist returns garbage for
                // WPS objects
                pNewItem->swctl.idProcess = pidWPS;
                pNewItem->swctl.idSession = sidWPS;

                // set HWND to object-in-use-list data,
                // because the tasklist also returns garbage
                // for that
                if (_wpFindUseItem(pObject, USAGE_OPENVIEW, NULL))
                    pNewItem->swctl.hwnd = (_wpFindViewItem(pObject,
                                                            VIEW_ANY,
                                                            NULL))->handle;
            }
            else
                // invalid object:
                pNewItem->pObject = NULL;
        else {
            // no WPS object: get window class name
            WinQueryClassName(swctl.hwnd,
                sizeof(pNewItem->szClass)-1,
                pNewItem->szClass);
        }

        // append to list
        lstAppendItem(pList, pNewItem);
    }

    return (pNewItem);
}

/*
 *@@ xsdBuildShutList:
 *      this routine builds a new ShutList by evaluating the
 *      system task list; this list is built in pList.
 *      NOTE: It is entirely the job of the caller to serialize
 *      access to the list, using mutex semaphores.
 *      We call xsdAppendShutListItem for each task list entry,
 *      if that entry is to be closed, so we're doing a few checks.
 *
 *      This gets called from both xsdUpdateListBox (on the
 *      Shutdown thread) and the Update thread (fntUpdateThread)
 *      directly.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 *@@changed V0.9.0 [umoeller]: PSHUTDOWNPARAMS added to prototype
 */

void xsdBuildShutList(PSHUTDOWNPARAMS psdp,   // in: shutdown parameters
                      PLINKLIST pList)
{
    PSWBLOCK        pSwBlock   = NULL;         // Pointer to information returned
    ULONG           ul,
                    cbItems    = 0,            // Number of items in list
                    ulBufSize  = 0;            // Size of buffer for information

    HAB             habDesktop = WinQueryAnchorBlock(HWND_DESKTOP);
    CHAR            szSwUpperTitle[100];
    WPObject        *pObj;
    BOOL            Append;

    // get all the tasklist entries into a buffer
    cbItems = WinQuerySwitchList(NULLHANDLE, NULL, 0);
    ulBufSize = (cbItems * sizeof(SWENTRY)) + sizeof(HSWITCH);
    pSwBlock = (PSWBLOCK)malloc(ulBufSize);
    cbItems = WinQuerySwitchList(NULLHANDLE, pSwBlock, ulBufSize);

    // loop through all the tasklist entries
    for (ul = 0; ul < (pSwBlock->cswentry); ul++)
    {
        strcpy(szSwUpperTitle, pSwBlock->aswentry[ul].swctl.szSwtitle);
        WinUpper(habDesktop, 0, 0, szSwUpperTitle);

        if (pSwBlock->aswentry[ul].swctl.bProgType == PROG_DEFAULT) {
            // in this case, we need to find out what
            // type the program has ourselves
            // ULONG ulType = 0;
            PRCPROCESS prcp;
            // default for errors
            pSwBlock->aswentry[ul].swctl.bProgType = PROG_WINDOWABLEVIO;
            if (prcQueryProcessInfo(pSwBlock->aswentry[ul].swctl.idProcess, &prcp))
                // according to bsedos.h, the PROG_* types are identical
                // to the SSF_TYPE_* types, so we can use the data from
                // DosQProcStat
                pSwBlock->aswentry[ul].swctl.bProgType = prcp.ulSessionType;
        }

        // now we check which windows we add to the shutdown list
        if (    // skip if PID == 0:
                (pSwBlock->aswentry[ul].swctl.idProcess != 0)
                // skip the Shutdown windows:
             && (pSwBlock->aswentry[ul].swctl.hwnd != hwndMain)
             && (pSwBlock->aswentry[ul].swctl.hwnd != hwndVioDlg)
             && (pSwBlock->aswentry[ul].swctl.hwnd != hwndShutdownStatus)
                // skip invisible tasklist entries; this
                // includes a PMWORKPLACE cmd.exe:
             && (pSwBlock->aswentry[ul].swctl.uchVisibility == SWL_VISIBLE)
                // skip VAC debugger, which is probably debugging
                // PMSHELL.EXE
             && (strcmp(szSwUpperTitle, "ICSDEBUG.EXE") != 0)

             #ifdef DEBUG_SHUTDOWN
                    // if we're in debug mode, skip the PMPRINTF window
                    // because we want to see debug output
                 && (strcmp(szSwUpperTitle, "PMPRINTF - 2.56") != 0)
             #endif
           )
        {
            // add window:
            Append = TRUE;
            pObj = NULL;

            if (pSwBlock->aswentry[ul].swctl.bProgType == PROG_WINDOWEDVDM)
                // DOS/Win-OS/2 window: get real PID/SID, because
                // the tasklist contains false data
                WinQueryWindowProcess(pSwBlock->aswentry[ul].swctl.hwnd,
                    &(pSwBlock->aswentry[ul].swctl.idProcess),
                    &(pSwBlock->aswentry[ul].swctl.idSession));
            else if (pSwBlock->aswentry[ul].swctl.idProcess == pidWPS)
            {
                PCKERNELGLOBALS pKernelGlobals = krnQueryGlobals();

                // PID == Workplace Shell PID: get SOM pointer from hwnd
                pObj = _wpclsQueryObjectFromFrame(_WPDesktop,
                            pSwBlock->aswentry[ul].swctl.hwnd);
                if (!pObj) {
                    if (pSwBlock->aswentry[ul].swctl.hwnd == hwndActiveDesktop) {
                        pObj = pActiveDesktop;
                    }
                    else
                        Append = FALSE;
                }

                // check for special objects
                if (    (pObj == pActiveDesktop)
                     || (pObj == pKernelGlobals->pAwakeWarpCenter)
                   )
                    // neither store Desktop nor WarpCenter,
                    // because we will close these manually
                    // during shutdown; these two need special
                    // handling
                    Append = FALSE;
            }

            if (Append)
                // if we have (Restart WPS) && ~(close all windows), append
                // only open WPS objects and not all windows
                if ( (!(psdp->optWPSCloseWindows)) && (pObj == NULL) )
                    Append = FALSE;

            if (Append)
                xsdAppendShutListItem(pList,
                                      pSwBlock->aswentry[ul].swctl,
                                      pObj);
        }
    }

    free(pSwBlock);
}

/*
 *@@ xsdUpdateListBox:
 *      this routine builds a new PSHUTITEM list from the
 *      pointer to the pointer of the first item (*ppliShutdownFirst)
 *      by setting its value to xsdBuildShutList's return value;
 *      it also fills the listbox in the "main" window, which
 *      is only visible in Debug mode.
 *      But even if it's invisible, the listbox is used for closing
 *      windows. Ugly, but nobody can see it. ;-)
 *      If *ppliShutdownFirst is != NULL the old shutlist is cleared
 *      also.
 *
 *      Runs on the Shutdown thread.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 *@@changed V0.9.0 [umoeller]: PSHUTDOWNPARAMS added to prototype
 */

void xsdUpdateListBox(PSHUTDOWNPARAMS psdp,   // in: shutdown parameters
                      HWND hwndListbox)
{
    PSHUTLISTITEM   pItem;
    CHAR            szTitle[1024];

    BOOL            fSemOwned = FALSE;

    TRY_QUIET(excpt1, NULL)
    {
        fSemOwned = (DosRequestMutexSem(hmtxShutdown, 4000) == NO_ERROR);
        if (fSemOwned)
        {
            PLISTNODE pNode = 0;
            lstClear(pllShutdown);
            xsdBuildShutList(psdp, pllShutdown);

            // clear list box
            WinEnableWindowUpdate(hwndListbox, FALSE);
            WinSendMsg(hwndListbox, LM_DELETEALL, MPNULL, MPNULL);

            // and insert all list items as strings
            pNode = lstQueryFirstNode(pllShutdown);
            while (pNode)
            {
                pItem = pNode->pItemData;
                xsdLongTitle(szTitle, pItem);
                WinInsertLboxItem(hwndListbox, 0, szTitle);
                pNode = pNode->pNext;
            }
            WinEnableWindowUpdate(hwndListbox, TRUE);
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fSemOwned)
    {
        DosReleaseMutexSem(hmtxShutdown);
        fSemOwned = FALSE;
    }
}

/*
 *@@ fncbUpdateINIStatus:
 *      this is a callback func for prfhSaveINIs for
 *      updating the progress bar.
 *
 *      Runs on the Shutdown thread.
 */

MRESULT EXPENTRY fncbUpdateINIStatus(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    if (hwnd)
        WinSendMsg(hwnd, msg, mp1, mp2);
    return ((MPARAM)TRUE);
}

/*
 *@@ fncbINIError:
 *      callback func for prfhSaveINIs (/helpers/winh.c).
 *
 *      Runs on the Shutdown thread.
 */

MRESULT EXPENTRY fncbSaveINIError(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    return ((MRESULT)cmnMessageBox(HWND_DESKTOP,
                                   "XShutdown: Error",
                                  (PSZ)mp1,     // error text
                                  (ULONG)mp2)   // MB_ABORTRETRYIGNORE or something
       );
}

/* ******************************************************************
 *                                                                  *
 *   Global variables for Shutdown thread                           *
 *                                                                  *
 ********************************************************************/

// shutdown parameters
PSHUTDOWNPARAMS psdParams = NULL;

HAB             habShutdownThread;
HMQ             hmqShutdownThread;
PNLSSTRINGS     pNLSStrings = NULL;
HMODULE         hmodResource = NULLHANDLE;
// CHAR            szLog[2000] = "";   // for XSHUTDWN.LOG  // removed V0.9.0

// flags for whether we're currently owning semaphores
BOOL            fAwakeObjectsSemOwned = FALSE,
                fShutdownSemOwned = FALSE,
                fSkippedSemOwned = FALSE;

/* ******************************************************************
 *                                                                  *
 *   Shutdown thread                                                *
 *                                                                  *
 ********************************************************************/

/*
 *@@ fntShutdownThread:
 *      this is the main shutdown thread which is created by
 *      xfInitiateShutdown / xsdInitiateRestartWPS when shutdown is about
 *      to begin.
 *
 *      Parameters: this thread must be created using thrCreate
 *      (/helpers/threads.c), so it is passed a pointer
 *      to a THREADINFO structure. In that structure you
 *      must set ulData to point to a SHUTDOWNPARAMS structure.
 *
 *      Note: if you're trying to understand what's going on here,
 *      I recommend rebuilding XFolder with DEBUG_SHUTDOWN
 *      #define'd (see include/setup.h for that). This will allow you
 *      to switch XShutdown into "Debug" mode by holding down
 *      the "Shift" key while selecting "Shutdown" from the
 *      Desktop's context menu.
 *
 *      Shutdown / Restart WPS runs in the following phases:
 *
 *      1)  First, all necessary preparations are done, i.e. two
 *          windows are created (the status window with the progress
 *          bar and the "main" window, which is only visible in debug
 *          mode, but processes all the messages). These two windows
 *          daringly share the same msg proc (fnwpShutdown below),
 *          but receive different messages, so this shan't hurt.
 *
 *      2)  fntShutdownThread then remains in a standard PM message
 *          loop until shutdown is cancelled by the user or all
 *          windows have been closed.
 *          In both cases, fnwpShutdown posts a WM_QUIT then.
 *
 *          The order of msg processing in fntShutdownThread / fnwpShutdown
 *          is the following:
 *
 *          a)  Upon WM_INITDLG, fnwpShutdown will create the Update
 *              thread (xsd_fntUpdateThread below) and do nothing more.
 *              This Update thread is responsible for monitoring the
 *              task list; every time an item is closed (or even opened!),
 *              it will post a ID_SDMI_UPDATESHUTLIST command to fnwpShutdown,
 *              which will then start working again.
 *
 *          b)  ID_SDMI_UPDATESHUTLIST will update the list of currently
 *              open windows (which is not touched by any other thread)
 *              by calling xsdUpdateListBox.
 *              Unless we're in debug mode (where shutdown has to be
 *              started manually), the first time ID_SDMI_UPDATESHUTLIST
 *              is received, we will post ID_SDDI_BEGINSHUTDOWN (go to c)).
 *              Otherwise (subsequent calls), we post ID_SDMI_CLOSEITEM
 *              (go to e)).
 *
 *          c)  ID_SDDI_BEGINSHUTDOWN will begin processing the contents
 *              of the Shutdown folder. After this is done,
 *              ID_SDMI_BEGINCLOSINGITEMS is posted.
 *
 *          d)  ID_SDMI_BEGINCLOSINGITEMS will prepare closing all windows
 *              by setting flagClosingItems to TRUE and then post the
 *              first ID_SDMI_CLOSEITEM.
 *
 *          e)  ID_SDMI_CLOSEITEM will now undertake the necessary
 *              actions for closing the first / next item on the list
 *              of items to close, that is, post WM_CLOSE to the window
 *              or kill the process or whatever.
 *              If no more items are left to close, we post
 *              ID_SDMI_PREPARESAVEWPS (go to g)).
 *              Otherwise, after this, the Shutdown thread is idle.
 *
 *          f)  When the window has actually closed, the Update thread
 *              realizes this because the task list will have changed.
 *              The next ID_SDMI_UPDATESHUTLIST will be posted by the
 *              Update thread then. Go back to b).
 *
 *          g)  ID_SDMI_PREPARESAVEWPS will save the state of all currently
 *              awake WPS objects by using the list which was maintained
 *              by the Worker thread all the while during the whole WPS session.
 *
 *          i)  After this, ID_SDMI_FLUSHBUFFERS is posted, which will
 *              set fAllWindowsClosed to TRUE and post WM_QUIT, so that
 *              the PM message loop in fntShutdownThread will exit.
 *
 *      3)  Depending on whether fnwpShutdown set fAllWindowsClosed to
 *          TRUE, we will then actually restart the WPS or shut down the system
 *          or exit this thread (= shutdown cancelled), and the user may continue work.
 *
 *          Shutting down the system is done by calling xsdFinishShutdown,
 *          which will differentiate what needs to be done depending on
 *          what the user wants (new with V0.84).
 *          We will then either reboot the machine or run in an endless
 *          loop, if no reboot was desired, or call the functions for an
 *          APM 1.2 power-off in apm.c (V0.82).
 *
 *          When shutdown was cancelled by pressing the respective button,
 *          the Update thread is killed, all shutdown windows are closed,
 *          and then this thread also terminates.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 *@@changed V0.9.0 [umoeller]: changed shutdown logging to stdio functions (fprintf etc.)
 *@@changed V0.9.0 [umoeller]: code has been re-ordered for semaphore safety.
 *@@changed V0.9.1 (99-12-10) [umoeller]: extracted auto-close list code to xsdLoadAutoCloseItems
 */

void _Optlink fntShutdownThread(PVOID ulpti)
{
    PSZ             pszErrMsg = NULL;
    QMSG            qmsg;
    APIRET          arc;

    // get the THREADINFO structure;
    // ulData points to SHUTDOWNPARAMS
    PTHREADINFO     pti = (PTHREADINFO)ulpti;

    // exception-occured flag
    BOOL fExceptionOccured = FALSE;

    // flag for whether restarting the WPS after all this
    BOOL fRestartWPS = FALSE;

    // set the global XFolder threads structure pointer
    PCKERNELGLOBALS  pcKernelGlobals = krnQueryGlobals();

    // get shutdown params from thread info
    psdParams = (PSHUTDOWNPARAMS)pti->ulData;

    // set some global data for all the following
    pNLSStrings = cmnQueryNLSStrings();
    hmodResource = cmnQueryNLSModuleHandle(FALSE);

    fAllWindowsClosed = FALSE;
    fShutdownBegun = FALSE;
    fClosingApps = FALSE;

    hwndVioDlg = NULLHANDLE;
    strcpy(szVioTitle, "");

    if (habShutdownThread = WinInitialize(0))
    {
        if (hmqShutdownThread = WinCreateMsgQueue(habShutdownThread, 0))
        {
            WinCancelShutdown(hmqShutdownThread, TRUE);

            // open shutdown log file for writing, if enabled
            if (psdParams->optLog)
            {
                CHAR    szLogFileName[CCHMAXPATH];
                sprintf(szLogFileName, "%c:\\%s", doshQueryBootDrive(), XFOLDER_SHUTDOWNLOG);
                fileShutdownLog = fopen(szLogFileName, "a");  // text file, append
            }

            if (fileShutdownLog)
            {
                // write log header
                DATETIME DT;
                DosGetDateTime(&DT);
                fprintf(fileShutdownLog,
                        "\n\nXWorkplace Shutdown Log -- Date: %02d/%02d/%04d, Time: %02d:%02d:%02d\n",
                        DT.month, DT.day, DT.year,
                        DT.hours, DT.minutes, DT.seconds);
                fprintf(fileShutdownLog, "-----------------------------------------------------------\n");
                fprintf(fileShutdownLog, "\nXWorkplace version: %s\n", XFOLDER_VERSION);
                fprintf(fileShutdownLog, "\nShutdown thread started, TID: 0x%lX\n",
                        thrQueryID((PTHREADINFO)ulpti));
                fprintf(fileShutdownLog, "Settings: RestartWPS %s, Confirm %s, Reboot %s, WPSCloseWnds %s, CloseVIOs %s, APMPowerOff %s\n\n",
                        (psdParams->optRestartWPS) ? "ON" : "OFF",
                        (psdParams->optConfirm) ? "ON" : "OFF",
                        (psdParams->optReboot) ? "ON" : "OFF",
                        (psdParams->optWPSCloseWindows) ? "ON" : "OFF",
                        (psdParams->optAutoCloseVIO) ? "ON" : "OFF",
                        (psdParams->optAPMPowerOff) ? "ON" : "OFF");
            }

            // raise our own priority; we will
            // still use the REGULAR class, but
            // with the maximum delta, so we can
            // get above nasty (DOS?) sessions
            DosSetPriority(PRTYS_THREAD,
                           PRTYC_REGULAR,
                           PRTYD_MAXIMUM, // priority delta
                           0);

            TRY_LOUD(excpt1, NULL)
            {
                SWCNTRL     swctl;
                HSWITCH     hswitch;
                ULONG       ulKeyLength = 0,
                            ulAutoCloseItemsFound = 0;
                HPOINTER    hptrShutdown = WinLoadPointer(HWND_DESKTOP, hmodResource,
                                                          ID_SDICON);

                // create an event semaphore which signals to the Update thread
                // that the Shutlist has been updated by fnwpShutdown
                DosCreateEventSem(NULL,         // unnamed
                                  &hevUpdated,
                                  0,            // unshared
                                  FALSE);       // not posted

                // create mutex semaphores for linked lists
                if (hmtxShutdown == NULLHANDLE)
                {
                    DosCreateMutexSem("\\sem32\\ShutdownList",
                                      &hmtxShutdown, 0, FALSE);     // unnamed, unowned
                    DosCreateMutexSem("\\sem32\\SkippedList",
                                      &hmtxSkipped, 0, FALSE);      // unnamed, unowned
                }

                pllShutdown = lstCreate(TRUE);      // auto-free items
                pllSkipped = lstCreate(TRUE);       // auto-free items
                pllAutoClose = lstCreate(TRUE);     // auto-free items

                // set the global Desktop info
                pActiveDesktop = _wpclsQueryActiveDesktop(_WPDesktop);
                hwndActiveDesktop = _wpclsQueryActiveDesktopHWND(_WPDesktop);

                xsdLog("fntShutdownThread: Getting auto-close items from OS2.INI...\n");

                // check for auto-close items in OS2.INI
                // and build pliAutoClose list accordingly
                ulAutoCloseItemsFound = xsdLoadAutoCloseItems(pllAutoClose,
                                                              NULLHANDLE); // no list box

                xsdLog("Found %d auto-close items.\n", ulAutoCloseItemsFound);

                xsdLog("Creating shutdown windows...\n");

                // setup main (debug) window; this is hidden
                // if we're not in debug mode
                hwndMain = WinLoadDlg(HWND_DESKTOP, NULLHANDLE,
                                      fnwpShutdown,
                                      hmodResource,
                                      ID_SDD_MAIN,
                                      NULL);
                WinSendMsg(hwndMain,
                           WM_SETICON,
                           (MPARAM)hptrShutdown,
                            NULL);

                // query process and session IDs for
                //      a) the Workplace process
                WinQueryWindowProcess(hwndMain, &pidWPS, NULL);
                //      b) the PM process
                WinQueryWindowProcess(HWND_DESKTOP, &pidPM, NULL);
                sidPM = 1;  // should always be this, I hope

                // add ourselves to the tasklist
                swctl.hwnd = hwndMain;                  // window handle
                swctl.hwndIcon = hptrShutdown;               // icon handle
                swctl.hprog = NULLHANDLE;               // program handle
                swctl.idProcess = pidWPS;               // PID
                swctl.idSession = 0;                    // SID
                swctl.uchVisibility = SWL_VISIBLE;      // visibility
                swctl.fbJump = SWL_JUMPABLE;            // jump indicator
                WinQueryWindowText(hwndMain, sizeof(swctl.szSwtitle), (PSZ)&swctl.szSwtitle);
                swctl.bProgType = PROG_DEFAULT;         // program type

                hswitch = WinAddSwitchEntry(&swctl);
                WinQuerySwitchEntry(hswitch, &swctl);
                sidWPS = swctl.idSession;               // get the "real" WPS SID

                // setup status window (always visible)
                hwndShutdownStatus = WinLoadDlg(HWND_DESKTOP, NULLHANDLE,
                                                fnwpShutdown,
                                                hmodResource,
                                                ID_SDD_STATUS,
                                                NULL);

                // set an icon for the status window
                WinSendMsg(hwndShutdownStatus,
                           WM_SETICON,
                           (MPARAM)hptrShutdown,
                           NULL);

                // subclass the static rectangle control in the dialog to make
                // it a progress bar
                ctlProgressBarFromStatic(WinWindowFromID(hwndShutdownStatus,
                                                         ID_SDDI_PROGRESSBAR),
                                         PBA_ALIGNCENTER | PBA_BUTTONSTYLE);
                WinShowWindow(hwndShutdownStatus, TRUE);
                WinSetActiveWindow(HWND_DESKTOP, hwndShutdownStatus);

                // animate the traffic light
                xsdLoadAnimation(&sdAnim);
                ctlPrepareAnimation(WinWindowFromID(hwndShutdownStatus,
                                                    ID_SDDI_ICON),
                                    XSD_ANIM_COUNT,
                                    &(sdAnim.ahptr[0]),
                                    150,    // delay
                                    TRUE);  // start now

                if (psdParams->optDebug)
                {
                    // debug mode: show "main" window, which
                    // is invisible otherwise
                    winhCenterWindow(hwndMain);
                    WinShowWindow(hwndMain, TRUE);
                }

                xsdLog("Now entering shutdown message loop...\n");

                /*
                 * Standard PM message loop:
                 *
                 */

                // now enter the common message loop for the main (debug) and
                // status windows (fnwpShutdown); this will keep running
                // until closing all windows is complete or cancelled, upon
                // which fnwpShutdown will post WM_QUIT
                while (WinGetMsg(habShutdownThread, &qmsg, NULLHANDLE, 0, 0))
                    WinDispatchMsg(habShutdownThread, &qmsg);

                xsdLog("fntShutdownThread: Done with message loop. Left fnwpShutdown.\n");

            } // end TRY_LOUD(excpt1
            CATCH(excpt1)
            {
                // exception occured:
                krnUnlockGlobals();     // just to make sure
                fExceptionOccured = TRUE;

                if (pszErrMsg == NULL)
                {
                    // only report the first error, or otherwise we will
                    // jam the system with msg boxes
                    pszErrMsg = malloc(2000);
                    if (pszErrMsg)
                    {
                        strcpy(pszErrMsg, "An error occured in the XFolder Shutdown thread. "
                                "In the root directory of your boot drive, you will find a "
                                "file named XFLDTRAP.LOG, which contains debugging information. "
                                "If you had shutdown logging enabled, you will also find the "
                                "file XSHUTDWN.LOG there. If not, please enable shutdown "
                                "logging in the Desktop's settings notebook. "
                                "\n\nThe XShutdown procedure will be terminated now. We can "
                                "now also restart the Workplace Shell. This is recommended if "
                                "your Desktop has already been closed or if "
                                "the error occured during the saving of the INI files. In these "
                                "cases, please disable XShutdown and perform a regular OS/2 "
                                "shutdown to prevent loss of your WPS data."
                                "\n\nRestart the Workplace Shell now?");
                        krnPostThread1ObjectMsg(T1M_EXCEPTIONCAUGHT, (MPARAM)pszErrMsg,
                                                (MPARAM)1); // enforce WPS restart

                        xsdLog("\n*** CRASH\n%s\n", pszErrMsg);
                    }
                }
            } END_CATCH();

            /*
             * Cleanup:
             *
             */

            // we arrive here if
            //      a) fnwpShutdown successfully closed all windows;
            //         only in that case, fAllWindowsClosed is TRUE;
            //      b) shutdown was cancelled by the user;
            //      c) an exception occured.
            // In any of these cases, we need to clean up big time now.

            // close "main" window, but keep the status window for now
            WinDestroyWindow(hwndMain);

            xsdLog("fntShutdownThread: Entering cleanup...\n");

            {
                PKERNELGLOBALS pKernelGlobals = krnLockGlobals(5000);
                // check for whether we're owning semaphores;
                // if we do (e.g. after an exception), release them now
                if (fAwakeObjectsSemOwned)
                {
                    xsdLog("  Releasing awake-objects mutex\n");
                    DosReleaseMutexSem(pKernelGlobals->hmtxAwakeObjects);
                    fAwakeObjectsSemOwned = FALSE;
                }
                if (fShutdownSemOwned)
                {
                    xsdLog("  Releasing shutdown mutex\n");
                    DosReleaseMutexSem(hmtxShutdown);
                    fShutdownSemOwned = FALSE;
                }
                if (fSkippedSemOwned)
                {
                    xsdLog("  Releasing skipped mutex\n");
                    DosReleaseMutexSem(hmtxSkipped);
                    fSkippedSemOwned = FALSE;
                }

                xsdLog("  Done releasing semaphores.\n");

                // get rid of the Update thread;
                // this got closed by fnwpShutdown normally,
                // but with exceptions, this might not have happened
                if (thrQueryID(pKernelGlobals->ptiUpdateThread))
                {
                    xsdLog("  Closing Update thread...\n");
                    thrFree(&(pKernelGlobals->ptiUpdateThread)); // fixed V0.9.0
                    xsdLog("  Update thread closed.\n");
                }

                krnUnlockGlobals();
            }

            xsdLog("  Closing semaphores...\n");
            DosCloseEventSem(hevUpdated);
            if (hmtxShutdown != NULLHANDLE)
            {
                arc = DosCloseMutexSem(hmtxShutdown);
                if (arc)
                {
                    DosBeep(100, 1000);
                    xsdLog("    Error %d closing hmtxShutdown!\n",
                           arc);
                }
                hmtxShutdown = NULLHANDLE;
            }
            if (hmtxSkipped != NULLHANDLE)
            {
                arc = DosCloseMutexSem(hmtxSkipped);
                if (arc)
                {
                    DosBeep(100, 1000);
                    xsdLog("    Error %d closing hmtxSkipped!\n",
                           arc);
                }
                hmtxSkipped = NULLHANDLE;
            }
            xsdLog("  Done closing semaphores.\n");

            xsdLog("  Freeing lists...\n");
            TRY_LOUD(excpt1, NULL)
            {
                // destroy all global lists; this time, we need
                // no mutex semaphores, because the Update thread
                // is already down
                lstFree(pllShutdown);
                lstFree(pllSkipped);
            }
            CATCH(excpt1) {} END_CATCH();
            xsdLog("  Done freeing lists.\n");

            /*
             * Restart WPS or shutdown:
             *
             */

            if (fAllWindowsClosed)
            {
                // (fAllWindowsClosed = TRUE) only if shutdown was
                // not cancelled; this means that all windows have
                // been successfully closed and we can actually shut
                // down the system

                if (psdParams->optRestartWPS)
                {
                    // here we will actually restart the WPS
                    xsdLog("Preparing WPS restart...\n");

                    ctlStopAnimation(WinWindowFromID(hwndShutdownStatus, ID_SDDI_ICON));
                    WinDestroyWindow(hwndShutdownStatus);
                    xsdFreeAnimation(&sdAnim);

                    // set flag for restarting the WPS after cleanup (V0.9.0)
                    fRestartWPS = TRUE;
                }
                else
                {
                    // *** no restart WPS:
                    // call the termination routine, which
                    // will do the rest

                    xsdFinishShutdown();
                    // this will not return, except in debug mode

                    if (psdParams->optDebug)
                        // in debug mode, restart WPS
                        fRestartWPS = TRUE;     // V0.9.0
                }
            } // end if (fAllWindowsClosed)

            // the following code is only reached when shutdown was cancelled

            WinDestroyWindow(hwndShutdownStatus);
            WinDestroyMsgQueue(hmqShutdownThread);
        } // end if (hmqShutdownThread = WinCreateMsgQueue(habShutdownThread, 0))

        WinTerminate(habShutdownThread);
    } // end if (habShutdownThread = WinInitialize(0))

    // clean up on the way out

    // free shutdown parameters structure (thread parameter)
    if (psdParams)
    {
        free(psdParams);
        psdParams = NULL;
    }

    if (fRestartWPS)
    {
        if (fileShutdownLog)
        {
            xsdLog("Restarting WPS: Calling DosExit(), closing log.\n");
            fclose(fileShutdownLog);
            fileShutdownLog = NULL;
        }
        xsdRestartWPS();
        // this will not return
    }

    {
        PKERNELGLOBALS pKernelGlobals = krnLockGlobals(5000);
        // set the global flag for whether shutdown is
        // running to FALSE; this will re-enable the
        // items in the Desktop's context menu
        pKernelGlobals->fShutdownRunning = FALSE;
        krnUnlockGlobals();
    }

    // close logfile
    if (fileShutdownLog)
    {
        xsdLog("Reached cleanup, closing log.\n");
        fclose(fileShutdownLog);
        fileShutdownLog = NULL;
    }

    // end of Shutdown thread
    thrGoodbye(pti);

    // thread exits!
}

/*
 *@@ fncbShutdown:
 *      callback function for XFolder::xwpBeginProcessOrderedContent
 *      when processing Shutdown folder.
 *
 *      Runs on thread 1.
 */

ULONG   hPOC;

MRESULT EXPENTRY fncbShutdown(HWND hwndStatus, ULONG ulObject, MPARAM mpNow, MPARAM mpMax)
{
    CHAR szStarting2[200];
    HWND rc = 0;
    if (ulObject)
    {
        WinSetActiveWindow(HWND_DESKTOP, hwndStatus);
        sprintf(szStarting2,
                (cmnQueryNLSStrings())->pszStarting,
                _wpQueryTitle((WPObject*)ulObject));
        WinSetDlgItemText(hwndStatus, ID_SDDI_STATUS, szStarting2);

        rc = _wpViewObject((WPObject*)ulObject, 0, OPEN_DEFAULT, 0);

        xsdLog("      ""%s""\n", szStarting2);

        WinSetActiveWindow(HWND_DESKTOP, hwndStatus);

        WinSendMsg(WinWindowFromID(hwndStatus, ID_SDDI_PROGRESSBAR),
                   WM_UPDATEPROGRESSBAR, mpNow, mpMax);
    }
    else
    {
        WinPostMsg(hwndStatus, WM_COMMAND,
                   MPFROM2SHORT(ID_SDMI_BEGINCLOSINGITEMS, 0),
                   MPNULL);
        hPOC = 0;
    }
    return ((MRESULT)rc);
}

/*
 *@@ xsdUpdateClosingStatus:
 *      this gets called from fnwpShutdown to
 *      set the Shutdown status wnd text to "Closing xxx".
 *
 *      Runs on the Shutdown thread.
 */

VOID xsdUpdateClosingStatus(PSZ pszProgTitle)   // in: window title from SHUTLISTITEM
{
    CHAR szTitle[300];
    strcpy(szTitle,
        (cmnQueryNLSStrings())->pszSDClosing);
    strcat(szTitle, " \"");
    strcat(szTitle, pszProgTitle);
    strcat(szTitle, "\"...");
    WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
        szTitle);

    WinSetActiveWindow(HWND_DESKTOP, hwndShutdownStatus);
}

ULONG       ulAwakeNow, ulAwakeMax;

/*
 *@@ xsdCloseVIO:
 *      this gets called upon ID_SDMI_CLOSEVIO in
 *      fnwpShutdown when a VIO window is encountered.
 *      This function queries the list of auto-close
 *      items and closes the VIO window accordingly.
 *      If no corresponding item was found, we display
 *      a dialog, querying the user what to do with this.
 *
 *      Runs on the Shutdown thread.
 *
 *@@added V0.9.1 (99-12-10) [umoeller]
 */

VOID xsdCloseVIO(HWND hwndFrame)
{
    PSHUTLISTITEM   pItem;
    ULONG           ulReply;
    PAUTOCLOSELISTITEM pliAutoCloseFound = NULL; // fixed V0.9.0
    ULONG           ulSessionAction = 0;
                        // this will become one of the ACL_* flags
                        // if something is to be done with this session

    xsdLog("  ID_SDMI_CLOSEVIO, hwnd: 0x%lX; entering xsdCloseVIO\n", hwndFrame);

    // get VIO item to close
    pItem = xsdQueryCurrentItem();

    if (pItem)
    {
        // valid item: go thru list of auto-close items
        // if this item is on there
        PLISTNODE   pAutoCloseNode = 0;
        xsdLongTitle(szVioTitle, pItem);
        VioItem = *pItem;
        xsdLog("    xsdCloseVIO: VIO item: %s\n", szVioTitle);

        // activate VIO window
        WinSetActiveWindow(HWND_DESKTOP, VioItem.swctl.hwnd);

        // check if VIO window is on auto-close list
        pAutoCloseNode = lstQueryFirstNode(pllAutoClose);
        while (pAutoCloseNode)
        {
            PAUTOCLOSELISTITEM pliThis = pAutoCloseNode->pItemData;
            xsdLog("      Checking %s\n", pliThis->szItemName);
            // compare first characters
            if (strnicmp(VioItem.swctl.szSwtitle,
                         pliThis->szItemName,
                         strlen(pliThis->szItemName))
                   == 0)
            {
                // item found:
                xsdLog("        Matching item found, auto-closing item\n");
                pliAutoCloseFound = pliThis;
                break;
            }

            pAutoCloseNode = pAutoCloseNode->pNext;
        }

        if (pliAutoCloseFound)
        {
            // item was found on auto-close list:
            xsdLog("    Found on auto-close list\n");

            // store this item's action for later
            ulSessionAction = pliAutoCloseFound->usAction;
        } // end if (pliAutoCloseFound)
        else
        {
            // not on auto-close list:
            if (psdParams->optAutoCloseVIO)
            {
                // auto-close enabled globally though:
                xsdLog("    Not found on auto-close list, auto-close is on:\n");

                // "auto close VIOs" is on
                if (VioItem.swctl.idSession == 1)
                {
                    // for some reason, DOS/Windows sessions always
                    // run in the Shell process, whose SID == 1
                    WinPostMsg((WinWindowFromID(VioItem.swctl.hwnd, FID_SYSMENU)),
                               WM_SYSCOMMAND,
                               (MPARAM)SC_CLOSE, MPNULL);
                    xsdLog("      Posted SC_CLOSE to hwnd 0x%lX\n",
                           WinWindowFromID(VioItem.swctl.hwnd, FID_SYSMENU));
                }
                else
                {
                    // OS/2 windows: kill
                    DosKillProcess(DKP_PROCESS, VioItem.swctl.idProcess);
                    xsdLog("      Killed pid 0x%lX\n",
                           VioItem.swctl.idProcess);
                }
            } // end if (psdParams->optAutoCloseVIO)
            else
            {
                CHAR            szText[500];

                // no auto-close: confirmation wnd
                xsdLog("    Not found on auto-close list, auto-close is off, query-action dlg:\n");

                cmnSetHelpPanel(ID_XFH_CLOSEVIO);
                hwndVioDlg = WinLoadDlg(HWND_DESKTOP, hwndShutdownStatus,
                                        fnwpDlgGeneric,
                                        cmnQueryNLSModuleHandle(FALSE),
                                        ID_SDD_CLOSEVIO,
                                        NULL);

                // ID_SDDI_VDMAPPTEXT has ""cannot be closed automatically";
                // prefix session title
                strcpy(szText, "\"");
                strcat(szText, VioItem.swctl.szSwtitle);
                WinQueryDlgItemText(hwndVioDlg, ID_SDDI_VDMAPPTEXT,
                                    100, &(szText[strlen(szText)]));
                WinSetDlgItemText(hwndVioDlg, ID_SDDI_VDMAPPTEXT,
                                  szText);

                cmnSetControlsFont(hwndVioDlg, 1, 5000);
                winhCenterWindow(hwndVioDlg);
                ulReply = WinProcessDlg(hwndVioDlg);

                if (ulReply == DID_OK)
                {
                    xsdLog("      'OK' pressed\n");

                    // "OK" button pressed: check the radio buttons
                    // for what to do with this session
                    if (winhIsDlgItemChecked(hwndVioDlg, ID_XSDI_ACL_SKIP))
                    {
                        ulSessionAction = ACL_SKIP;
                        xsdLog("      xsdCloseVIO: 'Skip' selected\n");
                    }
                    else if (winhIsDlgItemChecked(hwndVioDlg, ID_XSDI_ACL_WMCLOSE))
                    {
                        ulSessionAction = ACL_WMCLOSE;
                        xsdLog("      xsdCloseVIO: 'WM_CLOSE' selected\n");
                    }
                    else if (winhIsDlgItemChecked(hwndVioDlg, ID_XSDI_ACL_KILLSESSION))
                    {
                        ulSessionAction = ACL_KILLSESSION;
                        xsdLog("      xsdCloseVIO: 'Kill' selected\n");
                    }

                    // "store item" checked?
                    if (ulSessionAction)
                        if (winhIsDlgItemChecked(hwndVioDlg, ID_XSDI_ACL_STORE))
                        {
                            ULONG ulInvalid = 0;
                            // "store" checked:
                            // add item to list
                            PAUTOCLOSELISTITEM pliNew = malloc(sizeof(AUTOCLOSELISTITEM));
                            strncpy(pliNew->szItemName,
                                    VioItem.swctl.szSwtitle,
                                    sizeof(pliNew->szItemName)-1);
                            pliNew->szItemName[99] = 0;
                            pliNew->usAction = ulSessionAction;
                            lstAppendItem(pllAutoClose,
                                          pliNew);

                            // write list back to OS2.INI
                            ulInvalid = xsdWriteAutoCloseItems(pllAutoClose);
                            xsdLog("         Updated auto-close list in OS2.INI, rc: %d\n",
                                        ulInvalid);
                        }
                }
                else if (ulReply == ID_SDDI_CANCELSHUTDOWN)
                {
                    // "Cancel shutdown" pressed:
                    // pass to main window
                    WinPostMsg(hwndMain, WM_COMMAND,
                               MPFROM2SHORT(ID_SDDI_CANCELSHUTDOWN, 0),
                               MPNULL);
                    xsdLog("      'Cancel shutdown' pressed\n");
                }
                // else: we could also get DID_CANCEL; this means
                // that the dialog was closed because the Update
                // thread determined that a session was closed
                // manually, so we just do nothing...

                WinDestroyWindow(hwndVioDlg);
                hwndVioDlg = NULLHANDLE;
            } // end else (optAutoCloseVIO)
        }

        xsdLog("      xsdCloseVIO: ulSessionAction is %d\n", ulSessionAction);

        // OK, let's see what to do with this session
        switch (ulSessionAction)
                    // this is 0 if nothing is to be done
        {
            case ACL_WMCLOSE:
                WinPostMsg((WinWindowFromID(VioItem.swctl.hwnd, FID_SYSMENU)),
                           WM_SYSCOMMAND,
                           (MPARAM)SC_CLOSE, MPNULL);
                xsdLog("      xsdCloseVIO: Posted SC_CLOSE to sysmenu, hwnd: 0x%lX\n",
                       WinWindowFromID(VioItem.swctl.hwnd, FID_SYSMENU));
            break;

            case ACL_CTRL_C:
                DosSendSignalException(VioItem.swctl.idProcess,
                                       XCPT_SIGNAL_INTR);
                xsdLog("      xsdCloseVIO: Sent INTR signal to pid 0x%lX\n",
                       VioItem.swctl.idProcess);
            break;

            case ACL_KILLSESSION:
                DosKillProcess(DKP_PROCESS, VioItem.swctl.idProcess);
                xsdLog("      xsdCloseVIO: Killed pid 0x%lX\n",
                       VioItem.swctl.idProcess);
            break;

            case ACL_SKIP:
                WinPostMsg(hwndMain, WM_COMMAND,
                           MPFROM2SHORT(ID_SDDI_SKIPAPP, 0),
                           MPNULL);
                xsdLog("      xsdCloseVIO: Posted ID_SDDI_SKIPAPP\n");
            break;
        }
    }

    xsdLog("  Done with xsdCloseVIO\n");
}

/*
 *@@ fnwpShutdown:
 *      window procedure for both the main (debug) window and
 *      the status window; it receives messages from the Update
 *      threads, so that it can update the windows' contents
 *      accordingly; it also controls this thread by suspending
 *      or killing it, if necessary, and setting semaphores.
 *      Note that the main (debug) window with the listbox is only
 *      visible in debug mode (signalled to xfInitiateShutdown).
 *
 *      Runs on the Shutdown thread.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist.c functions
 *@@changed V0.9.0 [umoeller]: fixed some inconsistensies in ID_SDMI_CLOSEVIO
 *@@changed V0.9.0 [umoeller]: changed shutdown logging to stdio functions (fprintf etc.)
 *@@changed V0.9.0 [umoeller]: changed "reuse WPS startup folder" to use DAEMONSHARED
 *@@changed V0.9.0 [umoeller]: added xsdFlushWPS2INI call
 *@@changed V0.9.1 (99-12-10) [umoeller]: extracted VIO code to xsdCloseVIO
 */

MRESULT EXPENTRY fnwpShutdown(HWND hwndFrame, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT         mrc = MRFALSE;
    PSHUTLISTITEM   pItem;
    HWND            hwndListbox = WinWindowFromID(hwndMain,
                                                  ID_SDDI_LISTBOX);


    switch(msg)
    {
        case WM_INITDLG:
        case WM_CREATE:
        {
            if (fileShutdownLog)
            {
                CHAR szTemp[2000];
                WinQueryWindowText(hwndFrame, sizeof(szTemp), szTemp);
                xsdLog("  WM_INITDLG/WM_CREATE (%s, hwnd: 0x%lX)\n",
                       szTemp,
                       hwndFrame);
                xsdLog("    HAB: 0x%lX, HMQ: 0x%lX, pidWPS: 0x%lX, pidPM: 0x%lX\n",
                       habShutdownThread, hmqShutdownThread, pidWPS, pidPM);
            }

            ulMaxItemCount = 0;
            ulLastItemCount = -1;

            hPOC = 0;

            {
                PKERNELGLOBALS pKernelGlobals = krnLockGlobals(5000);
                if (thrQueryID(pKernelGlobals->ptiUpdateThread) == NULLHANDLE)
                {
                    // first call; this one's called twice!
                    thrCreate(&(pKernelGlobals->ptiUpdateThread),
                              xsd_fntUpdateThread,
                              NULLHANDLE);

                    xsdLog("    Update thread started, tid: 0x%lX\n",
                           thrQueryID(pKernelGlobals->ptiUpdateThread));
                }
                krnUnlockGlobals();
            }
        break; }

        case WM_COMMAND:
        {
            switch (SHORT1FROMMP(mp1))
            {
                case ID_SDDI_BEGINSHUTDOWN:
                {
                    /* this is either posted by the "Begin shutdown"
                       button (in debug mode) or otherwise automatically
                       after the first update initiated by the Update
                       thread; we will look for a Shutdown folder first */

                    XFolder         *pShutdownFolder;

                    xsdLog("  ID_SDDI_BEGINSHUTDOWN, hwnd: 0x%lX\n", hwndFrame);

                    if (pShutdownFolder = _wpclsQueryFolder(_WPFolder, XFOLDER_SHUTDOWNID, TRUE))
                    {
                        xsdLog("    Processing shutdown folder...\n");

                        hPOC = _xwpBeginProcessOrderedContent(pShutdownFolder,
                                                             0, // wait mode
                                                             &fncbShutdown,
                                                             (ULONG)hwndShutdownStatus);

                        xsdLog("    Done processing shutdown filesys\n");
                    }
                    else
                        goto beginclosingitems;
                break; }

                case ID_SDMI_BEGINCLOSINGITEMS:
                beginclosingitems:
                {
                    /* this is posted after processing of the shutdown
                       folder; shutdown actually begins now with closing
                       all windows */
                    xsdLog("  ID_SDMI_BEGINCLOSINGITEMS, hwnd: 0x%lX\n", hwndFrame);

                    fClosingApps = TRUE;
                    WinShowWindow(hwndShutdownStatus, TRUE);
                    WinEnableControl(hwndMain, ID_SDDI_BEGINSHUTDOWN, FALSE);
                    WinPostMsg(hwndMain, WM_COMMAND,
                               MPFROM2SHORT(ID_SDMI_CLOSEITEM, 0),
                               MPNULL);
                break; }

                /*
                 * ID_SDMI_CLOSEITEM:
                 *     this msg is posted first upon receiving
                 *     ID_SDDI_BEGINSHUTDOWN and subsequently for every
                 *     window that is to be closed; we only INITIATE
                 *     closing the window here by posting messages
                 *     or killing the window; we then rely on the
                 *     update thread to realize that the window has
                 *     actually been removed from the Tasklist. The
                 *     Update thread then posts ID_SDMI_UPDATESHUTLIST,
                 *     which will then in turn post another ID_SDMI_CLOSEITEM
                 */

                case ID_SDMI_CLOSEITEM:
                {
                    xsdLog("  ID_SDMI_CLOSEITEM, hwnd: 0x%lX\n", hwndFrame);

                    // get task list item to close from linked list
                    pItem = xsdQueryCurrentItem();
                    if (pItem)
                    {
                        CHAR        szTitle[1024];
                        USHORT      usItem;

                        // compose string from item
                        xsdLongTitle(szTitle, pItem);

                        xsdLog("    Item: %s\n", szTitle);

                        // find string in the invisible list box
                        usItem = (USHORT)WinSendMsg(hwndListbox,
                                                    LM_SEARCHSTRING,
                                                    MPFROM2SHORT(0, LIT_FIRST),
                                                    (MPARAM)szTitle);
                        // and select it
                        if ((usItem != (USHORT)LIT_NONE))
                            WinPostMsg(hwndListbox,
                                       LM_SELECTITEM,
                                       (MPARAM)usItem,
                                       (MPARAM)TRUE);

                        // update status window: "Closing xxx"
                        xsdUpdateClosingStatus(pItem->swctl.szSwtitle);

                        // now check what kind of action needs to be done
                        if (pItem->pObject)
                        {   // we have a WPS window: use proper method
                            // to ensure that window data is saved
                            _wpClose(pItem->pObject);
                            xsdLog("      Open WPS object, called wpClose(pObject)\n");
                        }
                        else if (pItem->swctl.hwnd)
                        {
                            // no WPS window: differentiate further
                            xsdLog("      swctl.hwnd found: 0x%lX\n", pItem->swctl.hwnd);

                            if (    (pItem->swctl.bProgType == PROG_VDM)
                                 || (pItem->swctl.bProgType == PROG_WINDOWEDVDM)
                                 || (pItem->swctl.bProgType == PROG_FULLSCREEN)
                                 || (pItem->swctl.bProgType == PROG_WINDOWABLEVIO)
                                 // || (pItem->swctl.bProgType == PROG_DEFAULT)
                               )
                            {
                                // not a PM session: ask what to do (handled below)
                                xsdLog("      Seems to be VIO, swctl.bProgType: 0x%lX\n", pItem->swctl.bProgType);
                                if (    (hwndVioDlg == NULLHANDLE)
                                     || (strcmp(szTitle, szVioTitle) != 0)
                                   )
                                {
                                    // "Close VIO window" not currently open:
                                    xsdLog("      Posting ID_SDMI_CLOSEVIO\n");
                                    WinPostMsg(hwndMain, WM_COMMAND,
                                               MPFROM2SHORT(ID_SDMI_CLOSEVIO, 0),
                                               MPNULL);
                                }
                                /// else do nothing
                            }
                            else
                                // if (WinWindowFromID(pItem->swctl.hwnd, FID_SYSMENU))
                                // removed V0.9.0 (UM 99-10-22)
                            {
                                // window has system menu: close PM application;
                                // WM_SAVEAPPLICATION and WM_QUIT is what WinShutdown
                                // does too for every message queue per process
                                xsdLog("      Has system menu, posting WM_SAVEAPPLICATION to hwnd 0x%lX\n",
                                       pItem->swctl.hwnd);

                                WinPostMsg(pItem->swctl.hwnd,
                                           WM_SAVEAPPLICATION,
                                           MPNULL, MPNULL);

                                xsdLog("      Posting WM_QUIT to hwnd 0x%lX\n",
                                       pItem->swctl.hwnd);

                                WinPostMsg(pItem->swctl.hwnd,
                                           WM_QUIT,
                                           MPNULL, MPNULL);
                            }
                            /* else
                            {
                                // no system menu: try something more brutal
                                xsdLog("      Has no sys menu, posting WM_CLOSE to hwnd 0x%lX\n",
                                            pItem->swctl.hwnd);

                                WinPostMsg(pItem->swctl.hwnd,
                                           WM_CLOSE,
                                           MPNULL,
                                           MPNULL);
                            } */
                        }
                        else
                        {
                            xsdLog("      Helpless... leaving item alone, pid: 0x%lX\n", pItem->swctl.idProcess);
                            // DosKillProcess(DKP_PROCESS, pItem->swctl.idProcess);
                        }
                    } // end if (pItem)
                    else
                    {
                        // no more items left: enter phase 2 (save WPS)
                        xsdLog("    All items closed. Posting ID_SDMI_PREPARESAVEWPS\n");

                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_PREPARESAVEWPS, 0),
                                   MPNULL);    // first post
                    }
                break; }

                /*
                 * ID_SDDI_SKIPAPP:
                 *     comes from the "Skip" button
                 */

                case ID_SDDI_SKIPAPP:
                {
                    PSHUTLISTITEM   pSkipItem;
                    xsdLog("  ID_SDDI_SKIPAPP, hwnd: 0x%lX\n", hwndFrame);

                    pItem = xsdQueryCurrentItem();
                    if (pItem)
                    {
                        xsdLog("    Adding %s to the list of skipped items\n",
                               pItem->swctl.szSwtitle);

                        pSkipItem = malloc(sizeof(SHUTLISTITEM));
                        *pSkipItem = *pItem;

                        fSkippedSemOwned = (DosRequestMutexSem(hmtxSkipped, 4000) == NO_ERROR);
                        if (fSkippedSemOwned)
                        {
                            lstAppendItem(pllSkipped,
                                          pSkipItem);
                            DosReleaseMutexSem(hmtxSkipped);
                            fSkippedSemOwned = FALSE;
                        }
                    }
                    if (fClosingApps)
                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_UPDATEPROGRESSBAR, 0),
                                   MPNULL);
                break; }

                /*
                 * ID_SDMI_CLOSEVIO:
                 *     this is posted by ID_SDMI_CLOSEITEM when
                 *     a non-PM session is encountered; we will now
                 *     either close this session automatically or
                 *     open up a confirmation dlg
                 */

                case ID_SDMI_CLOSEVIO:
                {
                    xsdCloseVIO(hwndFrame);
                break; }

                /*
                 * ID_SDMI_UPDATESHUTLIST:
                 *    this cmd comes from the Update thread when
                 *    the task list has changed. This happens when
                 *    1)    something was closed by this function
                 *    2)    the user has closed something
                 *    3)    and even if the user has OPENED something
                 *          new.
                 *    We will then rebuilt the pliShutdownFirst list and
                 *    continue closing items, if Shutdown is currently in progress
                 */

                case ID_SDMI_UPDATESHUTLIST:
                {
                    #ifdef DEBUG_SHUTDOWN
                        DosBeep(10000, 50);
                    #endif

                    xsdLog("  ID_SDMI_UPDATESHUTLIST, hwnd: 0x%lX\n", hwndFrame);

                    xsdUpdateListBox(psdParams, hwndListbox);
                        // this updates the Shutdown linked list
                    DosPostEventSem(hevUpdated);
                        // signal update to Update thread

                    xsdLog("    Rebuilt shut list, %d items remaining\n",
                           xsdCountRemainingItems());

                    if (hwndVioDlg)
                    {
                        USHORT usItem;
                        // "Close VIO" confirmation window is open:
                        // check if the item that was to be closed
                        // is still in the listbox; if so, exit,
                        // if not, close confirmation window and
                        // continue
                        usItem = (USHORT)WinSendMsg(hwndListbox,
                                                    LM_SEARCHSTRING,
                                                    MPFROM2SHORT(0, LIT_FIRST),
                                                    (MPARAM)&szVioTitle);

                        if ((usItem != (USHORT)LIT_NONE))
                            break;
                        else
                        {
                            WinPostMsg(hwndVioDlg, WM_CLOSE, MPNULL, MPNULL);
                            // this will result in a DID_CANCEL return code
                            // for WinProcessDlg in ID_SDMI_CLOSEVIO above
                            xsdLog("    Closed open VIO confirm dlg' dlg\n");
                        }
                    }
                    goto updateprogressbar;
                    // continue with update progress bar
                }

                /*
                 * ID_SDMI_UPDATEPROGRESSBAR:
                 *     well, update the progress bar in the
                 *     status window
                 */

                case ID_SDMI_UPDATEPROGRESSBAR:
                updateprogressbar:
                {
                    ULONG           ulItemCount, ulMax, ulNow;

                    ulItemCount = xsdCountRemainingItems();
                    if (ulItemCount > ulMaxItemCount)
                    {
                        ulMaxItemCount = ulItemCount;
                        ulLastItemCount = -1; // enforce update
                    }

                    xsdLog("  ID_SDMI_UPDATEPROGRESSBAR, hwnd: 0x%lX, remaining: %d, total: %d\n",
                           hwndFrame, ulItemCount, ulMaxItemCount);

                    if ((ulItemCount) != ulLastItemCount)
                    {
                        ulMax = ulMaxItemCount;
                        ulNow = (ulMax - ulItemCount);

                        WinSendMsg(WinWindowFromID(hwndShutdownStatus, ID_SDDI_PROGRESSBAR),
                                   WM_UPDATEPROGRESSBAR,
                                   (MPARAM)ulNow, (MPARAM)ulMax);
                        ulLastItemCount = (ulItemCount);
                    }

                    if (fShutdownBegun == FALSE)
                        if (!(psdParams->optDebug))
                        {
                            /* if this is the first time we're here,
                               begin shutdown, unless we're in debug mode */
                            WinPostMsg(hwndMain, WM_COMMAND,
                                       MPFROM2SHORT(ID_SDDI_BEGINSHUTDOWN, 0),
                                       MPNULL);
                            fShutdownBegun = TRUE;
                            break;
                        }

                    if (fClosingApps)
                        /* if we're already in the process of shutting down, we will
                           initiate closing the next item */
                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_CLOSEITEM, 0),
                                   MPNULL);
                break; }

                case DID_CANCEL:
                case ID_SDDI_CANCELSHUTDOWN:
                    /* results from the "Cancel shutdown" buttons in both the
                       main (debug) and the status window; we set a semaphore
                       upon which the Update thread will terminate itself,
                       and WM_QUIT is posted to the main (debug) window, so that
                       the message loop in the main Shutdown thread function ends */
                    if (winhIsDlgItemEnabled(hwndShutdownStatus, ID_SDDI_CANCELSHUTDOWN))
                    {
                        xsdLog("  DID_CANCEL/ID_SDDI_CANCELSHUTDOWN, hwnd: 0x%lX\n");

                        if (hPOC)
                            ((PPROCESSCONTENTINFO)hPOC)->fCancelled = TRUE;
                        WinEnableControl(hwndShutdownStatus, ID_SDDI_CANCELSHUTDOWN, FALSE);
                        WinEnableControl(hwndShutdownStatus, ID_SDDI_SKIPAPP, FALSE);
                        WinEnableControl(hwndMain, ID_SDDI_BEGINSHUTDOWN, TRUE);
                        fClosingApps = FALSE;

                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_CLEANUPANDQUIT, 0),
                                   MPNULL);
                    }
                break;

            /* PHASE 2: after all windows have been closed,
               ID_SDMI_PREPARESAVEWPS is posted, so we will now
               call _wpSaveImmediate on all awake objects */

                /*
                 * ID_SDMI_PREPARESAVEWPS:
                 *      the first time, this will be posted with
                 *      mp2 == NULL, 2 or 3 afterwards
                 */

                case ID_SDMI_PREPARESAVEWPS:
                {
                    PCKERNELGLOBALS pKernelGlobals = krnQueryGlobals();
                    CHAR    szTitle[1000];
                    xsdLog("  ID_SDMI_PREPARESAVEWPS, hwnd: 0x%lX, mp2: 0x%lX\n", hwndFrame, mp2);

                    if (mp2 == (MPARAM)NULL)
                    {
                        // first post
                        WinSetActiveWindow(HWND_DESKTOP, hwndShutdownStatus);

                        WinEnableControl(hwndShutdownStatus, ID_SDDI_CANCELSHUTDOWN, FALSE);
                        WinEnableControl(hwndShutdownStatus, ID_SDDI_SKIPAPP, FALSE);


                        // close Desktop window (which we excluded from
                        // the regular folders list)
                        xsdUpdateClosingStatus(_wpQueryTitle(pActiveDesktop));
                        xsdLog("    Closing Desktop window\n");

                        _wpSaveImmediate(pActiveDesktop);
                        _wpClose(pActiveDesktop);

                        // close WarpCenter next
                        if (pKernelGlobals->pAwakeWarpCenter)
                        {
                            // WarpCenter open?
                            if (_wpFindUseItem(pKernelGlobals->pAwakeWarpCenter,
                                               USAGE_OPENVIEW,
                                               NULL)       // get first useitem
                                )
                            {
                                // if open: close it
                                xsdUpdateClosingStatus(_wpQueryTitle(pKernelGlobals->pAwakeWarpCenter));
                                xsdLog("    Closing WarpCenter\n");

                                _wpClose(pKernelGlobals->pAwakeWarpCenter);
                            }
                        }

                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_PREPARESAVEWPS, 0),
                                   (MPARAM)2);    // second post: mp2 == 2
                        // we need to post this msg twice, because
                        // otherwise the status wnd doesn't get updated
                        // properly
                    }
                    else if ((ULONG)mp2 == 2)
                    {
                        // mp2 == 2: second post,
                        // now prepare saving objects

                        // request the mutex sem for access to the
                        // awake objects list; we do not release
                        // this semaphore until we're done saving
                        // the WPS objects, which will prevent the
                        // Worker thread from messing with that
                        // list during that time
                        fAwakeObjectsSemOwned = (DosRequestMutexSem(pKernelGlobals->hmtxAwakeObjects,
                                                                    4000)
                                                    == NO_ERROR);
                        if (fAwakeObjectsSemOwned)
                            ulAwakeMax = lstCountItems(pKernelGlobals->pllAwakeObjects);

                        ulAwakeNow = 0;

                        sprintf(szTitle,
                                cmnQueryNLSStrings()->pszSDSavingDesktop,
                                    // "Saving xxx awake WPS objects..."
                                ulAwakeMax);
                        WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
                                          szTitle);

                        xsdLog("    Saving %d awake WPS objects...\n", ulAwakeMax);

                        // now we need a blank screen so that it looks
                        // as if we had closed all windows, even if we
                        // haven't; we do this by creating a "fake
                        // desktop", which is just an empty window w/out
                        // title bar which takes up the whole screen and
                        // has the color of the PM desktop
                        if (    (!(psdParams->optRestartWPS))
                             && (!(psdParams->optDebug))
                           )
                            winhCreateFakeDesktop(hwndShutdownStatus);

                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_PREPARESAVEWPS, 0),
                                   (MPARAM)3);    // third post: mp2 == 3
                    }
                    else if ((ULONG)mp2 == 3)
                    {
                        // third post:
                        // reset progress bar
                        WinSendMsg(WinWindowFromID(hwndShutdownStatus, ID_SDDI_PROGRESSBAR),
                                WM_UPDATEPROGRESSBAR,
                                (MPARAM)0, (MPARAM)ulAwakeMax);

                        // have the WPS flush its buffers
                        xsdFlushWPS2INI();  // added V0.9.0 (UM 99-10-22)

                        // and wait a while
                        DosSleep(500);

                        if (fAwakeObjectsSemOwned)
                        {
                            // finally, save WPS!!
                            WinPostMsg(hwndMain, WM_COMMAND,
                                       MPFROM2SHORT(ID_SDMI_SAVEWPSITEM, 0),
                                       // first item
                                       lstQueryFirstNode(pKernelGlobals->pllAwakeObjects));
                        }
                    }
                break; }

                /*
                 * ID_SDMI_SAVEWPSITEM:
                 *      this is then posted for each awake WPS object,
                 *      with the list node of the current
                 *      WPObject* in mp2 (changed with V0.9.0).
                 *      For the first post (above), this contains
                 *      the first object in the linked list of awake
                 *      WPS objects maintained by the Worker thread;
                 *      we can then just go for the next object (*pNext)
                 *      and post this message again.
                 */

                case ID_SDMI_SAVEWPSITEM:
                {
                    PCKERNELGLOBALS pKernelGlobals = krnQueryGlobals();
                    PLISTNODE pAwakeNode = (PLISTNODE)mp2;

                    /* now save all currently awake WPS objects; these will
                       be saved after the other windows are closed */


                    if (pAwakeNode)
                    {
                        WPObject *pAwakeObject = pAwakeNode->pItemData;
                        // mp2 != NULL: save that object;
                        // we install an exception handler because
                        // this crashes sometimes (V0.84) when
                        // the object no longer exists
                        TRY_QUIET(excpt2, NULL)
                        {
                            if (pAwakeObject)
                            {
                                // save neither Desktop nor WarpCenter
                                if (    (pAwakeObject != pActiveDesktop)
                                     && (pAwakeObject != pKernelGlobals->pAwakeWarpCenter)
                                   )
                                    // save object data synchroneously
                                    _wpSaveImmediate(pAwakeObject);
                            }
                        }
                        CATCH(excpt2)
                        {
                            // ignore exceptions. These occur only very
                            // rarely when the data in the awake objects
                            // list is no longer valid because the Worker
                            // thread hasn't kept up (it has idle priority).
                            DosBeep(10000, 10);
                        } END_CATCH();

                        ulAwakeNow++;
                        // post for next object
                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_SAVEWPSITEM, 0),
                                   pAwakeNode->pNext);
                                       // this is NULL for the last object
                    }
                    else
                    {
                        // mp2 == NULL: no more objects in list,
                        // so we're done

                        // free the list mutex semaphore
                        if (fAwakeObjectsSemOwned)
                        {
                            DosReleaseMutexSem(pKernelGlobals->hmtxAwakeObjects);
                            fAwakeObjectsSemOwned = FALSE;
                        }

                        xsdLog("    Done saving WPS\n");

                        WinPostMsg(hwndMain, WM_COMMAND,
                                   MPFROM2SHORT(ID_SDMI_FLUSHBUFFERS, 0),
                                   MPNULL);
                    }

                    WinSendMsg(WinWindowFromID(hwndShutdownStatus, ID_SDDI_PROGRESSBAR),
                               WM_UPDATEPROGRESSBAR,
                               (MPARAM)ulAwakeNow, (MPARAM)ulAwakeMax);
                break; }

             /* PHASE 3: after saving the WPS objects, ID_SDMI_FLUSHBUFFERS is
                posted, so we will now actually shut down the system or restart
                the WPS */

                case ID_SDMI_FLUSHBUFFERS:
                {
                    PKERNELGLOBALS pKernelGlobals = krnLockGlobals(5000);
                    xsdLog("  ID_SDMI_FLUSHBUFFERS, hwnd: 0x%lX\n", hwndFrame);

                    // close the Update thread to prevent it from interfering
                    // with what we're doing now
                    if (thrQueryID(pKernelGlobals->ptiUpdateThread))
                    {
                        #ifdef DEBUG_SHUTDOWN
                            WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
                                "Waiting for the Update thread to end...");
                        #endif
                        xsdLog("    fnwpShutdown: Closing Update thread, tid: 0x%lX...\n",
                               thrQueryID(pKernelGlobals->ptiUpdateThread));

                        thrFree(&(pKernelGlobals->ptiUpdateThread));  // close and wait
                        xsdLog("    fnwpShutdown: Update thread closed.\n");
                    }

                    if (psdParams->optRestartWPS)
                    {
                        // "Restart WPS" mode

                        WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
                                          (cmnQueryNLSStrings())->pszSDRestartingWPS);

                        // reuse startup folder?
                        krnSetProcessStartupFolder(psdParams->optWPSReuseStartupFolder);

                            /*
                            // delete the "lastpid" file in \bin,
                            // which will cause the startup folder to be
                            // processed by the Worker thread after the
                            // WPS restart
                            CHAR szPIDFile[CCHMAXPATH];
                            cmnQueryXFolderBasePath(szPIDFile);
                            strcat(szPIDFile, "\\bin\\lastpid");
                            remove(szPIDFile); */
                    }

                    // for both "Restart WPS" and "Shutdown" mode:
                    // exit fnwpShutdown and let the rest of the shutdown
                    // be handled by fntShutdown, which will take
                    // over after WM_QUIT

                    fAllWindowsClosed = TRUE;
                    WinPostMsg(hwndMain, WM_COMMAND,
                               MPFROM2SHORT(ID_SDMI_CLEANUPANDQUIT, 0),
                               MPNULL);
                    krnUnlockGlobals();
                break; }

                /*
                 * ID_SDMI_CLEANUPANDQUIT:
                 *      for "real" shutdown: let fntShutdownThread
                 *      handle the rest, so post WM_QUIT to exit the
                 *      message loop
                 */

                case ID_SDMI_CLEANUPANDQUIT:
                {
                    xsdLog("  ID_SDMI_CLEANUPANDQUIT, hwnd: 0x%lX\n", hwndFrame);

                    WinPostMsg(hwndMain, WM_QUIT, MPNULL, MPNULL);
                break; }

                default:
                    mrc = WinDefDlgProc(hwndFrame, msg, mp1, mp2);
            } // end switch;
        break; } // end case WM_COMMAND

        // other msgs: have them handled by the usual WC_FRAME wnd proc
        default:
           mrc = WinDefDlgProc(hwndFrame, msg, mp1, mp2);
        break;
    }

    return (mrc);
}

/* ******************************************************************
 *                                                                  *
 *   here comes the "Finish" routines                               *
 *                                                                  *
 ********************************************************************/


/*
 *  The following routines are called to finish shutdown
 *  after all windows have been closed and the system is
 *  to be shut down or APM power-off'ed or rebooted or
 *  whatever.
 *
 *  All of these routines run on the Shutdown thread.
 */

HPS         hpsScreen = NULLHANDLE;

/*
 *@@ xsdFinishShutdown:
 *      this is the generic routine called by
 *      fntShutdownThread after closing all windows
 *      is complete and the system is to be shut
 *      down, depending on the user settings.
 *
 *      This evaluates the current shutdown settings and
 *      calls xsdFinishStandardReboot, xsdFinishUserReboot,
 *      xsdFinishAPMPowerOff, or xsdFinishStandardMessage.
 *
 *      Note that this is only called for "real" shutdown.
 *      For "Restart WPS", xsdRestartWPS is called instead.
 *
 *      Runs on the Shutdown thread.
 */

VOID xsdFinishShutdown(VOID)
{
    // change the mouse pointer to wait state
    HPOINTER    hptrOld = winhSetWaitPointer();
    ULONG       ulShutdownFunc2 = 0;

    if (psdParams->optAnimate)
        // hps for animation later
        hpsScreen = WinGetScreenPS(HWND_DESKTOP);

    // enforce saving of INIs
    WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
            pNLSStrings->pszSDSavingProfiles);

    xsdLog("Saving INIs...\n");

    if (prfhSaveINIs(habShutdownThread, fileShutdownLog,
                     (PFNWP)fncbUpdateINIStatus,
                         // callback function for updating the progress bar
                     WinWindowFromID(hwndShutdownStatus, ID_SDDI_PROGRESSBAR),
                         // status window handle passed to callback
                     WM_UPDATEPROGRESSBAR,
                         // message passed to callback
                     (PFNWP)fncbSaveINIError,
                         // callback for errors
                     &ulShutdownFunc2)
                         // address where to store debug info
                 != NO_ERROR)
        // error occured: ask whether to restart the WPS
        if (cmnMessageBoxMsg(hwndShutdownStatus, 110, 111, MB_YESNO)
                    == MBID_YES)
            xsdRestartWPS();

    xsdLog("Done saving INIs; now preparing shutdown\n");

    // always say "releasing filesystems"
    WinShowPointer(HWND_DESKTOP, FALSE);
    WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
            pNLSStrings->pszSDFlushing);

    // here comes the settings-dependent part:
    // depending on what we are to do after shutdown,
    // we switch to different routines which handle
    // the rest.
    // I guess this is a better solution than putting
    // all this stuff in the same routine because after
    // DosShutdown(), even the swapper file is blocked,
    // and if the following code is in the swapper file,
    // it cannot be executed. From user reports, I suspect
    // this has happened on some memory-constrained
    // systems with XFolder < V0.84. So we better put
    // all the code together which will be used together.

    if (psdParams->optReboot)
    {
        // reboot:
        if (strlen(psdParams->szRebootCommand) == 0)
            // no user reboot action:
            xsdFinishStandardReboot();
        else
            // user reboot action:
            xsdFinishUserReboot();
    }
    else if (psdParams->optDebug)
    {
        // debug mode: just sleep a while
        DosSleep(2000);
    }
    else if (psdParams->optAPMPowerOff)
    {
        // no reboot, but APM power off?
        xsdFinishAPMPowerOff();
    }
    else
        // normal shutdown: show message
        xsdFinishStandardMessage();

    // the xsdFinish* functions never return;
    // so we only get here in debug mode and
    // return
}

/*
 *@@ xsdFinishStandardMessage:
 *      this finishes the shutdown procedure,
 *      displaying the "Shutdown is complete..."
 *      window and halting the system.
 *
 *      Runs on the Shutdown thread.
 */

VOID xsdFinishStandardMessage(VOID)
{
    // setup Ctrl+Alt+Del message window; this needs to be done
    // before DosShutdown, because we won't be able to reach the
    // resource files after that
    HWND hwndCADMessage = WinLoadDlg(HWND_DESKTOP, NULLHANDLE,
                    WinDefDlgProc,
                    hmodResource,
                    ID_SDD_CAD,
                    NULL);
    winhCenterWindow(hwndCADMessage);  // wnd is still invisible

    if (fileShutdownLog)
    {
        xsdLog("xsdFinishStandardMessage: Calling DosShutdown(0), closing log.\n");
        fclose(fileShutdownLog);
        fileShutdownLog = NULL;
    }

    DosShutdown(0);

    if (psdParams->optAnimate)
        // cute power-off animation
        anmPowerOff(hpsScreen, 50);
    else
        // only hide the status window if
        // animation is off, because otherwise
        // we get screen display errors
        // (hpsScreen...)
        WinShowWindow(hwndShutdownStatus, FALSE);

    // now, this is fun:
    // here's a fine example of how to completely
    // block the system without any chance to escape.
    // Since we have called DosShutdown(), all file
    // access is blocked already. In addition, we
    // do the following:
    // -- show "Press C-A-D..." window
    WinShowWindow(hwndCADMessage, TRUE);
    // -- make the CAD message system-modal
    WinSetSysModalWindow(HWND_DESKTOP, hwndCADMessage);
    // -- kill the tasklist (/helpers/winh.c)
    // so that Ctrl+Esc fails
    winhKillTasklist();
    // -- block all other WPS threads
    DosEnterCritSec();
    // -- and now loop forever!
    while (TRUE)
        DosSleep(10000);
}

/*
 *@@ xsdFinishStandardReboot:
 *      this finishes the shutdown procedure,
 *      rebooting the computer using the standard
 *      reboot procedure (DOS.SYS).
 *      There's no shutdown animation here.
 *
 *      Runs on the Shutdown thread.
 */

VOID xsdFinishStandardReboot(VOID)
{
    HFILE       hIOCTL;
    ULONG       ulAction;

    // if (optReboot), open DOS.SYS; this
    // needs to be done before DosShutdown() also
    if (DosOpen("\\DEV\\DOS$", &hIOCTL, &ulAction, 0L,
               FILE_NORMAL,
               OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,
               0L)
        != NO_ERROR)
    {
        DebugBox("XShutdown", "The DOS.SYS device driver could not be opened. "
                "XShutdown will be unable to reboot your computer. "
                "Please consult the XFolder Online Reference for a remedy of this problem.");
    }

    if (fileShutdownLog)
    {
        xsdLog("xsdFinishStandardReboot: Opened DOS.SYS, hFile: 0x%lX\n", hIOCTL);
        xsdLog("xsdFinishStandardReboot: Calling DosShutdown(0), closing log.\n");
        fclose(fileShutdownLog);
        fileShutdownLog = NULL;
    }

    DosShutdown(0);

    // say "Rebooting..."
    WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
            pNLSStrings->pszSDRebooting);

    // restart the machine via DOS.SYS (opened above)
    DosSleep(500);
    DosDevIOCtl(hIOCTL, 0xd5, 0xab, NULL, 0, NULL, NULL, 0, NULL);

    // don't know if this function returns, but
    // we better make sure we don't return from this
    // function
    while (TRUE)
        DosSleep(10000);
}

/*
 *@@ xsdFinishUserReboot:
 *      this finishes the shutdown procedure,
 *      rebooting the computer by starting a
 *      user reboot command.
 *      There's no shutdown animation here.
 *
 *      Runs on the Shutdown thread.
 */

VOID xsdFinishUserReboot(VOID)
{
    // user reboot item: in this case, we don't call
    // DosShutdown(), which is supposed to be done by
    // the user reboot command
    CHAR    szTemp[CCHMAXPATH];
    PID     pid;
    ULONG   sid;

    sprintf(szTemp,
            pNLSStrings->pszStarting, psdParams->szRebootCommand);
    WinSetDlgItemText(hwndShutdownStatus, ID_SDDI_STATUS,
            szTemp);

    if (fileShutdownLog)
    {
        xsdLog("Trying to start user reboot cmd: %s, closing log.\n",
               psdParams->szRebootCommand);
        fclose(fileShutdownLog);
        fileShutdownLog = NULL;
    }

    sprintf(szTemp, "/c %s", psdParams->szRebootCommand);
    if (doshQuickStartSession("cmd.exe", szTemp,
                              SSF_CONTROL_INVISIBLE, // but auto-close
                              TRUE,  // wait flag
                              &sid, &pid) != NO_ERROR)
    {
        DebugBox("XShutdown", "The user-defined restart command failed. "
            "We will now restart the WPS.");
        xsdRestartWPS();
    }
    else
        // no error:
        // we'll always get here, since the user reboot
        // is now taking place in a different process, so
        // we just stop here
        while (TRUE)
            DosSleep(10000);
}

/*
 *@@ xsdFinishAPMPowerOff:
 *      this finishes the shutdown procedure,
 *      using the functions in apm.c.
 *
 *      Runs on the Shutdown thread.
 */

VOID xsdFinishAPMPowerOff(VOID)
{
    CHAR        szAPMError[500];
    ULONG       ulrcAPM = 0;

    // prepare APM power off
    ulrcAPM = apmPreparePowerOff(szAPMError);
    xsdLog("xsdFinishAPMPowerOff: apmPreparePowerOff returned 0x%lX\n",
            ulrcAPM);

    if (ulrcAPM & APM_IGNORE)
    {
        // if this returns APM_IGNORE, we continue
        // with the regular shutdown w/out APM
        xsdLog("APM_IGNORE, continuing with normal shutdown\n",
               ulrcAPM);

        xsdFinishStandardMessage();
        // this does not return
    }
    else if (ulrcAPM & APM_CANCEL)
    {
        // APM_CANCEL means cancel shutdown
        WinShowPointer(HWND_DESKTOP, TRUE);
        xsdLog("  APM error message: %s",
               szAPMError);

        cmnMessageBox(HWND_DESKTOP, "APM Error", szAPMError,
                MB_CANCEL | MB_SYSTEMMODAL);
        // restart WPS
        xsdRestartWPS();
        // this does not return
    }
    // else: APM_OK means preparing went alright

    if (ulrcAPM & APM_DOSSHUTDOWN_0)
    {
        // shutdown request by apm.c:
        if (fileShutdownLog)
        {
            xsdLog("xsdFinishAPMPowerOff: Calling DosShutdown(0), closing log.\n");
            fclose(fileShutdownLog);
            fileShutdownLog = NULL;
        }

        DosShutdown(0);
    }
    // if apmPreparePowerOff requested this,
    // do DosShutdown(1)
    else if (ulrcAPM & APM_DOSSHUTDOWN_1)
    {
        if (fileShutdownLog)
        {
            xsdLog("xsdFinishAPMPowerOff: Calling DosShutdown(1), closing log.\n");
            fclose(fileShutdownLog);
            fileShutdownLog = NULL;
        }

        DosShutdown(1);
    }
    // or if apmPreparePowerOff requested this,
    // do no DosShutdown()
    if (fileShutdownLog)
    {
        fclose(fileShutdownLog);
        fileShutdownLog = NULL;
    }

    if (psdParams->optAnimate)
        // cute power-off animation
        anmPowerOff(hpsScreen, 50);
    else
        // only hide the status window if
        // animation is off, because otherwise
        // we get screen display errors
        // (hpsScreen...)
        WinShowWindow(hwndShutdownStatus, FALSE);

    // if APM power-off was properly prepared
    // above, this flag is still TRUE; we
    // will now call the function which
    // actually turns the power off
    apmDoPowerOff();
    // we do _not_ return from that function
}

/* ******************************************************************
 *                                                                  *
 *   Update thread                                                  *
 *                                                                  *
 ********************************************************************/

/*
 *@@ xsdOnKillUpdateThread:
 *      thread "exit list" func registered with
 *      the TRY_xxx macros (helpers\except.c).
 *      In case the Update thread gets killed,
 *      this function gets called. As opposed to
 *      real exit list functions, which always get
 *      called on thread 1, this gets called on
 *      the thread which registered the exception
 *      handler.
 *
 *      We release mutex semaphores here, which we
 *      must do, because otherwise the system might
 *      hang if another thread is waiting on the
 *      same semaphore.
 *
 *@@added V0.9.0 [umoeller]
 */

VOID APIENTRY xsdOnKillUpdateThread(VOID)
{
    DosReleaseMutexSem(hmtxShutdown);
}

/*
 *@@ xsd_fntUpdateThread:
 *          this thread is responsible for monitoring the window list
 *          while XShutdown is running and windows are being closed.
 *
 *          It is created from fnwpShutdown when shutdown is about to
 *          begin.
 *
 *          It builds an internal second PSHUTLISTITEM linked list
 *          from the PM window list every 100 ms and then compares
 *          it to the global pliShutdownFirst.
 *
 *          If they are different, this means that windows have been
 *          closed (or even maybe opened), so fnwpShutdown is posted
 *          ID_SDMI_UPDATESHUTLIST so that it can rebuild pliShutdownFirst
 *          and react accordingly, in most cases, close the next window.
 *
 *          This thread monitors its THREADINFO.fExit flag and exits if
 *          it is set to TRUE. See thrClose (threads.c) for how this works.
 *
 *          Global resources used by this thread:
 *          -- HEV hevUpdated: event semaphore posted by Shutdown thread
 *                             when it's done updating its shutitem list.
 *                             We wait for this semaphore after having
 *                             posted ID_SDMI_UPDATESHUTLIST.
 *          -- PLINKLIST pllShutdown: the global list of SHUTLISTITEMs
 *                                    which are left to be closed. This is
 *                                    also used by the Shutdown thread.
 *          -- HMTX hmtxShutdown: mutex semaphore for protecting pllShutdown.
 *
 *@@changed V0.9.0 [umoeller]: code has been re-ordered for semaphore safety.
 */

void _Optlink xsd_fntUpdateThread(PVOID ptiMyself)
{
    HAB             habUpdateThread;
    HMQ             hmqUpdateThread;
    PSZ             pszErrMsg = NULL;

    DosSetPriority(PRTYS_THREAD,
                   PRTYC_REGULAR,
                    -31,          // priority delta
                    0);           // this thread

    if (habUpdateThread = WinInitialize(0))
    {
        if (hmqUpdateThread = WinCreateMsgQueue(habUpdateThread, 0))
        {
            BOOL            fSemOwned = FALSE;
            LINKLIST        llTestList;
            lstInit(&llTestList, TRUE);     // auto-free items

            TRY_LOUD(excpt1, NULL)
            {
                ULONG           ulShutItemCount = 0,
                                ulTestItemCount = 0;
                BOOL            fUpdated = FALSE;
                PCKERNELGLOBALS  pKernelGlobals = krnQueryGlobals();

                WinCancelShutdown(hmqUpdateThread, TRUE);

                // wait until main shutdown window is up
                while ( (hwndMain == 0) && (!(pKernelGlobals->ptiUpdateThread->fExit)) )
                    DosSleep(100);

                while (!(pKernelGlobals->ptiUpdateThread->fExit))
                {
                    // this is the first loop: we arrive here every time
                    // the task list has changed */
                    #ifdef DEBUG_SHUTDOWN
                        _Pmpf(( "UT: Waiting for update..." ));
                    #endif

                    // have Shutdown thread update its list of items then
                    WinPostMsg(hwndMain, WM_COMMAND,
                               MPFROM2SHORT(ID_SDMI_UPDATESHUTLIST, 0),
                               MPNULL);
                    fUpdated = FALSE;

                    // now wait until Shutdown thread is done updating its
                    // list; it then posts an event semaphore
                    while (     (!fUpdated)
                            &&  (!(pKernelGlobals->ptiUpdateThread->fExit))
                          )
                    {
                        if (pKernelGlobals->ptiUpdateThread->fExit)
                        {
                            // we're supposed to exit:
                            fUpdated = TRUE;
                            #ifdef DEBUG_SHUTDOWN
                                _Pmpf(( "UT: Exit recognized" ));
                            #endif
                        }
                        else
                        {
                            ULONG   ulUpdate;
                            // query event semaphore post count
                            DosQueryEventSem(hevUpdated, &ulUpdate);
                            fUpdated = (ulUpdate > 0);
                            #ifdef DEBUG_SHUTDOWN
                                _Pmpf(( "UT: update recognized" ));
                            #endif
                            DosSleep(100);
                        }
                    } // while (!fUpdated);

                    ulTestItemCount = 0;
                    ulShutItemCount = 0;

                    #ifdef DEBUG_SHUTDOWN
                        _Pmpf(( "UT: Waiting for task list change, loop 2..." ));
                    #endif

                    while (     (ulTestItemCount == ulShutItemCount)
                             && (!(pKernelGlobals->ptiUpdateThread->fExit))
                          )
                    {
                        // this is the second loop: we stay in here until the
                        // task list has changed; for monitoring this, we create
                        // a second task item list similar to the pliShutdownFirst
                        // list and compare the two
                        lstClear(&llTestList);

                        // create a test list for comparing the task list;
                        // this is our private list, so we need no mutex
                        // semaphore
                        xsdBuildShutList(psdParams, &llTestList);

                        // count items in the test list
                        ulTestItemCount = lstCountItems(&llTestList);

                        // count items in the list of the Shutdown thread;
                        // here we need a mutex semaphore, because the
                        // Shutdown thread might be working on this too
                        fSemOwned = (DosRequestMutexSem(hmtxShutdown, 4000) == NO_ERROR);
                        if (fSemOwned)
                        {
                            ulShutItemCount = lstCountItems(pllShutdown);
                            DosReleaseMutexSem(hmtxShutdown);
                            fSemOwned = FALSE;
                        }

                        if (!(pKernelGlobals->ptiUpdateThread->fExit))
                            DosSleep(100);
                    } // end while; loop until either the Shutdown thread has set the
                      // Exit flag or the list has changed

                    #ifdef DEBUG_SHUTDOWN
                        _Pmpf(( "UT: Change or exit recognized" ));
                    #endif
                }  // end while; loop until exit flag set
            } // end TRY_LOUD(excpt1)
            CATCH(excpt1)
            {
                // exception occured:
                // complain to the user
                if (pszErrMsg == NULL)
                {
                    if (fSemOwned)
                    {
                        DosReleaseMutexSem(hmtxShutdown);
                        fSemOwned = FALSE;
                    }

                    // only report the first error, or otherwise we will
                    // jam the system with msg boxes
                    pszErrMsg = malloc(1000);
                    if (pszErrMsg)
                    {
                        strcpy(pszErrMsg, "An error occured in the XFolder Update thread. "
                                "In the root directory of your boot drive, you will find a "
                                "file named XFLDTRAP.LOG, which contains debugging information. "
                                "If you had shutdown logging enabled, you will also find the "
                                "file XSHUTDWN.LOG there. If not, please enable shutdown "
                                "logging in the Desktop's settings notebook. ");
                        krnPostThread1ObjectMsg(T1M_EXCEPTIONCAUGHT, (MPARAM)pszErrMsg,
                                (MPARAM)0); // don't enforce WPS restart

                        xsdLog("\n*** CRASH\n%s\n", pszErrMsg);
                    }
                }
            } END_CATCH();

            // clean up
            #ifdef DEBUG_SHUTDOWN
                _Pmpf(( "UT: Exiting..." ));
            #endif

            if (fSemOwned)
            {
                // release our mutex semaphore
                DosReleaseMutexSem(hmtxShutdown);
                fSemOwned = FALSE;
            }

            lstClear(&llTestList);

            WinDestroyMsgQueue(hmqUpdateThread);
        }
        WinTerminate(habUpdateThread);
    }

    thrGoodbye((PTHREADINFO)ptiMyself);
    #ifdef DEBUG_SHUTDOWN
        DosBeep(100, 100);
    #endif
}


