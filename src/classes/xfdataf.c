
/*
 *@@sourcefile xfdataf.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XFldDataFile (WPDataFile replacement)
 *
 *      XFldDataFile is responsible for menu manipulation
 *      and default icon replacements for data files.
 *      Since we should not override WPFileSystem to implement
 *      this functionality for folders and data files altogether,
 *      we must add this to WPDataFile separately.
 *
 *      Installation of this class is now optional (V0.9.0).
 *
 *      Starting with V0.9.0, the files in classes\ contain only
 *      i.e. the methods themselves.
 *      The implementation for this class is mostly in filesys\filesys.c
 *      and (for extended file types) in filesys\filetype.c.
 *
 *      <B>Extended File Associations</B>
 *
 *      -- To get the new associated programs into the "Open" menu,
 *         we override wpDisplayMenu (Warp 3) and wpModifyMenu
 *         (Warp 4).
 *
 *      -- For general WPS support (including icons), we override
 *         XFldDataFile::wpQueryAssociatedProgram.
 *
 *      -- The new selections are intercepted in XFldDataFile::wpOpen.
 *
 *@@somclass XFldDataFile xfdf_
 *@@somclass M_XFldDataFile xfdfM_
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

#ifndef SOM_Module_xfdf_Source
#define SOM_Module_xfdf_Source
#endif
#define XFldDataFile_Class_Source
#define M_XFldDataFile_Class_Source

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
#define INCL_WINMENUS
#define INCL_WINPROGRAMLIST     // needed for wppgm.h
#include <os2.h>

// C library headers
#include <stdio.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\linklist.h"           // linked list helper routines

// SOM headers which don't crash with prec. header files
#include "xfdataf.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)

#include "filesys\filesys.h"            // various file-system object implementation code
#include "filesys\filetype.h"           // extended file types implementation
#include "filesys\folder.h"             // XFolder implementation
#include "filesys\fdrmenus.h"           // shared folder menu logic
#include "filesys\program.h"            // program implementation

// other SOM headers
#pragma hdrstop                 // VAC++ keeps crashing otherwise
#include <wppgmf.h>             // WPProgramFIle

/* ******************************************************************
 *
 *   XFldDataFile instance methods
 *
 ********************************************************************/

/*
 *@@ wpDisplayMenu:
 *      this WPObject instance method creates and displays
 *      an object's popup menu, which is returned.
 *
 *      From my testing (after overriding _all_ WPDataFile methods...),
 *      I found out that wpDisplayMenu calls the following methods
 *      in this order:
 *
 *      --  wpFilterMenu (Warp-4-specific);
 *      --  wpFilterPopupMenu;
 *      --  wpModifyPopupMenu;
 *      --  wpModifyMenu (Warp-4-specific).
 *
 *      Normally, this method does not need to be overridden
 *      to modify menus. HOWEVER, with Warp 4, IBM was kind
 *      enough to ignore all changes to the list of associated
 *      programs added to the "Open" submenu, because most
 *      apparently, Warp 4 no longer adds the programs in
 *      the wpModifyPopupMenu method, but in the Warp-4-specific
 *      wpModifyMenu method, which is called too late for me.
 *
 *      Unfortunately, it is thus impossible to prevent the WPS
 *      from adding program objects to the "Open" submenu, except
 *      for overriding this method and removing them all again, or
 *      breaking compatibility with Warp 3. Duh.
 *
 *      Soooo... to support extended file assocs on both Warp 3
 *      and Warp 4, we had to split this.
 *
 *      1. For Warp 3, we do the processing in wpDisplayMenu.
 *         Seems to work. After the menu has been completely
 *         built, we call ftypModifyDataFileOpenSubmenu.
 *
 *      2. For Warp 4, we do some more ugly hacks. We override
 *         wpModifyMenu and do the processing there. See
 *         XFldDataFile::wpModifyMenu.
 *
 *@@added V0.9.0 [umoeller]
 */

