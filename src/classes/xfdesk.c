
/*
 *@@sourcefile xfdesk.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XFldDesktop (WPDesktop replacement)
 *
 *      XFldDesktop provides access to the eXtended shutdown
 *      feature (fntShutdownThread) by modifying popup menus.
 *
 *      Also, the XFldDesktop settings notebook pages offer access
 *      to startup and shutdown configuration data.
 *
 *      Installation of this class is now optional (V0.9.0).
 *
 *      Starting with V0.9.0, the files in classes\ contain only
 *      i.e. the methods themselves.
 *      The implementation for this class is mostly in filesys\desktop.c,
 *      filesys\shutdown.c, and filesys\archives.c.
 *
 *@@somclass XFldDesktop xfdesk_
 *@@somclass M_XFldDesktop xfdeskM_
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

/*
 *@@todo:
 *
 */

/*
 *  This file was generated by the SOM Compiler and Emitter Framework.
 *  Generated using:
 *      SOM Emitter emitctm: 2.41
 */

#ifndef SOM_Module_xfdesk_Source
#define SOM_Module_xfdesk_Source
#endif
#define XFldDesktop_Class_Source
#define M_XFldDesktop_Class_Source

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
#define INCL_DOSERRORS
#define INCL_WINWINDOWMGR
#define INCL_WINMENUS
#include <os2.h>

// C library headers
#include <stdio.h>
#include <io.h>
#include <math.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\stringh.h"            // string helper routines
#include "helpers\winh.h"               // PM helper routines

// SOM headers which don't crash with prec. header files
#include "xfdesk.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)

#include "filesys\desktop.h"            // XFldDesktop implementation
#include "filesys\xthreads.h"           // extra XWorkplace threads

#include "startshut\archives.h"         // WPSArcO declarations
#include "startshut\shutdown.h"         // XWorkplace eXtended Shutdown

// other SOM headers
#pragma hdrstop                 // VAC++ keeps crashing otherwise
#include "xfobj.h"
#include "xfldr.h"

/* ******************************************************************
 *                                                                  *
 *   Global variables                                               *
 *                                                                  *
 ********************************************************************/

BOOL    G_DesktopPopulated = FALSE;

/* ******************************************************************
 *                                                                  *
 *   here come the XFldDesktop instance methods                     *
 *                                                                  *
 ********************************************************************/

/*
 *@@ xwpInsertXFldDesktopMenuItemsPage:
 *      this actually adds the new "Menu items" page replacement
 *      to the Desktop's settings notebook.
 *
 *      This gets called from XFldDesktop::wpAddSettingsPages.
 *
 *      added V0.9.0
 */

SOM_Scope ULONG  SOMLINK xfdesk_xwpInsertXFldDesktopMenuItemsPage(XFldDesktop *somSelf,
                                                                 HWND hwndNotebook)
{
    PCREATENOTEBOOKPAGE pcnbp;
    HMODULE         savehmod = cmnQueryNLSModuleHandle(FALSE);
    PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_xwpInsertXFldDesktopMenuItemsPage");

    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndNotebook;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->fEnumerate = TRUE;
    pcnbp->pszName = pNLSStrings->pszDtpMenuPage;
    pcnbp->usFirstControlID = ID_XSDI_DTP_SORT;
    // pcnbp->ulFirstSubpanel = ID_XSH_SETTINGS_DTP1_SUB;   // help panel for "Sort"
    pcnbp->ulDlgID = ID_XSD_DTP_MENUITEMS;
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_DTP_MENUITEMS;
    pcnbp->ulPageID = SP_DTP_MENUITEMS;
    pcnbp->pfncbInitPage    = dtpMenuItemsInitPage;
    pcnbp->pfncbItemChanged = dtpMenuItemsItemChanged;
    return (ntbInsertPage(pcnbp));
}

/*
 *@@ xwpInsertXFldDesktopStartupPage:
 *      this actually adds the new "Startup" page replacement
 *      to the Desktop's settings notebook.
 *
 *      This gets called from XFldDesktop::wpAddSettingsPages.
 *
 *      added V0.9.0
 */

SOM_Scope ULONG  SOMLINK xfdesk_xwpInsertXFldDesktopStartupPage(XFldDesktop *somSelf,
                                                               HWND hwndNotebook)
{
    PCREATENOTEBOOKPAGE pcnbp;
    HMODULE         savehmod = cmnQueryNLSModuleHandle(FALSE);
    PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_xwpInsertXFldDesktopStartupPage");

    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndNotebook;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = pNLSStrings->pszStartupPage;
    pcnbp->ulDlgID = ID_XSD_DTP_STARTUP;
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_DTP_STARTUP;
    pcnbp->ulPageID = SP_DTP_STARTUP;
    pcnbp->pfncbInitPage    = dtpStartupInitPage;
    pcnbp->pfncbItemChanged = dtpStartupItemChanged;
    return (ntbInsertPage(pcnbp));
}

