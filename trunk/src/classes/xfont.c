
/*
 *@@sourcefile xtrash.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPFontFolder: a subclass of WPFolder, which implements
 *                  a "font folder".
 *
 *@@added V0.9.7 (2001-01-12) [umoeller]
 *@@somclass XWPFontFolder fon_
 *@@somclass M_XWPFontFolder fonM_
 */

/*
 *      Copyright (C) 2001 Ulrich M�ller.
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

#ifndef SOM_Module_xfont_Source
#define SOM_Module_xfont_Source
#endif
#define XWPFontFolder_Class_Source
#define M_XWPFontFolder_Class_Source

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
#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers

// SOM headers which don't crash with prec. header files
#include "xfont.ih"
#include "xfontobj.ih"
#include "xfldr.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel

#include "config\fonts.h"               // font folder implementation

// other SOM headers
#pragma hdrstop

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

// default font folder
static XWPFontFolder *G_pDefaultFontFolder = NULL;

/* ******************************************************************
 *
 *   XWPFontFolder instance methods
 *
 ********************************************************************/

/*
 *@@ xwpAddFontsPage:
 *
 */

SOM_Scope ULONG  SOMLINK fon_xwpAddFontsPage(XWPFontFolder *somSelf,
                                             HWND hwndDlg)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_xwpAddFontsPage");

    /* Return statement to be customized: */
    return 0;
}

/*
 *@@ xwpProcessObjectCommand:
 *      this XFolder method processes WM_COMMAND messages
 *      for objects in a container. For details refer to
 *      XFolder::xwpProcessObjectCommand.
 *
 *      This is really a method override... but since SOM
 *      IDL doesn't know that XWPTrashCan is in fact
 *      derived from XFolder, we have to do it this way.
 */

SOM_Scope BOOL  SOMLINK fon_xwpProcessObjectCommand(XWPFontFolder *somSelf,
                                                    USHORT usCommand,
                                                    HWND hwndCnr,
                                                    WPObject* pFirstObject,
                                                    ULONG ulSelectionFlags)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_xwpProcessObjectCommand");

    return (fonProcessObjectCommand(somSelf,
                                    usCommand,
                                    hwndCnr,
                                    pFirstObject,
                                    ulSelectionFlags));
}

/*
 *@@ xwpUpdateStatusBar:
 *      this XFolder instance method gets called when the status
 *      bar needs updating.
 *
 *      This always gets called using name-lookup resolution, so
 *      XFolder does not have to be installed for this to work.
 *      However, if it is, this method will be called. See
 *      XFolder::xwpUpdateStatusBar for more on this.
 */

SOM_Scope BOOL  SOMLINK fon_xwpUpdateStatusBar(XWPFontFolder *somSelf,
                                               HWND hwndStatusBar,
                                               HWND hwndCnr)
{
    CHAR szText[200];
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_xwpUpdateStatusBar");

    if (_ulFontsCurrent < _ulFontsMax)
    {
        // populating and not done yet:
        sprintf(szText, "Collecting fonts... %u out of %d done",
                _ulFontsCurrent, _ulFontsMax);
    }
    else
        sprintf(szText, "%d fonts installed.", _ulFontsCurrent);

    return (WinSetWindowText(hwndStatusBar, szText));
}

/*
 *@@ wpInitData:
 *      this WPObject instance method gets called when the
 *      object is being initialized (on wake-up or creation).
 *      We initialize our additional instance data here.
 *      Always call the parent method first.
 */

SOM_Scope void  SOMLINK fon_wpInitData(XWPFontFolder *somSelf)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpInitData");

    XWPFontFolder_parent_WPFolder_wpInitData(somSelf);

    _fFilledWithFonts = FALSE;      // attribute

    _ulFontsCurrent = 0;
    _ulFontsMax = 0;

    // tell XFolder to override wpAddToContent...
    _xwpSetDisableCnrAdd(somSelf, TRUE);
}

/*
 *@@ wpUnInitData:
 *      this WPObject instance method is called when the object
 *      is destroyed as a SOM object, either because it's being
 *      made dormant or being deleted. All allocated resources
 *      should be freed here.
 *      The parent method must always be called last.
 */

SOM_Scope void  SOMLINK fon_wpUnInitData(XWPFontFolder *somSelf)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpUnInitData");

    XWPFontFolder_parent_WPFolder_wpUnInitData(somSelf);
}

/*
 *@@ wpQueryDefaultHelp:
 *      this WPObject instance method specifies the default
 *      help panel for an object (when "Extended help" is
 *      selected from the object's context menu). This should
 *      describe what this object can do in general.
 *
 *      We'll display some help for the font folder.
 */