SOM_Scope HWND  SOMLINK xfdf_wpDisplayMenu(XFldDataFile *somSelf,
                                           HWND hwndOwner,
                                           HWND hwndClient,
                                           POINTL* ptlPopupPt,
                                           ULONG ulMenuType,
                                           ULONG ulReserved)
{
    HWND hwndMenu = 0;

    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // XFldDataFileData *somThis = XFldDataFileGetData(somSelf);
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpDisplayMenu");

    hwndMenu = XFldDataFile_parent_WPDataFile_wpDisplayMenu(somSelf,
                                                            hwndOwner,
                                                            hwndClient,
                                                            ptlPopupPt,
                                                            ulMenuType,
                                                            ulReserved);

    if (!doshIsWarp4())
    {
        // on Warp 3, manipulate the "Open" submenu...
        if (pGlobalSettings->fExtAssocs)
        {
            MENUITEM        mi;
            // find "Open" submenu
            if (WinSendMsg(hwndMenu,
                           MM_QUERYITEM,
                           MPFROM2SHORT(WPMENUID_OPEN, TRUE),
                           (MPARAM)&mi))
            {
                // found:
                ftypModifyDataFileOpenSubmenu(somSelf,
                                              mi.hwndSubMenu,
                                              TRUE);        // delete existing
            }
        }
    }

    return (hwndMenu);
}

/*
 *@@ wpFilterPopupMenu:
 *      this WPObject instance method allows the object to
 *      filter out unwanted menu items from the context menu.
 *      This gets called before wpModifyPopupMenu.
 *
 *      We remove default entries according to Global Settings;
 *      even though XFldObject does this already, we need to
 *      override this for XFldDataFile again, because the
 *      WPS does it too for WPDataFile.
 *
 *      Also we need to do some fiddling with the "Open"
 *      submenu for the extended associations mechanism.
 */

SOM_Scope ULONG  SOMLINK xfdf_wpFilterPopupMenu(XFldDataFile *somSelf,
                                                ULONG ulFlags,
                                                HWND hwndCnr,
                                                BOOL fMultiSelect)
{
    ULONG ulMenuFilter = 0;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    /* XFldDataFileData *somThis = XFldDataFileGetData(somSelf); */
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpFilterPopupMenu");

    ulMenuFilter = XFldDataFile_parent_WPDataFile_wpFilterPopupMenu(somSelf,
                                                                    ulFlags,
                                                                    hwndCnr,
                                                                    fMultiSelect);

    // now suppress default menu items according to
    // Global Settings;
    // the DefaultMenuItems field in pGlobalSettings is
    // ready-made for this function; the "Workplace Shell"
    // notebook page for removing menu items sets this field with
    // the proper CTXT_xxx flags
    ulMenuFilter &= ~pGlobalSettings->DefaultMenuItems;

    return (ulMenuFilter);
}

/*
 *@@ wpModifyMenu:
 *      this WPObject method is Warp-4 specific and
 *      is a "supermethod" to wpModifyPopupMenu. This
 *      supports manipulation of folder menu bars as
 *      well.
 *
 *      The standard version of this calls wpModifyPopupMenu
 *      in turn.
 *
 *      NOTE: This "method" isn't really a SOM method
 *      in that it doesn't appear in our IDL file. As
 *      a result, this prototype has _not_ been created
 *      by the SOM compiler.
 *
 *      Instead, we "manually" hack the WPDataFile
 *      method table to point to this function instead.
 *      This is done in M_XFldDataFile::wpclsInitData
 *      by calling wpshOverrideStaticMethod.
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 */

