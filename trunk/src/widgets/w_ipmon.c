
/*
 *@@sourcefile w_ipmon.c:
 *      XCenter IP monitor widget.
 *
 *      This is all new with V0.9.19.
 *
 *@@added V0.9.19 (2002-05-28) [umoeller]
 *@@header "shared\center.h"
 */

/*
 *      Copyright (C) 2002 Ulrich M�ller.
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
#define INCL_DOSMODULEMGR
#define INCL_DOSEXCEPTIONS
#define INCL_DOSSEMAPHORES
#define INCL_DOSDATETIME
#define INCL_DOSMISC
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WININPUT
#define INCL_WINPOINTERS
#define INCL_WINPROGRAMLIST
#define INCL_WINSWITCHLIST
#define INCL_WINSYS
#define INCL_WINTIMER
#define INCL_WINMENUS
#define INCL_WINWORKPLACE

#define INCL_GPIPRIMITIVES
#define INCL_GPILOGCOLORTABLE
#define INCL_GPILCIDS
#define INCL_GPIREGIONS
#include <os2.h>

// C library headers
#include <stdio.h>
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#define DONT_REPLACE_MALLOC         // in case mem debug is enabled
#include "setup.h"                      // code generation and debugging options

// disable wrappers, because we're not linking statically
#ifdef DOSH_STANDARDWRAPPERS
    #undef DOSH_STANDARDWRAPPERS
#endif
#ifdef WINH_STANDARDWRAPPERS
    #undef WINH_STANDARDWRAPPERS
#endif

// headers in /helpers
#include "helpers\comctl.h"             // common controls (window procs)
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\gpih.h"               // GPI helper routines
#include "helpers\stringh.h"            // string helper routines
#include "helpers\timer.h"              // replacement PM timers
#include "helpers\winh.h"               // PM helper routines
#include "helpers\xstring.h"            // extended string helpers

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\center.h"              // public XCenter interfaces
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs

#include "config\drivdlgs.h"            // driver configuration dialogs

#pragma hdrstop                     // VAC++ keeps crashing otherwise

/* ******************************************************************
 *
 *   Private definitions
 *
 ********************************************************************/

/*
 *  Some TCP/IP definitions copied from the Warp 4.5 toolkit.
 *  Since we are using the Warp 3 toolkit, we will have trouble
 *  getting this stuff from elsewhere.
 *
 *  Note that we use the 16-bit definitions.
 */

#ifndef IFMIB_ENTRIES
#define IFMIB_ENTRIES 42
#endif

typedef unsigned long   u_long;

#pragma pack(1) /* force on doubleword boundary */
struct ifmib {
  short ifNumber;  /* number of network interfaces */
  struct iftable {
    short  ifIndex;        /* index of this interface */
    char   ifDescr[45];    /* description             */
    short  ifType;         /* type of the interface   */
    short  ifMtu;          /* MTU of the interface   */
    char   ifPhysAddr[6];  /* MTU of the interface   */
    short  ifOperStatus;
    u_long ifSpeed;
    u_long ifLastChange;
    u_long ifInOctets;
    u_long ifOutOctets;
    u_long ifOutDiscards;
    u_long ifInDiscards;
    u_long ifInErrors;
    u_long ifOutErrors;
    u_long ifInUnknownProtos;
    u_long ifInUcastPkts;
    u_long ifOutUcastPkts;
    u_long ifInNUcastPkts;
    u_long ifOutNUcastPkts;
  } iftable[IFMIB_ENTRIES];
};
#pragma pack()   /* reset to default packing */

typedef char *caddr_t;

int _System sock_init( void );
int _System sock_errno( void );
int _System socket( int, int, int );
int _System soclose( int );
int _System ioctl(int, int, char *, int);

#define ioc(x,y)       ((x<<8)|y)
#define SIOSTATIF       ioc('n',48)

#define SOCK_STREAM     1               /* stream socket */
#define SOCK_DGRAM      2               /* datagram socket */
#define SOCK_RAW        3               /* raw-protocol interface */
#define SOCK_RDM        4               /* reliably-delivered message */
#define SOCK_SEQPACKET  5               /* sequenced packet stream */

#define AF_UNSPEC       0               /* unspecified */
#define AF_UNIX         1               /* local to host (pipes, portals) */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3               /* arpanet imp addresses */
#define AF_PUP          4               /* pup protocols: e.g. BSP */
#define AF_CHAOS        5               /* mit CHAOS protocols */
#define AF_NS           6               /* XEROX NS protocols */
#define AF_ISO          7               /* ISO protocols */
#define AF_OSI          AF_ISO          /* OSI is ISO */
#define AF_ECMA         8               /* european computer manufacturers */
#define AF_DATAKIT      9               /* datakit protocols */
#define AF_CCITT        10              /* CCITT protocols, X.25 etc */
#define AF_SNA          11              /* IBM SNA */
#define AF_DECnet       12              /* DECnet */
#define AF_DLI          13              /* Direct data link interface */
#define AF_LAT          14              /* LAT */
#define AF_HYLINK       15              /* NSC Hyperchannel */
#define AF_APPLETALK    16              /* AppleTalk */
#define AF_NETBIOS      17              /* NetBios-style addresses */

#define AF_MAX          18

#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_INET         AF_INET
#define PF_IMPLINK      AF_IMPLINK
#define PF_PUP          AF_PUP
#define PF_CHAOS        AF_CHAOS
#define PF_NS           AF_NS
#define PF_ISO          AF_ISO
#define PF_OSI          AF_OSI
#define PF_ECMA         AF_ECMA
#define PF_DATAKIT      AF_DATAKIT
#define PF_CCITT        AF_CCITT
#define PF_SNA          AF_SNA
#define PF_DECnet       AF_DECnet
#define PF_DLI          AF_DLI
#define PF_LAT          AF_LAT
#define PF_HYLINK       AF_HYLINK
#define PF_APPLETALK    AF_APPLETALK

#define PF_MAX          AF_MAX

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

/* ******************************************************************
 *
 *   XCenter widget class definition
 *
 ********************************************************************/

/*
 *      This contains the name of the PM window class and
 *      the XCENTERWIDGETCLASS definition(s) for the widget
 *      class(es) in this DLL.
 */

