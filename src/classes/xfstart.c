
/*
 *@@sourcefile xfstart.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XFldStartup (startup folder, XFolder subclass)
 *      --  XFldShutdown (shutdown folder, XFolder subclass)
 *
 *      Installation of XFldStartup and XFldShutdown is
 *      optional. However, both classes are derived from
 *      XFolder directly (and not from WPFolder), so
 *      installation of XFolder is required if any of
 *      XFldStartup and XFldShutdown are installed.
 *
 *      These two classes used to be in xfldr.c before
 *      V0.9.0, but have been moved to this new separate file
 *      because the XFolder class is complex enough to be
 *      in a file of its own.
 *
 *      Starting with V0.9.0, the files in classes\ contain only
 *      i.e. the methods themselves.
 *      The implementation for this class is in filesys\folder.c.
 *
 *@@somclass XFldStartup xfstup_
 *@@somclass M_XFldStartup xfstupM_
 *@@somclass XFldShutdown xfshut_
 *@@somclass M_XFldShutdown xfshutM_
 */

/*
 *      Copyright (C) 1997-2003 Ulrich M�ller.
 *
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
 *  This file was generated by the SOM Compiler and Emitter Framework.
 *  Generated using:
 *      SOM Emitter emitctm: 2.41
 */

#ifndef SOM_Module_xfstart_Source
#define SOM_Module_xfstart_Source
#endif
#define XFldStartup_Class_Source
#define M_XFldStartup_Class_Source

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

#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_WINWINDOWMGR
#define INCL_WINMENUS
#define INCL_WINDIALOGS
#define INCL_WINBUTTONS
#define INCL_WINSHELLDATA
#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\linklist.h"           // linked list helper routines
#include "helpers\winh.h"               // PM helper routines

// SOM headers which don't crash with prec. header files
#include "xfstart.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs
#include "shared\notebook.h"            // generic XWorkplace notebook handling
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\wpsh.h"                // WPS Helpers

#include "filesys\folder.h"             // XFolder implementation
#include "filesys\object.h"             // XFldObject implementation

// other SOM headers
#pragma hdrstop                         // VAC++ keeps crashing otherwise

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

// roots of linked lists for XStartup folders
// these hold plain WPObject pointers, no auto-free
#if 1 // V1.0.1 (2002-12-11) [umoeller]
static XWPObjList          *G_llSavedStartupFolders = NULL,
                           *G_llStartupFolders = NULL;
#else
static OBJECTLIST          G_llSavedStartupFolders = {0};
static OBJECTLIST          G_llStartupFolders = {0};
#endif

/* ******************************************************************
 *
 *   here come the XFldStartup methods
 *
 ********************************************************************/

/*
 *@@ xwpAddXFldStartupPage:
 *      this adds the "Startup" page into the startup folder's
 *      settings notebook.
 *
 *@@added V0.9.0 [umoeller]
 */

SOM_Scope ULONG  SOMLINK xfstup_xwpAddXFldStartupPage(XFldStartup *somSelf,
                                                      HWND hwndDlg)
{
    INSERTNOTEBOOKPAGE inbp;

    /* XFldStartupData *somThis = XFldStartupGetData(somSelf); */
    XFldStartupMethodDebug("XFldStartup","xfstup_xwpAddXFldStartupPage");

    memset(&inbp, 0, sizeof(INSERTNOTEBOOKPAGE));
    inbp.somSelf = somSelf;
    inbp.hwndNotebook = hwndDlg;
    inbp.hmod = cmnQueryNLSModuleHandle(FALSE);
    inbp.usPageStyleFlags = BKA_MAJOR;
    inbp.pcszName = cmnGetString(ID_XSSI_STARTUPPAGE);  // pszStartupPage
    inbp.ulDlgID = ID_XSD_STARTUPFOLDER;
    inbp.ulDefaultHelpPanel  = ID_XSH_SETTINGS_XFLDSTARTUP;
    inbp.ulPageID = SP_STARTUPFOLDER;
    inbp.pfncbInitPage    = fdrStartupFolderInitPage;
    inbp.pfncbItemChanged = fdrStartupFolderItemChanged;
    return ntbInsertPage(&inbp);
}

