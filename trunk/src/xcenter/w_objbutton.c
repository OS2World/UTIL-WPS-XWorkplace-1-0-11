
/*
 *@@sourcefile w_objbutton.c:
 *      XCenter "object button" widget implementation.
 *      This is built into the XCenter and not in
 *      a plugin DLL because it uses tons of WPS calls.
 *
 *      The PM window class actually implements two
 *      widget classes, the "X-Button" and the "object
 *      button", which are quite similar.
 *
 *      Function prefix for this file:
 *      --  Owgt*
 *
 *      This is all new with V0.9.7.
 *
 *@@added V0.9.7 (2000-11-27) [umoeller]
 *@@header "shared\center.h"
 */

/*
 *      Copyright (C) 2000 Ulrich M�ller.
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

#define INCL_DOSPROCESS
#define INCL_DOSEXCEPTIONS
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WININPUT
#define INCL_WINSYS
#define INCL_WINTIMER
#define INCL_WINPOINTERS
#define INCL_WINMENUS

#define INCL_GPICONTROL
#define INCL_GPIPRIMITIVES
#define INCL_GPILOGCOLORTABLE
#define INCL_GPIREGIONS
#include <os2.h>

// C library headers
#include <stdio.h>
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\comctl.h"             // common controls (window procs)
#include "helpers\gpih.h"               // GPI helper routines
#include "helpers\linklist.h"           // linked list helper routines
#include "helpers\prfh.h"               // INI file helper routines
#include "helpers\winh.h"               // PM helper routines
#include "helpers\xstring.h"            // extended string helpers

// SOM headers which don't crash with prec. header files
#include "xcenter.ih"
#include "xfobj.ih"
#include "xfldr.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\contentmenus.h"        // shared menu logic
#include "shared\kernel.h"              // XWorkplace Kernel

#include "shared\center.h"              // public XCenter interfaces

#include "filesys\object.h"             // XFldObject implementation

#include "startshut\shutdown.h"         // XWorkplace eXtended Shutdown

#pragma hdrstop                     // VAC++ keeps crashing otherwise
#include <wpdesk.h>
#include <wpdisk.h>
#include <wppower.h>
#include <wpshadow.h>
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

/* ******************************************************************
 *
 *   Private widget instance data
 *
 ********************************************************************/

/*
 * OBJBUTTONSETUP:
 *
 */

typedef struct _OBJBUTTONSETUP
{
    HOBJECT     hobj;                   // member object's handle

} OBJBUTTONSETUP, *POBJBUTTONSETUP;

/*
 *@@ OBJBUTTONPRIVATE:
 *      more window data for the "X-Button" widget.
 *
 *      An instance of this is created on WM_CREATE
 *      fnwpObjButtonWidget and stored in XCENTERWIDGET.pUser.
 */

typedef struct _OBJBUTTONPRIVATE
{
    PXCENTERWIDGET pWidget;
            // reverse ptr to general widget data ptr; we need
            // that all the time and don't want to pass it on
            // the stack with each function call

    OBJBUTTONSETUP Setup;
            // widget settings that correspond to a setup string

    ULONG       ulType;                 // either BTF_OBJBUTTON or BTF_XBUTTON

    BOOL        fMouseButton1Down;      // if TRUE, mouse button is currently down
    BOOL        fButtonSunk;            // if TRUE, button control is currently pressed;
                    // the control is painted "down" if either of the two are TRUE
    BOOL        fMouseCaptured; // if TRUE, mouse is currently captured

    HWND        hwndMenuMain;           // if != NULLHANDLE, this has the currently
                                        // open menu (X-button and object buttons)
    HWND        hwndObjectPopup;        // if != NULLHANDLE, this has the currently
                                        // open object WPS context menu (obj button only)
    BOOL        fOpenedWPSContextMenu;  // TRUE if next WM_COMMAND could be WPS context
                                        // menu; WM_COMMAND comes in AFTER WM_MENUEND,
                                        // so we can't check hwndObjectPoup

    WPObject    *pobjButton;            // object for this button
    WPObject    *pobjNotify;            // != NULL if xwpAddDestroyNotify has been
                                        // called (to avoid duplicate notifications)

    HPOINTER    hptrXMini;              // "X" icon for painting on button,
                                        // if BTF_XBUTTON

    WPPower     *pPower;                // for X-button: if != NULLHANDLE,
                                        // power management is enabled, and this
                                        // has the "<WP_POWER>" object

} OBJBUTTONPRIVATE, *POBJBUTTONPRIVATE;

/* ******************************************************************
 *
 *   Widget setup management
 *
 ********************************************************************/

/*
 *      This section contains shared code to manage the
 *      widget's settings. This can translate a widget
 *      setup string into the fields of a binary setup
 *      structure and vice versa. This code is used by
 *      both an open widget window and a settings dialog.
 */

/*
 *@@ OwgtClearSetup:
 *      cleans up the data in the specified setup
 *      structure, but does not free the structure
 *      itself.
 */

VOID OwgtClearSetup(POBJBUTTONSETUP pSetup)
{
}

/*
 *@@ OwgtScanSetup:
 *      scans the given setup string and translates
 *      its data into the specified binary setup
 *      structure.
 *
 *      NOTE: It is assumed that pSetup is zeroed
 *      out. We do not clean up previous data here.
 *
 *@@added V0.9.7 (2000-12-07) [umoeller]
 */

VOID OwgtScanSetup(const char *pcszSetupString,
                   POBJBUTTONSETUP pSetup)
{
    PSZ p;
    p = ctrScanSetupString(pcszSetupString,
                           "OBJECTHANDLE");
    if (p)
    {
        // scan hex object handle
        pSetup->hobj = strtol(p, NULL, 16);
        ctrFreeSetupValue(p);
    }

}

/*
 *@@ OwgtSaveSetup:
 *      composes a new setup string.
 *      The caller must invoke xstrClear on the
 *      string after use.
 */

