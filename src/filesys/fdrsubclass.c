
/*
 *@@sourcefile fdrsubclass.c:
 *      this file is ALL new with V0.9.3 and now implements
 *      folder frame subclassing, which has been largely
 *      redesigned with V0.9.3.
 *
 *      XWorkplace subclasses all WPFolder frame windows to
 *      intercept a large number of messages. This is needed
 *      for the majority of XWorkplace's features, which are
 *      not directly SOM/WPS-related, but are rather straight
 *      PM programming.
 *
 *      Most of the standard WPS methods are really encapsulations
 *      of PM messages. When the WPS opens a folder window, it
 *      creates a container control as a child of the folder frame
 *      window (with the id FID_CLIENT), which is subclassed to
 *      get all the container WM_CONTROL messages. Then, for
 *      example, if the user opens a context menu on an object
 *      in the container, the frame gets WM_CONTROL with
 *      CN_CONTEXTMENU. The WPS then finds the WPS (SOM) object
 *      which corresponds to the container record core on which
 *      the context menu was opened and invokes the WPS menu
 *      methods on it, that is, wpFilterPopupMenu and
 *      wpModifyPopupMenu. Similar things happen with WM_COMMAND
 *      and wpMenuItemSelected.
 *
 *      The trick is just how to get "past" the WPS, which does
 *      a lot of hidden stuff in its method code. By subclassing
 *      the folder frames ourselves, we get all the messages which
 *      are being sent or posted to the folder _before_ the WPS
 *      gets them and can thus modify the WPS's behavior even if
 *      no methods have been defined for a certain event. An
 *      example of this is that the contents of an XFolder status
 *      bar changes when object selections have changed in the
 *      folder: this reacts to WM_CONTROL with CN_EMPHASIS.
 *
 *      Subclassing is done in XFolder::wpOpen after having called
 *      the parent method (WPFolder's), which returns the handle
 *      of the new folder frame which the WPS has created.
 *      XFolder::wpOpen calls fdrManipulateNewView for doing this.
 *
 *      XFolder's subclassed frame window proc is called
 *      fnwpSubclassedFolderFrame. Take a look at it, it's one of
 *      the most interesting parts of XWorkplace. It handles status
 *      bars (which are frame controls), tree view auto-scrolling,
 *      special menu features, the "folder content" menus and more.
 *
 *      This gives us the following hierarchy of window procedures:
 *
 *      1. our fnwpSubclassedFolderFrame, first, followed by
 *
 *      2. the WPS folder frame window subclass, followed by
 *
 *      3. WC_FRAME's default frame window procedure, followed by
 *
 *      4. WinDefWindowProc last.
 *
 *      If additional WPS enhancers are installed (e.g. Object
 *      Desktop), they will appear at the top of the chain also.
 *      I guess it depends on the hierarchy of replacement classes
 *      in the WPS class list which one sits on top.
 *
 *      While we had an ugly global linked list of subclassed
 *      folder views in all XFolder and XWorkplace versions
 *      before V0.9.3, I have finally (after two years...)
 *      found a more elegant way of storing folder data. The
 *      secret is simply re-registing the folder view window
 *      class ("wpFolder window"), which is initially registered
 *      by the WPS somehow. No more global variables and mutex
 *      semaphores...
 *
 *      This hopefully fixes most folder serialization problems
 *      which have occured with previous XFolder/XWorkplace versions.
 *
 *      Since I was unable to find out where WinRegisterClass
 *      gets called from in the WPS, I have implemented a local
 *      send-message hook for PMSHELL.EXE only which re-registers
 *      that window class again when the first WM_CREATE for a
 *      folder view comes in. See fdr_SendMsgHook. Basically,
 *      this adds more bytes for the window words so we can store
 *      a SUBCLASSEDFOLDERVIEW structure for each folder view in
 *      the window words, instead of having to maintain a global
 *      linked list.
 *
 *      When a folder view is subclassed (during XFolder::wpOpen
 *      processing) by fdrSubclassFolderView, a SUBCLASSEDFOLDERVIEW is
 *      created and stored in the new window words. Appears to
 *      work fine so far.
 *
 *      Function prefix for this file:
 *      --  fdr*
 *
 *@@added V0.9.3 [umoeller]
 *@@header "filesys\folder.h"
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
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINMESSAGEMGR
#define INCL_WINFRAMEMGR
#define INCL_WININPUT
#define INCL_WINRECTANGLES
#define INCL_WINSYS             // needed for presparams
#define INCL_WINMENUS
#define INCL_WINTIMER
#define INCL_WINDIALOGS
#define INCL_WINSTATICS
#define INCL_WINBUTTONS
#define INCL_WINENTRYFIELDS
#define INCL_WINLISTBOXES
#define INCL_WINSTDCNR
#define INCL_WINSCROLLBARS
#define INCL_WINSHELLDATA       // Prf* functions
#define INCL_WINHOOKS
#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\cnrh.h"               // container helper routines
#include "helpers\except.h"             // exception handling
#include "helpers\linklist.h"           // linked list helper routines
#include "helpers\standards.h"          // some standard macros
#include "helpers\winh.h"               // PM helper routines

// SOM headers which don't crash with prec. header files
#include "xfldr.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\contentmenus.h"        // shared menu logic
#include "shared\errors.h"              // private XWorkplace error codes
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)

#include "filesys\fileops.h"            // file operations implementation
#include "filesys\folder.h"             // XFolder implementation
#include "filesys\fdrmenus.h"           // shared folder menu logic
#include "filesys\object.h"             // XFldObject implementation
#include "filesys\statbars.h"           // status bar translation logic
#include "filesys\xthreads.h"           // extra XWorkplace threads

// other SOM headers
#pragma hdrstop                         // VAC++ keeps crashing otherwise

// #include <wpdesk.h>                     // WPDesktop
// #include <wpshadow.h>                   // WPShadow

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

// flag for whether we have manipulated the "wpFolder window"
// PM window class already; this is done in fdr_SendMsgHook
static BOOL                G_WPFolderWinClassExtended = FALSE;

static CLASSINFO           G_WPFolderWinClassInfo;

static ULONG               G_SFVOffset = 0;

/* ******************************************************************
 *
 *   Send-message hook
 *
 ********************************************************************/

/*
 *@@ fdr_SendMsgHook:
 *      local send-message hook for PMSHELL.EXE only.
 *      This is installed from M_XFolder::wpclsInitData
 *      and needed to re-register the "wpFolder window"
 *      PM window class which is used by the WPS for
 *      folder views. We add more window words to that
 *      class for storing our window data.
 *
 *@@added V0.9.3 (2000-04-08) [umoeller]
 *@@changed V0.9.16 (2001-12-02) [umoeller]: now releasing hook again
 */

VOID EXPENTRY fdr_SendMsgHook(HAB hab,
                              PSMHSTRUCT psmh,
                              BOOL fInterTask)
{
    /*
     * WM_CREATE:
     *
     */

    CHAR    szClass[300];

    // re-register the WPFolder window class if we haven't
    // done this yet; this is needed because per default,
    // the WPS "wpFolder window" class apparently uses
    // QWL_USER for other purposes...

    if (    (psmh->msg == WM_CREATE)
         && (!G_WPFolderWinClassExtended)
         && (WinQueryClassName(psmh->hwnd,
                               sizeof(szClass),
                               szClass))
         && (!strcmp(szClass, WC_WPFOLDERWINDOW)) // "wpFolder window"))
            // it's a folder:
            // OK, we have the first WM_CREATE for a folder window
            // after Desktop startup now...
         && (WinQueryClassInfo(hab,
                               (PSZ)WC_WPFOLDERWINDOW, // "wpFolder window",
                               &G_WPFolderWinClassInfo))
        )
    {
        // _Pmpf(("    wpFolder cbWindowData: %d", G_WPFolderWinClassInfo.cbWindowData));
        // _Pmpf(("    QWL_USER is: %d", QWL_USER));

        // replace original window class
        if (WinRegisterClass(hab,
                             (PSZ)WC_WPFOLDERWINDOW,
                             G_WPFolderWinClassInfo.pfnWindowProc, // fdr_fnwpSubclassedFolder2,
                             G_WPFolderWinClassInfo.flClassStyle,
                             G_WPFolderWinClassInfo.cbWindowData + 16))
        {
            // _Pmpf(("    WinRegisterClass OK"));

            // OK, window class successfully re-registered:
            // store the offset of our window word for the
            // SUBCLASSEDFOLDERVIEW's in a global variable
            G_SFVOffset = G_WPFolderWinClassInfo.cbWindowData + 12;

            // don't do this again
            G_WPFolderWinClassExtended = TRUE;

            // we can now uninstall the hook, we've done
            // what we had to do...
            // V0.9.16 (2001-12-02) [umoeller]
            if (!WinReleaseHook(WinQueryAnchorBlock(HWND_DESKTOP),
                                HMQ_CURRENT,
                                HK_SENDMSG,
                                (PFN)fdr_SendMsgHook,
                                NULLHANDLE))  // module handle, can be 0 for local hook
                cmnLog(__FILE__, __LINE__, __FUNCTION__,
                       "WinReleaseHook failed.");
        }
        // else _Pmpf(("    WinRegisterClass failed"));
    }
}

/* ******************************************************************
 *
 *   Management of folder frame window subclassing
 *
 ********************************************************************/