/*
 *@@ xwpInsertXFldDesktopArchivesPage:
 *      this actually adds the new "Archives" page replacement
 *      to the Desktop's settings notebook.
 *
 *      This gets called from XFldDesktop::wpAddSettingsPages.
 *
 *      added V0.9.0
 */

SOM_Scope ULONG  SOMLINK xfdesk_xwpInsertXFldDesktopArchivesPage(XFldDesktop *somSelf,
                                                                HWND hwndNotebook)
{
    PCREATENOTEBOOKPAGE pcnbp;
    HMODULE         savehmod = cmnQueryNLSModuleHandle(FALSE);
    PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_xwpInsertXFldDesktopArchivesPage");

    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndNotebook;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = pNLSStrings->pszArchivesPage;
    pcnbp->ulDlgID = ID_XSD_DTP_ARCHIVES;
    // pcnbp->usFirstControlID = ID_SDDI_ARCHIVES;
    // pcnbp->ulFirstSubpanel = ID_XSH_SETTINGS_DTP_SHUTDOWN_SUB;   // help panel for "System setup"
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_DTP_ARCHIVES;
    pcnbp->ulPageID = SP_DTP_ARCHIVES;
    pcnbp->pfncbInitPage    = arcArchivesInitPage;
    pcnbp->pfncbItemChanged = arcArchivesItemChanged;
    return (ntbInsertPage(pcnbp));
}

/*
 *@@ xwpInsertXFldDesktopShutdownPage:
 *      this actually adds the new "Shutdown" page replacement
 *      to the Desktop's settings notebook.
 *
 *      This gets called from XFldDesktop::wpAddSettingsPages.
 *
 *      added V0.9.0
 */

SOM_Scope ULONG  SOMLINK xfdesk_xwpInsertXFldDesktopShutdownPage(XFldDesktop *somSelf,
                                                                HWND hwndNotebook)
{
    PCREATENOTEBOOKPAGE pcnbp;
    HMODULE         savehmod = cmnQueryNLSModuleHandle(FALSE);
    PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_xwpInsertXFldDesktopShutdownPage");

    // insert "XShutdown" page,
    // if Shutdown has been enabled in XWPConfig
    pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
    memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
    pcnbp->somSelf = somSelf;
    pcnbp->hwndNotebook = hwndNotebook;
    pcnbp->hmod = savehmod;
    pcnbp->usPageStyleFlags = BKA_MAJOR;
    pcnbp->pszName = pNLSStrings->pszXShutdownPage;
    pcnbp->ulDlgID = ID_XSD_DTP_SHUTDOWN;
    pcnbp->usFirstControlID = ID_SDDI_REBOOT;
    // pcnbp->ulFirstSubpanel = ID_XSH_SETTINGS_DTP_SHUTDOWN_SUB;   // help panel for "System setup"
    pcnbp->ulDefaultHelpPanel  = ID_XSH_SETTINGS_DTP_SHUTDOWN;
    pcnbp->ulPageID = SP_DTP_SHUTDOWN;
    pcnbp->pfncbInitPage    = xsdShutdownInitPage;
    pcnbp->pfncbItemChanged = xsdShutdownItemChanged;
    return (ntbInsertPage(pcnbp));
}

/*
 *@@ xwpQuerySetup2:
 *      this XFldObject method is overridden to support
 *      setup strings for the Desktop.
 *
 *      See XFldObject::xwpQuerySetup2 for details.
 *
 *@@added V0.9.1 (2000-01-08) [umoeller]
 */

SOM_Scope ULONG  SOMLINK xfdesk_xwpQuerySetup2(XFldDesktop *somSelf,
                                               PSZ pszSetupString,
                                               ULONG cbSetupString)
{
    ULONG ulReturn = 0;

    // method pointer for parent class
    somTD_XFldObject_xwpQuerySetup pfn_xwpQuerySetup2 = 0;

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_xwpQuerySetup2");

    // call XFldDesktop implementation
    ulReturn = dtpQuerySetup(somSelf, pszSetupString, cbSetupString);

    // manually resolve parent method
    pfn_xwpQuerySetup2
        = (somTD_XFldObject_xwpQuerySetup)wpshParentResolve(somSelf,
                                                            _XFldDesktop,
                                                            "xwpQuerySetup2");
    if (pfn_xwpQuerySetup2)
    {
        // now call parent method (probably XFolder)
        if ( (pszSetupString) && (cbSetupString) )
            // string buffer already specified:
            // tell XFolder to append to that string
            ulReturn += pfn_xwpQuerySetup2(somSelf,
                                           pszSetupString + ulReturn, // append to existing
                                           cbSetupString - ulReturn); // remaining size
        else
            // string buffer not yet specified: return length only
            ulReturn += pfn_xwpQuerySetup2(somSelf, 0, 0);
    }

    return (ulReturn);
}

