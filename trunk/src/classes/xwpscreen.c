
/*
 *@@sourcefile xwpscreen.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPScreen (WPSystem subclass)
 *
 *      XFldSystem implements the "Screen" settings object
 *      for PM manipulation and PageMage configuration.
 *
 *      Installation of this class is optional, but you cannot
 *      configure PageMage without it.
 *
 *      This is all new with V0.9.2 (2000-02-23) [umoeller].
 *
 *      Starting with V0.9.0, the files in classes\ contain only
 *      the SOM interface, i.e. the methods themselves.
 *      The implementation for this class is mostly in config\pagemage.c.
 *
 *@@added V0.9.2 (2000-02-23) [umoeller]
 *@@somclass XWPScreen xwpscr_
 *@@somclass M_XWPScreen xwpscrM_
 */

/*
 *      Copyright (C) 2000 Ulrich M�ller.
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

#ifndef SOM_Module_xwpscreen_Source
#define SOM_Module_xwpscreen_Source
#endif
#define XWPScreen_Class_Source
#define M_XWPScreen_Class_Source

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

#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS
#define INCL_WINWINDOWMGR
#include <os2.h>

// C library headers

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\dosh.h"               // Control Program helper routines

// SOM headers which don't crash with prec. header files
#include "xwpscreen.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling

#include "config\hookintf.h"            // daemon/hook interface
#include "config\pagemage.h"            // PageMage interface

// other SOM headers
#pragma hdrstop                         // VAC++ keeps crashing otherwise
// #include "xfobj.h"

/* ******************************************************************
 *                                                                  *
 *   here come the XWPScreen instance methods                       *
 *                                                                  *
 ********************************************************************/

/*
 *@@ xwpAddXWPScreenPages:
 *      adds the "PageMage" pages to the "Screen" notebook.
 *
 *@@added V0.9.3 (2000-04-09) [umoeller]
 *@@changed V0.9.9 (2001-03-15) [lafaix]: added a new 'pagemage window' page
 *@@changed V0.9.9 (2001-03-27) [umoeller]: moved "Corners" from XWPMouse to XWPScreen
 *@@changed V0.9.9 (2001-04-04) [lafaix]: renamed to "Screen borders" page
 */