/*
 *@@ fdrSubclassFolderView:
 *      creates a SUBCLASSEDFOLDERVIEW for the given folder
 *      view.
 *
 *      We also create a supplementary folder object window
 *      for the view here and store the HWND in the SFV.
 *
 *      This stores the SFV pointer in the frame's window
 *      words at the ulWindowWordOffset position (by calling
 *      WinSetWindowPtr). If ulWindowWordOffset is -1,
 *      this uses a special offset that was determined
 *      internally. This is safe, but ONLY with folder windows
 *      created from XFolder::wpOpen ("true" folder views).
 *
 *      The ulWindowWordOffset param has been added to allow
 *      subclassing container owners other than "true" folder
 *      frames, for example some container which was subclassed
 *      by the WPS because objects have been inserted using
 *      WPObject::wpCnrInsertObject. For example, if you have
 *      a standard frame, specify QWL_USER (0) in those cases.
 *
 *      This no longer actually subclasses the frame because
 *      fdr_fnwpSubclassedFolderFrame requires the
 *      SFV to be at a fixed position. After calling this,
 *      subclass the folder frame yourself.
 *
 *@@added V0.9.3 (2000-04-08) [umoeller]
 *@@changed V0.9.3 (2000-04-08) [umoeller]: no longer using the linked list
 *@@changed V0.9.9 (2001-03-11) [umoeller]: added ulWindowWordOffset param
 *@@changed V0.9.9 (2001-03-11) [umoeller]: no longer subclassing
 *@@changed V0.9.9 (2001-03-11) [umoeller]: renamed from fdrSubclassFolderView
 */

PSUBCLASSEDFOLDERVIEW fdrCreateSFV(HWND hwndFrame,
                                   HWND hwndCnr,
                                   ULONG ulWindowWordOffset,
                                       // in: offset at which to store
                                       // SUBCLASSEDFOLDERVIEW ptr in
                                       // frame's window words, or -1
                                       // for safe default
                                   WPFolder *somSelf,
                                        // in: folder; either XFolder's somSelf
                                        // or XFldDisk's root folder
                                   WPObject *pRealObject)
                                        // in: the "real" object; for XFolder, this is == somSelf,
                                        // for XFldDisk, this is the disk object (needed for object handles)
{
    PSUBCLASSEDFOLDERVIEW psliNew;

    if (psliNew = NEW(SUBCLASSEDFOLDERVIEW))
    {
        ZERO(psliNew);

        if (hwndCnr == NULLHANDLE)
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                       "hwndCnr is NULLHANDLE for folder %s.",
                      _wpQueryTitle(somSelf));

        // store various other data here
        psliNew->hwndFrame = hwndFrame;
        psliNew->somSelf = somSelf;
        psliNew->pRealObject = pRealObject;
        psliNew->hwndCnr = hwndCnr;
        psliNew->fRemoveSourceEmphasis = FALSE;
        // set status bar hwnd to zero at this point;
        // this will be created elsewhere
        psliNew->hwndStatusBar = NULLHANDLE;

        // create a supplementary object window
        // for this folder frame (see
        // fdr_fnwpSupplFolderObject for details)
        if (psliNew->hwndSupplObject = winhCreateObjectWindow(WNDCLASS_SUPPLOBJECT,
                                                              psliNew))
        {
            psliNew->ulWindowWordOffset
                    = (ulWindowWordOffset == -1)
                         ? G_SFVOffset        // window word offset which we've
                                          // calculated in fdr_SendMsgHook
                         : ulWindowWordOffset, // changed V0.9.9 (2001-03-11) [umoeller]

            // store SFV in frame's window words
            WinSetWindowPtr(hwndFrame,
                            psliNew->ulWindowWordOffset,
                            psliNew);
        }
        else
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "Unable to create suppl. folder object window.");
    }

    return (psliNew);
}

/*
 *@@ fdrSubclassFolderView:
 *      calls fdrCreateSFV and subclasses the folder
 *      frame.
 *
 *      This gets called for standard XFldDisk and
 *      XFolder frames.
 *
 *@@added V0.9.9 (2001-03-11) [umoeller]
 */

PSUBCLASSEDFOLDERVIEW fdrSubclassFolderView(HWND hwndFrame,
                                            HWND hwndCnr,
                                            WPFolder *somSelf,
                                                 // in: folder; either XFolder's somSelf
                                                 // or XFldDisk's root folder
                                            WPObject *pRealObject)
                                                 // in: the "real" object; for XFolder, this is == somSelf,
                                                 // for XFldDisk, this is the disk object (needed for object handles)
{
    PSUBCLASSEDFOLDERVIEW psfv;
    if (psfv = fdrCreateSFV(hwndFrame,
                            hwndCnr,
                            -1,    // default window word V0.9.9 (2001-03-11) [umoeller]
                            somSelf,
                            pRealObject))
    {
        psfv->pfnwpOriginal = WinSubclassWindow(hwndFrame,
                                                fdr_fnwpSubclassedFolderFrame);
    }

    return (psfv);
}

/*
 *@@ fdrQuerySFV:
 *      this retrieves the PSUBCLASSEDFOLDERVIEW from the
 *      specified subclassed folder frame. One of these
 *      structs is maintained for each open folder view
 *      to store window data which is needed everywhere.
 *
 *      Works only for "true" folder frames created by
 *      XFolder::wpOpen.
 *
 *      Returns NULL if not found.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist functions
 *@@changed V0.9.0 [umoeller]: pulIndex added to function prototype
 *@@changed V0.9.0 [umoeller]: moved this func here from common.c
 *@@changed V0.9.1 (2000-02-14) [umoeller]: reversed order of functions; now subclassing is last
 *@@changed V0.9.3 (2000-04-08) [umoeller]: completely replaced
 */

PSUBCLASSEDFOLDERVIEW fdrQuerySFV(HWND hwndFrame,        // in: folder frame to find
                                  PULONG pulIndex)       // out: index in linked list if found
{
    return ((PSUBCLASSEDFOLDERVIEW)WinQueryWindowPtr(hwndFrame,
                                                     G_SFVOffset));
}

/*
 *@@ fdrRemoveSFV:
 *      reverse to fdrSubclassFolderView, this removes
 *      a PSUBCLASSEDFOLDERVIEW from the folder frame again.
 *      Called upon WM_DESTROY in folder frames.
 *
 *@@changed V0.9.0 [umoeller]: adjusted for new linklist functions
 *@@changed V0.9.0 [umoeller]: moved this func here from common.c
 *@@changed V0.9.3 (2000-04-08) [umoeller]: completely replaced
 */

VOID fdrRemoveSFV(PSUBCLASSEDFOLDERVIEW psfv)
{
    WinSetWindowPtr(psfv->hwndFrame,
                    psfv->ulWindowWordOffset, // V0.9.9 (2001-03-11) [umoeller]
                    NULL);
    free(psfv);
}

/*
 *@@ fdrManipulateNewView:
 *      this gets called from XFolder::wpOpen
 *      after a new Icon, Tree, or Details view
 *      has been successfully opened by the parent
 *      method (WPFolder::wpOpen).
 *
 *      This manipulates the view according to
 *      our needs (subclassing, sorting, full
 *      status bar, path in title etc.).
 *
 *      This is ONLY used for folders, not for
 *      WPDisk's. This calls fdrSubclassFolderView in turn.
 *
 *@@added V0.9.0 (99-11-27) [umoeller]
 *@@changed V0.9.1 (2000-02-08) [umoeller]: status bars were added even if globally disabled; fixed.
 *@@changed V0.9.3 (2000-04-08) [umoeller]: adjusted for new folder frame subclassing
 */

VOID fdrManipulateNewView(WPFolder *somSelf,        // in: folder with new view
                          HWND hwndNewFrame,        // in: new view (frame) of folder
                          ULONG ulView)             // in: OPEN_CONTENTS, OPEN_TREE, or OPEN_DETAILS
{
    PSUBCLASSEDFOLDERVIEW psfv = 0;
    // PCGLOBALSETTINGS    pGlobalSettings = cmnQueryGlobalSettings();
    XFolderData         *somThis = XFolderGetData(somSelf);
    HWND                hwndCnr = wpshQueryCnrFromFrame(hwndNewFrame);

#ifndef __ALWAYSSUBCLASS__
    if (!cmnQuerySetting(sfNoSubclassing)) // V0.9.3 (2000-04-26) [umoeller]
#endif
    {
        ULONG flViews;

        // subclass the new folder frame window;
        // this creates a SUBCLASSEDFOLDERVIEW for the view
        psfv = fdrSubclassFolderView(hwndNewFrame,
                                     hwndCnr,
                                     somSelf,
                                     somSelf);  // "real" object; for folders, this is the folder too

        // change the window title to full path, if allowed
        if (    (_bFullPathInstance == 1)
             || ((_bFullPathInstance == 2) && (cmnQuerySetting(sfFullPath)))
           )
            fdrSetOneFrameWndTitle(somSelf, hwndNewFrame);

        // add status bar, if allowed:
            // 1) status bar only if allowed for the current folder
        if (
#ifndef __NOCFGSTATUSBARS__
                (cmnQuerySetting(sfStatusBars))
             &&
#endif
                (    (_bStatusBarInstance == STATUSBAR_ON)
                  || (   (_bStatusBarInstance == STATUSBAR_DEFAULT)
                      && (cmnQuerySetting(sfDefaultStatusBarVisibility))
                     )
                )
             // 2) no status bar for active Desktop
             && (somSelf != cmnQueryActiveDesktop())
             // 3) check that subclassed list item is valid
             && (psfv)
             // 4) status bar only if allowed for the current view type
             && (flViews = cmnQuerySetting(sflSBForViews))
             && (
                    (   (ulView == OPEN_CONTENTS)
                     && (flViews & SBV_ICON)
                    )
                 || (   (ulView == OPEN_TREE)
                     && (flViews & SBV_TREE)
                    )
                 || (   (ulView == OPEN_DETAILS)
                     && (flViews & SBV_DETAILS)
                    )
                )
           )
        {
            fdrCreateStatusBar(somSelf, psfv, TRUE);
        }

        // replace sort stuff
#ifndef __ALWAYSEXTSORT__
        if (cmnQuerySetting(sfExtendedSorting))
#endif
            if (hwndCnr)
            {
                #ifdef DEBUG_SORT
                    _Pmpf((__FUNCTION__ ": setting folder sort"));
                #endif
                fdrSetFldrCnrSort(somSelf,
                                  hwndCnr,
                                  TRUE);        // force
            }
    }
}