/*
 *@@ xwpSetXStartup:
 *      adds/removes the folder to/from the linked list
 *      of XStartup folders.
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope ULONG  SOMLINK xfstup_xwpSetXStartup(XFldStartup *somSelf,
                                               BOOL fInsert)
{
    // XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_xwpSetXStartup");

#if 1
    return (    (G_llStartupFolders)
             && (   (fInsert)
                        ? _Append(G_llStartupFolders,
                                  somSelf)
                        : _Remove(G_llStartupFolders,
                                  somSelf)
                )
           );
#else
    return objAddToList(somSelf,
                        &G_llStartupFolders,
                        fInsert,
                        INIKEY_XSTARTUPFOLDERS,
                        0);
#endif
}

/*
 *@@ xwpQueryXStartupType:
 *      queries the start type of the folder
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope ULONG  SOMLINK xfstup_xwpQueryXStartupType(XFldStartup *somSelf)
{
    XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_xwpQueryXStartupType");

    return(_ulType);
}

/*
 *@@ xwpQueryXStartupObjectDelay:
 *      queries the object delay time of the folder
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope ULONG  SOMLINK xfstup_xwpQueryXStartupObjectDelay(XFldStartup *somSelf)
{
    XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_xwpQueryXStartupObjectDelay");

    return(_ulObjectDelay);
}

/*
 *@@ wpInitData:
 *      initialises the object with safe defaults.
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope void  SOMLINK xfstup_wpInitData(XFldStartup *somSelf)
{
    XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_wpInitData");

    XFldStartup_parent_XFolder_wpInitData(somSelf);

    // set all the instance variables to safe defaults
    _ulType = XSTARTUP_REBOOTSONLY;
    _ulObjectDelay = XSTARTUP_DEFAULTOBJECTDELAY;
}

/*
 *@@ wpObjectReady:
 *      adds the object to the linked list when it is
 *      being awoken.
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope void  SOMLINK xfstup_wpObjectReady(XFldStartup *somSelf,
                                             ULONG ulCode, WPObject* refObject)
{
    // XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_wpObjectReady");

    XFldStartup_parent_XFolder_wpObjectReady(somSelf, ulCode,
                                             refObject);

    // add to the linked list
    _xwpSetXStartup(somSelf, TRUE);
}

/*
 *@@ wpFree:
 *      removes the object from the linked list when it
 *      is being deleted.
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope BOOL  SOMLINK xfstup_wpFree(XFldStartup *somSelf)
{
    // XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_wpFree");

    // remove from the linked list
    _xwpSetXStartup(somSelf, FALSE);
    return XFldStartup_parent_XFolder_wpFree(somSelf);
}

/*
 *@@ wpSaveState:
 *      saves the object's instance data.
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope BOOL  SOMLINK xfstup_wpSaveState(XFldStartup *somSelf)
{
    XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_wpSaveState");

    if (_ulType != XSTARTUP_REBOOTSONLY)
        _wpSaveLong(somSelf, (PSZ)G_pcszXFldStartup, 1, _ulType);

    if (_ulObjectDelay != XSTARTUP_DEFAULTOBJECTDELAY)
        _wpSaveLong(somSelf, (PSZ)G_pcszXFldStartup, 2, _ulObjectDelay);

    return XFldStartup_parent_XFolder_wpSaveState(somSelf);
}

/*
 *@@ wpRestoreState:
 *      restores the object's instance data.
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope BOOL  SOMLINK xfstup_wpRestoreState(XFldStartup *somSelf,
                                              ULONG ulReserved)
{
    ULONG   ul;
    XFldStartupData *somThis = XFldStartupGetData(somSelf);
    XFldStartupMethodDebug("XFldStartup","xfstup_wpRestoreState");

    if (_wpRestoreLong(somSelf, (PSZ)G_pcszXFldStartup, 1, &ul))
        _ulType = ul;

    if (_wpRestoreLong(somSelf, (PSZ)G_pcszXFldStartup, 2, &ul))
        _ulObjectDelay = ul;

    return XFldStartup_parent_XFolder_wpRestoreState(somSelf,
                                                     ulReserved);
}

/*

/*
 *@@ wpModifyPopupMenu:
 *      this WPObject instance methods gets called by the WPS
 *      when a context menu needs to be built for the object
 *      and allows the object to manipulate its context menu.
 *      This gets called _after_ wpFilterPopupMenu.
 *
 *      We add a "Process content" menu item to this
 *      popup menu; the other menu items are inherited
 *      from XFolder.
 */