/*
 *@@ wpFilterPopupMenu:
 *      this WPObject instance method allows the object to
 *      filter out unwanted menu items from the context menu.
 *      This gets called before wpModifyPopupMenu.
 *
 *      We remove "Create another" for Desktop, because
 *      we don't want to allow creating another Desktop.
 *      For some reason, the "Create another" option
 *      doesn't seem to be working right with XFolder,
 *      so we need to add all this manually (see
 *      XFldObject::wpFilterPopupMenu).
 */

SOM_Scope ULONG  SOMLINK xfdesk_wpFilterPopupMenu(XFldDesktop *somSelf,
                                                  ULONG ulFlags,
                                                  HWND hwndCnr,
                                                  BOOL fMultiSelect)
{
    // items to suppress
    ULONG   ulSuppress = CTXT_CRANOTHER;
    PCGLOBALSETTINGS     pGlobalSettings = cmnQueryGlobalSettings();

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_wpFilterPopupMenu");

    // suppress sort menu?
    if (!pGlobalSettings->fDTMSort)
        ulSuppress |= CTXT_SORT;
    if (!pGlobalSettings->fDTMArrange)
        ulSuppress |= CTXT_ARRANGE;

    return (XFldDesktop_parent_WPDesktop_wpFilterPopupMenu(somSelf,
                                                           ulFlags,
                                                           hwndCnr,
                                                           fMultiSelect)
            & ~(ulSuppress));
}

/*
 *@@ wpModifyPopupMenu:
 *      this WPObject instance methods gets called by the WPS
 *      when a context menu needs to be built for the object
 *      and allows the object to manipulate its context menu.
 *      This gets called _after_ wpFilterPopupMenu.
 *
 *      We play with the Desktop menu entries
 *      (Shutdown and such)
 *
 *@@changed V0.9.0 [umoeller]: reworked context menu items
 */

SOM_Scope BOOL  SOMLINK xfdesk_wpModifyPopupMenu(XFldDesktop *somSelf,
                                                 HWND hwndMenu,
                                                 HWND hwndCnr,
                                                 ULONG iPosition)
{
    BOOL rc;

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_wpModifyPopupMenu");

    // calling the parent (which is XFolder!) will insert all the
    // variable menu items
    rc = XFldDesktop_parent_WPDesktop_wpModifyPopupMenu(somSelf,
                                                           hwndMenu,
                                                           hwndCnr,
                                                           iPosition);

    if (rc)
        if (_wpIsCurrentDesktop(somSelf))
            dtpModifyPopupMenu(somSelf,
                               hwndMenu);

    return (rc);
}

/*
 *@@ wpMenuItemSelected:
 *      this WPObject method processes menu selections.
 *      This must be overridden to support new menu
 *      items which have been added in wpModifyPopupMenu.
 *
 *      Note that the WPS invokes this method upon every
 *      object which has been selected in the container.
 *      That is, if three objects have been selected and
 *      a menu item has been selected for all three of
 *      them, all three objects will receive this method
 *      call. This is true even if FALSE is returned from
 *      this method.
 *
 *@@changed V0.9.3 (2000-04-26) [umoeller]: now allowing dtpMenuItemSelected to change the menu ID
 */

SOM_Scope BOOL  SOMLINK xfdesk_wpMenuItemSelected(XFldDesktop *somSelf,
                                                  HWND hwndFrame,
                                                  ULONG ulMenuId)
{
    ULONG   ulMenuId2 = ulMenuId;
    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_wpMenuItemSelected");

    if (dtpMenuItemSelected(somSelf, hwndFrame, &ulMenuId2))
        // one of the new items processed:
        return (TRUE);
    else
        return (XFldDesktop_parent_WPDesktop_wpMenuItemSelected(somSelf,
                                                                hwndFrame,
                                                                ulMenuId2));
}

/*
 *@@ wpPopulate:
 *      this instance method populates a folder, in this case, the
 *      Desktop. After the active Desktop has been populated at
 *      WPS startup, we'll post a message to the Worker thread to
 *      initiate all the XWorkplace startup processing.
 *
 *@@changed V0.9.5 (2000-08-26) [umoeller]: this was previously done in wpOpen
 */