#define WNDCLASS_WIDGET_IPMON    "XWPCenterIPMonWidget"

static const XCENTERWIDGETCLASS G_WidgetClasses[] =
    {
        {
            WNDCLASS_WIDGET_IPMON,
            0,
            "IPMonitor",
            (PCSZ)(XCENTER_STRING_RESOURCE | ID_CRSI_WIDGET_IPMONITOR),
                                       // widget class name displayed to user
            WGTF_SIZEABLE | WGTF_TOOLTIP,
            NULL        // no settings dlg
        },
    };

/* ******************************************************************
 *
 *   Function imports from XFLDR.DLL
 *
 ********************************************************************/

/*
 *      To reduce the size of the widget DLL, it is
 *      compiled with the VAC subsystem libraries.
 *      In addition, instead of linking frequently
 *      used helpers against the DLL again, we import
 *      them from XFLDR.DLL, whose module handle is
 *      given to us in the INITMODULE export.
 *
 *      Note that importing functions from XFLDR.DLL
 *      is _not_ a requirement. We only do this to
 *      avoid duplicate code.
 *
 *      For each funtion that you need, add a global
 *      function pointer variable and an entry to
 *      the G_aImports array. These better match.
 */

// resolved function pointers from XFLDR.DLL
PCMNGETSTRING pcmnGetString = NULL;
PCMNQUERYDEFAULTFONT pcmnQueryDefaultFont = NULL;
PCMNQUERYHELPLIBRARY pcmnQueryHelpLibrary = NULL;

PCTRFREESETUPVALUE pctrFreeSetupValue = NULL;
PCTRPARSECOLORSTRING pctrParseColorString = NULL;
PCTRSCANSETUPSTRING pctrScanSetupString = NULL;

PDRV_SPRINTF pdrv_sprintf = NULL;

PGPIHBOX pgpihBox = NULL;
PGPIHCREATEBITMAP pgpihCreateBitmap = NULL;
PGPIHCREATEMEMPS pgpihCreateMemPS = NULL;
PGPIHDRAW3DFRAME pgpihDraw3DFrame = NULL;
PGPIHSWITCHTORGB pgpihSwitchToRGB = NULL;
PGPIHCREATEXBITMAP pgpihCreateXBitmap = NULL;
PGPIHDESTROYXBITMAP pgpihDestroyXBitmap = NULL;

PTMRSTARTXTIMER ptmrStartXTimer = NULL;
PTMRSTOPXTIMER ptmrStopXTimer = NULL;

PWINHFREE pwinhFree = NULL;
PWINHINSERTMENUITEM pwinhInsertMenuItem = NULL;
PWINHINSERTSUBMENU pwinhInsertSubmenu = NULL;
PWINHQUERYPRESCOLOR pwinhQueryPresColor = NULL;
PWINHQUERYWINDOWFONT pwinhQueryWindowFont = NULL;
PWINHSETWINDOWFONT pwinhSetWindowFont = NULL;

PXSTRCAT pxstrcat = NULL;
PXSTRCLEAR pxstrClear = NULL;
PXSTRINIT pxstrInit = NULL;

static const RESOLVEFUNCTION G_aImports[] =
    {
        "cmnGetString", (PFN*)&pcmnGetString,
        "cmnQueryDefaultFont", (PFN*)&pcmnQueryDefaultFont,
        "cmnQueryHelpLibrary", (PFN*)&pcmnQueryHelpLibrary,
        "ctrFreeSetupValue", (PFN*)&pctrFreeSetupValue,
        "ctrParseColorString", (PFN*)&pctrParseColorString,
        "ctrScanSetupString", (PFN*)&pctrScanSetupString,
        "drv_sprintf", (PFN*)&pdrv_sprintf,
        "gpihBox", (PFN*)&pgpihBox,
        "gpihCreateBitmap", (PFN*)&pgpihCreateBitmap,
        "gpihCreateMemPS", (PFN*)&pgpihCreateMemPS,
        "gpihDraw3DFrame", (PFN*)&pgpihDraw3DFrame,
        "gpihSwitchToRGB", (PFN*)&pgpihSwitchToRGB,
        "gpihCreateXBitmap", (PFN*)&pgpihCreateXBitmap,
        "gpihDestroyXBitmap", (PFN*)&pgpihDestroyXBitmap,
        "tmrStartXTimer", (PFN*)&ptmrStartXTimer,
        "tmrStopXTimer", (PFN*)&ptmrStopXTimer,
        "winhFree", (PFN*)&pwinhFree,
        "winhInsertMenuItem", (PFN*)&pwinhInsertMenuItem,
        "winhInsertSubmenu", (PFN*)&pwinhInsertSubmenu,
        "winhQueryPresColor", (PFN*)&pwinhQueryPresColor,
        "winhQueryWindowFont", (PFN*)&pwinhQueryWindowFont,
        "winhSetWindowFont", (PFN*)&pwinhSetWindowFont,
        "xstrcat", (PFN*)&pxstrcat,
        "xstrClear", (PFN*)&pxstrClear,
        "xstrInit", (PFN*)&pxstrInit
    };

/* ******************************************************************
 *
 *   Private widget instance data
 *
 ********************************************************************/

/*
 *@@ MONITORSETUP:
 *      instance data to which setup strings correspond.
 *      This is also a member of WIDGETPRIVATE.
 *
 *      Putting these settings into a separate structure
 *      is no requirement technically. However, once the
 *      widget uses a settings dialog, the dialog must
 *      support changing the widget settings even if the
 *      widget doesn't currently exist as a window, so
 *      separating the setup data from the widget window
 *      data will come in handy for managing the setup
 *      strings.
 */

typedef struct _MONITORSETUP
{
    LONG        lcolBackground,         // background color
                lcolForeground;         // foreground color (for text)

    LONG        lcolIn,
                lcolOut;

    PSZ         pszFont;
            // if != NULL, non-default font (in "8.Helv" format);
            // this has been allocated using local malloc()!

    LONG        cx;
            // current width; we're sizeable, and we wanna
            // store this

    ULONG       ulDevIndex;

} MONITORSETUP, *PMONITORSETUP;

/*
 *@@ SNAPSHOT:
 *
 */