/* ******************************************************************
 *
 *   New subclassed folder frame message processing
 *
 ********************************************************************/

/*
 * FormatFrame:
 *      this gets called from fdr_fnwpSubclassedFolderFrame
 *      when WM_FORMATFRAME is received. This implements
 *      folder status bars.
 *
 *@@changed V0.9.0 [umoeller]: moved this func here from xfldr.c
 *@@changed V0.9.18 (2002-03-24) [umoeller]: fixed stupid scroll bars when always sort is off
 */

static VOID FormatFrame(PSUBCLASSEDFOLDERVIEW psfv, // in: frame information
                        MPARAM mp1,            // in: mp1 from WM_FORMATFRAME (points to SWP array)
                        ULONG ulCount)         // in: frame control count (returned from default wnd proc)
{
    // access the SWP array that is passed to us
    // and search all the controls for the container child window,
    // which for folders always has the ID 0x8008
    ULONG       ul;
    PSWP        swpArr = (PSWP)mp1;
    CNRINFO     CnrInfo;

    for (ul = 0; ul < ulCount; ul++)
    {
        HWND        hwndThis = swpArr[ul].hwnd;
        if (WinQueryWindowUShort(hwndThis, QWS_ID ) == 0x8008)
                                                         // FID_CLIENT
        {
            // container found: reduce size of container by
            // status bar height
            POINTL      ptlBorderSizes;
            ULONG       ulStatusBarHeight = cmnQueryStatusBarHeight();

            WinSendMsg(psfv->hwndFrame,
                       WM_QUERYBORDERSIZE,
                       (MPARAM)&ptlBorderSizes,
                       0);

            // first initialize the _new_ SWP for the status bar.
            // Since the SWP array for the std frame controls is
            // zero-based, and the standard frame controls occupy
            // indices 0 thru ulCount-1 (where ulCount is the total
            // count), we use ulCount for our static text control.
            swpArr[ulCount].fl = SWP_MOVE | SWP_SIZE | SWP_NOADJUST | SWP_ZORDER;
            swpArr[ulCount].x  = ptlBorderSizes.x;
            swpArr[ulCount].y  = ptlBorderSizes.y;
            swpArr[ulCount].cx = swpArr[ul].cx;  // same as cnr's width
            swpArr[ulCount].cy = ulStatusBarHeight;
            swpArr[ulCount].hwndInsertBehind = HWND_BOTTOM; // HWND_TOP;
            swpArr[ulCount].hwnd = psfv->hwndStatusBar;

            // adjust the origin and height of the container to
            // accomodate our static text control
            swpArr[ul].y  += swpArr[ulCount].cy;
            swpArr[ul].cy -= swpArr[ulCount].cy;

            // now we need to adjust the workspace origin of the cnr
            // accordingly, or otherwise the folder icons will appear
            // outside the visible cnr workspace and scroll bars will
            // show up.
            // We only do this the first time we're arriving here
            // (which should be before the WPS is populating the folder);
            // psfv->fNeedCnrScroll has been initially set to TRUE
            // by fdrCreateStatusBar.
            #ifdef DEBUG_STATUSBARS
            {
                _Pmpf((__FUNCTION__ ": psfv->fNeedCnrScroll: %d", psfv->fNeedCnrScroll));
                cnrhQueryCnrInfo(hwndThis, &CnrInfo);

                #ifdef DEBUG_STATUSBARS
                    _Pmpf(( "Old CnrInfo.ptlOrigin.y: %lX", CnrInfo.ptlOrigin.y ));
                #endif
            }
            #endif

            if (psfv->fNeedCnrScroll)
            {
                cnrhQueryCnrInfo(hwndThis, &CnrInfo);

                if ((LONG)CnrInfo.ptlOrigin.y >= (LONG)ulStatusBarHeight)
                {
                    RECTL rclViewport;

                    CnrInfo.ptlOrigin.y -= ulStatusBarHeight;

                    #ifdef DEBUG_STATUSBARS
                        _Pmpf(( "New CnrInfo.ptlOrigin.y: %lX", CnrInfo.ptlOrigin.y ));
                    #endif

                    WinSendMsg(hwndThis,
                               CM_SETCNRINFO,
                               (MPARAM)&CnrInfo,
                               (MPARAM)CMA_PTLORIGIN);
                }

                // now scroll the damn container up the maximum;
                // we still get scroll bars in some situations if
                // always sort is off...
                // to scroll the container up _and_ get rid of
                // the scroll bars, we first post HOME to the
                // container's vertical scroll bar and _then_
                // another PAGEUP to the container itself
                // V0.9.18 (2002-03-24) [umoeller]
                WinPostMsg(WinWindowFromID(hwndThis, 0x7FF9),
                           WM_CHAR,
                           MPFROM2SHORT(KC_VIRTUALKEY | KC_CTRL,
                                        0),
                           MPFROM2SHORT(0,
                                        VK_HOME));
                WinPostMsg(hwndThis,
                           WM_CHAR,
                           MPFROM2SHORT(KC_VIRTUALKEY,
                                        0),
                           MPFROM2SHORT(0,
                                        VK_PAGEUP));

                // set flag to FALSE to prevent a second adjustment
                psfv->fNeedCnrScroll = FALSE;
            } // end if (psfv->fNeedCnrScroll)

            break;  // we're done
        } // end if WinQueryWindowUShort
    } // end for (ul = 0; ul < ulCount; ul++)
}

/*
 * CalcFrameRect:
 *      this gets called from fdr_fnwpSubclassedFolderFrame
 *      when WM_CALCFRAMERECT is received. This implements
 *      folder status bars.
 *
 *@@changed V0.9.0 [umoeller]: moved this func here from xfldr.c
 */

static VOID CalcFrameRect(MPARAM mp1, MPARAM mp2)
{
    PRECTL prclPassed = (PRECTL)mp1;
    ULONG ulStatusBarHeight = cmnQueryStatusBarHeight();

    if (SHORT1FROMMP(mp2))
        //     TRUE:  Frame rectangle provided, calculate client
        //     FALSE: Client area rectangle provided, calculate frame
    {
        //  TRUE: calculate the rectl of the client;
        //  call default window procedure to subtract child frame
        //  controls from the rectangle's height
        LONG lClientHeight;

        //  position the static text frame extension below the client
        lClientHeight = prclPassed->yTop - prclPassed->yBottom;
        if ( ulStatusBarHeight  > lClientHeight  )
        {
            // extension is taller than client, so set client height to 0
            prclPassed->yTop = prclPassed->yBottom;
        }
        else
        {
            //  set the origin of the client and shrink it based upon the
            //  static text control's height
            prclPassed->yBottom += ulStatusBarHeight;
            prclPassed->yTop -= ulStatusBarHeight;
        }
    }
    else
    {
        //  FALSE: calculate the rectl of the frame;
        //  call default window procedure to subtract child frame
        //  controls from the rectangle's height;
        //  set the origin of the frame and increase it based upon the
        //  static text control's height
        prclPassed->yBottom -= ulStatusBarHeight;
        prclPassed->yTop += ulStatusBarHeight;
    }
}

/*
 * InitMenu:
 *      this gets called from fdr_fnwpSubclassedFolderFrame
 *      when WM_INITMENU is received, _after_ the parent
 *      window proc has been given a chance to process this.
 *
 *      WM_INITMENU is sent to a menu owner right before a
 *      menu is about to be displayed. This applies to both
 *      pulldown and context menus.
 *
 *      This is needed for various menu features:
 *
 *      1)  for getting the object which currently has source
 *          emphasis in the folder container, because at the
 *          point WM_COMMAND comes in (and thus wpMenuItemSelected
 *          gets called in response), source emphasis has already
 *          been removed by the WPS.
 *
 *          This is needed for file operations on all selected
 *          objects in the container. We call wpshQuerySourceObject
 *          to find out more about this and store the result in
 *          our SUBCLASSEDFOLDERVIEW.
 *
 *      2)  for folder content menus, because these are
 *          inserted as empty stubs only in wpModifyPopupMenu
 *          and only filled after the user clicks on them.
 *          We will query a bunch of data first, which we need
 *          later for drawing our items, and then call
 *          mnuFillContentSubmenu, which populates the folder
 *          and fills the menu with the items therein.
 *
 *      3)  for the menu system sounds.
 *
 *      4)  for manipulating Warp 4 folder menu _bars_. We
 *          cannot use the Warp 4 WPS methods defined for
 *          that because we want XWorkplace to run on Warp 3
 *          also.
 *
 *      WM_INITMENU parameters:
 *          SHORT mp1   menu item id
 *          HWND  mp2   menu window handle
 *      Returns: NULL always.
 *
 *@@changed V0.9.0 [umoeller]: moved this func here from xfldr.c
 *@@changed V0.9.4 (2000-07-15) [umoeller]: fixed source object confusion in WM_INITMENU
 *@@changed V0.9.12 (2001-05-29) [umoeller]: fixed broken source object with folder menu bars, which broke new "View" menu items
 */