SOM_Scope BOOL  SOMLINK xfstup_wpModifyPopupMenu(XFldStartup *somSelf,
                                                 HWND hwndMenu,
                                                 HWND hwndCnr,
                                                 ULONG iPosition)
{
    BOOL rc;
    ULONG ulOfs = *G_pulVarMenuOfs;
    // PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();
    /* XFldStartupData *somThis = XFldStartupGetData(somSelf); */
    XFldStartupMethodDebug("XFldStartup","xfstup_wpModifyPopupMenu");

    rc = XFldStartup_parent_XFolder_wpModifyPopupMenu(somSelf,
                                                      hwndMenu,
                                                      hwndCnr,
                                                      iPosition);

    cmnInsertSeparator(hwndMenu, MIT_END);

    winhInsertMenuItem(hwndMenu,
                       MIT_END,
                       ulOfs + ID_XFMI_OFS_PROCESSCONTENT,
                       cmnGetString(ID_XSSI_PROCESSCONTENT),
                       MIS_TEXT, 0);

    return rc;
}

/*
 *@@ wpMenuItemSelected:
 *      this WPObject method processes menu selections.
 *      This must be overridden to support new menu
 *      items which have been added in wpModifyPopupMenu.
 *
 *      See XFldObject::wpMenuItemSelected for additional
 *      remarks.
 *
 *      We react to the "Process content" item we have
 *      inserted for the startup folder.
 *
 *@@changed V0.9.9 (2001-03-19) [pr]: changed message sent to thread 1
 *@@changed V0.9.12 (2001-05-22) [umoeller]: re-enabled "start this" menu item
 */

SOM_Scope BOOL  SOMLINK xfstup_wpMenuItemSelected(XFldStartup *somSelf,
                                                  HWND hwndFrame,
                                                  ULONG ulMenuId)
{
    /* XFldStartupData *somThis = XFldStartupGetData(somSelf); */
    XFldStartupMethodDebug("XFldStartup","xfstup_wpMenuItemSelected");

    if ( (ulMenuId - *G_pulVarMenuOfs) == ID_XFMI_OFS_PROCESSCONTENT )
    {
        if (cmnMessageBoxExt((hwndFrame)
                                ? hwndFrame
                                : HWND_DESKTOP,
                             116,
                             NULL, 0,
                             138,
                             MB_YESNO | MB_DEFBUTTON2)
                == MBID_YES)
        {
            // re-enabled this: V0.9.12 (2001-05-22) [umoeller]

            // start the folder contents synchronously;
            // this func now displays the progress dialog
            // and does not return until the folder was
            // fully processed (this calls another thrRunSync
            // internally, so the SIQ is not blocked)
            _xwpStartFolderContents(somSelf,        // XFolder method
                                    _xwpQueryXStartupObjectDelay(somSelf));
                                                    // XFldStartup method
                // this goes into fdrStartFolderContents
        }
        return TRUE;
    }

    return XFldStartup_parent_XFolder_wpMenuItemSelected(somSelf,
                                                         hwndFrame,
                                                         ulMenuId);
}

/*
 *@@ wpMenuItemHelpSelected:
 *      display help for "Process content"
 *      menu item. @@todo
 */