BOOL _System xfdf_wpModifyMenu(XFldDataFile *somSelf,
                               HWND hwndMenu,
                               HWND hwndCnr,
                               ULONG iPosition,
                               ULONG ulMenuType,
                               ULONG ulView,
                               ULONG ulReserved)
{
    BOOL    brc = FALSE;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    xfTD_wpModifyMenu _parent_wpModifyMenu = NULL;
    BOOL    fExtAssocs = FALSE;
    somMethodTabs pParentMTab;
    SOMClass      *pParentClass;
    /* XFldDataFileData *somThis = XFldDataFileGetData(somSelf); */
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpModifyMenu");

    fExtAssocs = pGlobalSettings->fExtAssocs;

    // resolve parent method.... this is especially sick:

    // a) if extended associations are disabled,
    //    we go for WPDataFile
    // b) if extended associations are enabled, this is fun:
    //    we skip the _WPDataFile method completely and call WPFileSystem directly!
    if (fExtAssocs)
    {
        // WPFileSystem
        pParentClass = _WPFileSystem;
        pParentMTab = WPFileSystemCClassData.parentMtab;
    }
    else
    {
        // WPDataFile
        pParentClass = _XFldDataFile;
        pParentMTab = XFldDataFileCClassData.parentMtab;
    }

    // resolve!
    _parent_wpModifyMenu
        = (xfTD_wpModifyMenu)wpshParentNumResolve(_WPFileSystem,
                                                  pParentMTab,
                                                  "wpModifyMenu");
    if (!_parent_wpModifyMenu)
        cmnLog(__FILE__, __LINE__, __FUNCTION__,
               "wpshParentNumResolve failed.");
    else
    {
        // call parent method
        brc = _parent_wpModifyMenu(somSelf, hwndMenu, hwndCnr,
                                   iPosition, ulMenuType,
                                   ulView, ulReserved);

        if ((brc) && (fExtAssocs))
        {
            // extended assocs have been enabled:
            // this means that the WPDataFile method has been SKIPPED
            // and we need to manually tweak some of the data file
            // settings, as WPDataFile would normally do it...

            // now check which type of menu we have
            switch (ulMenuType)
            {
                #ifndef MENU_SELECTEDPULLDOWN
                    // Warp 4 definition...
                    #define MENU_SELECTEDPULLDOWN     0x00000009
                #endif
                case MENU_OPENVIEWPOPUP:
                case MENU_TITLEBARPULLDOWN:
                case MENU_OBJECTPOPUP:
                case MENU_SELECTEDPULLDOWN:
                {
                    // find "Open" submenu
                    MENUITEM        mi;
                    if (WinSendMsg(hwndMenu,
                                   MM_QUERYITEM,
                                   MPFROM2SHORT(WPMENUID_OPEN, TRUE),
                                   (MPARAM)&mi))
                    {
                        // found:

                        // check for program files hack
                        BOOL    fIsProgramFile = _somIsA(somSelf, _WPProgramFile);
                        LONG    lDefaultView = _wpQueryDefaultView(somSelf);

                        if (    (lDefaultView == OPEN_RUNNING)
                             && (!fIsProgramFile)
                           )
                        {
                            // this is not a program file,
                            // but this doesn't have its default view set yet:
                            // set it then
                            _wpSetDefaultView(somSelf, 0x1000);
                            lDefaultView = 0x1000;
                        }
                        // but skip program files with OPEN_RUNNING

                        ftypModifyDataFileOpenSubmenu(somSelf,
                                                      mi.hwndSubMenu,
                                                      FALSE);        // do not delete existing
                    }
                break; }
            }
        }
    }

    return (brc);
}

/*
 *@@ wpModifyPopupMenu:
 *      this WPObject instance methods gets called by the WPS
 *      when a context menu needs to be built for the object
 *      and allows the object to manipulate its context menu.
 *      This gets called _after_ wpFilterPopupMenu.
 *
 *      We add the datafile object popup menu entries.
 *
 *      We don't need a wpMenuItemSelected method override
 *      for data files, because the new menu items are
 *      completely handled by the subclassed folder frame
 *      window procedure by calling the functions in fdrmenus.c.
 */