typedef struct _SNAPSHOT
{
    ULONG       ulIn,
                ulOut;
} SNAPSHOT, *PSNAPSHOT;

/*
 *@@ WIDGETPRIVATE:
 *      more window data for the various monitor widgets.
 *
 *      An instance of this is created on WM_CREATE in
 *      fnwpMonitorWidgets and stored in XCENTERWIDGET.pUser.
 */

typedef struct _WIDGETPRIVATE
{
    PXCENTERWIDGET  pWidget;
            // reverse ptr to general widget data ptr; we need
            // that all the time and don't want to pass it on
            // the stack with each function call

    MONITORSETUP    Setup;
            // widget settings that correspond to a setup string

    int             sock;
    struct ifmib    statif;
    ULONG           ulPrevTotalIn,
                    ulPrevTotalOut,
                    ulLastMilliseconds;

    BOOL            fCreating;      // TRUE while in WM_CREATE (anti-recursion)

    BOOL            fContextMenuHacked;

    ULONG           ulTimerID;      // if != NULLHANDLE, update timer is running

    PXBITMAP        pBitmap;        // bitmap for pulse graph; this contains only
                                    // the "client" (without the 3D frame)

    BOOL            fUpdateGraph;

    ULONG           cSnapshots;
    PSNAPSHOT       paSnapshots;

    ULONG           ulMax;              // maximimum in or out value in array
                                        // (for scaling)

    BOOL            fTooltipShowing;    // TRUE only while tooltip is currently
                                        // showing over this widget
    CHAR            szTooltipText[100]; // tooltip text

} WIDGETPRIVATE, *PWIDGETPRIVATE;

/* ******************************************************************
 *
 *   Widget settings dialog
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
 *@@ TwgtFreeSetup:
 *      cleans up the data in the specified setup
 *      structure, but does not free the structure
 *      itself.
 */

VOID TwgtFreeSetup(PMONITORSETUP pSetup)
{
    if (pSetup)
    {
        if (pSetup->pszFont)
        {
            free(pSetup->pszFont);
            pSetup->pszFont = NULL;
        }
    }
}

/*
 *@@ TwgtScanSetup:
 *      scans the given setup string and translates
 *      its data into the specified binary setup
 *      structure.
 *
 *      NOTE: It is assumed that pSetup is zeroed
 *      out. We do not clean up previous data here.
 *
 */

VOID TwgtScanSetup(PCSZ pcszSetupString,
                   PMONITORSETUP pSetup)
{
    PSZ p;

    // width
    if (p = pctrScanSetupString(pcszSetupString,
                                "WIDTH"))
    {
        pSetup->cx = atoi(p);
        pctrFreeSetupValue(p);
    }
    else
        pSetup->cx = 100;

    // background color
    if (p = pctrScanSetupString(pcszSetupString,
                                "BGNDCOL"))
    {
        pSetup->lcolBackground = pctrParseColorString(p);
        pctrFreeSetupValue(p);
    }
    else
        // default color:
        pSetup->lcolBackground = WinQuerySysColor(HWND_DESKTOP, SYSCLR_DIALOGBACKGROUND, 0);

    // text color:
    if (p = pctrScanSetupString(pcszSetupString,
                                "TEXTCOL"))
    {
        pSetup->lcolForeground = pctrParseColorString(p);
        pctrFreeSetupValue(p);
    }
    else
        pSetup->lcolForeground = WinQuerySysColor(HWND_DESKTOP, SYSCLR_WINDOWSTATICTEXT, 0);

    // font:
    // we set the font presparam, which automatically
    // affects the cached presentation spaces
    if (p = pctrScanSetupString(pcszSetupString,
                                "FONT"))
    {
        pSetup->pszFont = strdup(p);
        pctrFreeSetupValue(p);
    }
    else
        pSetup->pszFont = strdup("9.WarpSans");

    if (p = pctrScanSetupString(pcszSetupString,
                                "SOURCE"))
    {
        pSetup->ulDevIndex = atoi(p);
        pctrFreeSetupValue(p);
    }

    pSetup->lcolIn = RGBCOL_DARKGREEN;
    pSetup->lcolOut = RGBCOL_RED;
}

/*
 *@@ TwgtSaveSetup:
 *      composes a new setup string.
 *      The caller must invoke xstrClear on the
 *      string after use.
 */

VOID TwgtSaveSetup(PXSTRING pstrSetup,       // out: setup string (is cleared first)
                   PMONITORSETUP pSetup)
{
    CHAR    szTemp[100];
    pxstrInit(pstrSetup, 100);

    pdrv_sprintf(szTemp, "WIDTH=%d;",
            pSetup->cx);
    pxstrcat(pstrSetup, szTemp, 0);

    pdrv_sprintf(szTemp, "BGNDCOL=%06lX;",
            pSetup->lcolBackground);
    pxstrcat(pstrSetup, szTemp, 0);

    pdrv_sprintf(szTemp, "TEXTCOL=%06lX;",
            pSetup->lcolForeground);
    pxstrcat(pstrSetup, szTemp, 0);

    if (pSetup->pszFont)
    {
        // non-default font:
        pdrv_sprintf(szTemp, "FONT=%s;",
                pSetup->pszFont);
        pxstrcat(pstrSetup, szTemp, 0);
    }

    pdrv_sprintf(szTemp, "SOURCE=%d;",
                 pSetup->ulDevIndex);
    pxstrcat(pstrSetup, szTemp, 0);
}

/*
 *@@ TwgtSaveSetupAndSend:
 *
 */

VOID TwgtSaveSetupAndSend(HWND hwnd,
                          PMONITORSETUP pSetup)
{
    XSTRING strSetup;
    TwgtSaveSetup(&strSetup,
                  pSetup);
    if (strSetup.ulLength)
        // changed V0.9.13 (2001-06-21) [umoeller]:
        // post it to parent instead of fixed XCenter client
        // to make this trayable
        WinSendMsg(WinQueryWindow(hwnd, QW_PARENT), // pPrivate->pWidget->pGlobals->hwndClient,
                   XCM_SAVESETUP,
                   (MPARAM)hwnd,
                   (MPARAM)strSetup.psz);
    pxstrClear(&strSetup);
}

/* ******************************************************************
 *
 *   Widget settings dialog
 *
 ********************************************************************/

// None currently.

