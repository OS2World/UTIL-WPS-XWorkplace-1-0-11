
/*
 *@@sourcefile xwpnetwork.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPNetwork: the new network browser folder, which
 *          only contains XWPNetServer instances (which are
 *          transients).
 *
 *@@somclass XWPNetwork xnw_
 *@@somclass M_XWPNetwork xnwM_
 */

/*
 *      Copyright (C) 2001-2003 Ulrich M�ller.
 *
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

#ifndef SOM_Module_xwpnetwork_Source
#define SOM_Module_xwpnetwork_Source
#endif
#define XWPNetwork_Class_Source
#define M_XWPNetwork_Class_Source

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
#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSERRORS
#define INCL_WINPOINTERS
#define INCL_WINMENUS
#include <os2.h>

#include <netcons.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\lan.h"

// SOM headers which don't crash with prec. header files
#include "xwpnetsrv.ih"
#include "xwpnetwork.ih"

// XWorkplace implementation headers
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs
#include "shared\kernel.h"              // XWorkplace Kernel

// other SOM headers
#pragma hdrstop

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

// default trash can
static XWPNetwork *G_pNetworkFolder = NULL;

/* ******************************************************************
 *
 *   XWPNetwork instance methods
 *
 ********************************************************************/

/*
 *@@ wpInitData:
 *
 */

SOM_Scope void  SOMLINK xnw_wpInitData(XWPNetwork *somSelf)
{
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpInitData");

    XWPNetwork_parent_WPFolder_wpInitData(somSelf);

    _fAlreadyPopulated = FALSE;
}

/*
 *@@ wpPopulate:
 *
 */

SOM_Scope BOOL  SOMLINK xnw_wpPopulate(XWPNetwork *somSelf, ULONG ulReserved,
                                       PSZ pszPath, BOOL fFoldersOnly)
{
    BOOL brc;
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpPopulate");

    if (    (brc = XWPNetwork_parent_WPFolder_wpPopulate(somSelf,
                                                         ulReserved,
                                                         pszPath,
                                                         fFoldersOnly))
         && (!_fAlreadyPopulated)
       )
    {
        PSERVER paServers;
        ULONG   cServers;
        APIRET  arc;
        if (!(arc = lanQueryServers(&paServers,
                                    &cServers)))
        {
            ULONG ul;
            for (ul = 0;
                 ul < cServers;
                 ul++)
            {
                CHAR szSetup[1000];
                XWPNetServer *pServer;

                PSZ pszTitle;

                if (!(pszTitle = paServers[ul].pszComment))
                    // no comment available:
                    // use server name then
                    pszTitle = paServers[ul].achServerName;

                sprintf(szSetup,
                        "SERVERNAME=%s",
                        paServers[ul].achServerName);
                pServer = _wpclsNew(_XWPNetServer,
                                    pszTitle,
                                    szSetup,
                                    somSelf,        // folder
                                    TRUE);          // lock
            }
        }
        else
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "lanQueryServers returned %d",
                   arc);

        _fAlreadyPopulated = TRUE;
    }

    return brc;
}

/*
 *@@ wpRefresh:
 *
 */

SOM_Scope BOOL  SOMLINK xnw_wpRefresh(XWPNetwork *somSelf, ULONG ulView,
                                      PVOID pReserved)
{
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpRefresh");

    return (XWPNetwork_parent_WPFolder_wpRefresh(somSelf, ulView,
                                                 pReserved));
}

/*
 *@@ wpAddToContent:
 *
 */

SOM_Scope BOOL  SOMLINK xnw_wpAddToContent(XWPNetwork *somSelf,
                                           WPObject* Object)
{
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpAddToContent");

    return (XWPNetwork_parent_WPFolder_wpAddToContent(somSelf,
                                                      Object));
}

/*
 *@@ wpDeleteFromContent:
 *
 */

SOM_Scope BOOL  SOMLINK xnw_wpDeleteFromContent(XWPNetwork *somSelf,
                                                WPObject* Object)
{
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpDeleteFromContent");

    return (XWPNetwork_parent_WPFolder_wpDeleteFromContent(somSelf,
                                                           Object));
}

/*
 *@@ wpDeleteContents:
 *
 */

SOM_Scope ULONG  SOMLINK xnw_wpDeleteContents(XWPNetwork *somSelf,
                                              ULONG fConfirmations)
{
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpDeleteContents");

    return (XWPNetwork_parent_WPFolder_wpDeleteContents(somSelf,
                                                        fConfirmations));
}

/*
 *@@ wpDragOver:
 *
 */

SOM_Scope MRESULT  SOMLINK xnw_wpDragOver(XWPNetwork *somSelf,
                                          HWND hwndCnr, PDRAGINFO pdrgInfo)
{
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpDragOver");

    return (XWPNetwork_parent_WPFolder_wpDragOver(somSelf, hwndCnr,
                                                  pdrgInfo));
}

/*
 *@@ wpDrop:
 *
 */