SOM_Scope BOOL  SOMLINK fon_wpQueryDefaultHelp(XWPFontFolder *somSelf,
                                               PULONG pHelpPanelId,
                                               PSZ HelpLibrary)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpQueryDefaultHelp");

    return (XWPFontFolder_parent_WPFolder_wpQueryDefaultHelp(somSelf,
                                                             pHelpPanelId,
                                                             HelpLibrary));
}

/*
 *@@ wpOpen:
 *      this WPObject instance method gets called when
 *      a new view needs to be opened. Normally, this
 *      gets called after wpViewObject has scanned the
 *      object's USEITEMs and has determined that a new
 *      view is needed.
 *
 *      This _normally_ runs on thread 1 of the WPS, but
 *      this is not always the case. If this gets called
 *      in response to a menu selection from the "Open"
 *      submenu or a double-click in the folder, this runs
 *      on the thread of the folder (which _normally_ is
 *      thread 1). However, if this results from WinOpenObject
 *      or an OPEN setup string, this will not be on thread 1.
 *
 *      We do the regular folder open first. After that,
 *      if icon, details, or tree view were opened, we call
 *      fonFillWithFonts to create all the instances of
 *      XWPFontObject in the font folder, but only on the
 *      first open.
 */

SOM_Scope HWND  SOMLINK fon_wpOpen(XWPFontFolder *somSelf, HWND hwndCnr,
                                   ULONG ulView, ULONG param)
{
    HWND hwndNew;
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpOpen");

    hwndNew = XWPFontFolder_parent_WPFolder_wpOpen(somSelf, hwndCnr,
                                                   ulView, param);
    if (hwndNew)
    {
        switch (ulView)
        {
            case OPEN_CONTENTS:
            case OPEN_TREE:
            case OPEN_DETAILS:
                if (!_fFilledWithFonts)
                {
                    // very first call:
                    _ulFontsCurrent = 0;
                    _ulFontsMax = 0;

                    // now create font objects...
                    fonFillWithFontObjects(somSelf);
                    _fFilledWithFonts = TRUE;
                }
            }
    }

    return (hwndNew);
}

/*
 *@@ wpPopulate:
 *      this instance method populates a folder, in this
 *      case, the font folder.
 */

SOM_Scope BOOL  SOMLINK fon_wpPopulate(XWPFontFolder *somSelf,
                                       ULONG ulReserved, PSZ pszPath,
                                       BOOL fFoldersOnly)
{
    BOOL brc = TRUE;
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpPopulate");

    brc = XWPFontFolder_parent_WPFolder_wpPopulate(somSelf,
                                                 ulReserved,
                                                 pszPath,
                                                 fFoldersOnly);

    return (brc);
}

/*
 *@@ wpDragOver:
 *      this instance method is called to inform the object
 *      that other objects are being dragged over it.
 *      This corresponds to the DM_DRAGOVER message received by
 *      the object.
 */

SOM_Scope MRESULT  SOMLINK fon_wpDragOver(XWPFontFolder *somSelf,
                                          HWND hwndCnr,
                                          PDRAGINFO pdrgInfo)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpDragOver");

    return (fonDragOver(somSelf, pdrgInfo));
}

/*
 *@@ wpDrop:
 *      this instance method is called to inform an object that
 *      another object has been dropped on it.
 *      This corresponds to the DM_DROP message received by
 *      the object.
 */

SOM_Scope MRESULT  SOMLINK fon_wpDrop(XWPFontFolder *somSelf,
                                      HWND hwndCnr,
                                      PDRAGINFO pdrgInfo,
                                      PDRAGITEM pdrgItem)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpDrop");

    return (fonDrop(somSelf, pdrgInfo));
}

/*
 *@@ wpAddSettingsPages:
 *      this WPObject instance method gets called by the WPS
 *      when the Settings view is opened to have all the
 *      settings page inserted into hwndNotebook.
 *
 *      We add the font folder settings pages.
 */

SOM_Scope BOOL  SOMLINK fon_wpAddSettingsPages(XWPFontFolder *somSelf,
                                               HWND hwndNotebook)
{
    XWPFontFolderData *somThis = XWPFontFolderGetData(somSelf);
    XWPFontFolderMethodDebug("XWPFontFolder","fon_wpAddSettingsPages");

    return (XWPFontFolder_parent_WPFolder_wpAddSettingsPages(somSelf,
                                                             hwndNotebook));
}


/* ******************************************************************
 *
 *   XWPFontFolder class methods
 *
 ********************************************************************/

/*
 *@@ xwpclsQueryDefaultFontFolder:
 *      this returns the default font folder (with the object ID
 *      &lt;XWP_FONTFOLDER&gt;).
 */

