
/*
 *@@sourcefile xwpmouse.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPMouse (WPMouse replacement)
 *
 *      This class replaces the WPMouse class, which implements the
 *      "Mouse" settings object, to introduce new settings pages
 *      for sliding focus and such. This is all new with V0.9.0.
 *
 *      Installation of this class is optional, but you won't
 *      be able to influence certain XWorkplace hook settings without it.
 *
 *      Starting with V0.9.0, the files in classes\ contain only
 *      the SOM interface, i.e. the methods themselves.
 *      The implementation for this class is in in filesys\hookintf.c.
 *
 *@@added V0.9.0 [umoeller]
 *
 *@@somclass XWPMouse xms_
 *@@somclass M_XWPMouse xmsM_
 */

/*
 *      Copyright (C) 1999-2003 Ulrich M�ller.
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

#ifndef SOM_Module_xwpmouse_Source
#define SOM_Module_xwpmouse_Source
#endif
#define XWPMouse_Class_Source
#define M_XWPMouse_Class_Source

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

#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES
#define INCL_WINDIALOGS
#define INCL_WINSTDBOOK
#define INCL_GPIBITMAPS
#include <os2.h>

// C library headers
#include <stdio.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers

// SOM headers which don't crash with prec. header files
#include "xwpmouse.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling

#include "config\hookintf.h"            // daemon/hook interface

// other SOM headers
#pragma hdrstop

/*
 *@@ xwpAddMouseMovementPage:
 *      this actually adds the "Movement" page into the
 *      "Mouse" notebook to configure the XWorkplace hook.
 *      Gets called by XWPMouse::wpAddMouseCometPage.
 *
 *@@changed V0.9.9 (2001-03-27) [umoeller]: moved "Corners" from XWPMouse to XWPScreen
 *@@changed V0.9.14 (2001-08-02) [lafaix]: added a second movement page
 *@@changed V0.9.19 (2002-04-19) [pr]: page wasn't showing "page 2/2"; fixed
 */

SOM_Scope ULONG  SOMLINK xms_xwpAddMouseMovementPage(XWPMouse *somSelf,
                                                     HWND hwndDlg)
{
    INSERTNOTEBOOKPAGE  inbp;
    HMODULE             savehmod = cmnQueryNLSModuleHandle(FALSE);
    ULONG               ulrc = 0;

    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_xwpAddMouseMovementPage");

    // insert "MouseHook" page if the hook has been enabled
    if (hifXWPHookReady())
    {
        // moved "corners" to XWPScreen V0.9.9 (2001-03-27) [umoeller]

        // second movement page V0.9.14 (2001-08-02) [lafaix]
#ifndef __NOMOVEMENT2FEATURES__
        memset(&inbp, 0, sizeof(INSERTNOTEBOOKPAGE));
        inbp.somSelf = somSelf;
        inbp.hwndNotebook = hwndDlg;
        inbp.hmod = savehmod;
        inbp.ulDlgID = ID_XSD_MOUSE_MOVEMENT2;
        inbp.usPageStyleFlags = BKA_MINOR;
        // inbp.pcszName = cmnGetString(ID_XSSI_MOUSEHOOKPAGE);  // pszMouseHookPage
        inbp.fEnumerate = TRUE;
        inbp.ulDefaultHelpPanel  = ID_XSH_MOUSE_MOVEMENT2;
        inbp.ulPageID = SP_MOUSE_MOVEMENT2;
        inbp.pfncbInitPage    = hifMouseMovement2InitPage;
        inbp.pfncbItemChanged = hifMouseMovement2ItemChanged;
        ulrc = ntbInsertPage(&inbp);
#endif

        memset(&inbp, 0, sizeof(INSERTNOTEBOOKPAGE));
        inbp.somSelf = somSelf;
        inbp.hwndNotebook = hwndDlg;
        inbp.hmod = savehmod;
        inbp.ulDlgID = ID_XFD_EMPTYDLG; // ID_XSD_MOUSE_MOVEMENT;
        inbp.usPageStyleFlags = BKA_MAJOR;
        inbp.pcszName = cmnGetString(ID_XSSI_MOUSEHOOKPAGE);  // pszMouseHookPage
        // inbp.fEnumerate = TRUE;
        inbp.fEnumerate = TRUE; // re-enabled V0.9.14 (2001-08-02) [lafaix]
        inbp.ulDefaultHelpPanel  = ID_XSH_MOUSE_MOVEMENT;
        inbp.ulPageID = SP_MOUSE_MOVEMENT;
        inbp.pfncbInitPage    = hifMouseMovementInitPage;
        inbp.pfncbItemChanged = hifMouseMovementItemChanged;
        ulrc = ntbInsertPage(&inbp);
    }

    return ulrc;
}