SOM_Scope MRESULT  SOMLINK xnw_wpDrop(XWPNetwork *somSelf, HWND hwndCnr,
                                      PDRAGINFO pdrgInfo, PDRAGITEM pdrgItem)
{
    XWPNetworkData *somThis = XWPNetworkGetData(somSelf);
    XWPNetworkMethodDebug("XWPNetwork","xnw_wpDrop");

    return (XWPNetwork_parent_WPFolder_wpDrop(somSelf, hwndCnr,
                                              pdrgInfo, pdrgItem));
}


/* ******************************************************************
 *
 *   XWPNetwork class methods
 *
 ********************************************************************/

/*
 *@@ xwpclsQueryNetworkFolder:
 *
 */

SOM_Scope XWPNetwork*  SOMLINK xnwM_xwpclsQueryNetworkFolder(M_XWPNetwork *somSelf)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_xwpclsQueryNetworkFolder");

    return NULL;
}

/*
 *@@ wpclsInitData:
 *      this WPObject class method gets called when a class
 *      is loaded by the WPS (probably from within a
 *      somFindClass call) and allows the class to initialize
 *      itself.
 */

SOM_Scope void  SOMLINK xnwM_wpclsInitData(M_XWPNetwork *somSelf)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsInitData");

    M_XWPNetwork_parent_M_WPFolder_wpclsInitData(somSelf);

    if (krnClassInitialized(G_pcszXWPNetwork))
    {
        // first call:
        // enforce initialization of XWPNetServer class
        SOMClass *pClassObj;
        if (pClassObj = XWPNetServerNewClass(XWPNetServer_MajorVersion,
                                             XWPNetServer_MinorVersion))
        {
            // now increment the class's usage count by one to
            // ensure that the class is never unloaded; if we
            // didn't do this, we'd get WPS CRASHES in some
            // background class because if no more objects
            // exist, the class would get unloaded automatically -- sigh...
            _wpclsIncUsage(pClassObj);
        }
        else
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "Cannot initialize XWPNetServer class. Is it installed?!?");
    }
}

/*
 *@@ wpclsUnInitData:
 *
 */

SOM_Scope void  SOMLINK xnwM_wpclsUnInitData(M_XWPNetwork *somSelf)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsUnInitData");

    _wpclsDecUsage(_XWPNetServer);

    M_XWPNetwork_parent_M_WPFolder_wpclsUnInitData(somSelf);
}

/*
 *@@ wpclsCreateDefaultTemplates:
 *
 */

SOM_Scope BOOL  SOMLINK xnwM_wpclsCreateDefaultTemplates(M_XWPNetwork *somSelf,
                                                         WPObject* Folder)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsCreateDefaultTemplates");

    // pretend we've created the templates
    return TRUE;
}

/*
 *@@ wpclsQueryTitle:
 *
 */

SOM_Scope PSZ  SOMLINK xnwM_wpclsQueryTitle(M_XWPNetwork *somSelf)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsQueryTitle");

    return "Network";     // @@todo localize
}

/*
 *@@ wpclsQueryStyle:
 *
 *@@changed V0.9.16 (2001-11-25) [umoeller]: added nevertemplate
 */

SOM_Scope ULONG  SOMLINK xnwM_wpclsQueryStyle(M_XWPNetwork *somSelf)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsQueryStyle");

    return (CLSSTYLE_NEVERTEMPLATE      // V0.9.16 (2001-11-25) [umoeller]
                | CLSSTYLE_NEVERCOPY    // but allow move
                | CLSSTYLE_NEVERDELETE
                | CLSSTYLE_NEVERPRINT);
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

SOM_Scope BOOL  SOMLINK xnwM_wpclsQueryDefaultHelp(M_XWPNetwork *somSelf,
                                                   PULONG pHelpPanelId,
                                                   PSZ pszHelpLibrary)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsQueryDefaultHelp");

    strcpy(pszHelpLibrary, cmnQueryHelpLibrary());
    *pHelpPanelId = ID_XSH_XWPNETWORK_MAIN;
    return TRUE;
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
 */

SOM_Scope ULONG  SOMLINK xnwM_wpclsQueryIconData(M_XWPNetwork *somSelf,
                                                 PICONINFO pIconInfo)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsQueryIconData");

    /* if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXWPTRASHEMPTY;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return (sizeof(ICONINFO)); */

    return (M_XWPNetwork_parent_M_WPFolder_wpclsQueryIconData(somSelf,
                                                              pIconInfo));
}

/*
 *@@ wpclsQueryIconDataN:
 *
 */

SOM_Scope ULONG  SOMLINK xnwM_wpclsQueryIconDataN(M_XWPNetwork *somSelf,
                                                  ICONINFO* pIconInfo,
                                                  ULONG ulIconIndex)
{
    /* M_XWPNetworkData *somThis = M_XWPNetworkGetData(somSelf); */
    M_XWPNetworkMethodDebug("M_XWPNetwork","xnwM_wpclsQueryIconDataN");

    /* if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXWPTRASHEMPTY;
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return (sizeof(ICONINFO)); */

    return (M_XWPNetwork_parent_M_WPFolder_wpclsQueryIconDataN(somSelf,
                                                               pIconInfo,
                                                               ulIconIndex));
}

