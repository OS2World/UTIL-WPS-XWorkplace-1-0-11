
/*
 *@@sourcefile folder.h:
 *      header file for folder.c (XFolder implementation).
 *
 *      This file is ALL new with V0.9.0.
 *
 *@@include #include <os2.h>
 *@@include #include "helpers\linklist.h"  // for some structures
 *@@include #include <wpfolder.h>
 *@@include #include "shared\notebook.h"   // for notebook callback prototypes
 *@@include #include "filesys\folder.h"
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

#ifndef FOLDER_HEADER_INCLUDED
    #define FOLDER_HEADER_INCLUDED

    /* ******************************************************************
     *                                                                  *
     *   Additional declarations for xfldr.c                            *
     *                                                                  *
     ********************************************************************/

    #ifdef LINKLIST_HEADER_INCLUDED
        /*
         *@@ ENUMCONTENT:
         *      this is the structure which represents
         *      an enumeration handle for XFolder::xfBeginEnumContent
         *      etc.
         */

        typedef struct _ENUMCONTENT
        {
            PLINKLIST   pllOrderedContent;  // created by XFolder::xfBeginEnumContent
            PLISTNODE   pnodeLastQueried;   // initially NULL;
                                            // updated by XFolder::xfEnumNext
        } ENUMCONTENT, *PENUMCONTENT;
    #endif

    /*
     *@@ SORTBYICONPOS:
     *      structure for GetICONPOS.
     */

    typedef struct _SORTBYICONPOS
    {
        CHAR    szRealName[CCHMAXPATH];
        PBYTE   pICONPOS;
        USHORT  usICONPOSSize;
    } SORTBYICONPOS, *PSORTBYICONPOS;

    // prototype for wpSetMenuBarVisibility;
    // this is resolved by name (menus.c)

    typedef BOOL SOMLINK FN_WPSETMENUBARVISIBILITY(WPFolder *somSelf,
                                                   ULONG ulVisibility);
    #pragma linkage(FN_WPSETMENUBARVISIBILITY, system)

    /* ******************************************************************
     *                                                                  *
     *   Full path in title                                             *
     *                                                                  *
     ********************************************************************/

    BOOL fdrSetOneFrameWndTitle(WPFolder *somSelf, HWND hwndFrame);

    BOOL fdrUpdateAllFrameWndTitles(WPFolder *somSelf);

    /* ******************************************************************
     *                                                                  *
     *   Snap To Grid                                                   *
     *                                                                  *
     ********************************************************************/

    BOOL fdrSnapToGrid(WPFolder *somSelf,
                       BOOL fNotify);

    /* ******************************************************************
     *                                                                  *
     *   Extended Folder Sort                                           *
     *                                                                  *
     ********************************************************************/

    PFN fdrQuerySortFunc(USHORT usSort);

    MRESULT EXPENTRY fdrSortAllViews(HWND hwndView,
                                     ULONG ulSort,
                                     MPARAM mpView,
                                     MPARAM mpFolder);

    VOID fdrSetFldrCnrSort(WPFolder *somSelf,
                           HWND hwndCnr,
                           BOOL fForce);

    MRESULT EXPENTRY fdrUpdateFolderSorts(HWND hwndView,
                                           ULONG ulDummy,
                                           MPARAM mpView,
                                           MPARAM mpFolder);

    /*
     *@@ DEFAULT_SORT:
     *      returns a useable setting for the default sort criterion
     *      according to instance / global settings; before using this
     *      macro, you need to initialize the following:
     *          PGLOBALSETTINGS     pGlobalSettings
     *          XFolderData         *somThis
     */

    #define DEFAULT_SORT ((_bDefaultSortInstance == SET_DEFAULT) ? pGlobalSettings->DefaultSort : _bDefaultSortInstance)

    /*
     *@@ ALWAYS_SORT:
     *      the same for AlwaysSort
     */

    #define ALWAYS_SORT ((_bAlwaysSortInstance == SET_DEFAULT) ? pGlobalSettings->AlwaysSort : _bAlwaysSortInstance)

    /* ******************************************************************
     *                                                                  *
     *   Status bars                                                    *
     *                                                                  *
     ********************************************************************/

    MRESULT EXPENTRY fncbUpdateStatusBars(HWND hwndView, ULONG ulActivate,
                                          MPARAM mpView, MPARAM mpFolder);

    /********************************************************************
     *                                                                  *
     *   Folder frame window subclassing                                *
     *                                                                  *
     ********************************************************************/

    #ifdef SOM_WPFolder_h

        VOID fdrManipulateNewView(WPFolder *somSelf,
                                  HWND hwndNewFrame,
                                  ULONG ulView);

        /*
         *@@ SUBCLASSEDLISTITEM:
         *      linked list structure used with folder frame
         *      window subclassing. One of these structures
         *      is created for each folder view (window) which
         *      is subclassed by fdrSubclassFolderFrame and
         *      stored in a global linked list. Most importantly,
         *      this structure stores the original frame window
         *      procedure before the window was subclassed, but
         *      we also use this to store various other data
         *      for status bars etc.
         */

        typedef struct _SUBCLASSEDLISTITEM
        {
            HWND        hwndFrame;          // folder frame
            WPFolder    *somSelf;           // folder object
            WPObject    *pRealObject;       // "real" object; this is == somSelf
                                            // for folders, but the corresponding
                                            // disk object for WPRootFolders
            PFNWP       pfnwpOriginal;      // orig. frame wnd proc
            HWND        hwndStatusBar,      // status bar window
                        hwndCnr,            // cnr window
                        hwndSupplObject;    // supplementary object wnd
            ULONG       ulView;             // OPEN_CONTENTS
                                            //   or OPEN_TREE
                                            //   or OPEN_DETAILS
            BOOL        fNeedCnrScroll;     // scroll container after adding status bar?
            BOOL        fRemoveSrcEmphasis; // flag for whether XFolder has added
                                            // container source emphasis
            ULONG       ulLastSelMenuItem;  // last selected menu item ID
        } SUBCLASSEDLISTITEM, *PSUBCLASSEDLISTITEM;

        VOID fdrInitPSLI(VOID);

        PSUBCLASSEDLISTITEM fdrQueryPSLI(HWND hwndFrame,
                                         PULONG pulIndex);

        PSUBCLASSEDLISTITEM fdrSubclassFolderFrame(HWND hwndFrame,
                                                   WPFolder *somSelf,
                                                   WPObject *pRealObject,
                                                   ULONG ulView);

        VOID fdrRemovePSLI(PSUBCLASSEDLISTITEM psli);

        MRESULT EXPENTRY fdr_fnwpSubclassedFolderFrame(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

        MRESULT EXPENTRY fdr_fnwpSupplFolderObject(HWND hwndObject, ULONG msg, MPARAM mp1, MPARAM mp2);

        /* ******************************************************************
         *                                                                  *
         *   Folder linked lists                                            *
         *                                                                  *
         ********************************************************************/

        #ifdef LINKLIST_HEADER_INCLUDED

            BOOL fdrAddToList(WPFolder *somSelf,
                              PLINKLIST pllFolders,
                              BOOL fInsert,
                              const char *pcszIniKey);

            BOOL fdrIsOnList(WPFolder *somSelf,
                             PLINKLIST pllFolders);

            WPFolder* fdrEnumList(PLINKLIST pllFolders,
                                  WPFolder *pFolder,
                                  const char *pcszIniKey);
        #endif

        /* ******************************************************************
         *                                                                  *
         *   Folder status bars                                             *
         *                                                                  *
         ********************************************************************/

        HWND fdrCreateStatusBar(WPFolder *somSelf,
                                PSUBCLASSEDLISTITEM psli2,
                                BOOL fShow);

    #endif

    /* ******************************************************************
     *                                                                  *
     *   XFolder window procedures                                      *
     *                                                                  *
     ********************************************************************/

    MRESULT EXPENTRY fdr_fnwpStatusBar(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

    MRESULT EXPENTRY fdr_fnwpSubclFolderContentMenu(HWND hwndMenu, ULONG msg, MPARAM mp1, MPARAM mp2);

    MRESULT EXPENTRY fdr_fnwpSelectSome(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);

    SHORT EXPENTRY fdrSortByICONPOS(PVOID pItem1, PVOID pItem2, PVOID psip);

    /* ******************************************************************
     *                                                                  *
     *   Notebook callbacks (notebook.c) for XFldWPS  "View" page       *
     *                                                                  *
     ********************************************************************/

    #ifdef NOTEBOOK_HEADER_INCLUDED
        VOID fdrViewInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                        ULONG flFlags);

        MRESULT fdrViewItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                              USHORT usItemID, USHORT usNotifyCode,
                                              ULONG ulExtra);

    /* ******************************************************************
     *                                                                  *
     *   Notebook callbacks (notebook.c) for XFldWPS"Grid" page         *
     *                                                                  *
     ********************************************************************/

        VOID fdrGridInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                      ULONG flFlags);

        MRESULT fdrGridItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                            USHORT usItemID,
                                            USHORT usNotifyCode,
                                            ULONG ulExtra);

    /* ******************************************************************
     *                                                                  *
     *   Notebook callbacks (notebook.c) for "XFolder" instance page    *
     *                                                                  *
     ********************************************************************/


        VOID fdrXFolderInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                 ULONG flFlags);

        MRESULT fdrXFolderItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                       USHORT usItemID,
                                       USHORT usNotifyCode,
                                       ULONG ulExtra);

        VOID fdrSortInitPage(PCREATENOTEBOOKPAGE pcnbp, ULONG flFlags);

        MRESULT fdrSortItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                    USHORT usItemID,
                                    USHORT usNotifyCode,
                                    ULONG ulExtra);

    /* ******************************************************************
     *                                                                  *
     *   XFldStartup notebook callbacks (notebook.c)                    *
     *                                                                  *
     ********************************************************************/

        VOID fdrStartupFolderInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                       ULONG flFlags);

        MRESULT fdrStartupFolderItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                        USHORT usItemID, USHORT usNotifyCode,
                        ULONG ulExtra);
    #endif

    /********************************************************************
     *                                                                  *
     *   Folder hotkey functions (fdrhotky.c)                           *
     *                                                                  *
     ********************************************************************/

    // maximum no. of folder hotkeys
    #define FLDRHOTKEYCOUNT (ID_XSSI_LB_LAST-ID_XSSI_LB_FIRST+1)

    // maximum length of folder hotkey descriptions
    #define MAXLBENTRYLENGTH 50

    /*
     *@@ XFLDHOTKEY:
     *      XFolder folder hotkey definition.
     *      A static array of these exists in folder.c.
     */

    typedef struct _XFLDHOTKEY
    {
        USHORT  usFlags;     //  Keyboard control codes
        USHORT  usKeyCode;   //  Hardware scan code
        USHORT  usCommand;   //  corresponding menu item id to send to container
    } XFLDHOTKEY, *PXFLDHOTKEY;

    #define FLDRHOTKEYSSIZE sizeof(XFLDHOTKEY)*FLDRHOTKEYCOUNT

    PXFLDHOTKEY fdrQueryFldrHotkeys(VOID);

    void fdrLoadDefaultFldrHotkeys(VOID);

    void fdrLoadFolderHotkeys(VOID);

    void fdrStoreFldrHotkeys(VOID);

    BOOL fdrProcessFldrHotkey(HWND hwndFrame, MPARAM mp1, MPARAM mp2);

    #ifdef NOTEBOOK_HEADER_INCLUDED
        VOID fdrHotkeysInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                        ULONG flFlags);

        MRESULT fdrHotkeysItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                              USHORT usItemID,
                                              USHORT usNotifyCode,
                                              ULONG ulExtra);
    #endif

#endif


