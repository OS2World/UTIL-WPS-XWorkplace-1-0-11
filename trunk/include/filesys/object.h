
/*
 *@@sourcefile object.h:
 *      header file for object.c (XFldObject implementation).
 *
 *      This file is ALL new with V0.9.0.
 *
 *@@include #include <os2.h>
 *@@include #include <wpobject.h>   // or any other WPS SOM header
 *@@include #include "hook\xwphook.h"
 *@@include #include "filesys\object.h"
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

#ifndef OBJECT_HEADER_INCLUDED
    #define OBJECT_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   Object creation/destruction
     *
     ********************************************************************/

    BOOL objFree(WPObject *somSelf);

    /* ******************************************************************
     *
     *   Object linked lists
     *
     ********************************************************************/

    #ifdef LINKLIST_HEADER_INCLUDED

        /*
         *@@ OBJECTLIST:
         *      encapsulation for object lists, for
         *      use with the obj* list APIs.
         *
         *      See objAddToList for details.
         *
         *@@added V0.9.9 (2001-03-27) [umoeller]
         */

        typedef struct _OBJECTLIST
        {
            LINKLIST    ll;
            BOOL        fLoaded;
        } OBJECTLIST, *POBJECTLIST;

        BOOL objAddToList(WPObject *somSelf,
                          POBJECTLIST pllFolders,
                          BOOL fInsert,
                          const char *pcszIniKey,
                          ULONG ulListFlag);

        BOOL objIsOnList(WPObject *somSelf,
                         POBJECTLIST pllFolders);

        WPObject* objEnumList(POBJECTLIST pllFolders,
                              WPObject *pFolder,
                              const char *pcszIniKey,
                              ULONG ulListFlag);

    #endif

    /* ******************************************************************
     *
     *   Object flags
     *
     ********************************************************************/

    #define OBJLIST_RUNNINGSTORED           0x0001
    #define OBJLIST_CONFIGFOLDER            0x0002
#ifndef __NOFOLDERCONTENTS__
    #define OBJLIST_FAVORITEFOLDER          0x0004
#endif
#ifndef __NOQUICKOPEN__
    #define OBJLIST_QUICKOPENFOLDER         0x0008
#endif
    #define OBJLIST_HANDLESCACHE            0x0010 // V0.9.9 (2001-04-02) [umoeller]
    #define OBJLIST_DIRTYLIST               0x0020 // V0.9.11 (2001-04-18) [umoeller]

    /* ******************************************************************
     *
     *   Object handles cache
     *
     ********************************************************************/

    WPObject* objFindObjFromHandle(HOBJECT hobj);

    VOID objRemoveFromHandlesCache(WPObject *somSelf);

    /* ******************************************************************
     *
     *   Dirty objects list
     *
     ********************************************************************/

    BOOL objAddToDirtyList(WPObject *pobj);

    BOOL objRemoveFromDirtyList(WPObject *pobj);

    ULONG objQueryDirtyObjectsCount(VOID);

    typedef BOOL _Optlink FNFORALLDIRTIESCALLBACK(WPObject *pobjThis,
                                                  ULONG ulIndex,
                                                  ULONG cObjects,
                                                  PVOID pvUser);

    ULONG objForAllDirtyObjects(FNFORALLDIRTIESCALLBACK *pCallback,
                                PVOID pvUserForCallback);

    /* ******************************************************************
     *
     *   Object "Internals" page
     *
     ********************************************************************/

    // MRESULT EXPENTRY obj_fnwpSettingsObjDetails(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);

    /* ******************************************************************
     *
     *   Notebook callbacks (notebook.c) for new XFldObject "Icon" page
     *
     ********************************************************************/

    #ifdef NOTEBOOK_HEADER_INCLUDED

        #define ICONFL_TITLE            0x0001
        #define ICONFL_ICON             0x0002
        #define ICONFL_TEMPLATE         0x0004
        #define ICONFL_LOCKEDINPLACE    0x0008
        #define ICONFL_HOTKEY           0x0010
        #define ICONFL_DETAILS          0x0020

        VOID objFormatIconPage(PCREATENOTEBOOKPAGE pcnbp,
                               ULONG flFlags);

        VOID XWPENTRY objIcon1InitPage(PCREATENOTEBOOKPAGE pcnbp,
                                       ULONG flFlags);

        MRESULT XWPENTRY objIcon1ItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                             ULONG ulItemID, USHORT usNotifyCode,
                                             ULONG ulExtra);
    #endif

    /* ******************************************************************
     *
     *   Object details dialog
     *
     ********************************************************************/

    VOID objShowObjectDetails(HWND hwndOwner,
                              WPObject *pobj);

    /* ******************************************************************
     *
     *   Object hotkeys
     *
     ********************************************************************/

    #ifdef XWPHOOK_HEADER_INCLUDED
    PGLOBALHOTKEY objFindHotkey(PGLOBALHOTKEY pHotkeys,
                                ULONG cHotkeys,
                                HOBJECT hobj);
    #endif

    #ifdef SOM_XFldObject_h
        BOOL objQueryObjectHotkey(WPObject *somSelf,
                                  XFldObject_POBJECTHOTKEY pHotkey);

        BOOL objSetObjectHotkey(WPObject *somSelf,
                                XFldObject_POBJECTHOTKEY pHotkey);
    #endif

    BOOL objRemoveObjectHotkey(HOBJECT hobj);

    /* ******************************************************************
     *
     *   Object menus
     *
     ********************************************************************/

    VOID objModifyPopupMenu(WPObject* somSelf,
                            HWND hwndMenu);

    /* ******************************************************************
     *
     *   Object setup strings
     *
     ********************************************************************/

    BOOL objSetup(WPObject *somSelf,
                  PSZ pszSetupString);

    BOOL objQuerySetup(WPObject *somSelf,
                        PVOID pstrSetup);

    #define SCRFL_RECURSE           0x0001

    #ifdef LINKLIST_HEADER_INCLUDED
    #ifdef SOM_WPFolder_h

        APIRET objCreateObjectScript(WPObject *pObject,
                                     const char *pcszRexxFile,
                                     WPFolder *pFolderForFiles,
                                     ULONG flCreate);
    #endif
    #endif

#endif