VOID OwgtSaveSetup(PXSTRING pstrSetup,       // out: setup string (is cleared first)
                   ULONG cxCurrent,
                   POBJBUTTONSETUP pSetup)
{
    CHAR    szTemp[100];
    xstrInit(pstrSetup, 40);

    if (pSetup->hobj)
    {
        sprintf(szTemp, "OBJECTHANDLE=%lX;",
                pSetup->hobj);
        xstrcat(pstrSetup, szTemp, 0);
    }
}

/* ******************************************************************
 *
 *   Widget settings dialog
 *
 ********************************************************************/

// None currently.

/* ******************************************************************
 *
 *   Callbacks stored in XCENTERWIDGET
 *
 ********************************************************************/

/*
 *@@ OwgtSetupStringChanged:
 *      this gets called from ctrSetSetupString if
 *      the setup string for a widget has changed.
 *
 *      This procedure's address is stored in
 *      XCENTERWIDGET so that the XCenter knows that
 *      we can do this.
 */

VOID EXPENTRY OwgtSetupStringChanged(PXCENTERWIDGET pWidget,
                                     const char *pcszNewSetupString)
{
    POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
    if (pPrivate)
    {
        // reinitialize the setup data
        OwgtClearSetup(&pPrivate->Setup);
        OwgtScanSetup(pcszNewSetupString,
                      &pPrivate->Setup);
    }
}

// VOID EXPENTRY OwgtShowSettingsDlg(PWIDGETSETTINGSDLGDATA pData)

/* ******************************************************************
 *
 *   PM window class implementation
 *
 ********************************************************************/

/*
 *@@ FindObject:
 *      returns the SOM object pointer for the object handle
 *      which is stored in the widget's setup.
 *
 *      If the HOBJECT is a WPDisk, we return the root folder
 *      for the disk instead.
 *
 *      NOTE: The object is locked by this function. In addition,
 *      we set the OBJLIST_OBJWIDGET list notify flag (see
 *      XFldObject::xwpSetListNotify).
 *
 *@@added V0.9.7 (2000-12-13) [umoeller]
 */

WPObject* FindObject(POBJBUTTONPRIVATE pPrivate)
{
    WPObject *pobj = NULL;

    if (pPrivate->Setup.hobj)
    {
        pobj = _wpclsQueryObject(_WPObject,
                                 pPrivate->Setup.hobj);
        if (pobj)
        {
            // dereference shadows
            while ((pobj) && (_somIsA(pobj, _WPShadow)))
                pobj = _wpQueryShadowedObject(pobj, TRUE);

            if (pobj)
            {
                // now, if pObj is a disk object: get root folder
                if (_somIsA(pobj, _WPDisk))
                    pobj = wpshQueryRootFolder(pobj, FALSE, NULL);

                if ((pobj) && (pPrivate->pobjNotify != pobj))
                {
                    // set list notify so that the widget is destroyed
                    // when the object goes dormant
                    _wpLockObject(pobj);
                    _xwpAddDestroyNotify(pobj,
                                         pPrivate->pWidget->hwndWidget);
                    pPrivate->pobjNotify = pobj;
                }
            }
        }
    }

    return (pobj);
}

/*
 *@@ OwgtCreate:
 *      implementation for WM_CREATE.
 */

MRESULT OwgtCreate(HWND hwnd, MPARAM mp1)
{
    MRESULT mrc = 0;
    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)mp1;
    POBJBUTTONPRIVATE pPrivate = malloc(sizeof(OBJBUTTONPRIVATE));
    memset(pPrivate, 0, sizeof(OBJBUTTONPRIVATE));
    // link the two together
    pWidget->pUser = pPrivate;
    pPrivate->pWidget = pWidget;

    OwgtScanSetup(pWidget->pcszSetupString,
                  &pPrivate->Setup);

    // get type from class
    pPrivate->ulType = pWidget->pWidgetClass->ulExtra;
    if (pPrivate->ulType == BTF_XBUTTON)
        pPrivate->hptrXMini = WinLoadPointer(HWND_DESKTOP,
                                             cmnQueryMainResModuleHandle(),
                                             ID_ICONXMINI);

    // enable context menu help
    pWidget->pcszHelpLibrary = cmnQueryHelpLibrary();
    if (pPrivate->ulType == BTF_XBUTTON)
        pWidget->ulHelpPanelID = ID_XSH_WIDGET_XBUTTON_MAIN;
    else
        pWidget->ulHelpPanelID = ID_XSH_WIDGET_OBJBUTTON_MAIN;


    // return 0 for success
    return (mrc);
}

/*
 *@@ OwgtControl:
 *      implementation for WM_CONTROL.
 *
 *@@added V0.9.7 (2000-12-14) [umoeller]
 */