static VOID InitMenu(PSUBCLASSEDFOLDERVIEW psfv, // in: frame information
                     ULONG sMenuIDMsg,         // in: mp1 from WM_INITMENU
                     HWND hwndMenuMsg)         // in: mp2 from WM_INITMENU
{
    // PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

    // get XFolder instance data
    XFolderData     *somThis = XFolderGetData(psfv->somSelf);

    #ifdef DEBUG_MENUS
        _Pmpf(( "WM_INITMENU: sMenuIDMsg = %lX, hwndMenuMsg = %lX",
                (ULONG)sMenuIDMsg,
                hwndMenuMsg ));
        _Pmpf(( "  psfv->hwndCnr: 0x%lX", psfv->hwndCnr));
    #endif

    // store object with source emphasis for later use
    // (this gets lost before WM_COMMAND otherwise),
    // but only if the MAIN menu is being opened
    if (sMenuIDMsg == 0x8020) // main menu ID V0.9.4 (2000-07-15) [umoeller]
    {
        // the WPS has a bug in that source emphasis is removed
        // when going thru several context menus, so we must make
        // sure that we do this only when the main menu is opened
        psfv->pSourceObject = wpshQuerySourceObject(psfv->somSelf,
                                                    psfv->hwndCnr,
                                                    FALSE,      // menu mode
                                                    &psfv->ulSelection);
    }

    // store the container window handle in instance
    // data for wpModifyPopupMenu workaround;
    // buggy Warp 3 keeps setting hwndCnr to NULLHANDLE in there
    _hwndCnrSaved = psfv->hwndCnr;

    // play system sound
#ifndef __NOXSYSTEMSOUNDS__
    if (    (sMenuIDMsg < 0x8000) // avoid system menu
         || (sMenuIDMsg == 0x8020) // but include context menu
       )
        cmnPlaySystemSound(MMSOUND_XFLD_CTXTOPEN);
#endif

    // find out whether the menu of which we are notified
    // is a folder content menu; if so (and it is not filled
    // yet), the first menu item is ID_XFMI_OFS_DUMMY
    if ((ULONG)WinSendMsg(hwndMenuMsg,
                          MM_ITEMIDFROMPOSITION,
                          (MPARAM)0,        // menu item index
                          MPNULL)
               == (cmnQuerySetting(sulVarMenuOffset) + ID_XFMI_OFS_DUMMY))
    {
        // okay, let's go
#ifndef __NOFOLDERCONTENTS__
        if (cmnQuerySetting(sfFolderContentShowIcons))
#endif
        {
            // show folder content icons ON:

            #ifdef DEBUG_MENUS
                _Pmpf(( "  preparing owner draw"));
            #endif

            cmnuPrepareOwnerDraw(hwndMenuMsg);
        }

        // add menu items according to folder contents
        cmnuFillContentSubmenu(sMenuIDMsg, hwndMenuMsg);
    }
    else
    {
        // no folder content menu:

        // on Warp 4, check if the folder has a menu bar
        if (doshIsWarp4())
        {
            #ifdef DEBUG_MENUS
                _Pmpf(( "  checking for menu bar"));
            #endif

            if (sMenuIDMsg == 0x8005)
            {
                // seems to be some WPS menu item;
                // since the WPS seems to be using this
                // same ID for all the menu bar submenus,
                // we need to check the last selected
                // menu item, which was stored in the psfv
                // structure by WM_MENUSELECT (below).
                // PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();

                switch (psfv->ulLastSelMenuItem)
                {
                    case 0x2D0: // "Edit" submenu
                    {
                        // find position of "Deselect all" item
                        SHORT sPos = (SHORT)WinSendMsg(hwndMenuMsg,
                                                       MM_ITEMPOSITIONFROMID,
                                                       MPFROM2SHORT(0x73,
                                                                    FALSE),
                                                       MPNULL);
                        #ifdef DEBUG_MENUS
                            _Pmpf(("  'Edit' menu found"));
                        #endif

                        // set the "source" object for menu item
                        // selections to the folder
                        psfv->pSourceObject = psfv->somSelf;
                                // V0.9.12 (2001-05-29) [umoeller]

                        // insert "Select by name" after that item
                        winhInsertMenuItem(hwndMenuMsg,
                                           sPos+1,
                                           (cmnQuerySetting(sulVarMenuOffset)
                                                   + ID_XFMI_OFS_SELECTSOME),
                                           cmnGetString(ID_XSSI_SELECTSOME),  // pszSelectSome
                                           MIS_TEXT, 0);
                    }
                    break;

                    case 0x2D1: // "View" submenu
                    {
                        CNRINFO             CnrInfo;
                        #ifdef DEBUG_MENUS
                            _Pmpf(("  'View' menu found"));
                        #endif

                        // set the "source" object for menu item
                        // selections to the folder
                        psfv->pSourceObject = psfv->somSelf;
                                // V0.9.12 (2001-05-29) [umoeller]

                        // modify the "Sort" menu, as we would
                        // do it for context menus also
                        fdrModifySortMenu(psfv->somSelf,
                                          hwndMenuMsg);

                        cnrhQueryCnrInfo(psfv->hwndCnr, &CnrInfo);
                        // and now insert the "folder view" items
                        winhInsertMenuSeparator(hwndMenuMsg,
                                                MIT_END,
                                                (cmnQuerySetting(sulVarMenuOffset)
                                                        + ID_XFMI_OFS_SEPARATOR));
                        mnuInsertFldrViewItems(psfv->somSelf,
                                               hwndMenuMsg,  // hwndViewSubmenu
                                               FALSE,
                                               psfv->hwndCnr,
                                               wpshQueryView(psfv->somSelf,
                                                             psfv->hwndFrame));
                    }
                    break;

                    /* case 0x2D2:     // "Selected" submenu:
                    break; */

                    case 0x2D3: // "Help" submenu: add XFolder product info
                        #ifdef DEBUG_MENUS
                            _Pmpf(("  'Help' menu found"));
                        #endif

                        // set the "source" object for menu item
                        // selections to the folder
                        psfv->pSourceObject = psfv->somSelf;
                                // V0.9.12 (2001-05-29) [umoeller]

#ifndef __XWPLITE__
                        winhInsertMenuSeparator(hwndMenuMsg, MIT_END,
                                               (cmnQuerySetting(sulVarMenuOffset)
                                                       + ID_XFMI_OFS_SEPARATOR));
                        winhInsertMenuItem(hwndMenuMsg, MIT_END,
                                           (cmnQuerySetting(sulVarMenuOffset)
                                                   + ID_XFMI_OFS_PRODINFO),
                                           cmnGetString(ID_XSSI_PRODUCTINFO),  // pszProductInfo
                                           MIS_TEXT, 0);
#endif

                    break;

                } // end switch (psfv->usLastSelMenuItem)
            } // end if (SHORT1FROMMP(mp1) == 0x8005)
        } // end if (doshIsWarp4())
    }
}

/*
 * MenuSelect:
 *      this gets called from fdr_fnwpSubclassedFolderFrame
 *      when WM_MENUSELECT is received.
 *      We need this for three reasons:
 *
 *      1) we will play a system sound, if desired;
 *
 *      2) we need to swallow this for very large folder
 *         content menus, because for some reason, PM will
 *         select a random menu item after we have repositioned
 *         a menu window on the screen (duh);
 *
 *      3) we can intercept certain menu items so that
 *         these don't get passed to wpMenuItemSelected,
 *         which appears to get called when the WPS folder window
 *         procedure responds to WM_COMMAND (which comes after
 *         WM_MENUSELECT only).
 *         This is needed for menu items such as those in
 *         the "Sort" menu so that the menu is not dismissed
 *         after selection.
 *
 *      WM_MENUSELECT parameters:
 *      --  mp1 -- USHORT usItem - selected menu item
 *              -- USHORT usPostCommand - TRUE: if we return TRUE,
 *                  a message will be posted to the owner.
 *      --  mp2 HWND - menu control wnd handle
 *
 *      If we set pfDismiss to TRUE, wpMenuItemSelected will be
 *      called, and the menu will be dismissed.
 *      Otherwise the message will be swallowed.
 *      We return TRUE if the menu item has been handled here.
 *      Otherwise the default wnd proc will be used.
 *
 *@@changed V0.9.0 [umoeller]: moved this func here from xfldr.c
 */