SOM_Scope BOOL  SOMLINK xfdesk_wpPopulate(XFldDesktop *somSelf,
                                          ULONG ulReserved,
                                          PSZ pszPath,
                                          BOOL fFoldersOnly)
{
    BOOL    brc = FALSE;
    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_wpPopulate");

    #ifdef DEBUG_STARTUP
        _Pmpf(("XFldDesktop::wpPopulate"));
    #endif

    brc = XFldDesktop_parent_WPDesktop_wpPopulate(somSelf,
                                                  ulReserved,
                                                  pszPath,
                                                  fFoldersOnly);

    #ifdef DEBUG_STARTUP
        _Pmpf(("XFldDesktop::wpPopulate: checking whether Worker thread needs notify"));
    #endif

    if (!G_DesktopPopulated)
        if (_wpIsCurrentDesktop(somSelf))
        {
            // first call:
            G_DesktopPopulated = TRUE;

            #ifdef DEBUG_STARTUP
                _Pmpf(("  posting FIM_DESKTOPPOPULATED"));
            #endif
            xthrPostFileMsg(FIM_DESKTOPPOPULATED,
                            (MPARAM)somSelf,
                            0);
            // the worker thread will now loop until the Desktop
            // is populated also
        }

    #ifdef DEBUG_STARTUP
        _Pmpf(("End of XFldDesktop::wpPopulate"));
    #endif

    return (brc);
}

/*
 *@@ wpAddDesktopArcRest1Page:
 *      this instance method inserts the "Archives" page
 *      into the Desktop's settings notebook. If the
 *      extended archiving has been enabled, we return
 *      SETTINGS_PAGE_REMOVED because we want the "Archives"
 *      page at a different position, which gets inserted
 *      from XFldDesktop::wpAddSettingsPages then.
 *
 *@@added V0.9.0 [umoeller]
 */

SOM_Scope ULONG  SOMLINK xfdesk_wpAddDesktopArcRest1Page(XFldDesktop *somSelf,
                                                         HWND hwndNotebook)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_wpAddDesktopArcRest1Page");

    if (pGlobalSettings->fReplaceArchiving)
        // remove this, we'll add a new one at a different
        // location in wpAddSettingsPages
        return (SETTINGS_PAGE_REMOVED);
    else
        return (XFldDesktop_parent_WPDesktop_wpAddDesktopArcRest1Page(somSelf,
                                                                      hwndNotebook));
}

/*
 *@@ wpAddSettingsPages:
 *      this WPObject instance method gets called by the WPS
 *      when the Settings view is opened to have all the
 *      settings page inserted into hwndNotebook.
 *
 *      As opposed to the "XFolder" page, which deals with instance
 *      data, we save the Desktop settings in the GLOBALSETTINGS
 *      structure, because there should ever be only one active
 *      Desktop.
 *
 *@@changed V0.9.0 [umoeller]: reworked settings pages
 */

SOM_Scope BOOL  SOMLINK xfdesk_wpAddSettingsPages(XFldDesktop *somSelf,
                                                  HWND hwndNotebook)
{
    BOOL            rc;

    // XFldDesktopData *somThis = XFldDesktopGetData(somSelf);
    XFldDesktopMethodDebug("XFldDesktop","xfdesk_wpAddSettingsPages");

    rc = (XFldDesktop_parent_WPDesktop_wpAddSettingsPages(somSelf,
                                                            hwndNotebook));
    if (rc)
        if (_wpIsCurrentDesktop(somSelf))
        {
            PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

            // insert "Menu" page
            _xwpInsertXFldDesktopMenuItemsPage(somSelf, hwndNotebook);

            if (pGlobalSettings->fXShutdown)
                _xwpInsertXFldDesktopShutdownPage(somSelf, hwndNotebook);

            if (pGlobalSettings->fReplaceArchiving)
                // insert new "Archives" page;
                // at the same time, the old archives page method
                // will return SETTINGS_PAGE_REMOVED
                _xwpInsertXFldDesktopArchivesPage(somSelf, hwndNotebook);

            // insert "Startup" page
            _xwpInsertXFldDesktopStartupPage(somSelf, hwndNotebook);
        }

    return (rc);
}

/*
 *@@ wpclsInitData:
 *      initialize XFldDesktop class data.
 *
 *@@changed V0.9.0 [umoeller]: added class object to KERNELGLOBALS
 */