BOOL OwgtControl(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
    BOOL brc = FALSE;

    USHORT  usID = SHORT1FROMMP(mp1),
            usNotifyCode = SHORT2FROMMP(mp1);

    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);

    _Pmpf((__FUNCTION__ ": WM_CONTROL id 0x%lX", usID));

    if (pWidget)
    {
        POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
        if (pPrivate)
        {
            if (usID == ID_XCENTER_CLIENT)
            {
                switch (usNotifyCode)
                {
                    /*
                     * XN_QUERYSIZE:
                     *      XCenter wants to know our size.
                     */

                    case XN_QUERYSIZE:
                    {
                        PSIZEL pszl = (PSIZEL)mp2;
                        pszl->cx = pWidget->pGlobals->cxMiniIcon
                                   + 2;    // 2*1 spacing between icon and border
                        if ((pWidget->pGlobals->flDisplayStyle & XCS_FLATBUTTONS)
                                        == 0)
                            pszl->cx += 4;     // 2*2 for button borders

                        // we wanna be square
                        pszl->cy = pszl->cx;

                        brc = TRUE;
                    break; }

                    /*
                     * XN_OBJECTDESTROYED:
                     *      member object has been destroyed.
                     *      Destroy ourselves too.
                     */

                    case XN_OBJECTDESTROYED:
                        // simulate "remove widget" context menu command...
                        // that's easier
                        WinPostMsg(pWidget->hwndWidget,
                                   WM_COMMAND,
                                   (MPARAM)ID_CRMI_REMOVEWGT,
                                   NULL);
                    break;
                }
            } // if (usID == ID_XCENTER_CLIENT)
            else
            {
                if (usID == ID_XCENTER_TOOLTIP)
                {
                    _Pmpf((__FUNCTION__ ": ID_XCENTER_TOOLTIP"));
                    if (usNotifyCode == TTN_NEEDTEXT)
                    {
                        PTOOLTIPTEXT pttt = (PTOOLTIPTEXT)mp2;
                        _Pmpf((__FUNCTION__ ": TTN_NEEDTEXT"));
                        if (pPrivate->ulType == BTF_XBUTTON)
                            pttt->pszText = "X Button";
                        else
                        {
                            if (!pPrivate->pobjButton)
                                // object not queried yet:
                                pPrivate->pobjButton = FindObject(pPrivate);

                            if (pPrivate->pobjButton)
                                pttt->pszText = _wpQueryTitle(pPrivate->pobjButton);
                            else
                                pttt->pszText = "Invalid object...";
                        }

                        pttt->ulFormat = TTFMT_PSZ;
                    }
                }
            }
        } // end if (pPrivate)
    } // end if (pWidget)

    return (brc);
}

/*
 * OwgtPaintButton:
 *      implementation for WM_PAINT.
 */

VOID OwgtPaintButton(HWND hwnd)
{
    RECTL rclPaint;
    HPS hps = WinBeginPaint(hwnd, NULLHANDLE, &rclPaint);

    if (hps)
    {
        // get widget data and its button data from QWL_USER
        PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
        if (pWidget)
        {
            POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
            if (pPrivate)
            {
                RECTL   rclWin;
                ULONG   ulBorder = 0,
                        cx,
                        cy,
                        cxMiniIcon = pWidget->pGlobals->cxMiniIcon,
                        ulOfs = 0;
                LONG    lLeft,
                        lRight,
                        lMiddle = WinQuerySysColor(HWND_DESKTOP, SYSCLR_BUTTONMIDDLE, 0);
                HPOINTER hptr = NULLHANDLE;

                if ((pWidget->pGlobals->flDisplayStyle & XCS_FLATBUTTONS) == 0)
                    ulBorder = 2;

                WinQueryWindowRect(hwnd, &rclWin);      // exclusive
                gpihSwitchToRGB(hps);

                if (    (pPrivate->fMouseButton1Down)
                     || (pPrivate->fButtonSunk)
                   )
                {
                    // paint button "down":
                    lLeft = pWidget->pGlobals->lcol3DDark;
                    lRight = pWidget->pGlobals->lcol3DLight;
                    // add offset for icon painting at the bottom
                    ulOfs += 1;
                    if (ulBorder == 0)
                        ulBorder = 1;
                }
                else
                {
                    lLeft = pWidget->pGlobals->lcol3DLight;
                    lRight = pWidget->pGlobals->lcol3DDark;
                }

                if (ulBorder)
                {
                    // button border:

                    // now paint button frame
                    rclWin.xRight--;
                    rclWin.yTop--;
                    gpihDraw3DFrame(hps,
                                    &rclWin,        // inclusive
                                    ulBorder,
                                    lLeft,
                                    lRight);

                    // now paint button middle
                    rclWin.xLeft += ulBorder;
                    rclWin.yBottom += ulBorder;
                    rclWin.xRight -= ulBorder - 1;  // make exclusive again
                    rclWin.yTop -= ulBorder - 1;    // make exclusive again
                }

                WinFillRect(hps,
                            &rclWin,        // exclusive
                            lMiddle);

                // get icon
                if (pPrivate->ulType == BTF_XBUTTON)
                {
                    hptr = pPrivate->hptrXMini;
                }
                else
                {
                    if (!pPrivate->pobjButton)
                        // object not queried yet:
                        pPrivate->pobjButton = FindObject(pPrivate);

                    if (pPrivate->pobjButton)
                        hptr = _wpQueryIcon(pPrivate->pobjButton);
                }

                if (hptr)
                {
                    // now paint icon
                    cx = rclWin.xRight - rclWin.xLeft;
                    cy = rclWin.yTop - rclWin.yBottom;
                    GpiIntersectClipRectangle(hps, &rclWin);    // exclusive!
                    WinDrawPointer(hps,
                                   // center this in remaining rectl
                                   rclWin.xLeft + ((cx - cxMiniIcon) / 2) + ulOfs,
                                   rclWin.yBottom + ((cy - cxMiniIcon) / 2) - ulOfs,
                                   hptr,
                                   DP_MINI);
                }
            } // end if (pPrivate)
        } // end if (pWidget)

        WinEndPaint(hps);
    } // end if (hps)
}

/*
 * OwgtButton1Down:
 *      implementation for WM_BUTTON1DOWN.
 */

