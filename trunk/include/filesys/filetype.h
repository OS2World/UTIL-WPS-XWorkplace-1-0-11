
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

#ifndef FILETYPE_HEADER_INCLUDED
    #define FILETYPE_HEADER_INCLUDED

#ifdef __EXTASSOCS__

    /* ******************************************************************
     *                                                                  *
     *   XFldDataFile extended associations                             *
     *                                                                  *
     ********************************************************************/

    #ifdef SOM_XFldDataFile_h
        /* PSZ ftypQueryType(WPDataFile *somSelf,
                          PSZ pszOriginalTypes,
                          PCGLOBALSETTINGS pGlobalSettings); */

        PLINKLIST ftypBuildAssocsList(WPDataFile *somSelf);

        ULONG ftypFreeAssocsList(PLINKLIST pllAssocs);

        WPObject* ftypQueryAssociatedProgram(WPDataFile *somSelf,
                                             ULONG ulView);

        BOOL ftypModifyDataFileOpenSubmenu(WPDataFile *somSelf,
                                           HWND hwndOpenSubmenu,
                                           BOOL fDeleteExisting);
    #endif

    /* ******************************************************************
     *                                                                  *
     *   Notebook callbacks (notebook.c) for XFldWPS "File types" page  *
     *                                                                  *
     ********************************************************************/

    #ifdef NOTEBOOK_HEADER_INCLUDED
        VOID ftypFileTypesInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                      ULONG flFlags);

        MRESULT ftypFileTypesItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                            USHORT usItemID,
                                            USHORT usNotifyCode,
                                            ULONG ulExtra);

        extern MPARAM *G_pampFileTypesPage;
        extern ULONG G_cFileTypesPage;

        MRESULT EXPENTRY fnwpImportWPSFilters(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);
    #endif

#endif // __EXTASSOCS__

#endif