SOM_Scope BOOL  SOMLINK xfstup_wpMenuItemHelpSelected(XFldStartup *somSelf,
                                                      ULONG MenuId)
{
    /* XFldStartupData *somThis = XFldStartupGetData(somSelf); */
    XFldStartupMethodDebug("XFldStartup","xfstup_wpMenuItemHelpSelected");

    return XFldStartup_parent_XFolder_wpMenuItemHelpSelected(somSelf,
                                                             MenuId);
}

/*
 *@@ wpAddSettingsPages:
 *      this WPObject instance method gets called by the WPS
 *      when the Settings view is opened to have all the
 *      settings page inserted into hwndNotebook.
 *
 *      Starting with V0.9.0, we override this method too to add
 *      the XWorkplace Startup folder's settings
 *      page.
 *
 *@@added V0.9.0 [umoeller]
 */

SOM_Scope BOOL  SOMLINK xfstup_wpAddSettingsPages(XFldStartup *somSelf,
                                                  HWND hwndNotebook)
{
    /* XFldStartupData *somThis = XFldStartupGetData(somSelf); */
    XFldStartupMethodDebug("XFldStartup","xfstup_wpAddSettingsPages");

    XFldStartup_parent_XFolder_wpAddSettingsPages(somSelf, hwndNotebook);

    // add the "XWorkplace Startup" page on top
    return _xwpAddXFldStartupPage(somSelf, hwndNotebook);
}

/*
 *@@ wpclsQueryXStartupFolder:
 *      queries the linked list of XStartup folders
 *      for the next item in the chain.
 *
 *@@added V0.9.9 (2001-03-19) [pr]
 */

SOM_Scope XFldStartup*  SOMLINK xfstupM_xwpclsQueryXStartupFolder(M_XFldStartup *somSelf,
                                                                  XFldStartup* pFolder)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    WPObject *pDesktop;

    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_xwpclsQueryXStartupFolder");

    // _PmpfF(("getting next, pFolder is %s",
       //      (pFolder) ? _wpQueryTitle(pFolder) : "NULL"));
    pDesktop = cmnQueryActiveDesktop();
    do
    {
        #if 1       // V1.0.1 (2002-12-11) [umoeller]
            pFolder = _Enum(G_llSavedStartupFolders,
                            pFolder);
        #else
            pFolder = objEnumList(&G_llSavedStartupFolders,
                                  pFolder,
                                  INIKEY_XSAVEDSTARTUPFOLDERS,
                                  0);
        #endif

        // _Pmpf(("    got %s",
           //      (pFolder) ? _wpQueryTitle(pFolder) : "NULL"));

    } while (    (pFolder)
              && (!wpshResidesBelow(pFolder, pDesktop))
            );

    return pFolder;
}

/*
 *@@ wpclsInitData:
 *      this WPObject class method gets called when a class
 *      is loaded by the WPS (probably from within a
 *      somFindClass call) and allows the class to initialize
 *      itself.
 *
 *@@changed V0.9.0 [umoeller]: added class object to KERNELGLOBALS
 *@@changed V0.9.9 (2001-03-19) [pr]: multiple startup folder mods.
 */