VOID OwgtButton1Down(HWND hwnd)
{
    // get widget data and its button data from QWL_USER
    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pWidget)
    {
        POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
        if (pPrivate)
        {
            if (WinIsWindowEnabled(hwnd))
            {
                pPrivate->fMouseButton1Down = TRUE;
                WinInvalidateRect(hwnd, NULL, FALSE);

                // since we're not passing the message
                // to WinDefWndProc, we need to give
                // ourselves the focus; this will also
                // dismiss the button's menu, if open
                WinSetFocus(HWND_DESKTOP, hwnd);

                if (!pPrivate->fMouseCaptured)
                {
                    // capture mouse events while the
                    // mouse button is down
                    WinSetCapture(HWND_DESKTOP, hwnd);
                    pPrivate->fMouseCaptured = TRUE;
                }

                if (!pPrivate->fButtonSunk)
                {
                    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
                    // toggle state is still UP (i.e. button pressed
                    // for the first time): create menu

                    // prepare globals in fdrmenus.c
                    _Pmpf((__FUNCTION__ ": calling cmnuInitItemCache"));
                    cmnuInitItemCache(pGlobalSettings);

                    if (pPrivate->ulType == BTF_XBUTTON)
                    {
                        // it's an X-button: load default menu
                        WPDesktop *pActiveDesktop = cmnQueryActiveDesktop();
                        PSZ pszDesktopTitle = _wpQueryTitle(pActiveDesktop);
                        PCKERNELGLOBALS  pKernelGlobals = krnQueryGlobals();
                        BOOL fShutdownRunning = xsdIsShutdownRunning();

                        pPrivate->hwndMenuMain = WinLoadMenu(hwnd,
                                                             cmnQueryNLSModuleHandle(FALSE),
                                                             ID_CRM_XCENTERBUTTON);

                        if ((pGlobalSettings->ulXShutdownFlags & XSD_CONFIRM) == 0)
                        {
                            // if XShutdown confirmations have been disabled,
                            // remove "..." from the shutdown menu entries
                            winhMenuRemoveEllipse(pPrivate->hwndMenuMain,
                                                  ID_CRMI_RESTARTWPS);
                            winhMenuRemoveEllipse(pPrivate->hwndMenuMain,
                                                  ID_CRMI_SHUTDOWN);
                        }

                        WinEnableMenuItem(pPrivate->hwndMenuMain,
                                          ID_CRMI_RESTARTWPS,
                                          !fShutdownRunning);
                        WinEnableMenuItem(pPrivate->hwndMenuMain,
                                          ID_CRMI_SHUTDOWN,
                                          !fShutdownRunning);

                        if (!pKernelGlobals->pXWPShellShared)
                        {
                            // XWPShell not running:
                            // remove "logoff"
                            winhRemoveMenuItem(pPrivate->hwndMenuMain,
                                               ID_CRMI_LOGOFF);
                        }
                        else
                        {
                            if ((pGlobalSettings->ulXShutdownFlags & XSD_CONFIRM) == 0)
                                // if XShutdown confirmations have been disabled,
                                // remove "..." from menu entry
                                winhMenuRemoveEllipse(pPrivate->hwndMenuMain,
                                                      ID_CRMI_LOGOFF);
                            WinEnableMenuItem(pPrivate->hwndMenuMain,
                                              ID_CRMI_LOGOFF,
                                              !fShutdownRunning);
                        }

                        // check if we can find the "Power" object
                        if (pPrivate->pPower = wpshQueryObjectFromID("<WP_POWER>", NULL))
                            if (_somIsA(pPrivate->pPower, _WPPower))
                            {
                                // is power management enabled?
                                if (!_wpQueryPowerManagement(pPrivate->pPower))
                                    // no:
                                    pPrivate->pPower = NULL;
                            }
                            else
                                pPrivate->pPower = NULL;

                        if (!pPrivate->pPower)
                            winhRemoveMenuItem(pPrivate->hwndMenuMain,
                                               ID_CRMI_SUSPEND);
                        else
                            // power exists:
                            if (!_wpQueryPowerConfirmation(pPrivate->pPower))
                                // if power confirmations have been disabled,
                                // remove "..." from menu entry
                                winhMenuRemoveEllipse(pPrivate->hwndMenuMain,
                                                      ID_CRMI_SUSPEND);

                        // prepare folder content submenu for Desktop
                        cmnuPrepareContentSubmenu(pActiveDesktop,
                                                  pPrivate->hwndMenuMain,
                                                  pszDesktopTitle,
                                                  0,        // top item
                                                  FALSE); // no owner draw in main context menu
                    } // if (pPrivate->ulType == BTF_XBUTTON)
                    else
                    {
                        // regular object button:
                        // check if this is a folder...

                        if (!pPrivate->pobjButton)
                            // object not queried yet:
                            pPrivate->pobjButton = FindObject(pPrivate);

                        if (pPrivate->pobjButton)
                        {
                            if (_somIsA(pPrivate->pobjButton, _WPFolder))
                            {
                                // yes, it's a folder:
                                // prepare folder content menu by inserting
                                // a dummy menu item... the actual menu
                                // fill is done for WM_INITMENU, which comes
                                // in right afterwards
                                pPrivate->hwndMenuMain = WinCreateMenu(hwnd, NULL);
                                WinSetWindowBits(pPrivate->hwndMenuMain,
                                                 QWL_STYLE,
                                                 MIS_OWNERDRAW,
                                                 MIS_OWNERDRAW);

                                // insert a dummy menu item so that cmnuPrepareOwnerDraw
                                // can measure its size
                                _Pmpf((__FUNCTION__ ": inserting dummy"));
                                winhInsertMenuItem(pPrivate->hwndMenuMain,
                                                   0,
                                                   pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_DUMMY,
                                                   "test",
                                                   MIS_TEXT,
                                                   0);
                            }
                            // else no folder:
                            // do nothing at this point, just paint the
                            // button depressed... we'll open the object
                            // on button-up
                        }
                    }

                    if (pPrivate->hwndMenuMain)
                    {
                        RECTL rclButton;
                        WinQueryWindowRect(hwnd, &rclButton);
                        // rclButton now has button coordinates;
                        // convert this to screen coordinates:
                        WinMapWindowPoints(hwnd,
                                           HWND_DESKTOP,
                                           (PPOINTL)&rclButton,
                                           2);          // rectl == 2 points

                        if (pWidget->pGlobals->ulPosition == XCENTER_TOP)
                            cmnuSetPositionBelow((PPOINTL)&rclButton);

                        ctlDisplayButtonMenu(hwnd,
                                             pPrivate->hwndMenuMain);
                    }
                } // end if (!pPrivate->fButtonSunk)
            }
        } // end if (pPrivate)
    } // end if (pWidget)
}