SOM_Scope void  SOMLINK xfdeskM_wpclsInitData(M_XFldDesktop *somSelf)
{
    // M_XFldDesktopData *somThis = M_XFldDesktopGetData(somSelf);
    M_XFldDesktopMethodDebug("M_XFldDesktop","xfdeskM_wpclsInitData");

    M_XFldDesktop_parent_M_WPDesktop_wpclsInitData(somSelf);

    {
        PKERNELGLOBALS   pKernelGlobals = krnLockGlobals(5000);
        // store the class object in KERNELGLOBALS
        pKernelGlobals->fXFldDesktop = TRUE;
        krnUnlockGlobals();
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
 */

SOM_Scope BOOL  SOMLINK xfdeskM_wpclsQuerySettingsPageSize(M_XFldDesktop *somSelf,
                                                           PSIZEL pSizl)
{
    BOOL brc;
    /* M_XFldDesktopData *somThis = M_XFldDesktopGetData(somSelf); */
    M_XFldDesktopMethodDebug("M_XFldDesktop","xfdeskM_wpclsQuerySettingsPageSize");

    brc = M_XFldDesktop_parent_M_WPDesktop_wpclsQuerySettingsPageSize(somSelf,
                                                                        pSizl);
    if (brc)
    {
        LONG lCompCY = 153;
        if (doshIsWarp4())
            // on Warp 4, reduce again, because we're moving
            // the notebook buttons to the bottom
            lCompCY -= WARP4_NOTEBOOK_OFFSET;

        if (pSizl->cy < lCompCY)
            pSizl->cy = lCompCY;  // this is the height of the "XDesktop" page,
                                // which is pretty large
        if (pSizl->cx < 260)
            pSizl->cx = 260;    // and the width

    }
    return (brc);
}

/*
 *@@ wpclsQueryIconData:
 *      give XFldDesktop's a new default closed icon, if the
 *      global settings allow this.
 *      This is loaded from /ICONS/ICONS.DLL.
 */

SOM_Scope ULONG  SOMLINK xfdeskM_wpclsQueryIconData(M_XFldDesktop *somSelf,
                                                    PICONINFO pIconInfo)
{
    ULONG       ulrc;
    HMODULE     hmodIconsDLL = NULLHANDLE;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // M_XFldDesktopData *somThis = M_XFldDesktopGetData(somSelf);
    M_XFldDesktopMethodDebug("M_XFldDesktop","xfdeskM_wpclsQueryIconData");

    if (pGlobalSettings->fReplaceIcons)
    {
        hmodIconsDLL = cmnQueryIconsDLL();
        // icon replacements allowed:
        if ((pIconInfo) && (hmodIconsDLL))
        {
            pIconInfo->fFormat = ICON_RESOURCE;
            pIconInfo->hmod = hmodIconsDLL;
            pIconInfo->resid = 110;
        }
        ulrc = sizeof(ICONINFO);
    }

    if (hmodIconsDLL == NULLHANDLE)
        // icon replacements not allowed: call default
        ulrc = M_XFldDesktop_parent_M_WPDesktop_wpclsQueryIconData(somSelf,
                                                                   pIconInfo);
    return (ulrc);
}

/*
 *@@ wpclsQueryIconDataN:
 *      give XFldDesktop's a new open closed icon, if the
 *      global settings allow this.
 *      This is loaded from /ICONS/ICONS.DLL.
 */

SOM_Scope ULONG  SOMLINK xfdeskM_wpclsQueryIconDataN(M_XFldDesktop *somSelf,
                                                     ICONINFO* pIconInfo,
                                                     ULONG ulIconIndex)
{
    ULONG       ulrc;
    HMODULE     hmodIconsDLL = NULLHANDLE;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // M_XFldDesktopData *somThis = M_XFldDesktopGetData(somSelf);
    M_XFldDesktopMethodDebug("M_XFldDesktop","xfdeskM_wpclsQueryIconDataN");

    if (pGlobalSettings->fReplaceIcons)
    {
        hmodIconsDLL = cmnQueryIconsDLL();
        // icon replacements allowed:
        if ((pIconInfo) && (hmodIconsDLL))
        {
            pIconInfo->fFormat = ICON_RESOURCE;
            pIconInfo->hmod = hmodIconsDLL;
            pIconInfo->resid = 111;
        }
        ulrc = sizeof(ICONINFO);
    }

    if (hmodIconsDLL == NULLHANDLE)
        // icon replacements not allowed: call default
        ulrc = M_XFldDesktop_parent_M_WPDesktop_wpclsQueryIconDataN(somSelf,
                                                                    pIconInfo,
                                                                    ulIconIndex);
    return (ulrc);
}