SOM_Scope void  SOMLINK xfstupM_wpclsInitData(M_XFldStartup *somSelf)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_wpclsInitData");

    M_XFldStartup_parent_M_XFolder_wpclsInitData(somSelf);

    if (krnClassInitialized(G_pcszXFldStartup))
    {
        BOOL        brc = FALSE;
        ULONG       ulSize;
        PSZ         pszHandles;

        // first call:

        // initialize linked lists
        #if 1       // V1.0.1 (2002-12-11) [umoeller]
            G_llStartupFolders = _xwpclsCreateList(_XFldObject,
                                                   (PSZ)INIAPP_XWORKPLACE,
                                                   (PSZ)INIKEY_XSTARTUPFOLDERS);
            G_llSavedStartupFolders = _xwpclsCreateList(_XFldObject,
                                                        (PSZ)INIAPP_XWORKPLACE,
                                                        (PSZ)INIKEY_XSAVEDSTARTUPFOLDERS);
        #else
            lstInit(&G_llStartupFolders.ll, FALSE);    // no auto-free
            G_llStartupFolders.fLoaded = FALSE;
            lstInit(&G_llSavedStartupFolders.ll, FALSE);    // no auto-free
            G_llSavedStartupFolders.fLoaded = FALSE;
        #endif

        // copy INI setting
        if (    (brc = PrfQueryProfileSize(HINI_USERPROFILE,
                                           (PSZ)INIAPP_XWORKPLACE,
                                           (PSZ)INIKEY_XSTARTUPFOLDERS,
                                           &ulSize))
             && (pszHandles = malloc(ulSize))
           )
        {
            if (brc = PrfQueryProfileString(HINI_USERPROFILE,
                                            (PSZ)INIAPP_XWORKPLACE,
                                            (PSZ)INIKEY_XSTARTUPFOLDERS,
                                            "",
                                            pszHandles,
                                            ulSize))
                PrfWriteProfileString(HINI_USERPROFILE,
                                      (PSZ)INIAPP_XWORKPLACE,
                                      (PSZ)INIKEY_XSAVEDSTARTUPFOLDERS,
                                      pszHandles);

            free(pszHandles);
        }
    }
}

/*
 *@@ wpclsQueryTitle:
 *      this WPObject class method tells the WPS the clear
 *      name of a class, which is shown in the third column
 *      of a Details view and also used as the default title
 *      for new objects of a class.
 *
 *@@changed V0.9.6 (2000-11-20) [umoeller]: changed to "XWorkplace"
 *@@changed V1.0.0 (2002-08-31) [umoeller]: finally localized
 */

SOM_Scope PSZ  SOMLINK xfstupM_wpclsQueryTitle(M_XFldStartup *somSelf)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_wpclsQueryTitle");

    // return ("XWorkplace Startup");
    return (PSZ)cmnGetString(ID_XFSI_XWPSTARTUPFDR);
}

/*
 *@@ wpclsQueryStyle:
 *      we return a flag so that no templates are created
 *      for the Startup folder.
 *
 *@@changed V0.9.9 (2001-03-19) [pr]: allow copy
 *@@changed V1.0.1 (2002-11-25) [pr]: allow Create Another
 */

SOM_Scope ULONG  SOMLINK xfstupM_wpclsQueryStyle(M_XFldStartup *somSelf)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_wpclsQueryStyle");

    return (M_XFldStartup_parent_M_XFolder_wpclsQueryStyle(somSelf)
                // | CLSSTYLE_NEVERTEMPLATE
                // | CLSSTYLE_NEVERCOPY
                // | CLSSTYLE_NEVERDELETE
           );

}

/*
 *@@ wpclsQueryDefaultHelp:
 *      this WPObject class method returns the default help
 *      panel for objects of this class. This gets called
 *      from WPObject::wpQueryDefaultHelp if no instance
 *      help settings (HELPLIBRARY, HELPPANEL) have been
 *      set for an individual object. It is thus recommended
 *      to override this method instead of the instance
 *      method to change the default help panel for a class
 *      in order not to break instance help settings (fixed
 *      with 0.9.20).
 *
 *      We override the standard folder help to return help
 *      for the startup folder here.
 *
 *@@added V0.9.20 (2002-07-12) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xfstupM_wpclsQueryDefaultHelp(M_XFldStartup *somSelf,
                                                      PULONG pHelpPanelId,
                                                      PSZ pszHelpLibrary)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_wpclsQueryDefaultHelp");

    strcpy(pszHelpLibrary, cmnQueryHelpLibrary());
    *pHelpPanelId = ID_XMH_STARTUPSHUTDOWN;
    return TRUE;
}

/*
 *@@ wpclsQueryIconData:
 *      this WPObject class method must return information
 *      about how to build the default icon for objects
 *      of a class. This gets called from various other
 *      methods whenever a class default icon is needed;
 *      most importantly, M_WPObject::wpclsQueryIcon
 *      calls this to build a class default icon, which
 *      is then cached in the class's instance data.
 *      If a subclass wants to change a class default icon,
 *      it should always override _this_ method instead of
 *      wpclsQueryIcon.
 *
 *      Note that the default WPS implementation does not
 *      allow for specifying the ICON_FILE format here,
 *      which is why we have overridden
 *      M_XFldObject::wpclsQueryIcon too. This allows us
 *      to return icon _files_ for theming too. For details
 *      about the WPS's crappy icon management, refer to
 *      src\filesys\icons.c.
 *
 *      We give the Startup folder a new closed icon.
 */

