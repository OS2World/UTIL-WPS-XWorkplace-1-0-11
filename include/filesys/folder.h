
/*
 *@@sourcefile folder.h:
 *      header file for folder.c (XFolder implementation).
 *
 *      This file is ALL new with V0.9.0.
 *
 *@@include #include <os2.h>
 *@@include #include "helpers\linklist.h"  // for some structures
 *@@include #include "helpers\tree.h"  // for some structures
 *@@include #include <wpfolder.h>
 *@@include #include "shared\notebook.h"   // for notebook callback prototypes
 *@@include #include "filesys\folder.h"
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

#ifndef FOLDER_HEADER_INCLUDED
    #define FOLDER_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   Global variables
     *
     ********************************************************************/

    // extern PFNWP        G_pfnwpFolderContentMenuOriginal;

    /* ******************************************************************
     *
     *   Additional declarations for xfldr.c
     *
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
    // this is resolved by name (fdrmenus.c)

    typedef BOOL SOMLINK FN_WPSETMENUBARVISIBILITY(WPFolder *somSelf,
                                                   ULONG ulVisibility);
    #pragma linkage(FN_WPSETMENUBARVISIBILITY, system)

    /* ******************************************************************
     *
     *   Setup strings
     *
     ********************************************************************/

    BOOL fdrHasShowAllInTreeView(WPFolder *somSelf);

    BOOL fdrSetup(WPFolder *somSelf,
                  const char *pszSetupString);

    BOOL fdrQuerySetup(WPObject *somSelf,
                       PVOID pstrSetup);

    /* ******************************************************************
     *
     *   Folder view helpers
     *
     ********************************************************************/

    BOOL fdrForEachOpenInstanceView(WPFolder *somSelf,
                                    ULONG ulMsg,
                                    PFNWP pfnwpCallback);

    BOOL fdrForEachOpenGlobalView(ULONG ulMsg,
                                  PFNWP pfnwpCallback);

    VOID fdrUpdateStatusBars(WPFolder *pFolder);

    /* ******************************************************************
     *
     *   Full path in title
     *
     ********************************************************************/

    BOOL fdrSetOneFrameWndTitle(WPFolder *somSelf, HWND hwndFrame);

    BOOL fdrUpdateAllFrameWndTitles(WPFolder *somSelf);

    /* ******************************************************************
     *
     *   Quick Open
     *
     ********************************************************************/

    typedef BOOL _Optlink FNCBQUICKOPEN(WPFolder *pFolder,
                                        WPObject *pObject,
                                        ULONG ulNow,
                                        ULONG ulMax,
                                        ULONG ulCallbackParam);
    typedef FNCBQUICKOPEN *PFNCBQUICKOPEN;

    BOOL fdrQuickOpen(WPFolder *pFolder,
                      PFNCBQUICKOPEN pfnCallback,
                      ULONG ulCallbackParam);

    /* ******************************************************************
     *
     *   Snap To Grid
     *
     ********************************************************************/

    BOOL fdrSnapToGrid(WPFolder *somSelf,
                       BOOL fNotify);

    /* ******************************************************************
     *
     *   Extended Folder Sort (fdrsort.c)
     *
     ********************************************************************/

    BOOL fdrModifySortMenu(WPFolder *somSelf,
                           HWND hwndSortMenu);

    BOOL fdrSortMenuItemSelected(WPFolder *somSelf,
                                 HWND hwndFrame,
                                 HWND hwndMenu,
                                 ULONG ulMenuId,
                                 PBOOL pbDismiss);

    PFN fdrQuerySortFunc(WPFolder *somSelf,
                         LONG lSort);

    BOOL fdrHasAlwaysSort(WPFolder *somSelf);

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

    /* ******************************************************************
     *
     *   Status bars
     *
     ********************************************************************/

    #define BARPULL_FOLDER      0       // never set
    #define BARPULL_EDIT        2
    #define BARPULL_VIEW        3
    #define BARPULL_HELP        4

    /*
     *@@ SUBCLASSEDFOLDERVIEW:
     *      linked list structure used with folder frame
     *      window subclassing. One of these structures
     *      is created for each folder view (window) which
     *      is subclassed by fdrSubclassFolderView and
     *      stored in a global linked list.
     *
     *      This is one of the most important kludges which
     *      XFolder uses to hook itself into the WPS.
     *      Most importantly, this structure stores the
     *      original frame window procedure before the
     *      folder frame window was subclassed, but we also
     *      use this to store various other data for status
     *      bars etc.
     *
     *      We need this additional structure because all
     *      the data in here is _view_-specific, not
     *      folder-specific, so we cannot store this in
     *      the instance data.
     *
     *@@changed V0.9.1 (2000-01-29) [umoeller]: added pSourceObject and ulSelection fields
     *@@changed V0.9.2 (2000-03-06) [umoeller]: removed ulView, because this might change
     *@@changed V0.9.3 (2000-04-07) [umoeller]: renamed from SUBCLASSEDLISTITEM
     *@@changed V0.9.9 (2001-03-10) [umoeller]: added ulWindowWordOffset
     */

    typedef struct _SUBCLASSEDFOLDERVIEW
    {
        HWND        hwndFrame;          // folder view frame window
        WPFolder    *somSelf;           // folder object
        WPObject    *pRealObject;       // "real" object; this is == somSelf
                                        // for folders, but the corresponding
                                        // disk object for WPRootFolders
        PFNWP       pfnwpOriginal;      // orig. frame wnd proc before subclassing
                                        // (WPS folder proc)
        ULONG       ulWindowWordOffset; // as passed to fdrSubclassFolderView

        HWND        hwndStatusBar,      // status bar window; NULL if there's no
                                        // status bar for this view
                    hwndCnr,            // cnr window (child of hwndFrame)
                    hwndSupplObject;    // supplementary object wnd
                                        // (fdr_fnwpSupplFolderObject)

        BOOL        fNeedCnrScroll;     // scroll container after adding status bar?
        BOOL        fRemoveSourceEmphasis; // flag for whether XFolder has added
                                        // container source emphasis

        ULONG       ulLastSelMenuItem;  // last selected menu item ID
        WPObject    *pSourceObject;     // object whose record core has source
                                        // emphasis;
                                        // this field is valid only between
                                        // WM_INITMENU and WM_COMMAND; if this
                                        // is NULL, the entire folder whitespace
                                        // has source emphasis
        ULONG       ulSelection;        // SEL_* flags;
                                        // this field is valid only between
                                        // WM_INITMENU and WM_COMMAND
    } SUBCLASSEDFOLDERVIEW, *PSUBCLASSEDFOLDERVIEW;

    HWND fdrCreateStatusBar(WPFolder *somSelf,
                            PSUBCLASSEDFOLDERVIEW psli2,
                            BOOL fShow);

    MRESULT EXPENTRY fncbUpdateStatusBars(HWND hwndView, ULONG ulActivate,
                                          MPARAM mpView, MPARAM mpFolder);

    MRESULT EXPENTRY fncbStatusBarPost(HWND hwndView, ULONG msg,
                                       MPARAM mpView, MPARAM mpFolder);

    VOID fdrCallResolvedUpdateStatusBar(WPFolder *pFolder,
                                        HWND hwndStatusBar,
                                        HWND hwndCnr);

    VOID fdrUpdateStatusBars(WPFolder *pFolder);

    /********************************************************************
     *
     *   Folder frame window subclassing
     *
     ********************************************************************/

    PSUBCLASSEDFOLDERVIEW fdrCreateSFV(HWND hwndFrame,
                                       HWND hwndCnr,
                                       ULONG ulWindowWordOffset,
                                       WPFolder *somSelf,
                                       WPObject *pRealObject);

    PSUBCLASSEDFOLDERVIEW fdrSubclassFolderView(HWND hwndFrame,
                                                HWND hwndCnr,
                                                WPFolder *somSelf,
                                                WPObject *pRealObject);

    PSUBCLASSEDFOLDERVIEW fdrQuerySFV(HWND hwndFrame,
                                      PULONG pulIndex);

    VOID fdrManipulateNewView(WPFolder *somSelf,
                              HWND hwndNewFrame,
                              ULONG ulView);

    VOID fdrRemoveSFV(PSUBCLASSEDFOLDERVIEW psfv);

    BOOL fdrProcessObjectCommand(WPFolder *somSelf,
                                 USHORT usCommand,
                                 HWND hwndCnr,
                                 WPObject* pFirstObject,
                                 ULONG ulSelectionFlags);

    MRESULT fdrProcessFolderMsgs(HWND hwndFrame,
                                 ULONG msg,
                                 MPARAM mp1,
                                 MPARAM mp2,
                                 PSUBCLASSEDFOLDERVIEW psfv,
                                 PFNWP pfnwpOriginal);

    MRESULT EXPENTRY fdr_fnwpSubclassedFolderFrame(HWND hwnd,
                                                   ULONG msg,
                                                   MPARAM mp1,
                                                   MPARAM mp2);

    // Supplementary object window msgs (for each
    // subclassed folder frame, xfldr.c)
    #define SOM_ACTIVATESTATUSBAR       (WM_USER+100)
    // #define SOM_CREATEFROMTEMPLATE      (WM_USER+101)
                // removed V0.9.9 (2001-03-27) [umoeller]

    MRESULT EXPENTRY fdr_fnwpSupplFolderObject(HWND hwndObject,
                                               ULONG msg,
                                               MPARAM mp1,
                                               MPARAM mp2);

    /* ******************************************************************
     *
     *   XFolder window procedures
     *
     ********************************************************************/

    MRESULT EXPENTRY fdr_fnwpStatusBar(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

    MRESULT EXPENTRY fdr_fnwpSubclFolderContentMenu(HWND hwndMenu, ULONG msg, MPARAM mp1, MPARAM mp2);

    MRESULT EXPENTRY fdr_fnwpSelectSome(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);

    SHORT EXPENTRY fdrSortByICONPOS(PVOID pItem1, PVOID pItem2, PVOID psip);

    /* ******************************************************************
     *
     *   Folder semaphores
     *
     ********************************************************************/

    #ifdef SOM_WPFolder_h

        /*
         * xfTP_wpRequestFolderMutexSem:
         *      prototype for WPFolder::wpRequestFolderMutexSem.
         *
         *      See fdrRequestFolderMutexSem.
         *
         *      This returns 0 if the semaphore was successfully obtained.
         */

        typedef ULONG _System xfTP_wpRequestFolderMutexSem(WPFolder *somSelf,
                                                           ULONG ulTimeout);
        typedef xfTP_wpRequestFolderMutexSem *xfTD_wpRequestFolderMutexSem;

        /*
         * xfTP_wpReleaseFolderMutexSem:
         *      prototype for WPFolder::wpReleaseFolderMutexSem.
         *
         *      This is the reverse to WPFolder::wpRequestFolderMutexSem.
         */

        typedef ULONG _System xfTP_wpReleaseFolderMutexSem(WPFolder *somSelf);
        typedef xfTP_wpReleaseFolderMutexSem *xfTD_wpReleaseFolderMutexSem;

        typedef SOMAny* _System xfTP_wpQueryRWMonitorObject(WPFolder *somSelf);
        typedef xfTP_wpQueryRWMonitorObject *xfTD_wpQueryRWMonitorObject;

        typedef ULONG _System xfTP_RequestWrite(SOMAny *somSelf);
        typedef xfTP_RequestWrite *xfTD_RequestWrite;

        typedef ULONG _System xfTP_ReleaseWrite(SOMAny *somSelf);
        typedef xfTP_ReleaseWrite *xfTD_ReleaseWrite;

        /*
         * xfTP_wpRequestFindMutexSem:
         *      prototype for WPFolder::wpRequestFindMutexSem.
         *
         *      See fdrRequestFindMutexSem.
         *
         *      This returns 0 if the semaphore was successfully obtained.
         */

        typedef ULONG _System xfTP_wpRequestFindMutexSem(WPFolder *somSelf,
                                                         ULONG ulTimeout);
        typedef xfTP_wpRequestFindMutexSem *xfTD_wpRequestFindMutexSem;

        /*
         * xfTP_wpReleaseFindMutexSem:
         *      prototype for WPFolder::wpReleaseFindMutexSem.
         *
         *      This is the reverse to WPFolder::wpRequestFindMutexSem.
         */

        typedef ULONG _System xfTP_wpReleaseFindMutexSem(WPFolder *somSelf);
        typedef xfTP_wpReleaseFindMutexSem *xfTD_wpReleaseFindMutexSem;

        /*
         *@@ xfTP_wpFSNotifyFolder:
         *      prototype for WPFolder::wpFSNotifyFolder.
         *
         *      This undocumented method normally gets
         *      called when auto-refresh notifications are
         *      processed. This method apparently stores
         *      a new notification for the folder and
         *      auto-ages it. This is probably where
         *      the "Ager thread" comes in that is briefly
         *      described in the WPS programming reference.
         *
         *@@added V0.9.9 (2001-01-31) [umoeller]
         */

        typedef VOID _System xfTP_wpFSNotifyFolder(WPFolder *somSelf,
                                                   PVOID pvInfo);
        typedef xfTP_wpFSNotifyFolder *xfTD_wpFSNotifyFolder;

        /*
         * xfTP_wpFlushNotifications:
         *      prototype for WPFolder::wpFlushNotifications.
         *
         *      See the Warp 4 Toolkit documentation for details.
         */

        typedef BOOL _System xfTP_wpFlushNotifications(WPFolder *somSelf);
        typedef xfTP_wpFlushNotifications *xfTD_wpFlushNotifications;

        /*
         * xfTP_wpclsGetNotifySem:
         *      prototype for M_WPFolder::wpclsGetNotifySem.
         *
         *      This "notify mutex" is used before the background
         *      threads in the WPS attempt to update folder contents
         *      for auto-refreshing folders. By requesting this
         *      semaphore, any other WPS thread which does file
         *      operations can therefore keep these background
         *      threads from interfering.
         */

        typedef BOOL _System xfTP_wpclsGetNotifySem(M_WPFolder *somSelf,
                                                    ULONG ulTimeout);
        typedef xfTP_wpclsGetNotifySem *xfTD_wpclsGetNotifySem;

        /*
         * xfTP_wpclsReleaseNotifySem:
         *      prototype for M_WPFolder::wpclsReleaseNotifySem.
         *
         *      This is the reverse to xfTP_wpclsGetNotifySem.
         */

        typedef VOID _System xfTP_wpclsReleaseNotifySem(M_WPFolder *somSelf);
        typedef xfTP_wpclsReleaseNotifySem *xfTD_wpclsReleaseNotifySem;

    #endif

    // wrappers
    ULONG fdrRequestFolderMutexSem(WPFolder *somSelf,
                                    ULONG ulTimeout);

    ULONG fdrReleaseFolderMutexSem(WPFolder *somSelf);

    ULONG fdrRequestFolderWriteMutexSem(WPFolder *somSelf);

    ULONG fdrReleaseFolderWriteMutexSem(WPFolder *somSelf);

    ULONG fdrRequestFindMutexSem(WPFolder *somSelf,
                                    ULONG ulTimeout);

    ULONG fdrReleaseFindMutexSem(WPFolder *somSelf);

    ULONG fdrFlushNotifications(WPFolder *somSelf);

    BOOL fdrGetNotifySem(ULONG ulTimeout);

    VOID fdrReleaseNotifySem(VOID);

    /* ******************************************************************
     *
     *   Folder content management
     *
     ********************************************************************/

    #ifdef XWPTREE_INCLUDED

        typedef struct _FDRCONTENTITEM
        {
            TREE        Tree;
                    // -- for file-system objects, ulKey is
                    //    a PSZ with the object's short real name
                    //    which _must_ be upper-cased.
                    //    WARNING: This PSZ points into XWPFileSystem's
                    //    instance data!!
                    // -- for abstracts, ulKey has the 32-bit
                    //    object handle (_wpQueryHandle)
            WPObject    *pobj;
                    // object pointer
        } FDRCONTENTITEM, *PFDRCONTENTITEM;

        /*
         *@@ FDRCONTENTS:
         *      holds the folder contents tree for
         *      both file-system and abstract objects.
         *      The _pvFdrContents instance data pointer
         *      points to such a structure which is
         *      allocated with each XFolder::wpInitData.
         *
         *      The reason for this is that we can't
         *      use the TREE struct in IDL.
         *
         *      The tree is updated with every
         *      XFolder::wpAddToContent and
         *      XFolder::wpDeleteFromContent.
         *
         *@@added V0.9.16 (2001-10-23) [umoeller]
         */

        typedef struct _FDRCONTENTS
        {
            // tree of WPFileSystem objects
            TREE    *FileSystemsTreeRoot;
            LONG    cFileSystems;

            // tree of WPAbstract objects;
            TREE    *AbstractsTreeRoot;
            LONG    cAbstracts;
        } FDRCONTENTS, *PFDRCONTENTS;

    #endif

    WPObject* fdrFindFSFromName(WPFolder *pFolder,
                                const char *pcszShortName);

    BOOL fdrAddToContent(WPFolder *somSelf,
                         WPObject *pObject,
                         BOOL *pfCallParent);

    BOOL fdrRealNameChanged(WPFolder *somSelf,
                            WPObject *pFSObject);

    BOOL fdrDeleteFromContent(WPFolder *somSelf,
                              WPObject *pObject);

    WPObject* fdrQueryContent(WPFolder *somSelf,
                              WPObject *pobjFind,
                              ULONG ulOption);

    #define QCAFL_FILTERINSERTED        0x0001

    WPObject** fdrQueryContentArray(WPFolder *pFolder,
                                    ULONG flFilter,
                                    PULONG pulItems);

    BOOL fdrNukeContents(WPFolder *pFolder);

    /* ******************************************************************
     *
     *   Folder population
     *
     ********************************************************************/

    #ifdef __DEBUG__
        VOID fdrDebugDumpFolderFlags(WPFolder *somSelf);
    #else
        #define fdrDebugDumpFolderFlags(x)
    #endif

    BOOL fdrPopulate(WPFolder *somSelf,
                     PCSZ pcszFolderFullPath,
                     HWND hwndReserved,
                     BOOL fFoldersOnly,
                     PBOOL pfExit);

    /* ******************************************************************
     *
     *   Awake-objects test
     *
     ********************************************************************/

    BOOL fdrRegisterAwakeRootFolder(WPFolder *somSelf);

    BOOL fdrRemoveAwakeRootFolder(WPFolder *somSelf);

    #ifdef SOM_WPFileSystem_h
        WPFileSystem* fdrQueryAwakeFSObject(PCSZ pcszFQPath);
    #endif

    /* ******************************************************************
     *
     *   Object insertion
     *
     ********************************************************************/

    BOOL fdrCnrInsertObject(WPObject *pObject);

    ULONG fdrInsertAllContents(WPFolder *pFolder);

    /* ******************************************************************
     *
     *   Notebook callbacks (notebook.c) for XFldWPS  "View" page
     *
     ********************************************************************/

    #ifdef NOTEBOOK_HEADER_INCLUDED
        VOID XWPENTRY fdrViewInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                      ULONG flFlags);

        MRESULT XWPENTRY fdrViewItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                   ULONG ulItemID, USHORT usNotifyCode,
                                   ULONG ulExtra);

    /* ******************************************************************
     *
     *   Notebook callbacks (notebook.c) for XFldWPS"Grid" page
     *
     ********************************************************************/