SOM_Scope BOOL  SOMLINK xfdf_wpModifyPopupMenu(XFldDataFile *somSelf,
                                               HWND hwndMenu,
                                               HWND hwndCnr,
                                               ULONG iPosition)
{
    BOOL brc = TRUE;

    /* XFldDataFileData *somThis = XFldDataFileGetData(somSelf); */
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpModifyPopupMenu");

    brc = XFldDataFile_parent_WPDataFile_wpModifyPopupMenu(somSelf,
                                                           hwndMenu,
                                                           hwndCnr,
                                                           iPosition);

    if (brc)
        // manipulate the data file menu according to our needs
        brc = mnuModifyDataFilePopupMenu(somSelf, hwndMenu, hwndCnr, iPosition);

    if (brc)
        fdrAddHotkeysToMenu(somSelf,
                            hwndCnr,
                            hwndMenu);

    return (brc);
}

/*
 *@@ wpMenuItemHelpSelected:
 *      display help for a context menu item.
 */

SOM_Scope BOOL  SOMLINK xfdf_wpMenuItemHelpSelected(XFldDataFile *somSelf,
                                                    ULONG MenuId)
{
    /* XFldDataFileData *somThis = XFldDataFileGetData(somSelf); */
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpMenuItemHelpSelected");

    // call the common help processor in fdrmenus.c;
    if (mnuMenuItemHelpSelected(somSelf, MenuId))
        // if this returns TRUE, help was requested for one
        // of the new menu items
        return TRUE;
    else
        // else: none of our menu items, call default
        return (XFldDataFile_parent_WPDataFile_wpMenuItemHelpSelected(somSelf,
                                                                 MenuId));
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
 *      Of course, for data files, the "views" are the
 *      various associations in the "Open" submenu, which
 *      have ulView IDs >= 0x1000. If the default view
 *      has been manually set for the object, wpOpen
 *      always receives this ID.
 *
 *      By contrast, we can also get OPEN_RUNNING or
 *      OPEN_DEFAULT. I think OPEN_RUNNING comes in
 *      for "standard" data files which have been
 *      double-clicked on.
 *
 *      There are two more problems with starting the
 *      associated program (damn it, IBM, do you ever
 *      read the specs that you've written yourself?):
 *
 *      --  The WPS always uses its internal list of
 *          associations, no matter what we return from
 *          XFldDataFile::wpQueryAssociatedProgram, so
 *          we need to override this method as well and
 *          intercept the new menu items that we have changed.
 *
 *      --  We cannot use the WPProgram/WPProgramFile classes
 *          to start the associated programs. These things do
 *          not accept parameters, and of course there's no
 *          export for resolving all the parameter placeholders.
 *          So we need to start the program ourselves with
 *          the data file as the parameter, using progOpenProgram.
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 */

SOM_Scope HWND  SOMLINK xfdf_wpOpen(XFldDataFile *somSelf,
                                    HWND hwndCnr,
                                    ULONG ulView,
                                    ULONG param)
{
    HWND        hwnd = NULLHANDLE;
    BOOL        fCallParent = TRUE;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    /* XFldDataFileData *somThis = XFldDataFileGetData(somSelf); */
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpOpen");

    _Pmpf(("xfdf_wpOpen, ulView: 0x%lX", ulView));

    if (pGlobalSettings->fExtAssocs)
    {
        // "extended associations" allowed:
        if (    ((ulView >= 0x1000) && (ulView < 0x1100))
             || (ulView == OPEN_RUNNING)    // double-click on data file... what's this, IBM?
             || (ulView == OPEN_DEFAULT)
           )
            // use our replacement mechanism
            fCallParent = FALSE;
    }

    if (!fCallParent)
    {
        // replacement desired:
        ULONG       ulView2 = ulView;
        WPObject    *pAssocObject
            = ftypQueryAssociatedProgram(somSelf,
                                         &ulView2,
                                         // use "plain text" as default:
                                         TRUE);
                                            // we've used "plain text" as default
                                            // in wpModifyMenu, so we need to do
                                            // the same again here
                            // object is locked

        if (pAssocObject)
        {
            hwnd = (HWND)progOpenProgram(pAssocObject,
                                         somSelf,
                                         ulView2);

            // _wpUnlockObject(pAssocObject);
                    // do not unlock the assoc object...
                    // this is still needed in the use list!!!
        }
    }
    else
        hwnd = XFldDataFile_parent_WPDataFile_wpOpen(somSelf,
                                                     hwndCnr,
                                                     ulView,
                                                     param);

    // _Pmpf(("End of xfdf_wpOpen, returning hwnd 0x%lX", hwnd));

    return (hwnd);

}

/*
 *@@ wpAddFile1Page:
 *      this normally adds the first "File" page to
 *      the file's settings notebook; if allowed,
 *      we will replace this with our own version,
 *      which combines the three "File" pages into
 *      one single page.
 *
 *      We cannot override this in XWPFileSystem because
 *      WPFolder overrides this too.
 *
 *@@added V0.9.0
 */

SOM_Scope ULONG  SOMLINK xfdf_wpAddFile1Page(XFldDataFile *somSelf,
                                             HWND hwndNotebook)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // XFldDataFileData *somThis = XFldDataFileGetData(somSelf);
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpAddFile1Page");

    if (pGlobalSettings->fReplaceFilePage)
    {
        return (fsysInsertFilePages(somSelf,
                                    hwndNotebook));
    }
    else
        return (XFldDataFile_parent_WPDataFile_wpAddFile1Page(somSelf,
                                                              hwndNotebook));
}

/*
 *@@ wpAddFile2Page:
 *      this normally adds the second "File" page to
 *      the file's settings notebook; since we
 *      combine the three "File" pages into one,
 *      we'll remove this page, if allowed.
 *
 *      We cannot override this in XWPFileSystem because
 *      WPFolder overrides this too.
 *
 *@@added V0.9.0
 */

SOM_Scope ULONG  SOMLINK xfdf_wpAddFile2Page(XFldDataFile *somSelf,
                                             HWND hwndNotebook)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // XFldDataFileData *somThis = XFldDataFileGetData(somSelf);
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpAddFile2Page");

    if (pGlobalSettings->fReplaceFilePage)
        return (SETTINGS_PAGE_REMOVED);
    else
        return (XFldDataFile_parent_WPDataFile_wpAddFile2Page(somSelf,
                                                              hwndNotebook));
}

