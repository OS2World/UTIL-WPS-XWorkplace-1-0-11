
/*
 *@@sourcefile xwpsetup.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPSetup ("XWorkplace Setup" object, WPAbstract subclass)
 *
 *      This class implements the "XWorkplace Setup"
 *      setting object, which has the main XWorkplace settings.
 *      This object is an example of how to create a WPAbstract
 *      subclass settings object from scratch, without subclassing
 *      from one of the other WPAbstract settings objects.
 *
 *      This is entirely new with XWorkplace V0.9.0.
 *
 *      Theoretically, installation of this class is optional, but
 *      you won't be able to change XWorkplace's most basic settings
 *      without it.
 *
 *      Starting with V0.9.0, the files in classes\ contain only
 *      the SOM interface, i.e. the methods themselves.
 *      The implementation for this class is in in shared\xsetup.c.
 *
 *@@somclass XWPSetup xwset_
 *@@somclass M_XWPSetup xwsetM_
 */

/*
 *      Copyright (C) 1999-2000 Ulrich M�ller.
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

#ifndef SOM_Module_xwpsetup_Source
#define SOM_Module_xwpsetup_Source
#endif
#define XWPSetup_Class_Source
#define M_XWPSetup_Class_Source

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
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2.h>

// C library headers
#include <stdio.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\procstat.h"           // DosQProcStat handling
#include "helpers\stringh.h"            // string helper routines
#include "helpers\winh.h"               // PM helper routines
#include "helpers\xstring.h"            // extended string helpers

// SOM headers which don't crash with prec. header files
#include "xwpsetup.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)
#include "shared\xsetup.h"              // XWPSetup implementation

#include "filesys\xthreads.h"           // extra XWorkplace threads

#include "media\media.h"                // XWorkplace multimedia support

#include "startshut\apm.h"              // APM power-off for XShutdown

#include "hook\xwphook.h"

// other SOM headers
#pragma hdrstop
#include <wpfsys.h>             // WPFileSystem

/* ******************************************************************
 *
 *   XWPSetup instance methods
 *
 ********************************************************************/

static MPARAM G_ampFeaturesPage[] =
    {
        MPFROM2SHORT(ID_XFDI_CNR_GROUPTITLE, XAC_SIZEX | XAC_SIZEY),
        MPFROM2SHORT(ID_XCDI_CONTAINER, XAC_SIZEX | XAC_SIZEY)
    };

/*
 *@@ xwpAddXWPSetupPages:
 *      this actually adds the new pages into the
 *      "XWorkplace Configuration" notebook.
 *
 *@@changed V0.9.6 (2000-11-04) [umoeller]: added logo on top
 */

SOM_Scope ULONG  SOMLINK xwset_xwpAddXWPSetupPages(XWPSetup *somSelf,
                                                   HWND hwndDlg)
{
    PCREATENOTEBOOKPAGE pcnbp;
    HMODULE         savehmod = cmnQueryNLSModuleHandle(FALSE);
    // PNLSSTRINGS     pNLSStrings = cmnQueryNLSStrings();

    /* XWPSetupData *somThis = XWPSetupGetData(somSelf); */
    XWPSetupMethodDebug("XWPSetup","xwset_xwpAddXWPSetupPages");

#ifndef __XWPLITE__
    // insert "Paranoia" page
    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndDlg;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = cmnGetString(ID_XSSI_PARANOIA);  // pszParanoia
    pcnbp->ulDlgID = ID_XCD_PARANOIA;
    // pcnbp->usFirstControlID = ID_XCDI_VARMENUOFFSET;
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_PARANOIA;
    pcnbp->ulPageID = SP_SETUP_PARANOIA;
    pcnbp->pfncbInitPage    = setParanoiaInitPage;
    pcnbp->pfncbItemChanged = setParanoiaItemChanged;
    ntbInsertPage(pcnbp);

    // insert "objects" page
    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndDlg;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = cmnGetString(ID_XSSI_OBJECTS);  // pszObjects
    pcnbp->ulDlgID = ID_XCD_OBJECTS;
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_OBJECTS;
    pcnbp->ulPageID = SP_SETUP_OBJECTS;
    pcnbp->pfncbInitPage    = setObjectsInitPage;
    pcnbp->pfncbItemChanged = setObjectsItemChanged;
    ntbInsertPage(pcnbp);

    // insert "Threads" page
    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndDlg;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = cmnGetString(ID_XSSI_THREADSPAGE);  // pszThreadsPage
    pcnbp->ulDlgID = ID_XFD_CONTAINERPAGE; // generic cnr page;
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_XC_THREADS;
    pcnbp->ulPageID = SP_SETUP_THREADS;
    pcnbp->pampControlFlags = G_pampGenericCnrPage;
    pcnbp->cControlFlags = G_cGenericCnrPage;
    pcnbp->pfncbInitPage    = setThreadsInitPage;
    ntbInsertPage(pcnbp);

    // insert "XWorkplace Info" page
    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndDlg;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = cmnGetString(ID_XSSI_XWPSTATUS);  // pszXWPStatus
    pcnbp->ulDlgID = ID_XCD_STATUS;
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_XC_INFO;
    pcnbp->ulPageID = SP_SETUP_INFO;
    pcnbp->pfncbInitPage    = setStatusInitPage;
    pcnbp->pfncbItemChanged = setStatusItemChanged;
    // for this page, start a timer
    pcnbp->ulTimer = 1000;
    pcnbp->pfncbTimer       = setStatusTimer;
    ntbInsertPage(pcnbp);
#endif

    // insert "XWorkplace Features" page
    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndDlg;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = cmnGetString(ID_XSSI_FEATURES);  // pszFeatures
    pcnbp->ulDlgID = ID_XCD_FEATURES;
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_XC_FEATURES;
    pcnbp->ulPageID = SP_SETUP_FEATURES;
    pcnbp->pampControlFlags = G_ampFeaturesPage;
    pcnbp->cControlFlags = sizeof(G_ampFeaturesPage) / sizeof(G_ampFeaturesPage[0]);
    pcnbp->pfncbInitPage    = setFeaturesInitPage;
    pcnbp->pfncbItemChanged = setFeaturesItemChanged;
    pcnbp->pfncbMessage = setFeaturesMessages;
#ifndef __XWPLITE__
    ntbInsertPage(pcnbp);

    // insert logo page  V0.9.6 (2000-11-04) [umoeller]
    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndDlg;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = "XWorkplace";
    pcnbp->ulDlgID = ID_XCD_FIRST;
    pcnbp->ulPageID = SP_SETUP_XWPLOGO;
    pcnbp->pfncbInitPage    = setLogoInitPage;
    pcnbp->pfncbMessage = setLogoMessages;
#endif

    return (ntbInsertPage(pcnbp));
}