/* ******************************************************************
 *
 *   PM window class implementation
 *
 ********************************************************************/

/*
 *      This code has the actual PM window class.
 *
 */

/*
 *@@ TwgtCreate:
 *      implementation for WM_CREATE.
 */

MRESULT TwgtCreate(HWND hwnd,
                   PXCENTERWIDGET pWidget)
{
    MRESULT mrc = 0;        // continue window creation

    PWIDGETPRIVATE pPrivate = malloc(sizeof(WIDGETPRIVATE));
    memset(pPrivate, 0, sizeof(WIDGETPRIVATE));
    // link the two together
    pWidget->pUser = pPrivate;
    pPrivate->pWidget = pWidget;

    pPrivate->fCreating = TRUE;

    // initialize binary setup structure from setup string
    TwgtScanSetup(pWidget->pcszSetupString,
                  &pPrivate->Setup);

    sock_init();

    pPrivate->ulMax = 1;        // avoid division by zero

    // create socket for while the widget is running
    pPrivate->sock = socket(PF_INET, SOCK_STREAM, 0);

    if (pPrivate->sock > 0)
    {
        // socket created OK: get first shot of data
        // so we can do calculations from now on
        if (!ioctl(pPrivate->sock,
                   SIOSTATIF,
                   (caddr_t)&pPrivate->statif,
                   sizeof(pPrivate->statif)))
        {
            // now update "latest" with the data of the
            // selected device
            ULONG i;
            if (pPrivate->Setup.ulDevIndex >= IFMIB_ENTRIES)
                pPrivate->Setup.ulDevIndex = 0;

            i = pPrivate->Setup.ulDevIndex;

            pPrivate->ulPrevTotalIn = pPrivate->statif.iftable[i].ifInOctets;
            pPrivate->ulPrevTotalOut = pPrivate->statif.iftable[i].ifOutOctets;

            DosQuerySysInfo(QSV_MS_COUNT,
                            QSV_MS_COUNT,
                            (PVOID)&pPrivate->ulLastMilliseconds,
                            sizeof(pPrivate->ulLastMilliseconds));
        }
    }

    // set window font (this affects all the cached presentation
    // spaces we use)
    pwinhSetWindowFont(hwnd,
                       pPrivate->Setup.pszFont);

    // enable context menu help
    pWidget->pcszHelpLibrary = pcmnQueryHelpLibrary();
    pWidget->ulHelpPanelID = ID_XSH_WIDGET_SENTINEL_MAIN;

    // start update timer
    pPrivate->ulTimerID = ptmrStartXTimer(pWidget->pGlobals->pvXTimerSet,
                                         hwnd,
                                         1,
                                         1000);


    pPrivate->fCreating = FALSE;

    pPrivate->szTooltipText[0] = '\0';

    return (mrc);
}

/*
 *@@ TwgtControl:
 *      implementation for WM_CONTROL.
 */

BOOL TwgtControl(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
    BOOL brc = FALSE;

    PXCENTERWIDGET pWidget;
    PWIDGETPRIVATE pPrivate;
    if (    (pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER))
         && (pPrivate = (PWIDGETPRIVATE)pWidget->pUser)
       )
    {
        USHORT  usID = SHORT1FROMMP(mp1),
                usNotifyCode = SHORT2FROMMP(mp1);

        switch (usID)
        {
            case ID_XCENTER_CLIENT:
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
                        pszl->cx = pPrivate->Setup.cx;
                        pszl->cy = 10;
                        brc = TRUE;
                    }
                    break;
                }
            }
            break;

            case ID_XCENTER_TOOLTIP:
            {
                switch (usNotifyCode)
                {
                    case TTN_NEEDTEXT:
                    {
                        PTOOLTIPTEXT pttt = (PTOOLTIPTEXT)mp2;
                        pttt->pszText = pPrivate->szTooltipText;
                        pttt->ulFormat = TTFMT_PSZ;
                    }
                    break;

                    case TTN_SHOW:
                        pPrivate->fTooltipShowing = TRUE;
                    break;

                    case TTN_POP:
                        pPrivate->fTooltipShowing = FALSE;
                    break;
                }
            }
        } // end if (pPrivate)
    } // end if (pWidget)

    return (brc);
}

/*
 *@@ TwgtUpdateGraph:
 *      updates the graph bitmap. This does not paint
 *      on the screen.
 *
 *      Preconditions:
 *      --  pPrivate->hbmGraph must be selected into
 *          pPrivate->hpsMem.
 */

VOID TwgtUpdateGraph(HWND hwnd,
                     PWIDGETPRIVATE pPrivate)
{
    PXCENTERWIDGET pWidget = pPrivate->pWidget;
    RECTL   rclBmp;

    // size for bitmap: same as widget, except
    // for the border
    WinQueryWindowRect(hwnd, &rclBmp);
    rclBmp.xRight -= 2;
    rclBmp.yTop -= 2;

    if (!pPrivate->pBitmap)
    {
        // bitmap needs to be created:
        pPrivate->pBitmap = pgpihCreateXBitmap(pWidget->habWidget,
                                               rclBmp.xRight,
                                               rclBmp.yTop);
    }

    if (pPrivate->pBitmap)
    {
        HPS     hpsMem = pPrivate->pBitmap->hpsMem;
        POINTL  ptl;

        GpiSetColor(hpsMem, pPrivate->Setup.lcolBackground);

        pgpihBox(hpsMem,
                 DRO_FILL,
                 &rclBmp);

        if (pPrivate->paSnapshots)
        {
            // paint input graph
            GpiSetColor(hpsMem, pPrivate->Setup.lcolIn);

            ptl.x = 0;
            ptl.y =    pPrivate->paSnapshots[0].ulIn
                     * rclBmp.yTop
                     / pPrivate->ulMax;
            GpiMove(hpsMem,
                    &ptl);

            for (ptl.x = 1;
                 ptl.x < pPrivate->cSnapshots;
                 ++ptl.x)
            {
                ptl.y =    pPrivate->paSnapshots[ptl.x].ulIn
                         * rclBmp.yTop
                         / pPrivate->ulMax;
                GpiLine(hpsMem,
                        &ptl);
            }

            // paint output graph
            GpiSetColor(hpsMem, pPrivate->Setup.lcolOut);

            ptl.x = 0;
            ptl.y =    pPrivate->paSnapshots[0].ulOut
                     * rclBmp.yTop
                     / pPrivate->ulMax;
            GpiMove(hpsMem,
                    &ptl);

            for (ptl.x = 1;
                 ptl.x < pPrivate->cSnapshots;
                 ++ptl.x)
            {
                ptl.y =    pPrivate->paSnapshots[ptl.x].ulOut
                         * rclBmp.yTop
                         / pPrivate->ulMax;
                GpiLine(hpsMem,
                        &ptl);
            }
        }
    }

    pPrivate->fUpdateGraph = FALSE;
}