static BOOL MenuSelect(PSUBCLASSEDFOLDERVIEW psfv, // in: frame information
                       MPARAM mp1,               // in: mp1 from WM_MENUSELECT
                       MPARAM mp2,               // in: mp2 from WM_MENUSELECT
                       BOOL *pfDismiss)          // out: dismissal flag
{
    BOOL fHandled = FALSE;
    // return value for WM_MENUSELECT;
    // TRUE means dismiss menu

    USHORT      usItem = SHORT1FROMMP(mp1),
                usPostCommand = SHORT2FROMMP(mp1);

    psfv->ulLastSelMenuItem = usItem;

    // _Pmpf((__FUNCTION__ ": usPostCommand = 0x%lX", usPostCommand));

    if (    (usPostCommand)
        && (    (usItem <  0x8000) // avoid system menu
             || (usItem == 0x8020) // include context menu
           )
       )
    {
        HWND hwndCnr = wpshQueryCnrFromFrame(psfv->hwndFrame);

#ifndef __NOXSYSTEMSOUNDS__
        // play system sound
        cmnPlaySystemSound(MMSOUND_XFLD_CTXTSELECT);
#endif

        // now check if we have a menu item which we don't
        // want to see dismissed

        if (hwndCnr)
        {
            // first find out what kind of objects we have here
            WPObject *pObject = psfv->pSourceObject;
                                // set with WM_INITMENU

            #ifdef DEBUG_MENUS
                _Pmpf(( "  Object selections: %d", ulSelection));
            #endif

            // now call the functions in fdrmenus.c for this,
            // depending on the class of the object for which
            // the menu was opened
            if (pObject = objResolveIfShadow(pObject))
            {
                if (_somIsA(pObject, _WPFileSystem))
                {
                    fHandled = mnuFileSystemSelectingMenuItem(
                                   psfv->pSourceObject,
                                        // set in WM_INITMENU;
                                        // note that we're passing
                                        // psfv->pSourceObject instead of pObject;
                                        // psfv->pSourceObject might be a shadow!
                                   usItem,
                                   (BOOL)usPostCommand,
                                   (HWND)mp2,               // hwndMenu
                                   hwndCnr,
                                   psfv->ulSelection,       // SEL_* flags
                                   pfDismiss);              // dismiss-menu flag

                    if (    (!fHandled)
                         && (_somIsA(pObject, _WPFolder))
                       )
                    {
                        fHandled = mnuFolderSelectingMenuItem(pObject,
                                       usItem,
                                       (BOOL)usPostCommand, // fPostCommand
                                       (HWND)mp2,               // hwndMenu
                                       hwndCnr,
                                       psfv->ulSelection,       // SEL_* flags
                                       pfDismiss);              // dismiss-menu flag
                    }
                }

                if (    (fHandled)
                     && (!(*pfDismiss))
                   )
                {
                    // menu not to be dismissed: set the flag
                    // which will remove cnr source
                    // emphasis when the main menu is dismissed
                    // later (WM_ENDMENU msg here)
                    psfv->fRemoveSourceEmphasis = TRUE;
                }
            }
        }
    }

    return (fHandled);
}

/*
 *@@ WMChar_Delete:
 *      this gets called if "delete into trash can"
 *      is enabled and WM_CHAR has been detected with
 *      the "Delete" key. We start a file task job
 *      to delete all selected objects in the container
 *      into the trash can, using the oh-so-much advanced
 *      functions in fileops.c.
 *
 *@@added V0.9.1 (2000-01-31) [umoeller]
 *@@changed V0.9.9 (2001-02-16) [umoeller]: added "shift-delete" support; thanks [pr]
 *@@changed V0.9.19 (2002-04-02) [umoeller]: fixed broken true delete if trashcan is disabled
 */

static VOID WMChar_Delete(PSUBCLASSEDFOLDERVIEW psfv,
                          BOOL fTrueDelete)             // in: do true delete instead of trash?
{
    ULONG       ulSelection = 0;
    WPObject    *pSelected = 0;

    pSelected = wpshQuerySourceObject(psfv->somSelf,
                                      psfv->hwndCnr,
                                      TRUE,       // keyboard mode
                                      &ulSelection);
    #ifdef DEBUG_TRASHCAN
        _Pmpf(("WM_CHAR delete: first obj is %s",
                (pSelected) ? _wpQueryTitle(pSelected) : "NULL"));
    #endif

    if (    (pSelected)
         && (ulSelection != SEL_NONEATALL)
       )
    {
        // collect objects from cnr and start
        // moving them to trash can
        FOPSRET frc = fopsStartDeleteFromCnr(NULLHANDLE,   // no anchor block, ansynchronously
                                             pSelected,    // first selected object
                                             ulSelection,  // can only be SEL_SINGLESEL
                                                            // or SEL_MULTISEL
                                             psfv->hwndCnr,
                                             fTrueDelete);  // V0.9.19 (2002-04-02) [umoeller]
        #ifdef DEBUG_TRASHCAN
            _Pmpf(("    got FOPSRET %d", frc));
        #endif
    }
}

/*
 *@@ WMChar:
 *      handler for WM_CHAR in fdrProcessFolderMsgs.
 *
 *      Returns TRUE if msg was processed and should
 *      be swallowed.
 *
 *@@added V0.9.18 (2002-03-23) [umoeller]
 *@@changed V0.9.19 (2002-04-02) [umoeller]: fixed broken true delete if trashcan is disabled
 */

static BOOL WMChar(HWND hwndFrame,
                   PSUBCLASSEDFOLDERVIEW psfv,
                   MPARAM mp1,
                   MPARAM mp2)
{
    USHORT usFlags    = SHORT1FROMMP(mp1);
    // whatever happens, we're only interested
    // in key-down messages
    if ((usFlags & KC_KEYUP) == 0)
    {
        XFolderData         *somThis = XFolderGetData(psfv->somSelf);

        USHORT usch       = SHORT1FROMMP(mp2);
        USHORT usvk       = SHORT2FROMMP(mp2);

        // intercept DEL key
        if (    (usFlags & KC_VIRTUALKEY)
             && (usvk == VK_DELETE)
           )
        {
            // check whether "delete to trash can" is on
#ifndef __ALWAYSTRASHANDTRUEDELETE__
            if (cmnQuerySetting(sfReplaceDelete))       // V0.9.19 (2001-04-13) [umoeller]
#endif
            {
                BOOL fTrueDelete;

                // use true delete if the user doesn't want the
                // trash can or if the shift key is pressed
                if (!(fTrueDelete = cmnQuerySetting(sfAlwaysTrueDelete)))
                    fTrueDelete = doshQueryShiftState();
                WMChar_Delete(psfv,
                              fTrueDelete);

                // swallow this key,
                // do not process default winproc
                return (TRUE);
            }
        }

        // check whether folder hotkeys are allowed at all
        if (
#ifndef __ALWAYSFDRHOTKEYS__
                (cmnQuerySetting(sfFolderHotkeys))
             &&
#endif
                // yes: check folder and global settings
                (    (_bFolderHotkeysInstance == 1)
                  || (    (_bFolderHotkeysInstance == 2)   // use global settings:
                       && (cmnQuerySetting(sfFolderHotkeysDefault))
                     )
                )
           )
        {
            if (fdrProcessFldrHotkey(psfv->somSelf,
                                     hwndFrame,
                                     usFlags,
                                     usch,
                                     usvk))
            {
                // was a hotkey:
                // swallow this key,
                // do not process default winproc
                return (TRUE);
            }
        }
    }

    return (FALSE);
}

/*
 *@@ fdrProcessObjectCommand:
 *      implementation for XFolder::xwpProcessObjectCommand.
 *      See remarks there.
 *
 *      Yes, it looks strange that ProcessFolderMsgs calls
 *      xwpProcessObjectCommand, which in turn calls this
 *      function... but this gives folder subclasses such
 *      as the trash can a chance to override that method
 *      to implement their own processing.
 *
 *@@added V0.9.7 (2001-01-13) [umoeller]
 *@@changed V0.9.9 (2001-02-18) [pr]: fix delete folder from menu bar
 */

BOOL fdrProcessObjectCommand(WPFolder *somSelf,
                             USHORT usCommand,
                             HWND hwndCnr,
                             WPObject* pFirstObject,
                             ULONG ulSelectionFlags)
{
    BOOL brc = FALSE;       // default: not processed, call parent

    if (usCommand == WPMENUID_DELETE)
    {
#ifndef __ALWAYSTRASHANDTRUEDELETE__
        if (cmnQuerySetting(sfReplaceDelete))
#endif
        {
            FOPSRET frc;
            BOOL fTrueDelete;

            // use true delete if the user doesn't want the
            // trash can or if the shift key is pressed
            if (!(fTrueDelete = cmnQuerySetting(sfAlwaysTrueDelete)))
                fTrueDelete = doshQueryShiftState();

            // need this to handle deleting folder from menu bar as
            // there is no source emphasis
            if (!pFirstObject && !ulSelectionFlags)
            {
                pFirstObject = somSelf;
                ulSelectionFlags = SEL_WHITESPACE;
            }

            // collect objects from container and start deleting
            frc = fopsStartDeleteFromCnr(NULLHANDLE,
                                            // no anchor block,
                                            // ansynchronously
                                         pFirstObject,
                                            // first source object
                                         ulSelectionFlags,
                                         hwndCnr,
                                         fTrueDelete);
            #ifdef DEBUG_TRASHCAN
                _Pmpf(("WM_COMMAND WPMENUID_DELETE: got FOPSRET %d", frc));
            #endif

            // return "processed", skip default processing
            brc = TRUE;
        }
    } // end if (usCommand == WPMENUID_DELETE)

    return (brc);
}

/*
 *@@ ProcessFolderMsgs:
 *      actual folder view message processing. Called
 *      from fdr_fnwpSubclassedFolderFrame. See remarks
 *      there.
 *
 *@@added V0.9.3 (2000-04-08) [umoeller]
 *@@changed V0.9.7 (2001-01-13) [umoeller]: introduced xwpProcessObjectCommand for WM_COMMAND
 *@@changed V0.9.9 (2001-03-11) [umoeller]: renamed from ProcessFolderMsgs, exported now
 */

