
/*
 *@@sourcefile filesys.h:
 *      header file for filetype.c (extended file types implementation).
 *
 *      This file is ALL new with V0.9.0.
 *
 *@@include #include <os2.h>
 *@@include #include <wpdataf.h>                    // WPDataFile
 *@@include #include "helpers\linklist.h"
 *@@include #include "shared\notebook.h"            // for notebook callbacks
 *@@include #include "filesys\filetype.h"
 */

/*
 *      Copyright (C) 1997-2002 Ulrich M�ller.
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

#ifndef FILETYPE_HEADER_INCLUDED
    #define FILETYPE_HEADER_INCLUDED

    #ifdef SOM_WPFileSystem_h

        ULONG ftypRegisterInstanceTypesAndFilters(M_WPFileSystem *pClassObject);

        PCSZ ftypFindClassFromInstanceType(PCSZ pcszType);

        PCSZ ftypFindClassFromInstanceFilter(PCSZ pcszObjectTitle,
                                             ULONG ulTitleLen);

    #endif

    /* ******************************************************************
     *
     *   XFldDataFile extended associations
     *
     ********************************************************************/

    /*
     *@@ XWPTYPEWITHFILTERS:
     *      structure representing one entry in "XWorkplace:FileFilters"
     *      from OS2.INI. These are now cached for speed.
     *
     *      A linked list of these exists in G_llTypesWithFilters,
     *      which is built on the first call to
     *      ftypGetCachedTypesWithFilters.
     *
     *@@added V0.9.9 (2001-02-06) [umoeller]
     */

    typedef struct _XWPTYPEWITHFILTERS
    {
        PSZ         pszType;        // e.g. "C Code" (points to after structure)
        PSZ         pszFilters;     // e.g. "*.c\0*.h\0\0" (points to after structure)
                // together with this structure we allocate enough
                // room for storing the two strings
    } XWPTYPEWITHFILTERS, *PXWPTYPEWITHFILTERS;

#ifndef __NEVEREXTASSOCS__
    VOID ftypInvalidateCaches(VOID);

    #ifdef LINKLIST_HEADER_INCLUDED
        PLINKLIST ftypGetCachedTypesWithFilters(VOID);
    #endif

    BOOL ftypLockCaches(VOID);

    VOID ftypUnlockCaches(VOID);

#endif

    APIRET ftypRenameFileType(PCSZ pcszOld,
                              PCSZ pcszNew);

    ULONG ftypAssocObjectDeleted(HOBJECT hobj);

#ifndef __NEVEREXTASSOCS__
    #if defined (SOM_XFldDataFile_h) && defined (LINKLIST_HEADER_INCLUDED)
        PLINKLIST ftypBuildAssocsList(WPDataFile *somSelf,
                                      ULONG ulBuildMax,
                                      BOOL fUsePlainTextAsDefault);

        ULONG ftypFreeAssocsList(PLINKLIST *ppllAssocs);

        WPObject* ftypQueryAssociatedProgram(WPDataFile *somSelf,
                                             PULONG pulView,
                                             BOOL fUsePlainTextAsDefault);

        BOOL ftypModifyDataFileOpenSubmenu(WPDataFile *somSelf,
                                           HWND hwndOpenSubmenu,
                                           BOOL fDeleteExisting);
    #endif

    #ifdef NOTEBOOK_HEADER_INCLUDED

        /* ******************************************************************
         *
         *   Notebook callbacks (notebook.c) for XFldWPS "File types" page
         *
         ********************************************************************/

        extern MPARAM *G_pampFileTypesPage;
        extern ULONG G_cFileTypesPage;

        VOID XWPENTRY ftypFileTypesInitPage(PNOTEBOOKPAGE pnbp,
                                            ULONG flFlags);

        MRESULT XWPENTRY ftypFileTypesItemChanged(PNOTEBOOKPAGE pnbp,
                                         ULONG ulItemID,
                                         USHORT usNotifyCode,
                                         ULONG ulExtra);

        MRESULT EXPENTRY fnwpImportWPSFilters(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);

        /* ******************************************************************
         *
         *   XFldDataFile notebook callbacks (notebook.c)
         *
         ********************************************************************/

        extern MPARAM *G_pampDatafileTypesPage;
        extern ULONG G_cDatafileTypesPage;

        VOID XWPENTRY ftypDatafileTypesInitPage(PNOTEBOOKPAGE pnbp,
                                                ULONG flFlags);

        MRESULT XWPENTRY ftypDatafileTypesItemChanged(PNOTEBOOKPAGE pnbp,
                                             ULONG ulItemID,
                                             USHORT usNotifyCode,
                                             ULONG ulExtra);

        /* ******************************************************************
         *
         *   XWPProgram/XWPProgramFile notebook callbacks (notebook.c)
         *
         ********************************************************************/

        ULONG ftypInsertAssociationsPage(WPObject *somSelf,
                                         HWND hwndNotebook);

        VOID XWPENTRY ftypAssociationsInitPage(PNOTEBOOKPAGE pnbp,
                                               ULONG flFlags);

        MRESULT XWPENTRY ftypAssociationsItemChanged(PNOTEBOOKPAGE pnbp,
                                            ULONG ulItemID,
                                            USHORT usNotifyCode,
                                            ULONG ulExtra);
    #endif

    /* ******************************************************************
     *
     *   Import/Export facility
     *
     ********************************************************************/

    #ifdef XSTRING_HEADER_INCLUDED
    APIRET ftypImportTypes(PCSZ pcszFilename,
                           PXSTRING pstrError);
    #endif

    APIRET ftypExportTypes(PCSZ pcszFileName);

#endif

#endif