/*
 * OwgtButton1Up:
 *      implementation for WM_BUTTON1UP.
 */

VOID OwgtButton1Up(HWND hwnd)
{
    // get widget data and its button data from QWL_USER
    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pWidget)
    {
        POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
        if (pPrivate)
        {
            if (WinIsWindowEnabled(hwnd))
            {
                // un-capture the mouse first
                if (pPrivate->fMouseCaptured)
                {
                    WinSetCapture(HWND_DESKTOP, NULLHANDLE);
                    pPrivate->fMouseCaptured = FALSE;
                }

                pPrivate->fMouseButton1Down = FALSE;

                // toggle state with each WM_BUTTON1UP
                pPrivate->fButtonSunk = !pPrivate->fButtonSunk;

                if (pPrivate->ulType == BTF_OBJBUTTON)
                    // we have an object button (not X-button):
                    if (pPrivate->pobjButton)
                        // object was successfully retrieved on button-down:
                        if (!_somIsA(pPrivate->pobjButton, _WPFolder))
                        {
                            // object is not a folder:
                            // open it on button up!
                            _wpViewObject(pPrivate->pobjButton,
                                          NULLHANDLE,
                                          OPEN_DEFAULT, // default view, same as dblclick
                                          0);
                            // unset button sunk state
                            // (no toggle)
                            pPrivate->fButtonSunk = FALSE;
                        }
                        // else folder: we do nothing, the work for the menu
                        // has been set up in button-down and init-menu

                // repaint sunk button state
                WinInvalidateRect(hwnd, NULL, FALSE);
            }
        } // end if (pPrivate)
    } // end if (pWidget)
}

/*
 * OwgtInitMenu:
 *      implementation for WM_INITMENU.
 *
 *      Note that this comes only in for...
 *
 *      -- the X-button;
 *
 *      -- an object button if the object button represents
 *         a folder (or disk).
 */

VOID OwgtInitMenu(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
    // get widget data and its button data from QWL_USER
    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
    _Pmpf((__FUNCTION__ ": entering"));
    if (pWidget)
    {
        POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
        if (pPrivate)
        {
            PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
            SHORT sMenuIDMsg = (SHORT)mp1;
            HWND hwndMenuMsg = (HWND)mp2;

            if (   (pPrivate->ulType == BTF_OBJBUTTON)
                && (hwndMenuMsg == pPrivate->hwndMenuMain)
               )
            {
                // WM_INITMENU for object's button main menu:
                // we then need to load the objects directly into
                // the menu

                _Pmpf((__FUNCTION__ ": for main menu: calling cmnuPrepareOwnerDraw"));
                cmnuPrepareOwnerDraw(hwndMenuMsg);

                // remove dummy item
                _Pmpf((__FUNCTION__ ":   removing dummy"));
                winhRemoveMenuItem(pPrivate->hwndMenuMain,
                                   pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_DUMMY);

                if (!pPrivate->pobjButton)
                    // object not queried yet:
                    pPrivate->pobjButton = FindObject(pPrivate);

                if (pPrivate->pobjButton)
                {
                    // just to make sure it's a folder:
                    if (_somIsA(pPrivate->pobjButton, _WPFolder))
                    {
                        // show "Wait" pointer
                        HPOINTER    hptrOld = winhSetWaitPointer();

                        // populate
                        wpshCheckIfPopulated(pPrivate->pobjButton,
                                             FALSE);    // full populate

                        WinSetPointer(HWND_DESKTOP, hptrOld);

                        if (_wpQueryContent(pPrivate->pobjButton, NULL, QC_FIRST))
                        {
                            // folder does contain objects: go!
                            // insert all objects (this takes a long time)...
                            _Pmpf((__FUNCTION__ ": calling cmnuInsertObjectsIntoMenu"));
                            cmnuInsertObjectsIntoMenu(pPrivate->pobjButton,
                                                      pPrivate->hwndMenuMain);
                            _Pmpf((__FUNCTION__ ": cmnuInsertObjectsIntoMenu returned"));

                            /* winhDumpSWP(__FUNCTION__ " hmenu",
                                        pPrivate->hwndMenuMain); */

                            // fix menu position...
                        }
                    }
                } // end if (pobj)

                // mark this as non-WPS context menu
                pPrivate->fOpenedWPSContextMenu = FALSE;

            } // end if (   (pPrivate->ulType == BTF_OBJBUTTON) ...
            else
            {
                // find out whether the menu of which we are notified
                // is a folder content menu; if so (and it is not filled
                // yet), the first menu item is ID_XFMI_OFS_DUMMY
                _Pmpf((__FUNCTION__ ":   for submenu"));
                if ((ULONG)WinSendMsg(hwndMenuMsg,
                                      MM_ITEMIDFROMPOSITION,
                                      (MPARAM)0,        // menu item index
                                      MPNULL)
                           == (pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_DUMMY))
                {
                   // okay, let's go
                   if (pGlobalSettings->FCShowIcons)
                   {
                       // show folder content icons ON:

                       #ifdef DEBUG_MENUS
                           _Pmpf(( "  preparing owner draw"));
                       #endif

                       cmnuPrepareOwnerDraw(hwndMenuMsg);
                   }

                   // add menu items according to folder contents
                   _Pmpf((__FUNCTION__ ":   calling cmnuFillContentSubmenu"));
                   cmnuFillContentSubmenu(sMenuIDMsg,
                                          hwndMenuMsg);
                }
            }
        }
    }
    _Pmpf((__FUNCTION__ ": leaving"));
        // strange... after this, another flurry of WM_MEASUREITEM
        // things comes in...
}