MRESULT fdrProcessFolderMsgs(HWND hwndFrame,
                             ULONG msg,
                             MPARAM mp1,
                             MPARAM mp2,
                             PSUBCLASSEDFOLDERVIEW psfv,  // in: folder view data
                             PFNWP pfnwpOriginal)       // in: original frame window proc
{
    MRESULT         mrc = 0;
    BOOL            fCallDefault = FALSE;

    TRY_LOUD(excpt1)
    {
        switch(msg)
        {
            /* *************************
             *                         *
             * Status bar:             *
             *                         *
             **************************/

            /*
             *  The following code adds status bars to folder frames.
             *  The XFolder status bars are implemented as frame controls
             *  (similar to the title-bar buttons and menus). In order
             *  to do this, we need to intercept the following messages
             *  which are sent to the folder's frame window when the
             *  frame is updated as a reaction to WM_UPDATEFRAME or WM_SIZE.
             *
             *  Note that wpOpen has created the status bar window (which
             *  is a static control subclassed with fdr_fnwpStatusBar) already
             *  and stored the HWND in the SUBCLASSEDFOLDERVIEW.hwndStatusBar
             *  structure member (which psfv points to now).
             *
             *  Here we only relate the status bar to the frame. The actual
             *  painting etc. is done in fdr_fnwpStatusBar.
             */

            /*
             * WM_QUERYFRAMECTLCOUNT:
             *      this gets sent to the frame when PM wants to find out
             *      how many frame controls the frame has. According to what
             *      we return here, SWP structures are allocated for WM_FORMATFRAME.
             *      We call the "parent" window proc and add one for the status bar.
             */

            case WM_QUERYFRAMECTLCOUNT:
            {
                // query the standard frame controls count
                ULONG ulrc = (ULONG)(pfnwpOriginal(hwndFrame, msg, mp1, mp2));

                // if we have a status bar, increment the count
                if (psfv->hwndStatusBar)
                    ulrc++;

                mrc = (MPARAM)ulrc;
            }
            break;

            /*
             * WM_FORMATFRAME:
             *    this message is sent to a frame window to calculate the sizes
             *    and positions of all of the frame controls and the client window.
             *
             *    Parameters:
             *          mp1     PSWP    pswp        structure array; for each frame
             *                                      control which was announced with
             *                                      WM_QUERYFRAMECTLCOUNT, PM has
             *                                      allocated one SWP structure.
             *          mp2     PRECTL  pprectl     pointer to client window
             *                                      rectangle of the frame
             *          returns USHORT  ccount      count of the number of SWP
             *                                      arrays returned
             *
             *    It is the responsibility of this message code to set up the
             *    SWP structures in the given array to position the frame controls.
             *    We call the "parent" window proc and then set up our status bar.
             */

            case WM_FORMATFRAME:
            {
                //  query the number of standard frame controls
                ULONG ulCount = (ULONG)(pfnwpOriginal(hwndFrame, msg, mp1, mp2));

                #ifdef DEBUG_STATUSBARS
                    _Pmpf(( "WM_FORMATFRAME ulCount = %d", ulCount ));
                #endif

                if (psfv->hwndStatusBar)
                {
                    // we have a status bar:
                    // format the frame
                    FormatFrame(psfv, mp1, ulCount);

                    // increment the number of frame controls
                    // to include our status bar
                    mrc = (MRESULT)(ulCount + 1);
                } // end if (psfv->hwndStatusBar)
                else
                    // no status bar:
                    mrc = (MRESULT)ulCount;
            }
            break;

            /*
             * WM_CALCFRAMERECT:
             *     this message occurs when an application uses the
             *     WinCalcFrameRect function.
             *
             *     Parameters:
             *          mp1     PRECTL  pRect      rectangle structure
             *          mp2     USHORT  usFrame    frame indicator
             *          returns BOOL    rc         rectangle-calculated indicator
             */

            case WM_CALCFRAMERECT:
                mrc = pfnwpOriginal(hwndFrame, msg, mp1, mp2);

                if (psfv->hwndStatusBar)
                    // we have a status bar: calculate its rectangle
                    CalcFrameRect(mp1, mp2);
            break;

            /* *************************
             *                         *
             * Menu items:             *
             *                         *
             **************************/

            /*
             * WM_INITMENU:
             *      this message is sent to a frame whenever a menu
             *      is about to be displayed. This is needed for
             *      various menu features; see InitMenu() above.
             */

            case WM_INITMENU:
                // call the default, in case someone else
                // is subclassing folders (ObjectDesktop?!?);
                // from what I've checked, the WPS does NOTHING
                // with this message, not even for menu bars...
                mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);

                if (cmnQuerySetting(sfNoFreakyMenus) == FALSE)
                    // added V0.9.3 (2000-03-28) [umoeller]
                    InitMenu(psfv,
                             (ULONG)mp1,
                             (HWND)mp2);
            break;

            /*
             * WM_MENUSELECT:
             *      this is SENT to a menu owner by the menu
             *      control to determine what to do right after
             *      a menu item has been selected. If we return
             *      TRUE, the menu will be dismissed.
             *
             *      See MenuSelect() above.
             */

            case WM_MENUSELECT:
            {
                // PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
                BOOL fDismiss = TRUE;

                #ifdef DEBUG_MENUS
                    _Pmpf(( "WM_MENUSELECT: mp1 = %lX/%lX, mp2 = %lX",
                            SHORT1FROMMP(mp1),
                            SHORT2FROMMP(mp1),
                            mp2 ));
                #endif

                // always call the default, in case someone else
                // is subclassing folders (ObjectDesktop?!?)
                mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);

                if (cmnQuerySetting(sfNoFreakyMenus) == FALSE)
                    // added V0.9.3 (2000-03-28) [umoeller]
                    // now handle our stuff; this might modify mrc to
                    // have the menu stay on the screen
                    if (MenuSelect(psfv, mp1, mp2, &fDismiss))
                        // processed: return the modified flag instead
                        mrc = (MRESULT)fDismiss;
            }
            break;

            /*
             * WM_MENUEND:
             *      this message occurs when a menu control is about to
             *      terminate. We need to remove cnr source emphasis
             *      if the user has requested a context menu from a
             *      status bar.
             *
             *      Note: WM_MENUEND comes in BEFORE WM_COMMAND.
             */

            case WM_MENUEND:
                #ifdef DEBUG_MENUS
                    _Pmpf(( "WM_MENUEND: mp1 = %lX, mp2 = %lX",
                            mp1, mp2 ));
                    /* _Pmpf(( "  fFolderContentWindowPosChanged: %d",
                            fFolderContentWindowPosChanged));
                    _Pmpf(( "  fFolderContentButtonDown: %d",
                            fFolderContentButtonDown)); */
                #endif

                if (cmnQuerySetting(sfNoFreakyMenus) == FALSE)
                {
                    // added V0.9.3 (2000-03-28) [umoeller]

                    // menu opened from status bar?
                    if (psfv->fRemoveSourceEmphasis)
                    {
                        // if so, remove cnr source emphasis
                        /* WinSendMsg(psfv->hwndCnr,
                                   CM_SETRECORDEMPHASIS,
                                   (MPARAM)NULL,   // undocumented: if precc == NULL,
                                                   // the whole cnr is given emphasis
                                   MPFROM2SHORT(FALSE,  // remove emphasis
                                                CRA_SOURCE)); */
                        // and make sure the container has the
                        // focus
                        // WinSetFocus(HWND_DESKTOP, psfv->hwndCnr);
                        // reset flag for next context menu
                        psfv->fRemoveSourceEmphasis = FALSE;
                    }

                    // unset flag for WM_MENUSELECT above
                    // G_fFldrContentMenuMoved = FALSE;
                }

                mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
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
             *      Return value: check mnuMeasureItem.
             */

            case WM_MEASUREITEM:
                if ( (SHORT)mp1 > (cmnQuerySetting(sulVarMenuOffset)+ID_XFMI_OFS_VARIABLE) )
                {
                    // call the measure-item func in fdrmenus.c
                    mrc = cmnuMeasureItem((POWNERITEM)mp2); // , pGlobalSettings);
                }
                else
                    // none of our items: pass to original wnd proc
                    mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
            break;

            /*
             * WM_DRAWITEM:
             *      this msg is sent for each item every time it
             *      needs to be redrawn. This gets sent to us for
             *      items in folder content menus (if icons are on).
             */

            case WM_DRAWITEM:
                if (    ((SHORT)mp1 > cmnQuerySetting(sulVarMenuOffset) + ID_XFMI_OFS_VARIABLE)
                     && ((ULONG)mp1 != FID_CLIENT)      // rule out container V0.9.16 (2002-01-13) [umoeller]
                   )
                {
                    // variable menu item: this must be a folder-content
                    // menu item, because for others no WM_DRAWITEM is sent
                    // (fdrmenus.c)
                    if (cmnuDrawItem(mp1, mp2))
                        mrc = (MRESULT)TRUE;
                    else // error occured:
                        fCallDefault = TRUE;    // V0.9.3 (2000-04-26) [umoeller]
                        // mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
                }
                else
                    fCallDefault = TRUE;    // V0.9.3 (2000-04-26) [umoeller]
                    // mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
            break;

            /* *************************
             *                         *
             * Miscellaneae:           *
             *                         *
             **************************/

            /*
             * WM_COMMAND:
             *      this is intercepted to provide "delete" menu
             *      item support if "delete into trashcan"
             *      is on. We cannot use wpMenuItemSelected for
             *      that because that method gets called for
             *      every object, and we'd never know when it's
             *      called for the last object. So we do this
             *      here instead, and wpMenuItemSelected never
             *      gets called.
             *
             *      This now calls xwpProcessObjectCommand,
             *      resolved by name.
             *
             *      Note: WM_MENUEND comes in BEFORE WM_COMMAND.
             */

            case WM_COMMAND:
            {
                USHORT usCommand = SHORT1FROMMP(mp1);

                // resolve method by name
                somTD_XFolder_xwpProcessObjectCommand pxwpProcessObjectCommand
                    = (somTD_XFolder_xwpProcessObjectCommand)somResolveByName(
                                          psfv->somSelf,
                                          "xwpProcessObjectCommand");

                if (!pxwpProcessObjectCommand)
                    fCallDefault = TRUE;
                else
                    if (!pxwpProcessObjectCommand(psfv->somSelf,
                                                  usCommand,
                                                  psfv->hwndCnr,
                                                  psfv->pSourceObject,
                                                  psfv->ulSelection))
                        // not processed:
                        fCallDefault = TRUE;

                psfv->pSourceObject = NULL;
            }
            break;

#ifndef __NOXSHUTDOWN__
            /*
             * WM_SYSCOMMAND:
             *      intercept "close" for the desktop so we can
             *      invoke shutdown on Alt+F4.
             *
             * V0.9.16 (2002-01-04) [umoeller]
             */

            case WM_SYSCOMMAND:
                fCallDefault = TRUE;
                if (    (SHORT1FROMMP(mp1) == SC_CLOSE)
                     && (hwndFrame == cmnQueryActiveDesktopHWND())
                   )
                {
                    if (cmnQuerySetting(sflXShutdown) & XSD_CANDESKTOPALTF4)
                    {
                        WinPostMsg(hwndFrame,
                                   WM_COMMAND,
                                   MPFROMSHORT(WPMENUID_SHUTDOWN),
                                   MPFROM2SHORT(CMDSRC_MENU,
                                                FALSE));
                        fCallDefault = FALSE;
                    }
                }
            break;
#endif

            /*
             * WM_CHAR:
             *      this is intercepted to provide folder hotkeys
             *      and "Del" key support if "delete into trashcan"
             *      is on.
             */

            case WM_CHAR:
                if (WMChar(hwndFrame, psfv, mp1, mp2))
                    // processed:
                    mrc = (MRESULT)TRUE;
                else
                    fCallDefault = TRUE;
            break;

            /*
             * WM_CONTROL:
             *      this is intercepted to check for container
             *      notifications we might be interested in.
             */

            case WM_CONTROL:
            {
                if (SHORT1FROMMP(mp1) /* id */ == 0x8008) // container!!
                {
                    #ifdef DEBUG_CNRCNTRL
                        CHAR szTemp2[30];
                        sprintf(szTemp2, "unknown: %d", SHORT2FROMMP(mp1));
                        _Pmpf(("Cnr cntrl msg: %s, mp2: %lX",
                            (SHORT2FROMMP(mp1) == CN_BEGINEDIT) ? "CN_BEGINEDIT"
                                : (SHORT2FROMMP(mp1) == CN_COLLAPSETREE) ? "CN_COLLAPSETREE"
                                : (SHORT2FROMMP(mp1) == CN_CONTEXTMENU) ? "CN_CONTEXTMENU"
                                : (SHORT2FROMMP(mp1) == CN_DRAGAFTER) ? "CN_DRAGAFTER"
                                : (SHORT2FROMMP(mp1) == CN_DRAGLEAVE) ? "CN_DRAGLEAVE"
                                : (SHORT2FROMMP(mp1) == CN_DRAGOVER) ? "CN_DRAGOVER"
                                : (SHORT2FROMMP(mp1) == CN_DROP) ? "CN_DROP"
                                : (SHORT2FROMMP(mp1) == CN_DROPNOTIFY) ? "CN_DROPNOTIFY"
                                : (SHORT2FROMMP(mp1) == CN_DROPHELP) ? "CN_DROPHELP"
                                : (SHORT2FROMMP(mp1) == CN_EMPHASIS) ? "CN_EMPHASIS"
                                : (SHORT2FROMMP(mp1) == CN_ENDEDIT) ? "CN_ENDEDIT"
                                : (SHORT2FROMMP(mp1) == CN_ENTER) ? "CN_ENTER"
                                : (SHORT2FROMMP(mp1) == CN_EXPANDTREE) ? "CN_EXPANDTREE"
                                : (SHORT2FROMMP(mp1) == CN_HELP) ? "CN_HELP"
                                : (SHORT2FROMMP(mp1) == CN_INITDRAG) ? "CN_INITDRAG"
                                : (SHORT2FROMMP(mp1) == CN_KILLFOCUS) ? "CN_KILLFOCUS"
                                : (SHORT2FROMMP(mp1) == CN_PICKUP) ? "CN_PICKUP"
                                : (SHORT2FROMMP(mp1) == CN_QUERYDELTA) ? "CN_QUERYDELTA"
                                : (SHORT2FROMMP(mp1) == CN_REALLOCPSZ) ? "CN_REALLOCPSZ"
                                : (SHORT2FROMMP(mp1) == CN_SCROLL) ? "CN_SCROLL"
                                : (SHORT2FROMMP(mp1) == CN_SETFOCUS) ? "CN_SETFOCUS"
                                : szTemp2,
                            mp2));
                    #endif

                    switch (SHORT2FROMMP(mp1))      // usNotifyCode
                    {
                        /*
                         * CN_BEGINEDIT:
                         *      this is sent by the container control
                         *      when direct text editing is about to
                         *      begin, that is, when the user alt-clicks
                         *      on an object title.
                         *      We'll select the file stem of the object.
                         */

                        /* case CN_BEGINEDIT: {
                            PCNREDITDATA pced = (PCNREDITDATA)mp2;
                            mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
                            if (pced) {
                                PMINIRECORDCORE pmrc = (PMINIRECORDCORE)pced->pRecord;
                                if (pmrc) {
                                    // editing WPS record core, not title etc.:
                                    // get the window ID of the MLE control
                                    // in the cnr window
                                    HWND hwndMLE = WinWindowFromID(pced->hwndCnr,
                                                        CID_MLE);
                                    if (hwndMLE) {
                                        ULONG cbText = WinQueryWindowTextLength(
                                                                hwndMLE)+1;
                                        PSZ pszText = malloc(cbText);
                                        _Pmpf(("textlen: %d", cbText));
                                        if (WinQueryWindowText(hwndMLE,
                                                               cbText,
                                                               pszText))
                                        {
                                            PSZ pszLastDot = strrchr(pszText, '.');
                                            _Pmpf(("text: %s", pszText));
                                            WinSendMsg(hwndMLE,
                                                    EM_SETSEL,
                                                    MPFROM2SHORT(
                                                        // first char: 0
                                                        0,
                                                        // last char:
                                                        (pszLastDot)
                                                            ? (pszLastDot-pszText)
                                                            : 10000
                                                    ), MPNULL);
                                        }
                                        free(pszText);
                                    }
                                }
                            }
                        }
                        break;  */

#ifndef __NOXSYSTEMSOUNDS__
                        /*
                         * CN_ENTER:
                         *      double-click or enter key:
                         *      play sound
                         */

                        case CN_ENTER:
                            cmnPlaySystemSound(MMSOUND_XFLD_CNRDBLCLK);
                            mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
                        break;
#endif

                        /*
                         * CN_EMPHASIS:
                         *      selection changed:
                         *      update status bar
                         */

                        case CN_EMPHASIS:
                            mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
                            if (psfv->hwndStatusBar)
                            {
                                #ifdef DEBUG_STATUSBARS
                                    _Pmpf(( "CN_EMPHASIS: posting STBM_UPDATESTATUSBAR to hwnd %lX",
                                            psfv->hwndStatusBar ));
                                #endif

                                WinPostMsg(psfv->hwndStatusBar,
                                           STBM_UPDATESTATUSBAR,
                                           MPNULL,
                                           MPNULL);
                            }
                        break;

                        /*
                         * CN_EXPANDTREE:
                         *      tree view has been expanded:
                         *      do cnr auto-scroll in File thread
                         */

                        case CN_EXPANDTREE:
                        {
                            // PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
                            mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
                            if (cmnQuerySetting(sfTreeViewAutoScroll))
                                xthrPostBushMsg(QM_TREEVIEWAUTOSCROLL,
                                                  (MPARAM)hwndFrame,
                                                  mp2); // PMINIRECORDCORE
                        }
                        break;

                        default:
                            fCallDefault = TRUE;
                        break;
                    } // end switch (SHORT2FROMMP(mp1))      // usNotifyCode
                }
            }
            break;

            /*
             * WM_DESTROY:
             *      clean up resources we allocated for
             *      this folder view.
             */

            case WM_DESTROY:
                // destroy the supplementary object window for this folder
                // frame window; do this first because this references
                // the SFV
                WinDestroyWindow(psfv->hwndSupplObject);

                // upon closing the window, undo the subclassing, in case
                // some other message still comes in
                // (there are usually still two more, even after WM_DESTROY!!)
                WinSubclassWindow(hwndFrame, pfnwpOriginal);

                // and remove this window from our subclassing linked list
                fdrRemoveSFV(psfv);

                // do the default stuff
                fCallDefault = TRUE;
            break;

            default:
                fCallDefault = TRUE;
            break;

        } // end switch
    } // end TRY_LOUD
    CATCH(excpt1)
    {
        // exception occured:
        return (0);
    } END_CATCH();

    if (fCallDefault)
    {
        // this has only been set to TRUE for "default" in
        // the switch statement above; we then call the
        // default window procedure.
        // This is either the original folder frame window proc
        // of the WPS itself or maybe the one of other WPS enhancers
        // which have subclassed folder windows (ObjectDesktop
        // and the like).
        // We do this outside the TRY/CATCH stuff above so that
        // we don't get blamed for exceptions which we are not
        // responsible for, which was the case with XFolder < 0.85
        // (i.e. exceptions in WPFolder or Object Desktop or whatever).
        if (pfnwpOriginal)
            mrc = (MRESULT)pfnwpOriginal(hwndFrame, msg, mp1, mp2);
        else
        {
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "Folder's pfnwpOrig not found.");
            mrc = WinDefWindowProc(hwndFrame, msg, mp1, mp2);
        }
    }

    return (mrc);
}