SOM_Scope ULONG  SOMLINK xfstupM_wpclsQueryIconData(M_XFldStartup *somSelf,
                                                    PICONINFO pIconInfo)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_wpclsQueryIconData");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_STARTICON1;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return sizeof(ICONINFO);
}

/*
 *@@ wpclsQueryIconDataN:
 *      give the Startup folder a new animated icon.
 */

SOM_Scope ULONG  SOMLINK xfstupM_wpclsQueryIconDataN(M_XFldStartup *somSelf,
                                                     ICONINFO* pIconInfo,
                                                     ULONG ulIconIndex)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_wpclsQueryIconDataN");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_STARTICON2;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return sizeof(ICONINFO);
}

/*
 *@@ wpclsCreateDefaultTemplates:
 *      This method is used to create templates for the class.
 *      We prevent this creation by not calling the parent method.
 *
 *@@added V1.0.1 (2002-11-25) [pr]
 */

SOM_Scope BOOL  SOMLINK xfstupM_wpclsCreateDefaultTemplates(M_XFldStartup *somSelf,
                                                            WPObject* Folder)
{
    /* M_XFldStartupData *somThis = M_XFldStartupGetData(somSelf); */
    M_XFldStartupMethodDebug("M_XFldStartup","xfstupM_wpclsCreateDefaultTemplates");

    return TRUE;
}


/* ******************************************************************
 *
 *   here come the XFldShutdown methods
 *
 ********************************************************************/

/*
 *@@ wpclsInitData:
 *      this WPObject class method gets called when a class
 *      is loaded by the WPS (probably from within a
 *      somFindClass call) and allows the class to initialize
 *      itself.
 *
 *@@changed V0.9.0 [umoeller]: added class object to KERNELGLOBALS
 */

SOM_Scope void  SOMLINK xfshutM_wpclsInitData(M_XFldShutdown *somSelf)
{
    /* M_XFldShutdownData *somThis = M_XFldShutdownGetData(somSelf); */
    M_XFldShutdownMethodDebug("M_XFldShutdown","xfshutM_wpclsInitData");

    M_XFldShutdown_parent_M_XFolder_wpclsInitData(somSelf);

    krnClassInitialized(G_pcszXFldShutdown);
}

/*
 *@@ wpclsQueryTitle:
 *      this WPObject class method tells the WPS the clear
 *      name of a class, which is shown in the third column
 *      of a Details view and also used as the default title
 *      for new objects of a class.
 *
 *@@changed V0.9.6 (2000-11-20) [umoeller]: changed to "XWorkplace"
 *@@changed V1.0.0 (2002-08-31) [umoeller]: finally localized
 */

SOM_Scope PSZ  SOMLINK xfshutM_wpclsQueryTitle(M_XFldShutdown *somSelf)
{
    /* M_XFldShutdownData *somThis = M_XFldShutdownGetData(somSelf); */
    M_XFldShutdownMethodDebug("M_XFldShutdown","xfshutM_wpclsQueryTitle");

    return (PSZ)cmnGetString(ID_XFSI_XWPSHUTDOWNFDR);
                // V1.0.0 (2002-08-31) [umoeller]
}