/*
 * OwgtMenuEnd:
 *      implementation for WM_MENUEND.
 */

VOID OwgtMenuEnd(HWND hwnd, MPARAM mp2)
{
    // get widget data and its button data from QWL_USER
    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pWidget)
    {
        POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
        if (pPrivate)
        {
            if ((HWND)mp2 == pPrivate->hwndMenuMain)
            {
                // main menu is ending:
                if (!pPrivate->fMouseButton1Down)
                {
                    // mouse button not currently down
                    // --> menu dismissed for some other reason:
                    pPrivate->fButtonSunk = FALSE;
                    WinInvalidateRect(hwnd, NULL, FALSE);
                }

                WinDestroyWindow(pPrivate->hwndMenuMain);
                pPrivate->hwndMenuMain = NULLHANDLE;
            }

            if ((HWND)mp2 == pPrivate->hwndObjectPopup)
            {
                // object popup (copy of WPS context menu for object button):
                WinDestroyWindow(pPrivate->hwndObjectPopup);
                pPrivate->hwndObjectPopup = NULLHANDLE;
                // remove source emphasis
                WinInvalidateRect(pWidget->pGlobals->hwndClient, NULL, FALSE);
            }
        } // end if (pPrivate)
    } // end if (pWidget)
}

/*
 *@@ OwgtCommand:
 *      implementation for WM_COMMAND.
 *
 *      If this returns FALSE, the parent winproc is called.
 *
 *@@changed V0.9.9 (2001-03-07) [umoeller]: added "run" to X-button
 *@@changed V0.9.11 (2001-04-18) [umoeller]: now opening objects on thread 1 always
 *@@changed V0.9.11 (2001-04-25) [umoeller]: fixed broken standard widget menu items
 *@@changed V0.9.12 (2001-04-28) [umoeller]: moved XShutdown init to thread 1 object window
 */

BOOL OwgtCommand(HWND hwnd, MPARAM mp1)
{
    BOOL fProcessed = FALSE;
    ULONG ulMenuId = (ULONG)mp1;

    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pWidget)
    {
        POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
        if (pPrivate)
        {
            if (pPrivate->ulType == BTF_XBUTTON)
            {
                fProcessed = TRUE;

                switch (ulMenuId)
                {
                    case ID_CRMI_SUSPEND:
                        // check if the "Power" object has been set up
                        // in OwgtInitMenu
                        if (pPrivate->pPower)
                        {
                            BOOL fGo = FALSE;
                            if (_wpQueryPowerConfirmation(pPrivate->pPower))
                                // yeah, that's funny: why do they export this function
                                // and have this setting, and wpChangePowerState doesn't
                                // confirm anything?!?
                                // so do it now
                                fGo = (cmnMessageBoxMsg(pWidget->hwndWidget,
                                                        197,        // xcenter
                                                        198,        // sure suspend?
                                                        MB_YESNO)
                                                == MBID_YES);
                            else
                                fGo = TRUE;

                            if (fGo)
                            {
                                // sleep a little while... otherwise the
                                // "key up" or tiny "mouse move" will immediately
                                // wake up the system again
                                winhSleep(300);
                                // tell "Power" object to suspend
                                _wpChangePowerState(pPrivate->pPower,
                                                    MAKEULONG(6,        // set power state
                                                              0),       // reserved
                                                    MAKEULONG(1,        // all devices
                                                              2));      // suspend
                            }
                        }
                    break;

                    case ID_CRMI_LOGOFF:
                    case ID_CRMI_RESTARTWPS:
                    case ID_CRMI_SHUTDOWN:
                        // do this on thread 1, or otherwise we'll
                        // get problems with the XCenter thread
                        // V0.9.12 (2001-04-28) [umoeller]
                        krnPostThread1ObjectMsg(T1M_INITIATEXSHUTDOWN,
                                                (MPARAM)ulMenuId,
                                                0);
                    break;

                    case ID_CRMI_RUN:       // V0.9.9 (2001-03-07) [umoeller]
                        cmnRunCommandLine(pWidget->pGlobals->hwndFrame,
                                          NULL);        // boot drive
                    break;

                    default:
                        fProcessed = FALSE;
                }
            } // if (pPrivate->ulType == BTF_XBUTTON)

            if (!fProcessed)
            {
                // we get here...
                // -- for object buttons; fProcessed is still FALSE
                // -- for the x-button if none of the standard items
                //    was selected; this can be a subitem of "desktop" folder contents too
                PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
                ULONG ulFirstVarMenuId = pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_VARIABLE;
                if (     (ulMenuId >= ulFirstVarMenuId)
                      && (ulMenuId <  ulFirstVarMenuId + G_ulVarItemCount)
                      && (ulMenuId <  0x7f00)       // standard widget menu IDs
                   )
                {
                    // yes, variable menu item selected:
                    // get corresponding menu list item from the list that
                    // was created by mnuModifyFolderPopupMenu
                    PVARMENULISTITEM pItem = cmnuGetVarItem(ulMenuId - ulFirstVarMenuId);
                    WPObject    *pObject = NULL;

                    if (pItem)
                        pObject = pItem->pObject;

                    if (pObject)    // defaults to NULL
                    {
                        // _wpViewObject(pObject, NULLHANDLE, OPEN_DEFAULT, 0);
                        // no.... we're running on the XCenter thread here
                        // and we don't want the objects to open on the XCenter
                        // thread because if the XCenter is closed, all those
                        // views will go away (because the thread's msgq is
                        // destroyed)... redirect this to thread 1
                        // V0.9.11 (2001-04-18) [umoeller]
                        krnPostThread1ObjectMsg(T1M_OPENOBJECTFROMPTR,
                                               (MPARAM)pObject,
                                               (MPARAM)OPEN_DEFAULT);

                        fProcessed = TRUE;
                    }
                } // end if ((ulMenuId >= ID_XFM_VARIABLE) && (ulMenuId < ID_XFM_VARIABLE+varItemCount))
                else
                    // other:
                    // this MIGHT be a command from a WPS context menu...
                    if (    pPrivate->ulType == BTF_OBJBUTTON
                         && pPrivate->fOpenedWPSContextMenu
                         && pPrivate->pobjButton
                         && (ulMenuId <  0x7f00)       // standard widget menu IDs
                       )
                    {
                        // invoke the command on the object...
                        // this is probably "open" or "help"

                        // but do this on thread 1 also V0.9.11 (2001-04-18) [umoeller]
                        krnPostThread1ObjectMsg(T1M_MENUITEMSELECTED,
                                                (MPARAM)pPrivate->pobjButton,
                                                (MPARAM)ulMenuId);
                        fProcessed = TRUE;
                        /* fProcessed = _wpMenuItemSelected(pPrivate->pobjButton,
                                                         NULLHANDLE,     // hwndFrame
                                                         ulMenuId); */
                    }
            }

            pPrivate->fOpenedWPSContextMenu = FALSE;
        }
    }

    return (fProcessed);
}