/*
 * TwgtPaint2:
 *      this does the actual painting of the frame (if
 *      fDrawFrame==TRUE) and the pulse bitmap.
 *
 *      Gets called by TwgtPaint.
 *
 *      The specified HPS is switched to RGB mode before
 *      painting.
 *
 *      If DosPerfSysCall succeeds, this diplays the pulse.
 *      Otherwise an error msg is displayed.
 */

VOID TwgtPaint2(HWND hwnd,
                PWIDGETPRIVATE pPrivate,
                HPS hps,
                BOOL fDrawFrame)     // in: if TRUE, everything is painted
{
    PXCENTERWIDGET pWidget = pPrivate->pWidget;
    PMONITORSETUP pSetup = &pPrivate->Setup;
    RECTL       rclWin;
    ULONG       ulBorder = 1;
    CHAR        szTemp[200];

    // now paint button frame
    WinQueryWindowRect(hwnd,
                       &rclWin);        // exclusive
    pgpihSwitchToRGB(hps);

    rclWin.xRight--;
    rclWin.yTop--;
        // rclWin is now inclusive

    if (fDrawFrame)
    {
        LONG lDark, lLight;

        if (pPrivate->pWidget->pGlobals->flDisplayStyle & XCS_SUNKBORDERS)
        {
            lDark = pWidget->pGlobals->lcol3DDark;
            lLight = pWidget->pGlobals->lcol3DLight;
        }
        else
        {
            lDark =
            lLight = pPrivate->Setup.lcolBackground;
        }

        pgpihDraw3DFrame(hps,
                         &rclWin,        // inclusive
                         ulBorder,
                         lDark,
                         lLight);
    }

    if (pPrivate->fUpdateGraph)
        // graph bitmap needs to be updated:
        TwgtUpdateGraph(hwnd, pPrivate);

    if (pPrivate->pBitmap)
    {
        POINTL      ptlBmpDest;
        ptlBmpDest.x = rclWin.xLeft + ulBorder;
        ptlBmpDest.y = rclWin.yBottom + ulBorder;
        // now paint graph from bitmap
        WinDrawBitmap(hps,
                      pPrivate->pBitmap->hbm,
                      NULL,     // entire bitmap
                      &ptlBmpDest,
                      0, 0,
                      DBM_NORMAL);

        if (pPrivate->paSnapshots)
        {
            ULONG       ulIn, ulOut, ul;
            ulIn = pPrivate->paSnapshots[pPrivate->cSnapshots - 1].ulIn * 10 / 1024;
            ulOut = pPrivate->paSnapshots[pPrivate->cSnapshots - 1].ulOut * 10 / 1024;
            ul = pdrv_sprintf(szTemp,
                              "%d.%d | %d.%d",
                              ulIn / 10,
                              ulIn % 10,
                              ulOut / 10,
                              ulOut % 10);

            GpiSetColor(hps, RGBCOL_BLACK);
            WinDrawText(hps,
                        ul,
                        szTemp,
                        &rclWin,
                        0,
                        0,
                        DT_CENTER | DT_VCENTER | DT_TEXTATTRS);
        }
    }
}

/*
 *@@ TwgtPaint:
 *      implementation for WM_PAINT.
 */

VOID TwgtPaint(HWND hwnd)
{
    HPS hps;
    if (hps = WinBeginPaint(hwnd, NULLHANDLE, NULL))
    {
        // get widget data and its button data from QWL_USER
        PXCENTERWIDGET pWidget;
        PWIDGETPRIVATE pPrivate;
        if (    (pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER))
             && (pPrivate = (PWIDGETPRIVATE)pWidget->pUser)
           )
        {
            TwgtPaint2(hwnd,
                       pPrivate,
                       hps,
                       TRUE);        // draw frame
        } // end if (pPrivate)

        WinEndPaint(hps);

    } // end if (hps)
}

/*
 *@@ GetSnapshot:
 *      updates the newest entry in the snapshots
 *      array with the current IP values.
 */

VOID GetSnapshot(PWIDGETPRIVATE pPrivate)
{
    PSNAPSHOT pLatest = &pPrivate->paSnapshots[pPrivate->cSnapshots - 1];

    if (pPrivate->sock > 0)
    {
        if (!ioctl(pPrivate->sock,
                   SIOSTATIF,
                   (caddr_t)&pPrivate->statif,
                   sizeof(pPrivate->statif)))
        {
            ULONG ulMilliseconds;
            ULONG ulDivisor;
            ULONG i;

            DosQuerySysInfo(QSV_MS_COUNT,
                            QSV_MS_COUNT,
                            (PVOID)&ulMilliseconds,
                            sizeof(ULONG));

            if (!(ulDivisor = ulMilliseconds - pPrivate->ulLastMilliseconds))
                // avoid div by zero
                ulDivisor = 1;

            pPrivate->ulLastMilliseconds = ulMilliseconds;

            // do not crash the array
            if (pPrivate->Setup.ulDevIndex >= IFMIB_ENTRIES)
                pPrivate->Setup.ulDevIndex = 0;

            i = pPrivate->Setup.ulDevIndex;

            // now update "latest" with the data of the
            // selected device; the point is, we get the
            // current bandwidth by checking how much time
            // has elapsed and then subtracting the old
            // total bytes value from the new one

            // 1) input bytes
            pLatest->ulIn =   (   (double)pPrivate->statif.iftable[i].ifInOctets
                                 - pPrivate->ulPrevTotalIn
                               ) * 1000
                               / ulDivisor;
            if (pLatest->ulIn > pPrivate->ulMax)
                pPrivate->ulMax = pLatest->ulIn;

            pPrivate->ulPrevTotalIn = pPrivate->statif.iftable[i].ifInOctets;

            // 2) output bytes
            pLatest->ulOut =   (   (double)pPrivate->statif.iftable[i].ifOutOctets
                                 - pPrivate->ulPrevTotalOut
                               ) * 1000
                               / ulDivisor;
            if (pLatest->ulOut > pPrivate->ulMax)
                pPrivate->ulMax = pLatest->ulOut;

            pPrivate->ulPrevTotalOut = pPrivate->statif.iftable[i].ifOutOctets;
        }
    }
}