/*
 *@@ wpQueryDefaultView:
 *      this WPObject method returns the default view of an object,
 *      that is, which view is opened if the program file is
 *      double-clicked upon. This is also used to mark
 *      the default view in the "Open" context submenu.
 *
 *      This must be overridden for direct WPAbstract subclasses,
 *      because otherwise double-clicks on the object won't
 *      work.
 */

SOM_Scope ULONG  SOMLINK xwset_wpQueryDefaultView(XWPSetup *somSelf)
{
    XWPSetupMethodDebug("XWPSetup","xwset_wpQueryDefaultView");

    return (OPEN_SETTINGS);     // settings view is default
}

/*
 *@@ wpFilterPopupMenu:
 *      this WPObject instance method allows the object to
 *      filter out unwanted menu items from the context menu.
 *      This gets called before wpModifyPopupMenu.
 *
 *      We remove the "Create another" menu item.
 *
 *@@added V0.9.2 (2000-02-26) [umoeller]
 */

SOM_Scope ULONG  SOMLINK xwset_wpFilterPopupMenu(XWPSetup *somSelf,
                                                 ULONG ulFlags,
                                                 HWND hwndCnr,
                                                 BOOL fMultiSelect)
{
    /* XWPSetupData *somThis = XWPSetupGetData(somSelf); */
    XWPSetupMethodDebug("XWPSetup","xwset_wpFilterPopupMenu");

    return (XWPSetup_parent_WPAbstract_wpFilterPopupMenu(somSelf,
                                                         ulFlags,
                                                         hwndCnr,
                                                         fMultiSelect)
            & ~CTXT_NEW
           );
}

/*
 *@@ wpQueryDefaultHelp:
 *      this WPObject instance method specifies the default
 *      help panel for an object (when "Extended help" is
 *      selected from the object's context menu). This should
 *      describe what this object can do in general.
 *      We must return TRUE to report successful completion.
 *
 *      We will display some introduction to "XWorkplace Setup".
 *
 *@@added V0.9.0 [umoeller]
 */

SOM_Scope BOOL  SOMLINK xwset_wpQueryDefaultHelp(XWPSetup *somSelf,
                                                 PULONG pHelpPanelId,
                                                 PSZ HelpLibrary)
{
    /* XWPSetupData *somThis = XWPSetupGetData(somSelf); */
    XWPSetupMethodDebug("XWPSetup","xwset_wpQueryDefaultHelp");

    strcpy(HelpLibrary, cmnQueryHelpLibrary());
    *pHelpPanelId = ID_XSH_XWPSETUP;
    return (TRUE);
}

/*
 *@@ wpAddObjectWindowPage:
 *      this WPObject instance method normally adds the
 *      "Standard Options" page to the settings notebook
 *      (that's what the WPS reference calls it; it's actually
 *      the "Window" page).
 *
 *      We don't want that page here, so we remove it.
 */