/*
 *@@ wpAddFile3Page:
 *      this normally adds the second "File" page to
 *      the file's settings notebook; since we
 *      combine the three "File" pages into one,
 *      we'll remove this page, if allowed.
 *
 *      We cannot override this in XWPFileSystem because
 *      WPFolder overrides this too.
 *
 *@@added V0.9.0
 */

SOM_Scope ULONG  SOMLINK xfdf_wpAddFile3Page(XFldDataFile *somSelf,
                                             HWND hwndNotebook)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // XFldDataFileData *somThis = XFldDataFileGetData(somSelf);
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpAddFile3Page");

    if (pGlobalSettings->fReplaceFilePage)
        return (SETTINGS_PAGE_REMOVED);
    else
        return (XFldDataFile_parent_WPDataFile_wpAddFile3Page(somSelf,
                                                              hwndNotebook));
}

/*
 *@@ wpQueryAssociatedProgram:
 *      this method returns the associated WPProgram or
 *      WPProgramFile object for the specified view ID of
 *      a data file.
 *
 *      The WPS docs say that this should be overridden
 *      to introduce new association mechanisms.
 *
 *      Yeah, right. Per default, this gets called from
 *      XFldDataFile::wpQueryAssociatedFileIcon -- that
 *      is, only while a folder is being populated, but
 *      _not_ when a context menu is opened. There seems
 *      to be a second association mechanism in wpModifyPopupMenu
 *      somewhere to add all the associated programs to
 *      the "Open" submenu. I have tested this by returning
 *      NULL in this method, and all the program objects
 *      still appear in the "Open" menu -- it's just the
 *      data file icons which get set to the default icon
 *      then.
 *
 *      Soooo... to give the data files new icons, we need
 *      to override this method since it gets called from
 *      wpQueryAssociatedFileIcon (which normally gets called
 *      during folder populate).
 *
 *      From what I see, ulView is normally 0x1000, which
 *      is the WPS-internal code for the first associated
 *      program. This is only > 1000 if the default
 *      association has been changed on the "Menu" page
 *      of a data file (1001 for the second assoc, 1002
 *      for the third, etc.).
 *
 *      However, this also needs to support OPEN_RUNNING
 *      and OPEN_DEFAULT.
 *
 *      This returns NULL if there's no associated program
 *      for the specified view. Otherwise it returns a
 *      WPProgram or WPProgramFile, which is locked.
 *
 *      The caller should unlock the object after it's
 *      done using it. AFAIK, WPS wpQueryAssociatedFileIcon
 *      does this.
 *
 *@@added V0.9.0 [umoeller]
 */