/*
 *@@ TwgtTimer:
 *      updates the snapshots array, updates the
 *      graph bitmap, and invalidates the window.
 */

VOID TwgtTimer(HWND hwnd)
{
    PXCENTERWIDGET pWidget;
    PWIDGETPRIVATE pPrivate;
    if (    (pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER))
         && (pPrivate = (PWIDGETPRIVATE)pWidget->pUser)
       )
    {
        HPS hps;
        RECTL rclClient;
        WinQueryWindowRect(hwnd, &rclClient);
        if (rclClient.xRight)
        {
            ULONG ulGraphCX = rclClient.xRight - 2;    // minus border

            if (pPrivate->paSnapshots == NULL)
            {
                // create array of loads
                ULONG cb = sizeof(SNAPSHOT) * ulGraphCX;
                pPrivate->cSnapshots = ulGraphCX;
                pPrivate->paSnapshots = (PSNAPSHOT)malloc(cb);
                memset(pPrivate->paSnapshots, 0, cb);
            }

            if (pPrivate->paSnapshots)
            {
                // in the array of loads, move each entry one to the front;
                // drop the oldest entry
                memmove(&pPrivate->paSnapshots[0],
                        &pPrivate->paSnapshots[1],
                        sizeof(SNAPSHOT) * (pPrivate->cSnapshots - 1));

                // and update the last entry with the current value
                GetSnapshot(pPrivate);

                // update display
                pPrivate->fUpdateGraph = TRUE;

                hps = WinGetPS(hwnd);
                TwgtPaint2(hwnd,
                           pPrivate,
                           hps,
                           FALSE);       // do not draw frame
                WinReleasePS(hps);
            }
        } // end if (rclClient.xRight)
    } // end if (pPrivate)
}

/*
 *@@ TwgtWindowPosChanged:
 *      implementation for WM_WINDOWPOSCHANGED.
 *
 */

VOID TwgtWindowPosChanged(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
    PXCENTERWIDGET pWidget;
    PWIDGETPRIVATE pPrivate;
    if (    (pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER))
         && (pPrivate = (PWIDGETPRIVATE)pWidget->pUser)
       )
    {
        PSWP pswpNew = (PSWP)mp1,
             pswpOld = pswpNew + 1;
        if (pswpNew->fl & SWP_SIZE)
        {
            // window was resized:

            // destroy the buffer bitmap because we
            // need a new one with a different size
            if (pPrivate->pBitmap)
                pgpihDestroyXBitmap(&pPrivate->pBitmap);

            if (pswpNew->cx != pswpOld->cx)
            {
                // width changed:
                if (pPrivate->paSnapshots)
                {
                    // we also need a new array of past loads
                    // since the array is cx items wide...
                    // so reallocate the array, but keep past
                    // values
                    ULONG ulNewClientCX = pswpNew->cx - 2;
                    PSNAPSHOT paNewSnapshots =
                        (PSNAPSHOT)malloc(sizeof(SNAPSHOT) * ulNewClientCX);

                    if (ulNewClientCX > pPrivate->cSnapshots)
                    {
                        // window has become wider:
                        // fill the front with zeroes
                        memset(paNewSnapshots,
                               0,
                               (ulNewClientCX - pPrivate->cSnapshots) * sizeof(SNAPSHOT));
                        // and copy old values after that
                        memmove(&paNewSnapshots[(ulNewClientCX - pPrivate->cSnapshots)],
                                pPrivate->paSnapshots,
                                pPrivate->cSnapshots * sizeof(SNAPSHOT));
                    }
                    else
                    {
                        // window has become smaller:
                        // e.g. ulnewClientCX = 100
                        //      pPrivate->cLoads = 200
                        // drop the first items
                        // ULONG ul = 0;
                        memmove(paNewSnapshots,
                                &pPrivate->paSnapshots[pPrivate->cSnapshots - ulNewClientCX],
                                ulNewClientCX * sizeof(SNAPSHOT));
                    }

                    pPrivate->cSnapshots = ulNewClientCX;
                    free(pPrivate->paSnapshots);
                    pPrivate->paSnapshots = paNewSnapshots;
                } // end if (pPrivate->palLoads)

                pPrivate->Setup.cx = pswpNew->cx;
                TwgtSaveSetupAndSend(hwnd, &pPrivate->Setup);
            } // end if (pswpNew->cx != pswpOld->cx)

            // force recreation of bitmap
            pPrivate->fUpdateGraph = TRUE;
            WinInvalidateRect(hwnd, NULL, FALSE);
        } // end if (pswpNew->fl & SWP_SIZE)
    } // end if (pPrivate)
}

/*
 *@@ TwgtPresParamChanged:
 *      implementation for WM_PRESPARAMCHANGED.
 *
 */