/*
 *@@ fdr_fnwpSubclassedFolderFrame:
 *      new window proc for subclassed folder frame windows.
 *      Folder frame windows are subclassed in fdrSubclassFolderView
 *      (which gets called from XFolder::wpOpen or XFldDisk::wpOpen
 *      for Disk views) with the address of this window procedure.
 *
 *      This is maybe the most central (and most complex) part of
 *      XWorkplace. Since most WPS methods are really just reacting
 *      to messages in the default WPS frame window proc, but for
 *      some features methods are just not sufficient, basically we
 *      simulate what the WPS does here by intercepting _lots_
 *      of messages before the WPS gets them.
 *
 *      Unfortunately, this leads to quite a mess, but we have
 *      no choice.
 *
 *      Things we do in this proc:
 *
 *      --  frame control manipulation for status bars
 *
 *      --  Warp 4 folder menu bar manipulation (WM_INITMENU)
 *
 *      --  handling of certain menu items w/out dismissing
 *          the menu; this calls functions in fdrmenus.c
 *
 *      --  menu owner draw (folder content menus w/ icons);
 *          this calls functions in fdrmenus.c also
 *
 *      --  complete interception of file-operation menu items
 *          such as "delete" for deleting all objects into the
 *          XWorkplace trash can; this is now done thru a
 *          new method, which can be overridden by WPFolder
 *          subclasses (such as the trash can). See
 *          XFolder::xwpProcessObjectCommand.
 *
 *      --  container control messages: tree view auto-scroll,
 *          updating status bars etc.
 *
 *      --  playing the new system sounds for menus and such.
 *
 *      Note that this function calls lots of "external" functions
 *      spread across all over the XWorkplace code. I have tried to
 *      reduce the size of this window procedure to an absolute
 *      minimum because this function gets called very often (for
 *      every single folder message) and large message procedures
 *      may thrash the processor caches.
 *
 *      The actual message processing is now in fdrProcessFolderMsgs.
 *      This allows us to use the same message processing from
 *      other (future) parts of XWorkplace which no longer rely
 *      on subclassing the default WPS folder frames.
 *
 *@@changed V0.9.0 [umoeller]: moved cleanup code from WM_CLOSE to WM_DESTROY; un-subclassing removed
 *@@changed V0.9.0 [umoeller]: moved this func here from xfldr.c
 *@@changed V0.9.1 (2000-01-29) [umoeller]: added WPMENUID_DELETE support
 *@@changed V0.9.1 (2000-01-31) [umoeller]: added "Del" key support
 *@@changed V0.9.2 (2000-02-22) [umoeller]: moved default winproc out of exception handler
 *@@changed V0.9.3 (2000-03-28) [umoeller]: added freaky menus setting
 *@@changed V0.9.3 (2000-04-08) [umoeller]: extracted ProcessFolderMsgs
 */

