
/*
 *@@sourcefile statbars.h:
 *      header file for the status bar translation logic (statbars.c)
 *
 *      Most declarations for statbars.c are still in common.h however.
 *
 *@@include #define INCL_WINWINDOWMGR
 *@@include #define INCL_DOSMODULEMGR
 *@@include #include <os2.h>
 *@@include #include "classes\xfldr.h"  // for common.h
 *@@include #include "shared\common.h"
 *@@include #include "shared\notebook.h" // for status bar notebook callback prototypes
 *@@include #include "filesys\folder.h"
 *@@include #include "filesys\statbars.h"
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

#ifndef STATBARS_HEADER_INCLUDED
    #define STATBARS_HEADER_INCLUDED

    /********************************************************************
     *                                                                  *
     *   Declarations                                                   *
     *                                                                  *
     ********************************************************************/

    // status bar styles
    #define SBSTYLE_WARP3RAISED     1
    #define SBSTYLE_WARP3SUNKEN     2
    #define SBSTYLE_WARP4RECT       3
    #define SBSTYLE_WARP4MENU       4

    #define SBS_STATUSBARFONT       1
    #define SBS_TEXTNONESEL         2
    #define SBS_TEXTMULTISEL        4

    #define SBV_ICON                1
    #define SBV_TREE                2
    #define SBV_DETAILS             4

    // max length of status bar mnemonics
    #define CCHMAXMNEMONICS         256

    #ifdef FOLDER_HEADER_INCLUDED
        /*
         *@@ STATUSBARDATA:
         *      stored in QWL_USER wnd data to further describe
         *      a status bar and store some data
         */

        typedef struct _STATUSBARDATA
        {
            WPFolder   *somSelf;            // the folder of the status bar
            PSUBCLASSEDLISTITEM psli;       // frame info struct (common.h)
            ULONG      idTimer;             // update delay timer
            BOOL       fDontBroadcast;      // anti-recursion flag for presparams
            BOOL       fFolderPopulated;    // anti-recursion flag for wpPopulate
            PFNWP      pfnwpStatusBarOriginal; // original static control wnd proc
        } STATUSBARDATA, *PSTATUSBARDATA;
    #endif

    // msgs to status bar window (STBM_xxx)
    #define STBM_UPDATESTATUSBAR        (WM_USER+1000)
    #define STBM_PROHIBITBROADCASTING     (WM_USER+1002)

    /********************************************************************
     *                                                                  *
     *   Prototypes                                                     *
     *                                                                  *
     ********************************************************************/

    #ifdef SOM_WPFolder_h

        BOOL stbClassAddsNewMnemonics(SOMClass *pClassObject);

        BOOL stbSetClassMnemonics(SOMClass *pClassObject,
                                  PSZ pszText);

        PSZ stbQueryClassMnemonics(SOMClass *pClassObject);

        ULONG  stbTranslateSingleMnemonics(SOMClass *pObject,
                                           PSZ* ppszText,
                                           CHAR cThousands);

        PSZ stbComposeText(WPFolder* somSelf,
                           HWND hwndCnr);

    #endif

    /* ******************************************************************
     *                                                                  *
     *   Notebook callbacks (notebook.c) for "Status bars" pages        *
     *                                                                  *
     ********************************************************************/

    #ifdef NOTEBOOK_HEADER_INCLUDED
        VOID stbStatusBar1InitPage(PCREATENOTEBOOKPAGE pcnbp,
                                       ULONG flFlags);

        MRESULT stbStatusBar1ItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                             USHORT usItemID,
                                             USHORT usNotifyCode,
                                             ULONG ulExtra);

        VOID stbStatusBar2InitPage(PCREATENOTEBOOKPAGE pcnbp,
                                       ULONG flFlags);

        MRESULT stbStatusBar2ItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                             USHORT usItemID,
                                             USHORT usNotifyCode,
                                             ULONG ulExtra);
    #endif
#endif