#ifndef __NOSNAPTOGRID__
        VOID XWPENTRY fdrGridInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                      ULONG flFlags);

        MRESULT XWPENTRY fdrGridItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                   ULONG ulItemID,
                                   USHORT usNotifyCode,
                                   ULONG ulExtra);
#endif

    /* ******************************************************************
     *
     *   Notebook callbacks (notebook.c) for "XFolder" instance page
     *
     ********************************************************************/


        VOID XWPENTRY fdrXFolderInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                         ULONG flFlags);

        MRESULT XWPENTRY fdrXFolderItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                      ULONG ulItemID,
                                      USHORT usNotifyCode,
                                      ULONG ulExtra);

        VOID XWPENTRY fdrSortInitPage(PCREATENOTEBOOKPAGE pcnbp, ULONG flFlags);

        MRESULT XWPENTRY fdrSortItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                   ULONG ulItemID,
                                   USHORT usNotifyCode,
                                   ULONG ulExtra);

    /* ******************************************************************
     *
     *   XFldStartup notebook callbacks (notebook.c)
     *
     ********************************************************************/

        VOID XWPENTRY fdrStartupFolderInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                               ULONG flFlags);

        MRESULT XWPENTRY fdrStartupFolderItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                        ULONG ulItemID, USHORT usNotifyCode,
                        ULONG ulExtra);
    #endif

    /********************************************************************
     *
     *   Folder hotkey functions (fdrhotky.c)
     *
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

    BOOL fdrFindHotkey(USHORT usCommand,
                       PUSHORT pusFlags,
                       PUSHORT pusKeyCode);

    BOOL fdrProcessFldrHotkey(WPFolder *somSelf,
                              HWND hwndFrame,
                              USHORT usFlags,
                              USHORT usch,
                              USHORT usvk);

    VOID fdrAddHotkeysToMenu(WPObject *somSelf,
                             HWND hwndCnr,
                             HWND hwndMenu);

    #ifdef NOTEBOOK_HEADER_INCLUDED
        VOID XWPENTRY fdrHotkeysInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                         ULONG flFlags);

        MRESULT XWPENTRY fdrHotkeysItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                      ULONG ulItemID,
                                      USHORT usNotifyCode,
                                      ULONG ulExtra);
    #endif

    /********************************************************************
     *
     *   Folder messaging (fdrsubclass.c)
     *
     ********************************************************************/

    #ifdef INCL_WINHOOKS
        VOID EXPENTRY fdr_SendMsgHook(HAB hab,
                                      PSMHSTRUCT psmh,
                                      BOOL fInterTask);
    #endif

    /* ******************************************************************
     *
     *   Start folder contents
     *
     ********************************************************************/

    ULONG fdrStartFolderContents(WPFolder *pFolder,
                                 ULONG ulTiming);

#endif