VOID TwgtPresParamChanged(HWND hwnd,
                          ULONG ulAttrChanged,
                          PXCENTERWIDGET pWidget)
{
    PWIDGETPRIVATE pPrivate = (PWIDGETPRIVATE)pWidget->pUser;
    if (pPrivate)
    {
        BOOL fInvalidate = TRUE;
        switch (ulAttrChanged)
        {
            case 0:     // layout palette thing dropped
            case PP_BACKGROUNDCOLOR:
            case PP_FOREGROUNDCOLOR:
                pPrivate->Setup.lcolBackground
                    = pwinhQueryPresColor(hwnd,
                                          PP_BACKGROUNDCOLOR,
                                          FALSE,
                                          SYSCLR_DIALOGBACKGROUND);
                pPrivate->Setup.lcolForeground
                    = pwinhQueryPresColor(hwnd,
                                          PP_FOREGROUNDCOLOR,
                                          FALSE,
                                          SYSCLR_WINDOWSTATICTEXT);
            break;

            case PP_FONTNAMESIZE:
            {
                // HPS hps;
                PSZ pszFont = 0;
                if (pPrivate->Setup.pszFont)
                {
                    free(pPrivate->Setup.pszFont);
                    pPrivate->Setup.pszFont = NULL;
                }

                if (pszFont = pwinhQueryWindowFont(hwnd))
                {
                    // we must use local malloc() for the font
                    pPrivate->Setup.pszFont = strdup(pszFont);
                    pwinhFree(pszFont);
                }

                // do not do this during WM_CREATE
                if (!pPrivate->fCreating)
                {
                    WinPostMsg(pWidget->pGlobals->hwndClient,
                               XCM_REFORMAT,
                               (MPARAM)XFMF_GETWIDGETSIZES,
                               0);
                }
            }
            break;

            default:
                fInvalidate = FALSE;
        }

        if (fInvalidate)
        {
            XSTRING strSetup;
            WinInvalidateRect(hwnd, NULL, FALSE);

            TwgtSaveSetup(&strSetup,
                          &pPrivate->Setup);
            if (strSetup.ulLength)
                // changed V0.9.13 (2001-06-21) [umoeller]:
                // post it to parent instead of fixed XCenter client
                // to make this trayable
                WinSendMsg(WinQueryWindow(hwnd, QW_PARENT), // pPrivate->pWidget->pGlobals->hwndClient,
                           XCM_SAVESETUP,
                           (MPARAM)hwnd,
                           (MPARAM)strSetup.psz);
            pxstrClear(&strSetup);
        }
    } // end if (pPrivate)
}

/*
 *@@ HackContextMenu:
 *
 */

VOID HackContextMenu(PWIDGETPRIVATE pPrivate)
{
    HWND hwndSubmenu;
    SHORT s = (SHORT)WinSendMsg(pPrivate->pWidget->hwndContextMenu,
                                MM_ITEMPOSITIONFROMID,
                                MPFROM2SHORT(ID_CRMI_PROPERTIES,
                                             FALSE),
                                0);
    if (hwndSubmenu = pwinhInsertSubmenu(pPrivate->pWidget->hwndContextMenu,
                                         s + 2,
                                         1999,
                                         "Source",
                                         MIS_TEXT,
                                         0,
                                         NULL,
                                         0,
                                         0))
    {
        ULONG i;
        for (i = 0; i < IFMIB_ENTRIES; i++)
        {
            _Pmpf(("pPrivate->statif.iftable[%d].ifDescr: %s",
                    i,
                    pPrivate->statif.iftable[i].ifDescr));

            if (pPrivate->statif.iftable[i].ifDescr[0])
            {
                pwinhInsertMenuItem(hwndSubmenu,
                                    MIT_END,
                                    2000 + i,
                                    pPrivate->statif.iftable[i].ifDescr,
                                    MIS_TEXT,
                                    (i == pPrivate->Setup.ulDevIndex)
                                        ? MIA_CHECKED
                                        : 0);
            }
        }

        pwinhInsertMenuItem(pPrivate->pWidget->hwndContextMenu,
                            s + 3,
                            0,
                            "",
                            MIS_SEPARATOR,
                            0);

        pPrivate->fContextMenuHacked = TRUE;
    }
}

/*
 *@@ fnwpMonitorWidgets:
 *      window procedure for the "Sentinel".
 *
 */

MRESULT EXPENTRY fnwpMonitorWidgets(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;
    PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr(hwnd, QWL_USER);
                    // this ptr is valid after WM_CREATE

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
         *      WIDGETPRIVATE for our own stuff.
         */

        case WM_CREATE:
            WinSetWindowPtr(hwnd, QWL_USER, mp1);
            pWidget = (PXCENTERWIDGET)mp1;
            if ((pWidget) && (pWidget->pfnwpDefWidgetProc))
                mrc = TwgtCreate(hwnd, pWidget);
            else
                // stop window creation!!
                mrc = (MPARAM)TRUE;
        break;

        /*
         * WM_CONTROL:
         *      process notifications/queries from the XCenter.
         */

        case WM_CONTROL:
            mrc = (MPARAM)TwgtControl(hwnd, mp1, mp2);
        break;

        /*
         * WM_PAINT:
         *
         */

        case WM_PAINT:
            TwgtPaint(hwnd);
        break;

        /*
         * WM_TIMER:
         *      clock timer --> repaint.
         */

        case WM_TIMER:
            TwgtTimer(hwnd);
        break;

        /*
         * WM_WINDOWPOSCHANGED:
         *      on window resize, allocate new bitmap.
         */

        case WM_WINDOWPOSCHANGED:
            TwgtWindowPosChanged(hwnd, mp1, mp2);
        break;

        /*
         * WM_PRESPARAMCHANGED:
         *
         */

        case WM_PRESPARAMCHANGED:
            if (pWidget)
                // this gets sent before this is set!
                TwgtPresParamChanged(hwnd, (ULONG)mp1, pWidget);
        break;

        case WM_CONTEXTMENU:
        {
            PWIDGETPRIVATE pPrivate;
            if (    (pWidget)
                 && (pPrivate = (PWIDGETPRIVATE)pWidget->pUser)
               )
            {
                if (    (pWidget->hwndContextMenu)
                     && (!pPrivate->fContextMenuHacked)
                   )
                {
                    // first call for diskfree:
                    // hack the context menu given to us
                    HackContextMenu(pPrivate);
                }

                mrc = pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
            }
        }
        break;

        case WM_COMMAND:
        {
            PWIDGETPRIVATE pPrivate;
            if (    (pPrivate = (PWIDGETPRIVATE)pWidget->pUser)
                 && ((SHORT)mp1) >= 2000
                 && ((SHORT)mp1) < 2000 + IFMIB_ENTRIES
               )
            {
                pPrivate->Setup.ulDevIndex = (SHORT)mp1 - 2000;
                TwgtSaveSetupAndSend(hwnd, &pPrivate->Setup);
            }
            else
                mrc = pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
        }
        break;


        /*
         * WM_DESTROY:
         *      clean up. This _must_ be passed on to
         *      ctrDefWidgetProc.
         */

        case WM_DESTROY:
        {
            PWIDGETPRIVATE pPrivate;
            if (pPrivate = (PWIDGETPRIVATE)pWidget->pUser)
            {
                if (pPrivate->ulTimerID)
                    ptmrStopXTimer(pPrivate->pWidget->pGlobals->pvXTimerSet,
                                   hwnd,
                                   pPrivate->ulTimerID);

                if (pPrivate->pBitmap)
                    pgpihDestroyXBitmap(&pPrivate->pBitmap);
                            // this was missing V0.9.12 (2001-05-20) [umoeller]

                if (pPrivate->paSnapshots)
                    free(pPrivate->paSnapshots);

                soclose(pPrivate->sock);

                free(pPrivate);
            } // end if (pPrivate)
            mrc = pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
        }
        break;

        default:
            if (pWidget)
                mrc = pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
            else
                mrc = WinDefWindowProc(hwnd, msg, mp1, mp2);
    } // end switch(msg)

    return (mrc);
}

