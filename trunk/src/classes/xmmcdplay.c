
/*
 *@@sourcefile xmmcdplay.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XMMCDPlayer: a new MMPM/2 CD player.
 *
 *      This class is new with V0.9.6.
 *
 *      Installation of this class is completely optional.
 *
 *@@added V0.9.7 (2000-12-20) [umoeller]
 *@@somclass XMMCDPlayer cdp_
 *@@somclass M_XMMCDPlayer cdpM_
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

/*
 *  This file was generated by the SOM Compiler and Emitter Framework.
 *  Generated using:
 *      SOM Emitter emitctm: 2.41
 */

#ifndef SOM_Module_xmmcdplay_Source
#define SOM_Module_xmmcdplay_Source
#endif
#define XMMCDPlayer_Class_Source
#define M_XMMCDPlayer_Class_Source

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

#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES

#define INCL_WINMENUS
#define INCL_WINSYS

#define INCL_GPI                // required for INCL_MMIO_CODEC
#define INCL_GPIBITMAPS         // required for INCL_MMIO_CODEC
#include <os2.h>

// multimedia includes
#define INCL_MCIOS2
#define INCL_MMIOOS2
#define INCL_MMIO_CODEC
#include <os2me.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\except.h"             // exception handling
#include "helpers\stringh.h"            // string helper routines
#include "helpers\winh.h"               // PM helper routines

// SOM headers which don't crash with prec. header files
#include "xmmcdplay.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel

#include "media\media.h"                // XWorkplace multimedia support

// other SOM headers

#pragma hdrstop

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

STATIC const char *G_pszCDPlayer = "XMMCDPlayer";

/* ******************************************************************
 *
 *   XMMCDPlayer Instance Methods
 *
 ********************************************************************/

/*
 *@@ xwpCDQueryStatus:
 *      returns the current status of the CD player.
 *
 */

SOM_Scope ULONG  SOMLINK cdp_xwpCDQueryStatus(XMMCDPlayer *somSelf)
{
    ULONG ulrc = 0;
    // XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDQueryStatus");

    if (xmmQueryStatus() == MMSTAT_WORKING)
        ulrc = 0;       // @@todo

    return (ulrc);
}

/*
 *@@ xwpCDPlay:
 *      starts playing at the current position. If nothing
 *      has been called before for this CD, this will be
 *      the start of the CD.
 */