/*
 *@@ wpclsQueryStyle:
 *      we return a flag so that no templates are created
 *      for the Shutdown folder.
 */

SOM_Scope ULONG  SOMLINK xfshutM_wpclsQueryStyle(M_XFldShutdown *somSelf)
{
    /* M_XFldShutdownData *somThis = M_XFldShutdownGetData(somSelf); */
    M_XFldShutdownMethodDebug("M_XFldShutdown","xfshutM_wpclsQueryStyle");

    return (M_XFldShutdown_parent_M_XFolder_wpclsQueryStyle(somSelf)
                | CLSSTYLE_NEVERTEMPLATE
                | CLSSTYLE_NEVERCOPY
                // | CLSSTYLE_NEVERDELETE
           );
}

/*
 *@@ wpclsQueryDefaultHelp:
 *      this WPObject class method returns the default help
 *      panel for objects of this class. This gets called
 *      from WPObject::wpQueryDefaultHelp if no instance
 *      help settings (HELPLIBRARY, HELPPANEL) have been
 *      set for an individual object. It is thus recommended
 *      to override this method instead of the instance
 *      method to change the default help panel for a class
 *      in order not to break instance help settings (fixed
 *      with 0.9.20).
 *
 *      We override the standard folder help to return help
 *      for the shutdown folder here.
 *
 *@@added V0.9.20 (2002-07-12) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xfshutM_wpclsQueryDefaultHelp(M_XFldShutdown *somSelf,
                                                      PULONG pHelpPanelId,
                                                      PSZ pszHelpLibrary)
{
    /* M_XFldShutdownData *somThis = M_XFldShutdownGetData(somSelf); */
    M_XFldShutdownMethodDebug("M_XFldShutdown","xfshutM_wpclsQueryDefaultHelp");

    strcpy(pszHelpLibrary, cmnQueryHelpLibrary());
    *pHelpPanelId = ID_XMH_STARTUPSHUTDOWN;
    return TRUE;

}

/*
 *@@ wpclsQueryIconData:
 *      this WPObject class method must return information
 *      about how to build the default icon for objects
 *      of a class. This gets called from various other
 *      methods whenever a class default icon is needed;
 *      most importantly, M_WPObject::wpclsQueryIcon
 *      calls this to build a class default icon, which
 *      is then cached in the class's instance data.
 *      If a subclass wants to change a class default icon,
 *      it should always override _this_ method instead of
 *      wpclsQueryIcon.
 *
 *      Note that the default WPS implementation does not
 *      allow for specifying the ICON_FILE format here,
 *      which is why we have overridden
 *      M_XFldObject::wpclsQueryIcon too. This allows us
 *      to return icon _files_ for theming too. For details
 *      about the WPS's crappy icon management, refer to
 *      src\filesys\icons.c.
 *
 *      We give the Shutdown folder a new closed icon.
 */

SOM_Scope ULONG  SOMLINK xfshutM_wpclsQueryIconData(M_XFldShutdown *somSelf,
                                                    PICONINFO pIconInfo)
{
    /* M_XFldShutdownData *somThis = M_XFldShutdownGetData(somSelf); */
    M_XFldShutdownMethodDebug("M_XFldShutdown","xfshutM_wpclsQueryIconData");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_SHUTICON1;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return sizeof(ICONINFO);
}

/*
 *@@ wpclsQueryIconDataN:
 *      give the Shutdown folder a new animated icon.
 */

SOM_Scope ULONG  SOMLINK xfshutM_wpclsQueryIconDataN(M_XFldShutdown *somSelf,
                                                     ICONINFO* pIconInfo,
                                                     ULONG ulIconIndex)
{
    /* M_XFldShutdownData *somThis = M_XFldShutdownGetData(somSelf); */
    M_XFldShutdownMethodDebug("M_XFldShutdown","xfshutM_wpclsQueryIconDataN");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_SHUTICON2;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return sizeof(ICONINFO);
}