/* ******************************************************************
 *
 *   Exported procedures
 *
 ********************************************************************/

/*
 *@@ TwgtInitModule:
 *      required export with ordinal 1, which must tell
 *      the XCenter how many widgets this DLL provides,
 *      and give the XCenter an array of XCENTERWIDGETCLASS
 *      structures describing the widgets.
 *
 *      With this call, you are given the module handle of
 *      XFLDR.DLL. For convenience, you may resolve imports
 *      for some useful functions which are exported thru
 *      src\shared\xwp.def. See the code below.
 *
 *      This function must also register the PM window classes
 *      which are specified in the XCENTERWIDGETCLASS array
 *      entries. For this, you are given a HAB which you
 *      should pass to WinRegisterClass. For the window
 *      class style (4th param to WinRegisterClass),
 *      you should specify
 *
 +          CS_PARENTCLIP | CS_SIZEREDRAW | CS_SYNCPAINT
 *
 *      Your widget window _will_ be resized, even if you're
 *      not planning it to be.
 *
 *      This function only gets called _once_ when the widget
 *      DLL has been successfully loaded by the XCenter. If
 *      there are several instances of a widget running (in
 *      the same or in several XCenters), this function does
 *      not get called again. However, since the XCenter unloads
 *      the widget DLLs again if they are no longer referenced
 *      by any XCenter, this might get called again when the
 *      DLL is re-loaded.
 *
 *      There will ever be only one load occurence of the DLL.
 *      The XCenter manages sharing the DLL between several
 *      XCenters. As a result, it doesn't matter if the DLL
 *      has INITINSTANCE etc. set or not.
 *
 *      If this returns 0, this is considered an error, and the
 *      DLL will be unloaded again immediately.
 *
 *      If this returns any value > 0, *ppaClasses must be
 *      set to a static array (best placed in the DLL's
 *      global data) of XCENTERWIDGETCLASS structures,
 *      which must have as many entries as the return value.
 *
 */

ULONG EXPENTRY TwgtInitModule(HAB hab,         // XCenter's anchor block
                              HMODULE hmodPlugin, // module handle of the widget DLL
                              HMODULE hmodXFLDR,    // XFLDR.DLL module handle
                              PCXCENTERWIDGETCLASS *ppaClasses,
                              PSZ pszErrorMsg)  // if 0 is returned, 500 bytes of error msg
{
    ULONG   ulrc = 0,
            ul = 0;
    BOOL    fImportsFailed = FALSE;

    // resolve imports from XFLDR.DLL (this is basically
    // a copy of the doshResolveImports code, but we can't
    // use that before resolving...)
    for (ul = 0;
         ul < sizeof(G_aImports) / sizeof(G_aImports[0]); // array item count
         ul++)
    {
        if (DosQueryProcAddr(hmodXFLDR,
                             0,               // ordinal, ignored
                             (PSZ)G_aImports[ul].pcszFunctionName,
                             G_aImports[ul].ppFuncAddress)
                    != NO_ERROR)
        {
            strcpy(pszErrorMsg, "Import ");
            strcat(pszErrorMsg, G_aImports[ul].pcszFunctionName);
            strcat(pszErrorMsg, " failed.");
            fImportsFailed = TRUE;
            break;
        }
    }

    if (!fImportsFailed)
    {
        if (!WinRegisterClass(hab,
                              WNDCLASS_WIDGET_IPMON,
                              fnwpMonitorWidgets,
                              CS_PARENTCLIP | CS_SIZEREDRAW | CS_SYNCPAINT,
                              sizeof(PWIDGETPRIVATE))
                                    // extra memory to reserve for QWL_USER
                            )
            strcpy(pszErrorMsg, "WinRegisterClass failed.");
        else
        {
            // no error:
            // return classes
            *ppaClasses = G_WidgetClasses;
            // no. of classes in this DLL:
            ulrc = sizeof(G_WidgetClasses) / sizeof(G_WidgetClasses[0]);
        }
    }

    return (ulrc);
}

/*
 *@@ TwgtUnInitModule:
 *      optional export with ordinal 2, which can clean
 *      up global widget class data.
 *
 *      This gets called by the XCenter right before
 *      a widget DLL gets unloaded. Note that this
 *      gets called even if the "init module" export
 *      returned 0 (meaning an error) and the DLL
 *      gets unloaded right away.
 */

VOID EXPENTRY TwgtUnInitModule(VOID)
{
}

/*
 *@@ TwgtQueryVersion:
 *      this new export with ordinal 3 can return the
 *      XWorkplace version number which is required
 *      for this widget to run. For example, if this
 *      returns 0.9.10, this widget will not run on
 *      earlier XWorkplace versions.
 *
 *      NOTE: This export was mainly added because the
 *      prototype for the "Init" export was changed
 *      with V0.9.9. If this returns 0.9.9, it is
 *      assumed that the INIT export understands
 *      the new FNWGTINITMODULE_099 format (see center.h).
 */

VOID EXPENTRY TwgtQueryVersion(PULONG pulMajor,
                               PULONG pulMinor,
                               PULONG pulRevision)
{
    *pulMajor = XFOLDER_MAJOR;              // dlgids.h
    *pulMinor = XFOLDER_MINOR;
    *pulRevision = XFOLDER_REVISION;
}