SOM_Scope ULONG  SOMLINK xwset_wpAddObjectWindowPage(XWPSetup *somSelf,
                                                     HWND hwndNotebook)
{
    /* XWPSetupData *somThis = XWPSetupGetData(somSelf); */
    XWPSetupMethodDebug("XWPSetup","xwset_wpAddObjectWindowPage");

    return (SETTINGS_PAGE_REMOVED);
}

/*
 *@@ wpAddSettingsPages:
 *      this WPObject instance method gets called by the WPS
 *      when the Settings view is opened to have all the
 *      settings page inserted into hwndNotebook.
 *
 *      We add the various new pages here.
 */

SOM_Scope BOOL  SOMLINK xwset_wpAddSettingsPages(XWPSetup *somSelf,
                                                 HWND hwndNotebook)
{
    /* XWPSetupData *somThis = XWPSetupGetData(somSelf); */
    XWPSetupMethodDebug("XWPSetup","xwset_wpAddSettingsPages");

    XWPSetup_parent_WPAbstract_wpAddSettingsPages(somSelf,
                                                  hwndNotebook);
    // add XWorkplace pages on top
    _xwpAddXWPSetupPages(somSelf, hwndNotebook);

    return (TRUE);
}

/* ******************************************************************
 *
 *   XWPSetup class methods
 *
 ********************************************************************/

/*
 *@@ wpclsInitData:
 *      initialize XWPSetup class data.
 */

SOM_Scope void  SOMLINK xwsetM_wpclsInitData(M_XWPSetup *somSelf)
{
    /* M_XWPSetupData *somThis = M_XWPSetupGetData(somSelf); */
    M_XWPSetupMethodDebug("M_XWPSetup","xwsetM_wpclsInitData");

    M_XWPSetup_parent_M_WPAbstract_wpclsInitData(somSelf);

    krnClassInitialized(G_pcszXWPSetup);
}

/*
 * wpclsQueryStyle:
 *      prevent copy, delete, print.
 */

SOM_Scope ULONG  SOMLINK xwsetM_wpclsQueryStyle(M_XWPSetup *somSelf)
{
    /* M_XWPSetupData *somThis = M_XWPSetupGetData(somSelf); */
    M_XWPSetupMethodDebug("M_XWPSetup","xwsetM_wpclsQueryStyle");

    return (M_XWPSetup_parent_M_WPAbstract_wpclsQueryStyle(somSelf)
                | CLSSTYLE_DONTTEMPLATE
                | CLSSTYLE_NEVERPRINT
                | CLSSTYLE_NEVERCOPY
                | CLSSTYLE_NEVERDELETE);
}

/*
 *@@ wpclsQueryTitle:
 *      this WPObject class method tells the WPS the clear
 *      name of a class, which is shown in the third column
 *      of a Details view and also used as the default title
 *      for new objects of a class.
 */

SOM_Scope PSZ  SOMLINK xwsetM_wpclsQueryTitle(M_XWPSetup *somSelf)
{
    /* M_XWPSetupData *somThis = M_XWPSetupGetData(somSelf); */
    M_XWPSetupMethodDebug("M_XWPSetup","xwsetM_wpclsQueryTitle");

    return ("XWorkplace Setup");
}

/*
 *@@ wpclsQueryIconData:
 *      this WPObject class method builds the default
 *      icon for objects of a class (i.e. the icon which
 *      is shown if no instance icon is assigned). This
 *      apparently gets called from some of the other
 *      icon instance methods if no instance icon was
 *      found for an object. The exact mechanism of how
 *      this works is not documented.
 *
 *      We override this to give XWPSetup objects a new
 *      icon (src\shared\xwpsetup.ico).
 */

SOM_Scope ULONG  SOMLINK xwsetM_wpclsQueryIconData(M_XWPSetup *somSelf,
                                                   PICONINFO pIconInfo)
{
    /* M_XWPSetupData *somThis = M_XWPSetupGetData(somSelf); */
    M_XWPSetupMethodDebug("M_XWPSetup","xwsetM_wpclsQueryIconData");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXWPCONFG;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return (sizeof(ICONINFO));
}

/*
 *@@ wpclsQuerySettingsPageSize:
 *      this WPObject class method should return the
 *      size of the largest settings page in dialog
 *      units; if a settings notebook is initially
 *      opened, i.e. no window pos has been stored
 *      yet, the WPS will use this size, to avoid
 *      truncated settings pages.
 */

SOM_Scope BOOL  SOMLINK xwsetM_wpclsQuerySettingsPageSize(M_XWPSetup *somSelf,
                                                          PSIZEL pSizl)
{
    /* M_XWPSetupData *somThis = M_XWPSetupGetData(somSelf); */
    M_XWPSetupMethodDebug("M_XWPSetup","xwsetM_wpclsQuerySettingsPageSize");

    return (M_XWPSetup_parent_M_WPAbstract_wpclsQuerySettingsPageSize(somSelf,
                                                                       pSizl));
}