/*
 *@@ OwgtContextMenu:
 *      implementation for WM_CONTEXTMENU.
 *
 *      For object buttons, this does some major hacks to
 *      display part of the object's WPS context menu on
 *      the object button.
 *
 *      For X-buttons, this calls ctrDefWidgetProc only.
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 *@@changed V0.9.11 (2001-04-25) [umoeller]: fixed context menus for broken objects
 */

MRESULT OwgtContextMenu(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
    if (pWidget)
    {
        POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
        if (pPrivate)
        {
            pPrivate->fOpenedWPSContextMenu = FALSE;

            if (pPrivate->ulType == BTF_OBJBUTTON)
            {
                // for object buttons, show the WPS context menu

                if (!pPrivate->pobjButton)
                    // object not queried yet:
                    pPrivate->pobjButton = FindObject(pPrivate);

                if (pPrivate->pobjButton)
                {
                    SHORT sIndex;
                    POINTL ptl;
                    HWND hmenuTemp;
                    ptl.x = SHORT1FROMMP(mp1);
                    ptl.y = SHORT2FROMMP(mp1);

#ifndef MENU_NODISPLAY
    #define MENU_NODISPLAY            0x40000000
#endif
                    // compose the object menu... we can't let
                    // the WPS display it because the WPS expects
                    // the object to be in a PM container. But
                    // the half-documented MENU_NODISPLAY flag
                    // takes care of this: it will only build the
                    // menu, but not display it.
                    // NOTE: Apparently this flag is not supported
                    // on Warp 3 (tested Warp 3 without fixpaks
                    // V0.9.11 (2001-04-25) [umoeller]).
                    hmenuTemp = _wpDisplayMenu(pPrivate->pobjButton,
                                               hwnd,            // owner
                                               NULLHANDLE,
                                               &ptl,
                                               MENU_OBJECTPOPUP | MENU_NODISPLAY,
                                               0);

                    if (hmenuTemp)  // V0.9.11 (2001-04-25) [umoeller]
                    {
                        // NOW... we still can't use this because there's
                        // many menu items in there which will cause the
                        // WPS to hang if the owner is not a container.
                        // The WPS simply expects popups to show in containers
                        // only, so there ain't much we can do about this.
                        // While "copy", "move" etc. will simply not work,
                        // "Pickup" will even hang the WPS solidly.

                        // SOOOO.... what we do is make a COPY of the
                        // WPS context menu with only the items that we support.
                        pPrivate->hwndObjectPopup = WinCreateMenu(HWND_DESKTOP, NULL);
                        if (pPrivate->hwndObjectPopup)
                        {
                            HWND hwndWidgetSubmenu;

                            winhCopyMenuItem(pPrivate->hwndObjectPopup,
                                             hmenuTemp,
                                             WPMENUID_OPEN, // 1, "open" submenu
                                             MIT_END);
                            winhCopyMenuItem(pPrivate->hwndObjectPopup,
                                             hmenuTemp,
                                             WPMENUID_PROPERTIES, // 0x70, properties
                                             MIT_END);
                            winhCopyMenuItem(pPrivate->hwndObjectPopup,
                                             hmenuTemp,
                                             WPMENUID_HELP, // 2, "help" submenu
                                             MIT_END);

                            winhInsertMenuSeparator(pPrivate->hwndObjectPopup,
                                                    MIT_END,
                                                    1234);

                            // add standard widget menu as submenu
                            hwndWidgetSubmenu
                                = winhMergeIntoSubMenu(pPrivate->hwndObjectPopup,
                                                       MIT_END,
                                                       "Object button widget", // @@todo NLS
                                                       2000,    // submenu ID
                                                       pPrivate->pWidget->hwndContextMenu);
                            if (hwndWidgetSubmenu)
                                // disable "Properties"... we have none
                                WinEnableMenuItem(hwndWidgetSubmenu,
                                                  ID_CRMI_PROPERTIES,
                                                  FALSE);

                            ctrShowContextMenu(pWidget, pPrivate->hwndObjectPopup);
                                    // destroyed on WM_MENUEND;
                                    // we need not care about the WPS context
                                    // menu we built because the WPS has its
                                    // internal management for that
                            pPrivate->fOpenedWPSContextMenu = TRUE;
                        } // end if (pPrivate->hwndObjectPopup)
                    } // end if (hmenuTemp)  // V0.9.11 (2001-04-25) [umoeller]
                } // end if (pPrivate->pobjButton)
            } // end if (pPrivate->ulType == BTF_OBJBUTTON)

            if (!pPrivate->fOpenedWPSContextMenu) // V0.9.11 (2001-04-25) [umoeller]
                // x-button, or cannot build WPS popup (Warp 3),
                // or object broken: show default widget menu
                mrc = ctrDefWidgetProc(hwnd, WM_CONTEXTMENU, mp1, mp2);
        }
    }

    return (mrc);
}