SOM_Scope ULONG  SOMLINK xwpscr_xwpAddXWPScreenPages(XWPScreen *somSelf,
                                                     HWND hwndDlg)
{
    ULONG ulrc = 0;

    /* XWPScreenData *somThis = XWPScreenGetData(somSelf); */
    XWPScreenMethodDebug("XWPScreen","xwpscr_xwpAddXWPScreenPages");

    // hook installed?
    if (hifXWPHookReady())
    {
        PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
        PCREATENOTEBOOKPAGE pcnbp;
        HMODULE         savehmod = cmnQueryNLSModuleHandle(FALSE);
        // PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();

        // moved this here from "Mouse" V0.9.9 (2001-03-27) [umoeller]
        pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
        memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
        pcnbp->somSelf = somSelf;
        pcnbp->hwndNotebook = hwndDlg;
        pcnbp->hmod = savehmod;
        pcnbp->ulDlgID = ID_XSD_MOUSE_CORNERS;
        pcnbp->usPageStyleFlags = BKA_MAJOR;
        pcnbp->pszName = cmnGetString(ID_XSSI_SCREENBORDERSPAGE);  // pszScreenBordersPage
        // pcnbp->fEnumerate = TRUE;
        pcnbp->ulDefaultHelpPanel  = ID_XSH_MOUSE_CORNERS;
        pcnbp->ulPageID = SP_MOUSE_CORNERS;
        pcnbp->pfncbInitPage    = hifMouseCornersInitPage;
        pcnbp->pfncbItemChanged = hifMouseCornersItemChanged;
        ulrc = ntbInsertPage(pcnbp);

        if (pGlobalSettings->fEnablePageMage)
        {
            // "PageMage" colors
            pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
            memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
            pcnbp->somSelf = somSelf;
            pcnbp->hwndNotebook = hwndDlg;
            pcnbp->hmod = savehmod;
            pcnbp->pfncbInitPage    = pgmiPageMageColorsInitPage;
            pcnbp->pfncbItemChanged = pgmiPageMageColorsItemChanged;
            pcnbp->usPageStyleFlags = BKA_MINOR;
            pcnbp->fEnumerate = TRUE;
            pcnbp->pszName = "~PageMage";
            pcnbp->ulDlgID = ID_SCD_PAGEMAGE_COLORS;
            pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_PAGEMAGE_COLORS;
            // give this page a unique ID, which is
            // passed to the common config.sys callbacks
            pcnbp->ulPageID = SP_PAGEMAGE_COLORS;
            ntbInsertPage(pcnbp);

            // "PageMage" sticky windows
            pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
            memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
            pcnbp->somSelf = somSelf;
            pcnbp->hwndNotebook = hwndDlg;
            pcnbp->hmod = savehmod;
            pcnbp->pfncbInitPage    = pgmiPageMageStickyInitPage;
            pcnbp->pfncbItemChanged = pgmiPageMageStickyItemChanged;
            pcnbp->usPageStyleFlags = BKA_MINOR;
            pcnbp->fEnumerate = TRUE;
            pcnbp->pszName = "~PageMage";
            pcnbp->ulDlgID = ID_SCD_PAGEMAGE_STICKY;
            pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_PAGEMAGE_STICKY;
            // give this page a unique ID, which is
            // passed to the common config.sys callbacks
            pcnbp->ulPageID = SP_PAGEMAGE_STICKY;
            ntbInsertPage(pcnbp);

            // "PageMage" window settings V0.9.9 (2001-03-15) [lafaix]
            pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
            memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
            pcnbp->somSelf = somSelf;
            pcnbp->hwndNotebook = hwndDlg;
            pcnbp->hmod = savehmod;
            pcnbp->pfncbInitPage    = pgmiPageMageWindowInitPage;
            pcnbp->pfncbItemChanged = pgmiPageMageWindowItemChanged;
            pcnbp->usPageStyleFlags = BKA_MINOR;
            pcnbp->fEnumerate = TRUE;
            pcnbp->pszName = "~PageMage";
            pcnbp->ulDlgID = ID_SCD_PAGEMAGE_WINDOW;
            pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_PAGEMAGE_WINDOW;
            // give this page a unique ID, which is
            // passed to the common config.sys callbacks
            pcnbp->ulPageID = SP_PAGEMAGE_WINDOW;
            ntbInsertPage(pcnbp);

            // "PageMage" general settings
            pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
            memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
            pcnbp->somSelf = somSelf;
            pcnbp->hwndNotebook = hwndDlg;
            pcnbp->hmod = savehmod;
            pcnbp->pfncbInitPage    = pgmiPageMageGeneralInitPage;
            pcnbp->pfncbItemChanged = pgmiPageMageGeneralItemChanged;
            pcnbp->usPageStyleFlags = BKA_MAJOR;
            pcnbp->fEnumerate = TRUE;
            pcnbp->pszName = "~PageMage";
            pcnbp->ulDlgID = ID_SCD_PAGEMAGE_GENERAL;
            pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_PAGEMAGE_GENERAL;
            // give this page a unique ID, which is
            // passed to the common config.sys callbacks
            pcnbp->ulPageID = SP_PAGEMAGE_MAIN;
            ulrc = ntbInsertPage(pcnbp);
        }
    }

    return (ulrc);
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

SOM_Scope ULONG  SOMLINK xwpscr_wpFilterPopupMenu(XWPScreen *somSelf,
                                                  ULONG ulFlags,
                                                  HWND hwndCnr,
                                                  BOOL fMultiSelect)
{
    /* XWPScreenData *somThis = XWPScreenGetData(somSelf); */
    XWPScreenMethodDebug("XWPScreen","xwpscr_wpFilterPopupMenu");

    return (XWPScreen_parent_WPSystem_wpFilterPopupMenu(somSelf,
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
 *      We'll display some help for the "Screen" object.
 */

SOM_Scope BOOL  SOMLINK xwpscr_wpQueryDefaultHelp(XWPScreen *somSelf,
                                                  PULONG pHelpPanelId,
                                                  PSZ HelpLibrary)
{
    /* XWPScreenData *somThis = XWPScreenGetData(somSelf); */
    XWPScreenMethodDebug("XWPScreen","xwpscr_wpQueryDefaultHelp");

    strcpy(HelpLibrary, cmnQueryHelpLibrary());
    *pHelpPanelId = ID_XSH_XWPSCREEN;
    return (TRUE);
}

/*
 *@@ wpAddSettingsPages:
 *      this WPObject instance method gets called by the WPS
 *      when the Settings view is opened to have all the
 *      settings page inserted into hwndNotebook.
 *
 *      In order to to this, unlike the procedure used in
 *      the "Workplace Shell" object, we will explicitly
 *      call the WPSystem methods which insert the
 *      pages we want to see here into the notebook.
 *      As a result, the "Workplace Shell" object
 *      inherits all pages from the "System" object
 *      which might be added by other WPS utils, while
 *      this thing does not.
 *
 */

SOM_Scope BOOL  SOMLINK xwpscr_wpAddSettingsPages(XWPScreen *somSelf,
                                                  HWND hwndNotebook)
{
    /* XWPScreenData *somThis = XWPScreenGetData(somSelf); */
    XWPScreenMethodDebug("XWPScreen","xwpscr_wpAddSettingsPages");

    // do _not_ call the parent, but call the page methods
    // explicitly

    // XFolder "Internals" page bottommost
    // _xwpAddObjectInternalsPage(somSelf, hwndNotebook);

    // "Symbol" page next
    _wpAddObjectGeneralPage(somSelf, hwndNotebook);
        // this inserts the "Internals"/"Object" page now

    // "print screen" page next
    _wpAddSystemPrintScreenPage(somSelf, hwndNotebook);

    // XWorkplace screen pages
    _xwpAddXWPScreenPages(somSelf, hwndNotebook);

    // "Screen" page 2 next; this page may exist on some systems
    // depending on the video driver, and we want this in "Screen"
    // also
    _wpAddDMQSDisplayTypePage(somSelf, hwndNotebook);
    // "Screen" page 1 next
    _wpAddSystemScreenPage(somSelf, hwndNotebook);

    return (TRUE);
}

/* ******************************************************************
 *                                                                  *
 *   here come the XWPScreen class methods                          *
 *                                                                  *
 ********************************************************************/

/*
 *@@ wpclsInitData:
 *      initialize XWPScreen class data.
 */

SOM_Scope void  SOMLINK xwpscrM_wpclsInitData(M_XWPScreen *somSelf)
{
    /* M_XWPScreenData *somThis = M_XWPScreenGetData(somSelf); */
    M_XWPScreenMethodDebug("M_XWPScreen","xwpscrM_wpclsInitData");

    M_XWPScreen_parent_M_WPSystem_wpclsInitData(somSelf);

    {
        // store the class object in KERNELGLOBALS
        PKERNELGLOBALS   pKernelGlobals = krnLockGlobals(__FILE__, __LINE__, __FUNCTION__);
        if (pKernelGlobals)
        {
            pKernelGlobals->fXWPScreen = TRUE;
            krnUnlockGlobals();
        }
    }
}

/*
 *@@ wpclsQuerySettingsPageSize:
 *      this WPObject class method should return the
 *      size of the largest settings page in dialog
 *      units; if a settings notebook is initially
 *      opened, i.e. no window pos has been stored
 *      yet, the WPS will use this size, to avoid
 *      truncated settings pages.
 *
 *@@added V0.9.5 (2000-08-26) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xwpscrM_wpclsQuerySettingsPageSize(M_XWPScreen *somSelf,
                                                           PSIZEL pSizl)
{
    BOOL brc = FALSE;
    /* M_XWPScreenData *somThis = M_XWPScreenGetData(somSelf); */
    M_XWPScreenMethodDebug("M_XWPScreen","xwpscrM_wpclsQuerySettingsPageSize");

    brc = M_XWPScreen_parent_M_WPSystem_wpclsQuerySettingsPageSize(somSelf,
                                                                   pSizl);
    if (brc)
    {
        LONG lCompCY = 160;     // this is the height of the "PageMage General" page
                                // which seems to be the largest
        if (doshIsWarp4())
            // on Warp 4, reduce again, because we're moving
            // the notebook buttons to the bottom
            lCompCY -= WARP4_NOTEBOOK_OFFSET;
        if (pSizl->cy < lCompCY)
            pSizl->cy = lCompCY;
    }

    return (brc);
}

/*
 *@@ wpclsQueryTitle:
 *
 */

SOM_Scope PSZ  SOMLINK xwpscrM_wpclsQueryTitle(M_XWPScreen *somSelf)
{
    // PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();
    /* M_XWPScreenData *somThis = M_XWPScreenGetData(somSelf); */
    M_XWPScreenMethodDebug("M_XWPScreen","xwpscrM_wpclsQueryTitle");

    return (cmnGetString(ID_XSSI_XWPSCREENTITLE)) ; // pszXWPScreenTitle
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
 *      We give the "Screen" object a new standard icon here.
 */

SOM_Scope ULONG  SOMLINK xwpscrM_wpclsQueryIconData(M_XWPScreen *somSelf,
                                                    PICONINFO pIconInfo)
{
    /* M_XWPScreenData *somThis = M_XWPScreenGetData(somSelf); */
    M_XWPScreenMethodDebug("M_XWPScreen","xwpscrM_wpclsQueryIconData");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXWPSCREEN;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return (sizeof(ICONINFO));
}