MRESULT EXPENTRY fdr_fnwpSubclassedFolderFrame(HWND hwndFrame,
                                               ULONG msg,
                                               MPARAM mp1,
                                               MPARAM mp2)
{
    PSUBCLASSEDFOLDERVIEW psfv;

    if (psfv = fdrQuerySFV(hwndFrame,
                           NULL))
        return (fdrProcessFolderMsgs(hwndFrame,
                                     msg,
                                     mp1,
                                     mp2,
                                     psfv,
                                     psfv->pfnwpOriginal));

    // SFV not found: use the default
    // folder window procedure, but issue
    // a warning to the log
    cmnLog(__FILE__, __LINE__, __FUNCTION__,
           "Folder SUBCLASSEDFOLDERVIEW not found.");

    return (G_WPFolderWinClassInfo.pfnWindowProc(hwndFrame, msg, mp1, mp2));
}

/*
 *@@ fdr_fnwpSupplFolderObject:
 *      this is the wnd proc for the "Supplementary Object wnd"
 *      which is created for each folder frame window when it's
 *      subclassed. We need this window to handle additional
 *      messages which are not part of the normal message set,
 *      which is handled by fdr_fnwpSubclassedFolderFrame.
 *
 *      This window gets created in fdrSubclassFolderView, when
 *      the folder frame is also subclassed.
 *
 *      If we processed additional messages in fdr_fnwpSubclassedFolderFrame,
 *      we'd probably ruin other WPS enhancers which might use the same
 *      message in a different context (ObjectDesktop?), so we use a
 *      different window, which we own all alone.
 *
 *      We cannot use the global XFolder object window either
 *      (krn_fnwpThread1Object, kernel.c) because sometimes
 *      folder windows do not run in the main PM thread
 *      (TID 1), esp. when they're opened using WinOpenObject or
 *      REXX functions. I have found that manipulating windows
 *      from threads other than the one which created the window
 *      can produce really funny results or bad PM hangs.
 *
 *      This wnd proc always runs in the same thread as the folder
 *      frame wnd does.
 *
 *      This func is new with XFolder V0.82.
 *
 *@@changed V0.9.0 [umoeller]: moved this func here from xfldr.c
 */

MRESULT EXPENTRY fdr_fnwpSupplFolderObject(HWND hwndObject, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MPARAM mrc = NULL;
    PSUBCLASSEDFOLDERVIEW psfv = (PSUBCLASSEDFOLDERVIEW)
                WinQueryWindowPtr(hwndObject, QWL_USER);

    switch (msg)
    {
        case WM_CREATE:
            // set the USER window word to the SUBCLASSEDFOLDERVIEW
            // structure which is passed to us upon window
            // creation (see cmnSubclassFrameWnd, which creates us)
            mrc = WinDefWindowProc(hwndObject, msg, mp1, mp2);
            psfv = (PSUBCLASSEDFOLDERVIEW)mp1;
            WinSetWindowULong(hwndObject, QWL_USER, (ULONG)psfv);
        break;

        /*
         *@@ SOM_ACTIVATESTATUSBAR:
         *      add / remove / repaint the folder status bar;
         *      this is posted every time XFolder needs to change
         *      anything about status bars. We must not play with
         *      frame controls from threads other than the thread
         *      in which the status bar was created, i.e. the thread
         *      in which the folder frame is running (which, in most
         *      cases, is thread 1, the main PM thread of the WPS),
         *      because reformatting frame controls from other
         *      threads will cause PM hangs or WPS crashes.
         *
         *      Parameters:
         *      -- ULONG mp1   -- 0: disable (destroy) status bar
         *                     -- 1: enable (create) status bar
         *                     -- 2: update (reformat) status bar
         *      -- HWND  mp2:  hwndView (frame) to update
         */

        case SOM_ACTIVATESTATUSBAR:
        {
            HWND hwndFrame = (HWND)mp2;
            #ifdef DEBUG_STATUSBARS
                _Pmpf(( "SOM_ACTIVATESTATUSBAR, mp1: %lX, psfv: %lX", mp1, psfv));
            #endif

            if (psfv)
                switch ((ULONG)mp1)
                {
                    case 0:
                        fdrCreateStatusBar(psfv->somSelf, psfv, FALSE);
                    break;

                    case 1:
                        fdrCreateStatusBar(psfv->somSelf, psfv, TRUE);
                    break;

                    default:
                    {
                        // == 2 => update status bars; this is
                        // necessary if the font etc. has changed
                        const char* pszStatusBarFont =
                                cmnQueryStatusBarSetting(SBS_STATUSBARFONT);
                        // avoid recursing
                        WinSendMsg(psfv->hwndStatusBar, STBM_PROHIBITBROADCASTING,
                                   (MPARAM)TRUE, MPNULL);
                        // set font
                        WinSetPresParam(psfv->hwndStatusBar,
                                        PP_FONTNAMESIZE,
                                        (ULONG)(strlen(pszStatusBarFont) + 1),
                                        (PVOID)pszStatusBarFont);
                        // update frame controls
                        WinSendMsg(hwndFrame, WM_UPDATEFRAME, MPNULL, MPNULL);
                        // update status bar text synchronously
                        WinSendMsg(psfv->hwndStatusBar, STBM_UPDATESTATUSBAR, MPNULL, MPNULL);
                    }
                    break;
                }
        }
        break;

        default:
            mrc = WinDefWindowProc(hwndObject, msg, mp1, mp2);
    }

    return (mrc);
}


