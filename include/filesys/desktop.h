
/*
 *@@sourcefile desktop.h:
 *      header file for desktop.c (XFldDesktop implementation).
 *
 *      This file is ALL new with V0.9.0.
 *
 *@@include #include <os2.h>
 *@@include #include "shared\notebook.h"
 *@@include #include "filesys\desktop.h"
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

#ifndef SETPAGES_HEADER_INCLUDED
    #define SETPAGES_HEADER_INCLUDED

    /* ******************************************************************
     *                                                                  *
     *   Query setup strings                                            *
     *                                                                  *
     ********************************************************************/

    ULONG dtpQuerySetup(WPDesktop *somSelf,
                        PSZ pszSetupString,
                        ULONG cbSetupString);

    /* ******************************************************************
     *                                                                  *
     *   Desktop menus                                                  *
     *                                                                  *
     ********************************************************************/

    VOID dtpModifyPopupMenu(WPDesktop *somSelf,
                            HWND hwndMenu);

    BOOL dtpMenuItemSelected(XFldDesktop *somSelf,
                             HWND hwndFrame,
                             ULONG ulMenuId);

    /* ******************************************************************
     *                                                                  *
     *   XFldDesktop notebook settings pages callbacks (notebook.c)     *
     *                                                                  *
     ********************************************************************/

    #ifdef NOTEBOOK_HEADER_INCLUDED
        VOID dtpMenuItemsInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                      ULONG flFlags);

        MRESULT dtpMenuItemsItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                            USHORT usItemID,
                                            USHORT usNotifyCode,
                                            ULONG ulExtra);

        VOID dtpStartupInitPage(PCREATENOTEBOOKPAGE pcnbp,
                                    ULONG flFlags);

        MRESULT dtpStartupItemChanged(PCREATENOTEBOOKPAGE pcnbp,
                                          USHORT usItemID,
                                          USHORT usNotifyCode,
                                          ULONG ulExtra);
    #else
        #error "shared\notebook.h needs to be included before including desktop.h".
    #endif
#endif