SOM_Scope BOOL  SOMLINK cdp_xwpCDPlay(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDPlay");

    if (xmmQueryStatus() == MMSTAT_WORKING)
    {
        TRY_LOUD(excpt1)
        {
            PXMMCDPLAYER *ppPlayer = (PXMMCDPLAYER*)&_pvPlayer;

            if (!_pvPlayer)
                xmmCDOpenDevice(ppPlayer, 0);

            if (_pvPlayer)
            {
                // device is open:
                PXMMCDPLAYER pPlayer = _pvPlayer;
                brc = !xmmCDPlay(pPlayer,
                                 TRUE);  // show wait ptr

                if (_hwndOpenView)
                    xmmCDPositionAdvise(pPlayer,
                                        _hwndOpenView,
                                        CDM_POSITIONUPDATE);
            }
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return brc;
}

/*
 *@@ xwpCDTogglePlay:
 *      pauses the CD if it's currently playing; otherwise
 *      (if it's paused or stopped), starts playing.
 */

SOM_Scope BOOL  SOMLINK cdp_xwpCDTogglePlay(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDTogglePlay");

    if (xmmQueryStatus() == MMSTAT_WORKING)
    {
        TRY_LOUD(excpt1)
        {
            PXMMCDPLAYER *ppPlayer = (PXMMCDPLAYER*)&_pvPlayer;

            if (!_pvPlayer)
                xmmCDOpenDevice(ppPlayer, 0);

            if (_pvPlayer)
            {
                // device is open:
                PXMMCDPLAYER pPlayer = _pvPlayer;

                if (pPlayer->ulStatus == MCI_MODE_PLAY)
                    brc = !xmmCDPause(pPlayer);
                else
                    brc = !xmmCDPlay(pPlayer,
                                     TRUE);  // show wait ptr

                if (_hwndOpenView)
                    xmmCDPositionAdvise(pPlayer,
                                        _hwndOpenView,
                                        CDM_POSITIONUPDATE);
            }
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return brc;
}

/*
 *@@ xwpCDPause:
 *      pauses the CD.
 */

SOM_Scope BOOL  SOMLINK cdp_xwpCDPause(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDPause");

    if (xmmQueryStatus() == MMSTAT_WORKING)
    {
        TRY_LOUD(excpt1)
        {
            if (_pvPlayer)
            {
                // device is open:
                PXMMCDPLAYER pPlayer = _pvPlayer;

                if (pPlayer->ulStatus == MCI_MODE_PLAY)
                    brc = !xmmCDPause(pPlayer);

                if (_hwndOpenView)
                    xmmCDPositionAdvise(pPlayer,
                                        _hwndOpenView,
                                        CDM_POSITIONUPDATE);
            }
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return brc;
}

/*
 *@@ xwpCDStop:
 *      stops the CD. Also closes the device.
 */

SOM_Scope BOOL  SOMLINK cdp_xwpCDStop(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDStop");

    if (xmmQueryStatus() == MMSTAT_WORKING)
    {
        TRY_LOUD(excpt1)
        {
            PXMMCDPLAYER *ppPlayer = (PXMMCDPLAYER*)&_pvPlayer;

            if (_pvPlayer)
                xmmCDStop(ppPlayer);
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return brc;
}

/*
 *@@ xwpCDQueryCurrentTrack:
 *      returns the track no. that is currently playing (1-99)
 *      or 0 on errors.
 */

SOM_Scope ULONG  SOMLINK cdp_xwpCDQueryCurrentTrack(XMMCDPlayer *somSelf)
{
    ULONG ulrc = 0;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDQueryCurrentTrack");

    if (xmmQueryStatus() == MMSTAT_WORKING)
    {
        TRY_LOUD(excpt1)
        {
            // PXMMCDPLAYER *ppPlayer = (PXMMCDPLAYER*)&_pvPlayer;

            if (_pvPlayer)
            {
                // device is open:
                PXMMCDPLAYER pPlayer = _pvPlayer;
                ulrc = xmmCDQueryCurrentTrack(pPlayer);
            }
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return (ulrc);
}

/*
 *@@ xwpCDNextTrack:
 *      goes for the next track.
 */

SOM_Scope BOOL  SOMLINK cdp_xwpCDNextTrack(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDNextTrack");

    if (xmmQueryStatus() == MMSTAT_WORKING)
    {
        TRY_LOUD(excpt1)
        {
            PXMMCDPLAYER *ppPlayer = (PXMMCDPLAYER*)&_pvPlayer;

            if (!_pvPlayer)
                xmmCDOpenDevice(ppPlayer, 0);

            if (_pvPlayer)
            {
                // device is open:
                PXMMCDPLAYER pPlayer = _pvPlayer;
                brc = !xmmCDPlayTrack(pPlayer,
                                      xmmCDQueryCurrentTrack(pPlayer) + 1,
                                      TRUE);  // show wait ptr

                if (_hwndOpenView)
                    xmmCDPositionAdvise(pPlayer,
                                        _hwndOpenView,
                                        CDM_POSITIONUPDATE);
            }
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return brc;
}

/*
 *@@ xwpCDPrevTrack:
 *      goes for the previous track.
 */

SOM_Scope BOOL  SOMLINK cdp_xwpCDPrevTrack(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDPrevTrack");

    {
        TRY_LOUD(excpt1)
        {
            PXMMCDPLAYER *ppPlayer = (PXMMCDPLAYER*)&_pvPlayer;

            if (!_pvPlayer)
                xmmCDOpenDevice(ppPlayer, 0);

            if (_pvPlayer)
            {
                // device is open:
                PXMMCDPLAYER pPlayer = _pvPlayer;
                ULONG ulCurrentTrack = xmmCDQueryCurrentTrack(pPlayer);
                if (ulCurrentTrack > 1)
                    --ulCurrentTrack;

                brc = !xmmCDPlayTrack(pPlayer,
                                      ulCurrentTrack,
                                      TRUE);  // show wait ptr

                if (_hwndOpenView)
                    xmmCDPositionAdvise(pPlayer,
                                        _hwndOpenView,
                                        CDM_POSITIONUPDATE);
            }
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return brc;
}

/*
 *@@ xwpCDEject:
 *      ejects the CD. Closes the device also.
 */

SOM_Scope BOOL  SOMLINK cdp_xwpCDEject(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpCDEject");

    if (xmmQueryStatus() == MMSTAT_WORKING)
    {
        TRY_LOUD(excpt1)
        {
            PXMMCDPLAYER *ppPlayer = (PXMMCDPLAYER*)&_pvPlayer;

            // we need an open device for eject,
            // so create one if we don't have one
            if (!_pvPlayer)
                xmmCDOpenDevice(ppPlayer, 0);

            if (_pvPlayer)
                xmmCDEject(ppPlayer);       // closes the device
        }
        CATCH(excpt1) {} END_CATCH();
    }

    return brc;
}

/*
 *@@ xwpAddXMMCDPlayerPages:
 *
 */

SOM_Scope BOOL  SOMLINK cdp_xwpAddXMMCDPlayerPages(XMMCDPlayer *somSelf,
                                                   HWND hwndNotebook)
{
    /* XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf); */
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_xwpAddXMMCDPlayerPages");

    /* Return statement to be customized: */
    return 0;
}

/*
 *@@ wpInitData:
 *      this WPObject instance method gets called when the
 *      object is being initialized (on wake-up or creation).
 *      We initialize our additional instance data here.
 *      Always call the parent first.
 */

SOM_Scope void  SOMLINK cdp_wpInitData(XMMCDPlayer *somSelf)
{
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpInitData");

    XMMCDPlayer_parent_WPAbstract_wpInitData(somSelf);

    _hwndOpenView = NULLHANDLE;
    _pvPlayer = NULL;

    _lcolBackground = WinQuerySysColor(HWND_DESKTOP,
                                       SYSCLR_DIALOGBACKGROUND,
                                       0);
    _lcolForeground = WinQuerySysColor(HWND_DESKTOP,
                                       SYSCLR_WINDOWSTATICTEXT,
                                       0);
    _pszFont = NULL;

    _fShowingOpenViewMenu = FALSE;
}

/*
 *@@ wpUnInitData:
 *      this WPObject instance method is called when the object
 *      is destroyed as a SOM object, either because it's being
 *      made dormant or being deleted. All allocated resources
 *      should be freed here.
 *      The parent method must always be called last.
 */

SOM_Scope void  SOMLINK cdp_wpUnInitData(XMMCDPlayer *somSelf)
{
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpUnInitData");

    if (_pvPlayer)
        xmmCDStop((PXMMCDPLAYER*)&_pvPlayer);

    strhStore(&_pszFont, NULL, NULL);

    XMMCDPlayer_parent_WPAbstract_wpUnInitData(somSelf);
}

/*
 *@@ wpObjectReady:
 *      this WPObject notification method gets called by the
 *      WPS when object instantiation is complete, for any reason.
 *      ulCode and refObject signify why and where from the
 *      object was created.
 *      The parent method must be called first.
 *
 *      See XFldObject::wpObjectReady for remarks about using
 *      this method as a copy constructor.
 */

SOM_Scope void  SOMLINK cdp_wpObjectReady(XMMCDPlayer *somSelf,
                                          ULONG ulCode, WPObject* refObject)
{
    /* XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf); */
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpObjectReady");

    XMMCDPlayer_parent_WPAbstract_wpObjectReady(somSelf,
                                                ulCode,
                                                refObject);
}

/*
 *@@ wpSetup:
 *      this WPObject instance method is called to allow an
 *      object to set itself up according to setup strings.
 *      As opposed to wpSetupOnce, this gets called any time
 *      a setup string is invoked.
 *
 *      We use this interface to allow controlling the CD
 *      player thru setup strings. This allows the user to
 *      set up XWPString instances with hotkeys to control
 *      the CD player globally.
 *
 *      The syntax of the setup strings is "XWPCDPLAY=command",
 *      with "command" being a CD player command such as "PLAY",
 *      "PAUSE" etc.
 */

SOM_Scope BOOL  SOMLINK cdp_wpSetup(XMMCDPlayer *somSelf, PSZ pszSetupString)
{
    BOOL    brc = FALSE;
    ULONG   cbValue;
    CHAR    szValue[300];

    // XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpSetup");

    brc = XMMCDPlayer_parent_WPAbstract_wpSetup(somSelf, pszSetupString);

    cbValue = sizeof(szValue);
    if (_wpScanSetupString(somSelf,
                           pszSetupString,
                           "XWPCDPLAY",
                           szValue,
                           &cbValue))
    {
        _PmpfF(("got XWPCDPLAY=%s", szValue));

        /*
         * "PLAY":
         *
         */

        if (!strcmp(szValue, "PLAY"))
            brc = _xwpCDPlay(somSelf);

        /*
         * "STOP":
         *      stops the CD and closes the device.
         */

        else if (!strcmp(szValue, "STOP"))
            brc = _xwpCDStop(somSelf);

        /*
         * "PAUSE":
         *
         */

        else if (!strcmp(szValue, "PAUSE"))
            brc = _xwpCDPause(somSelf);

        /*
         * TOGGLEPLAY:
         *      play if paused/stopped;
         *      pause if playing.
         */

        else if (!strcmp(szValue, "TOGGLEPLAY"))
            brc = _xwpCDTogglePlay(somSelf);

        /*
         * "NEXTTRACK":
         *
         */

        else if (!strcmp(szValue, "NEXTTRACK"))
            brc = _xwpCDNextTrack(somSelf);

        /*
         * "PREVTRACK":
         *
         */

        else if (!strcmp(szValue, "PREVTRACK"))
            brc = _xwpCDPrevTrack(somSelf);

        /*
         * "EJECT":
         *
         */

        else if (!strcmp(szValue, "EJECT"))
            brc = _xwpCDEject(somSelf);

        else
            _Pmpf(("  not recognized"));

        _Pmpf(("Returning %d", brc));
    }

    return brc;
}

/*
 *@@ wpSaveState:
 *      this WPObject instance method saves an object's state
 *      persistently so that it can later be re-initialized
 *      with wpRestoreState. This gets called during wpClose,
 *      wpSaveImmediate or wpSaveDeferred processing.
 *      All persistent instance variables should be stored here.
 */

SOM_Scope BOOL  SOMLINK cdp_wpSaveState(XMMCDPlayer *somSelf)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpSaveState");

    brc = XMMCDPlayer_parent_WPAbstract_wpSaveState(somSelf);

    if (brc)
    {
        if (_pszFont)
            _wpSaveString(somSelf,
                          (PSZ)G_pszCDPlayer,
                          1,
                          _pszFont);
        _wpSaveLong(somSelf,
                    (PSZ)G_pszCDPlayer,
                    2,
                    _lcolBackground);
        _wpSaveLong(somSelf,
                    (PSZ)G_pszCDPlayer,
                    3,
                    _lcolForeground);
    }

    return brc;
}

/*
 *@@ wpRestoreState:
 *      this WPObject instance method gets called during object
 *      initialization (after wpInitData) to restore the data
 *      which was stored with wpSaveState.
 */

SOM_Scope BOOL  SOMLINK cdp_wpRestoreState(XMMCDPlayer *somSelf,
                                           ULONG ulReserved)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpRestoreState");

    brc = XMMCDPlayer_parent_WPAbstract_wpRestoreState(somSelf,
                                                       ulReserved);
    if (brc)
    {
        CHAR szFont[100];
        ULONG cb = sizeof(szFont),
              ul = 0;
        if (_wpRestoreString(somSelf,
                             (PSZ)G_pszCDPlayer,
                             1,
                             szFont,
                             &cb))
        {
            strhStore(&_pszFont, szFont, NULL);
        }

        if (_wpRestoreLong(somSelf,
                           (PSZ)G_pszCDPlayer,
                           2,
                           &ul))
            _lcolBackground = ul;

        if (_wpRestoreLong(somSelf,
                           (PSZ)G_pszCDPlayer,
                           3,
                           &ul))
            _lcolForeground = ul;
    }

    return brc;
}

/*
 *@@ wpFilterPopupMenu:
 *      this WPObject instance method allows the object to
 *      filter out unwanted menu items from the context menu.
 *      This gets called before wpModifyPopupMenu.
 */

SOM_Scope ULONG  SOMLINK cdp_wpFilterPopupMenu(XMMCDPlayer *somSelf,
                                               ULONG ulFlags,
                                               HWND hwndCnr,
                                               BOOL fMultiSelect)
{
    /* XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf); */
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpFilterPopupMenu");

    return (XMMCDPlayer_parent_WPAbstract_wpFilterPopupMenu(somSelf,
                                                            ulFlags,
                                                            hwndCnr,
                                                            fMultiSelect));
}

/*
 *@@ wpModifyPopupMenu:
 *      this WPObject instance methods gets called by the WPS
 *      when a context menu needs to be built for the object
 *      and allows the object to manipulate its context menu.
 *      This gets called _after_ wpFilterPopupMenu.
 */

SOM_Scope BOOL  SOMLINK cdp_wpModifyPopupMenu(XMMCDPlayer *somSelf,
                                              HWND hwndMenu,
                                              HWND hwndCnr, ULONG iPosition)
{
    BOOL brc = FALSE;
    XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf);
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpModifyPopupMenu");

    brc = XMMCDPlayer_parent_WPAbstract_wpModifyPopupMenu(somSelf,
                                                          hwndMenu,
                                                          hwndCnr,
                                                          iPosition);
    if ((brc) && (_fShowingOpenViewMenu))
    {
       cmnAddCloseMenuItem(hwndMenu);

       _fShowingOpenViewMenu = FALSE;
    }

    return brc;
}

/*
 *@@ wpQueryDefaultView:
 *      this WPObject method returns the default view of an object,
 *      that is, which view is opened if the program file is
 *      double-clicked upon. This is also used to mark
 *      the default view in the "Open" context submenu.
 *
 *      This must be overridden for direct WPAbstract subclasses,
 *      because otherwise double-clicks on the object won't
 *      work.
 */

SOM_Scope ULONG  SOMLINK cdp_wpQueryDefaultView(XMMCDPlayer *somSelf)
{
    /* XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf); */
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpQueryDefaultView");

    return *G_pulVarMenuOfs + ID_XFMI_OFS_XWPVIEW;
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
 */

SOM_Scope HWND  SOMLINK cdp_wpOpen(XMMCDPlayer *somSelf, HWND hwndCnr,
                                   ULONG ulView, ULONG param)
{
    HWND    hwndNewView = 0;
    /* XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf); */
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpOpen");

    if (ulView == *G_pulVarMenuOfs + ID_XFMI_OFS_XWPVIEW)
        hwndNewView = xmmCreateCDPlayerView(somSelf, hwndCnr, ulView);
                                // src/media/mmcdplay.c
    else
        // other view (probably settings):
        hwndNewView = XMMCDPlayer_parent_WPAbstract_wpOpen(somSelf, hwndCnr,
                                                           ulView, param);

    return (hwndNewView);
}

/*
 *@@ wpAddObjectWindowPage:
 *      this WPObject instance method normally adds the
 *      "Standard Options" page to the settings notebook
 *      (that's what the WPS reference calls it; it's actually
 *      the "Window" page).
 */

SOM_Scope ULONG  SOMLINK cdp_wpAddObjectWindowPage(XMMCDPlayer *somSelf,
                                                   HWND hwndNotebook)
{
    /* XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf); */
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpAddObjectWindowPage");

    return SETTINGS_PAGE_REMOVED;
}

/*
 *@@ wpAddSettingsPages:
 *      this WPObject instance method gets called by the WPS
 *      when the Settings view is opened to have all the
 *      settings page inserted into hwndNotebook.
 *
 */

SOM_Scope BOOL  SOMLINK cdp_wpAddSettingsPages(XMMCDPlayer *somSelf,
                                               HWND hwndNotebook)
{
    BOOL brc = FALSE;
    /* XMMCDPlayerData *somThis = XMMCDPlayerGetData(somSelf); */
    XMMCDPlayerMethodDebug("XMMCDPlayer","cdp_wpAddSettingsPages");

    brc = XMMCDPlayer_parent_WPAbstract_wpAddSettingsPages(somSelf,
                                                           hwndNotebook);
    if (brc)
        _xwpAddXMMCDPlayerPages(somSelf, hwndNotebook);

    return brc;
}

/* ******************************************************************
 *
 *   XMMCDPlayer class methods
 *
 ********************************************************************/

/*
 *@@ wpclsInitData:
 *      this WPObject class method gets called when a class
 *      is loaded by the WPS (probably from within a
 *      somFindClass call) and allows the class to initialize
 *      itself.
 */

SOM_Scope void  SOMLINK cdpM_wpclsInitData(M_XMMCDPlayer *somSelf)
{
    /* M_XMMCDPlayerData *somThis = M_XMMCDPlayerGetData(somSelf); */
    M_XMMCDPlayerMethodDebug("M_XMMCDPlayer","cdpM_wpclsInitData");

    M_XMMCDPlayer_parent_M_WPAbstract_wpclsInitData(somSelf);

    krnClassInitialized(G_pcszXMMCDPlayer);
}

/*
 *@@ wpclsQueryStyle:
 *      prevent print, template.
 */

SOM_Scope ULONG  SOMLINK cdpM_wpclsQueryStyle(M_XMMCDPlayer *somSelf)
{
    /* M_XMMCDPlayerData *somThis = M_XMMCDPlayerGetData(somSelf); */
    M_XMMCDPlayerMethodDebug("M_XMMCDPlayer","cdpM_wpclsQueryStyle");

    return (M_XMMCDPlayer_parent_M_WPAbstract_wpclsQueryStyle(somSelf)
                | CLSSTYLE_NEVERPRINT
                | CLSSTYLE_NEVERTEMPLATE);
}

/*
 *@@ wpclsQueryTitle:
 *      this WPObject class method tells the WPS the clear
 *      name of a class, which is shown in the third column
 *      of a Details view and also used as the default title
 *      for new objects of a class.
 */

SOM_Scope PSZ  SOMLINK cdpM_wpclsQueryTitle(M_XMMCDPlayer *somSelf)
{
    // PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();
    /* M_XMMCDPlayerData *somThis = M_XMMCDPlayerGetData(somSelf); */
    M_XMMCDPlayerMethodDebug("M_XMMCDPlayer","cdpM_wpclsQueryTitle");

    return (cmnGetString(ID_XSSI_CDPLAYER)) ; // pszCDPlayer
}

/*
 *@@ wpclsQueryDefaultHelp:
 *      this WPObject class method returns the default help
 *      panel for objects of this class. This gets called
 *      from WPObject::wpQueryDefaultHelp if no instance
 *      help settings (HELPLIBRARY, HELPPANEL) have been
 *      set for an individual object. It is thus recommended
 *      to override this method instead of the instance
 *      method to change the default help panel for a class
 *      in order not to break instance help settings (fixed
 *      with 0.9.20).
 *
 *@@added V0.9.20 (2002-07-12) [umoeller]
 */

SOM_Scope BOOL  SOMLINK cdpM_wpclsQueryDefaultHelp(M_XMMCDPlayer *somSelf,
                                                   PULONG pHelpPanelId,
                                                   PSZ pszHelpLibrary)
{
    /* M_XMMCDPlayerData *somThis = M_XMMCDPlayerGetData(somSelf); */
    M_XMMCDPlayerMethodDebug("M_XMMCDPlayer","cdpM_wpclsQueryDefaultHelp");

    return (M_XMMCDPlayer_parent_M_WPAbstract_wpclsQueryDefaultHelp(somSelf,
                                                                    pHelpPanelId,
                                                                    pszHelpLibrary));
}

/*
 *@@ wpclsQueryIconData:
 *      this WPObject class method must return information
 *      about how to build the default icon for objects
 *      of a class. This gets called from various other
 *      methods whenever a class default icon is needed;
 *      most importantly, M_WPObject::wpclsQueryIcon
 *      calls this to build a class default icon, which
 *      is then cached in the class's instance data.
 *      If a subclass wants to change a class default icon,
 *      it should always override _this_ method instead of
 *      wpclsQueryIcon.
 *
 *      Note that the default WPS implementation does not
 *      allow for specifying the ICON_FILE format here,
 *      which is why we have overridden
 *      M_XFldObject::wpclsQueryIcon too. This allows us
 *      to return icon _files_ for theming too. For details
 *      about the WPS's crappy icon management, refer to
 *      src\filesys\icons.c.
 *
 *      We override this to give the XMMVolume object a new
 *      icon (src\shared\cdplay.ico).
 */

SOM_Scope ULONG  SOMLINK cdpM_wpclsQueryIconData(M_XMMCDPlayer *somSelf,
                                                 PICONINFO pIconInfo)
{
    /* M_XMMCDPlayerData *somThis = M_XMMCDPlayerGetData(somSelf); */
    M_XMMCDPlayerMethodDebug("M_XMMCDPlayer","cdpM_wpclsQueryIconData");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXMMCDPLAY;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return (sizeof(ICONINFO));
}