SOM_Scope XWPFontFolder*  SOMLINK fonM_xwpclsQueryDefaultFontFolder(M_XWPFontFolder *somSelf)
{
    /* M_XWPFontFolderData *somThis = M_XWPFontFolderGetData(somSelf); */
    M_XWPFontFolderMethodDebug("M_XWPFontFolder","fonM_xwpclsQueryDefaultFontFolder");

    /* Return statement to be customized: */
    return NULL;
}

/*
 *@@ wpclsInitData:
 *      this class method allows the class to
 *      initialize itself. We set up some global
 *      folder data and also make sure that
 *      the XWPFontObj class gets initialized.
 */

SOM_Scope void  SOMLINK fonM_wpclsInitData(M_XWPFontFolder *somSelf)
{
    SOMClass *pFontObjectClassObject;
    /* M_XWPFontFolderData *somThis = M_XWPFontFolderGetData(somSelf); */
    M_XWPFontFolderMethodDebug("M_XWPFontFolder","fonM_wpclsInitData");

    M_XWPFontFolder_parent_M_WPFolder_wpclsInitData(somSelf);

    // enforce initialization of XWPFontObject class
    pFontObjectClassObject = XWPFontObjectNewClass(XWPFontObject_MajorVersion,
                                                    XWPFontObject_MinorVersion);

    if (pFontObjectClassObject)
        // now increment the class's usage count by one to
        // ensure that the class is never unloaded; if we
        // didn't do this, we'd get WPS CRASHES in some
        // background class because if no more trash objects
        // exist, the class would get unloaded automatically -- sigh...
        _wpclsIncUsage(pFontObjectClassObject);

    {
        // store the class object in KERNELGLOBALS
        PKERNELGLOBALS   pKernelGlobals = krnLockGlobals(__FILE__, __LINE__, __FUNCTION__);
        if (pKernelGlobals)
        {
            pKernelGlobals->fXWPFontFolder = TRUE;
            krnUnlockGlobals();
        }
    }
}

/*
 *@@ wpclsUnInitData:
 *
 */

SOM_Scope void  SOMLINK fonM_wpclsUnInitData(M_XWPFontFolder *somSelf)
{
    /* M_XWPFontFolderData *somThis = M_XWPFontFolderGetData(somSelf); */
    M_XWPFontFolderMethodDebug("M_XWPFontFolder","fonM_wpclsUnInitData");

    _wpclsDecUsage(_XWPFontObject);

    M_XWPFontFolder_parent_M_WPFolder_wpclsUnInitData(somSelf);
}

/*
 *@@ wpclsQueryTitle:
 *      tell the WPS the new class default title:
 *      "Font folder".
 */

SOM_Scope PSZ  SOMLINK fonM_wpclsQueryTitle(M_XWPFontFolder *somSelf)
{
    /* M_XWPFontFolderData *somThis = M_XWPFontFolderGetData(somSelf); */
    M_XWPFontFolderMethodDebug("M_XWPFontFolder","fonM_wpclsQueryTitle");

    // return (M_XWPFontFolder_parent_M_WPFolder_wpclsQueryTitle(somSelf));
    return ("Font folder"); // ###
}

/*
 *@@ wpclsQueryStyle:
 *
 */

SOM_Scope ULONG  SOMLINK fonM_wpclsQueryStyle(M_XWPFontFolder *somSelf)
{
    /* M_XWPFontFolderData *somThis = M_XWPFontFolderGetData(somSelf); */
    M_XWPFontFolderMethodDebug("M_XWPFontFolder","fonM_wpclsQueryStyle");

    return (CLSSTYLE_DONTTEMPLATE
                | CLSSTYLE_NEVERCOPY    // but allow move
                | CLSSTYLE_NEVERDELETE
                | CLSSTYLE_NEVERPRINT);
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
 *      We give this class a new standard icon here.
 */

SOM_Scope ULONG  SOMLINK fonM_wpclsQueryIconData(M_XWPFontFolder *somSelf,
                                                 PICONINFO pIconInfo)
{
    /* M_XWPFontFolderData *somThis = M_XWPFontFolderGetData(somSelf); */
    M_XWPFontFolderMethodDebug("M_XWPFontFolder","fonM_wpclsQueryIconData");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXWPFONTCLOSED;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return (sizeof(ICONINFO));
}

/*
 *@@ wpclsQueryIconDataN:
 *      this should return the class default
 *      "animation" icons (for open folders).
 */

SOM_Scope ULONG  SOMLINK fonM_wpclsQueryIconDataN(M_XWPFontFolder *somSelf,
                                                  ICONINFO* pIconInfo,
                                                  ULONG ulIconIndex)
{
    /* M_XWPFontFolderData *somThis = M_XWPFontFolderGetData(somSelf); */
    M_XWPFontFolderMethodDebug("M_XWPFontFolder","fonM_wpclsQueryIconDataN");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXWPFONTOPEN;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return (sizeof(ICONINFO));
}