/*
 *@@ xms_xwpAddMouseMappings2Page:
 *      this adds the second "Mappings" page to the
 *      "Mouse" settings notebook after the WPS's
 *      standard "Mappings" page.
 *      Gets called by XWPMouse::wpAddMouseMappingsPage.
 *
 *@@added V0.9.1 [umoeller]
 *@@changed V0.9.3 (2000-04-01) [umoeller]: page wasn't showing "page 2/2"; fixed
 *@@changed V0.9.19 (2002-04-19) [pr]: page wasn't showing "page 2/2"; fixed again
 */

SOM_Scope ULONG  SOMLINK xms_xwpAddMouseMappings2Page(XWPMouse *somSelf,
                                                      HWND hwndDlg)
{
    INSERTNOTEBOOKPAGE  inbp;
    HMODULE             savehmod = cmnQueryNLSModuleHandle(FALSE);
    ULONG               ulrc = 0;

    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_xwpAddMouseMappings2Page");

    // insert "MouseHook" page if the hook has been enabled
    if (hifXWPHookReady())
    {
        memset(&inbp, 0, sizeof(INSERTNOTEBOOKPAGE));
        inbp.somSelf = somSelf;
        inbp.hwndNotebook = hwndDlg;
        inbp.hmod = savehmod;
        inbp.ulDlgID = ID_XFD_EMPTYDLG; // ID_XSD_MOUSEMAPPINGS2;   V0.9.19 (2002-05-28) [umoeller]
        inbp.usPageStyleFlags = BKA_MINOR;
        inbp.fEnumerate = TRUE;
        // inbp.pcszName = cmnGetString(ID_XSSI_MAPPINGSPAGE);  // pszMappingsPage
        inbp.ulDefaultHelpPanel  = ID_XSH_MOUSEMAPPINGS2;
        inbp.ulPageID = SP_MOUSE_MAPPINGS2;
        inbp.pfncbInitPage    = hifMouseMappings2InitPage;
        inbp.pfncbItemChanged = hifMouseMappings2ItemChanged;
        ulrc = ntbInsertPage(&inbp);
    }

    return ulrc;
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

SOM_Scope ULONG  SOMLINK xms_wpFilterPopupMenu(XWPMouse *somSelf,
                                               ULONG ulFlags,
                                               HWND hwndCnr,
                                               BOOL fMultiSelect)
{
    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_wpFilterPopupMenu");

    return (XWPMouse_parent_WPMouse_wpFilterPopupMenu(somSelf,
                                                      ulFlags,
                                                      hwndCnr,
                                                      fMultiSelect)
            & ~CTXT_NEW
           );
}

/*
 *@@ wpAddMouseCometPage:
 *      this WPMouse instance method inserts the "Comet"
 *      page into the mouse object's settings notebook.
 *      We override this to get an opportunity to insert our
 *      own pages behind that page by calling
 *      XWPMouse::xwpAddMouseMovementPage to configure the hook.
 */

SOM_Scope ULONG  SOMLINK xms_wpAddMouseCometPage(XWPMouse *somSelf,
                                                 HWND hwndNotebook)
{
    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_wpAddMouseCometPage");

    _xwpAddMouseMovementPage(somSelf, hwndNotebook);

    return XWPMouse_parent_WPMouse_wpAddMouseCometPage(somSelf,
                                                       hwndNotebook);
}


/*
 *@@ xms_wpAddMouseMappingsPage:
 *      this WPMouse instance method inserts the "Mappings"
 *      page in the "Mouse" object's settings notebook.
 *      We override this to insert a second "Mappings" page
 *      for configuring more XWorkplace hook features.
 *
 *@@added V0.9.1 [umoeller]
 */

SOM_Scope ULONG  SOMLINK xms_wpAddMouseMappingsPage(XWPMouse *somSelf,
                                                    HWND hwndNotebook)
{
    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_wpAddMouseMappingsPage");

    _xwpAddMouseMappings2Page(somSelf, hwndNotebook);

    return XWPMouse_parent_WPMouse_wpAddMouseMappingsPage(somSelf,
                                                          hwndNotebook);
}


/*
 *@@ wpclsInitData:
 *      this WPObject class method gets called when a class
 *      is loaded by the WPS (probably from within a
 *      somFindClass call) and allows the class to initialize
 *      itself.
 */

SOM_Scope void  SOMLINK xmsM_wpclsInitData(M_XWPMouse *somSelf)
{
    /* M_XWPMouseData *somThis = M_XWPMouseGetData(somSelf); */
    M_XWPMouseMethodDebug("M_XWPMouse","xmsM_wpclsInitData");

    M_XWPMouse_parent_M_WPMouse_wpclsInitData(somSelf);

    krnClassInitialized(G_pcszXWPMouse);
}