SOM_Scope WPObject*  SOMLINK xfdf_wpQueryAssociatedProgram(XFldDataFile *somSelf,
                                                           ULONG ulView,
                                                           PULONG pulHowMatched,
                                                           PSZ pszMatchString,
                                                           ULONG cbMatchString,
                                                           PSZ pszDefaultType)
{
    WPObject* pobj = 0;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

    /* XFldDataFileData *somThis = XFldDataFileGetData(somSelf); */
    // XFldDataFileMethodDebug("XFldDataFile","xfdf_wpQueryAssociatedProgram");

    #if defined DEBUG_ASSOCS || defined DEBUG_SOMMETHODS
        _Pmpf(("Entering wpQueryAssociatedProgram for %s; ulView = %lX, "
               "*pulHowMatched = 0x%lX, "
               "pszMatchString = %s, pszDefaultType = %s",
               _wpQueryTitle(somSelf),
               ulView,
               (    (pulHowMatched)
                            ? (*pulHowMatched)
                            : 0
               ),
               pszMatchString, pszDefaultType
               ));
    #endif

    if (pGlobalSettings->fExtAssocs)
    {
        // "extended associations" allowed:
        // use our replacement mechanism...
        // this does NOT use "plain text" as the default
        ULONG   ulView2 = ulView;
        pobj = ftypQueryAssociatedProgram(somSelf,
                                          &ulView2,
                                          // do not use "plain text" as default,
                                          // this affects the icon:
                                          FALSE);
                        // locks the object
    }
    else
        pobj = XFldDataFile_parent_WPDataFile_wpQueryAssociatedProgram(somSelf,
                                                                       ulView,
                                                                       pulHowMatched,
                                                                       pszMatchString,
                                                                       cbMatchString,
                                                                       pszDefaultType);

    #if defined DEBUG_ASSOCS || defined DEBUG_SOMMETHODS
        _Pmpf(("End of wpQueryAssociatedProgram for %s", _wpQueryTitle(somSelf)));
    #endif

    return (pobj);
}

/*
 *@@ wpSetAssociatedFileIcon:
 *      this method gets called by the WPS when a data
 *      file object is being initialized, to have it
 *      set its icon to that of the default association.
 *
 *      From what I see, this method only seems to be called
 *      when a Settings view is _opened_, not when it's closed,
 *      which doesn't really make sense.
 *
 *@@added V0.9.0 [umoeller]
 */

SOM_Scope void  SOMLINK xfdf_wpSetAssociatedFileIcon(XFldDataFile *somSelf)
{
    /* XFldDataFileData *somThis = XFldDataFileGetData(somSelf); */
    XFldDataFileMethodDebug("XFldDataFile","xfdf_wpSetAssociatedFileIcon");

    // _Pmpf(("Entering wpSetAssociatedFileIcon for %s", _wpQueryTitle(somSelf)));
    XFldDataFile_parent_WPDataFile_wpSetAssociatedFileIcon(somSelf);
    // _Pmpf(("End of wpSetAssociatedFileIcon for %s", _wpQueryTitle(somSelf)));
}


/* ******************************************************************
 *
 *   XFldDataFile class methods
 *
 ********************************************************************/

/*
 *@@ wpclsInitData:
 *      initialize XFldDataFile class data.
 *
 *      We also manually patch the class method table to
 *      override some WPDataFile methods that IBM didn't
 *      want us to see.
 *
 *@@changed V0.9.0 [umoeller]: added class object to KERNELGLOBALS
 *@@changed V0.9.6 (2000-10-16) [umoeller]: added method tables override
 */