/*
 *@@ fnwpObjButtonWidget:
 *      window procedure for the desktop button widget class.
 *
 *      This is also the owner for the menu it displays and
 *      therefore handles control of the folder content menus.
 */

MRESULT EXPENTRY fnwpObjButtonWidget(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    switch (msg)
    {
        /*
         * WM_CREATE:
         *      as with all widgets, we receive a pointer to the
         *      XCENTERWIDGET in mp1, which was created for us.
         *
         *      The first thing the widget MUST do on WM_CREATE
         *      is to store the XCENTERWIDGET pointer (from mp1)
         *      in the QWL_USER window word by calling:
         *
         *          WinSetWindowPtr(hwnd, QWL_USER, mp1);
         *
         *      We use XCENTERWIDGET.pUser for allocating
         *      OBJBUTTONPRIVATE for our own stuff.
         *
         *      Each widget must write its desired width into
         *      XCENTERWIDGET.cx and cy.
         */

        case WM_CREATE:
            WinSetWindowPtr(hwnd, QWL_USER, mp1);
            mrc = OwgtCreate(hwnd, mp1);
        break;

        /*
         * WM_CONTROL:
         *      process notifications/queries from the XCenter.
         */

        case WM_CONTROL:
            mrc = (MPARAM)OwgtControl(hwnd, mp1, mp2);
        break;

        /*
         * WM_PAINT:
         *
         */

        case WM_PAINT:
            OwgtPaintButton(hwnd);
        break;

        /*
         * WM_BUTTON1DOWN:
         * WM_BUTTON1UP:
         *      these show/hide the menu.
         *
         *      Showing the menu follows these steps:
         *          a)  first WM_BUTTON1DOWN hilites the button;
         *          b)  first WM_BUTTON1UP shows the menu.
         *
         *      When the button is pressed again, the open
         *      menu loses focus, which results in WM_MENUEND
         *      and destroys the window automatically.
         */

        case WM_BUTTON1DOWN:
        case WM_BUTTON1DBLCLK:
            OwgtButton1Down(hwnd);
            mrc = (MPARAM)TRUE;     // message processed
        break;

        /*
         * WM_BUTTON1UP:
         *
         */

        case WM_BUTTON1UP:
            OwgtButton1Up(hwnd);
            mrc = (MPARAM)TRUE;     // message processed
        break;

        /*
         * WM_BUTTON1CLICK:
         *      swallow this
         */

        case WM_BUTTON1CLICK:
            mrc = (MPARAM)TRUE;
        break;

        /*
         * WM_INITMENU:
         *
         */

        case WM_INITMENU:
            OwgtInitMenu(hwnd, mp1, mp2);
        break;

        /*
         * WM_MEASUREITEM:
         *      this msg is sent only once per owner-draw item when
         *      PM needs to know its size. This gets sent to us for
         *      items in folder content menus (if icons are on); the
         *      height of our items will be the same as with
         *      non-owner-draw ones, but we need to calculate the width
         *      according to the item text.
         *
         *      (SHORT)mp1 is supposed to contain a "menu identifier",
         *      but from my testing this contains some random value.
         *
         *      Return value: check mnuMeasureItem.
         */

        case WM_MEASUREITEM:
        {
            PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
            mrc = cmnuMeasureItem((POWNERITEM)mp2, pGlobalSettings);
            _Pmpf((__FUNCTION__ ": WM_MEASUREITEM, returning %d", mrc));
        break; }

        /*
         * WM_DRAWITEM:
         *      this msg is sent for each item every time it
         *      needs to be redrawn. This gets sent to us for
         *      items in folder content menus (if icons are on).
         *
         *      (SHORT)mp1 is supposed to contain a "menu identifier",
         *      but from my testing this contains some random value.
         */

        case WM_DRAWITEM:
        {
            PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
            if (cmnuDrawItem(pGlobalSettings,
                             mp1, mp2))
                mrc = (MRESULT)TRUE;
        break; }

        /*
         * WM_MENUEND:
         *
         */

        case WM_MENUEND:
            OwgtMenuEnd(hwnd, mp2);
            mrc = ctrDefWidgetProc(hwnd, msg, mp1, mp2);
        break;

        /*
         * WM_COMMAND:
         *      handle command from menus.
         */

        case WM_COMMAND:
            if (!OwgtCommand(hwnd, mp1))
                // not processed:
                mrc = ctrDefWidgetProc(hwnd, msg, mp1, mp2);
        break;

        case WM_CONTEXTMENU:
            mrc = OwgtContextMenu(hwnd, mp1, mp2);
        break;

        /*
         * WM_DESTROY:
         *      clean up. This _must_ be passed on to
         *      ctrDefWidgetProc.
         */

        case WM_DESTROY:
        {
            PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
            if (pWidget)
            {
                POBJBUTTONPRIVATE pPrivate = (POBJBUTTONPRIVATE)pWidget->pUser;
                if (pPrivate)
                {
                    if (pPrivate->hptrXMini)
                        WinDestroyPointer(pPrivate->hptrXMini);

                    if (pPrivate->pobjNotify)
                    {
                        _xwpRemoveDestroyNotify(pPrivate->pobjNotify,
                                                hwnd);
                        _wpUnlockObject(pPrivate->pobjNotify);
                    }

                    // free private data
                    free(pPrivate);
                            // pWidget is cleaned up by DestroyWidgets
                }
            }
            mrc = ctrDefWidgetProc(hwnd, msg, mp1, mp2);
        break; }

        default:
            mrc = ctrDefWidgetProc(hwnd, msg, mp1, mp2);
    } // end switch(msg)

    return (mrc);
}