SOM_Scope void  SOMLINK xfdfM_wpclsInitData(M_XFldDataFile *somSelf)
{
    // M_XFldDataFileData *somThis = M_XFldDataFileGetData(somSelf);
    M_XFldDataFileMethodDebug("M_XFldDataFile","xfdfM_wpclsInitData");

    M_XFldDataFile_parent_M_WPDataFile_wpclsInitData(somSelf);

    {
        // store the class object in KERNELGLOBALS
        PKERNELGLOBALS   pKernelGlobals = krnLockGlobals(__FILE__, __LINE__, __FUNCTION__);
        if (pKernelGlobals)
        {
            pKernelGlobals->fXFldDataFile = TRUE;
            krnUnlockGlobals();
        }
    }

    /*
     *  Manually patch method tables of this class...
     *
     */

    // this gets called for subclasses too, so patch
    // this only for the parent class... descendant
    // classes will inherit this anyway
    if (somSelf == _XFldDataFile)
    {
        if (doshIsWarp4())
        {
            // on Warp 4, override wpModifyMenu (Warp 4-specific method)
            wpshOverrideStaticMethod(somSelf,
                                     "wpModifyMenu",
                                     (somMethodPtr)xfdf_wpModifyMenu);
        }
    }
}

/*
 *@@ wpclsCreateDefaultTemplates:
 *      this WPObject class method is called by the
 *      Templates folder to allow a class to
 *      create its default templates.
 *
 *      The default WPS behavior is to create new templates
 *      if the class default title is different from the
 *      existing templates.
 *
 *      Since we are replacing the class, we will have to
 *      suppress this in order not to crowd the Templates
 *      folder.
 */

SOM_Scope BOOL  SOMLINK xfdfM_wpclsCreateDefaultTemplates(M_XFldDataFile *somSelf,
                                                          WPObject* Folder)
{
    // M_XFldDataFileData *somThis = M_XFldDataFileGetData(somSelf);
    M_XFldDataFileMethodDebug("M_XFldDataFile","xfdfM_wpclsCreateDefaultTemplates");

    // we only override this class method if it is
    // being called for the _XFldDataFile class object itself.
    // If this is being called for a subclass, we use
    // the parent method, because we do not want to
    // break the default behavior for subclasses.
    // this is not working on Warp 3
    if (somSelf == _XFldDataFile)
        return (TRUE);
        // means that the Templates folder should _not_ create templates
        // by itself; we pretend that we've done this
    else
        return (M_XFldDataFile_parent_M_WPDataFile_wpclsCreateDefaultTemplates(somSelf,
                                                                               Folder));

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
 *      We give data files a new default icon, if the
 *      global settings allow this.
 *      This is loaded from /ICONS/ICONS.DLL.
 */

SOM_Scope ULONG  SOMLINK xfdfM_wpclsQueryIconData(M_XFldDataFile *somSelf,
                                                  PICONINFO pIconInfo)
{
    ULONG       ulrc;
    HMODULE     hmodIconsDLL = NULLHANDLE;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    // M_XFldDataFileData *somThis = M_XFldDataFileGetData(somSelf);
    M_XFldDataFileMethodDebug("M_XFldDataFile","xfdfM_wpclsQueryIconData");

    if (pGlobalSettings->fReplaceIcons)
    {
        hmodIconsDLL = cmnQueryIconsDLL();
        // icon replacements allowed:
        if ((pIconInfo) && (hmodIconsDLL))
        {
            pIconInfo->fFormat = ICON_RESOURCE;
            pIconInfo->hmod = hmodIconsDLL;
            pIconInfo->resid = 109;
        }
        ulrc = sizeof(ICONINFO);
    }

    if (hmodIconsDLL == NULLHANDLE)
        // icon replacements not allowed: call default
        ulrc = M_XFldDataFile_parent_M_WPDataFile_wpclsQueryIconData(somSelf,
                                                                  pIconInfo);

    return (ulrc);
}

