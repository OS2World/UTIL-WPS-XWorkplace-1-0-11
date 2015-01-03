
/*
 *@@sourcefile xfontfile.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPFontObject: a subclass of WPDataFile.
 *
 *@@added V0.9.7 (2001-01-12) [umoeller]
 *@@somclass XWPFontFile fonf_
 *@@somclass M_XWPFontFile fonfM_
 */

/*
 *      Copyright (C) 2001-2015 Ulrich M�ller.
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

#ifndef SOM_Module_xfontfile_Source
#define SOM_Module_xfontfile_Source
#endif
#define XWPFontFile_Class_Source
#define M_XWPFontFile_Class_Source

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

#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSERRORS

#define INCL_WINSHELLDATA
#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\nls.h"                // National Language Support helpers
#include "helpers\prfh.h"               // INI file helper routines

// SOM headers which don't crash with prec. header files
#include "xfontfile.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs
#include "shared\kernel.h"              // XWorkplace Kernel

#include "config\fonts.h"               // font folder implementation

// other SOM headers
#pragma hdrstop

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

static const char *G_pcszFontFileFilter = "*.OFM,*.AFM,*.FON,*.TTF,*.TTC,*.OTF";

/* ******************************************************************
 *
 *   XWPFontFile instance methods
 *
 ********************************************************************/

/*
 *@@ xwpIsInstalled:
 *      returns TRUE if the font is currently installed.
 *
 *@@added V0.9.20 (2002-07-25) [umoeller]
 */

SOM_Scope BOOL  SOMLINK fonf_xwpIsInstalled(XWPFontFile *somSelf)
{
    CHAR szFilename[CCHMAXPATH];

    /* XWPFontFileData *somThis = XWPFontFileGetData(somSelf); */
    XWPFontFileMethodDebug("XWPFontFile","fonf_xwpIsInstalled");

    // no matter what the reason is that we've been created,
    // we check whether this font file is installed in OS2.INI...
    if (_wpQueryFilename(somSelf,
                         szFilename,
                         FALSE))        // not qualified... we need the key in "PM_Fonts"
    {
        ULONG cb = 0;
        nlsUpper(szFilename);
        if (    (PrfQueryProfileSize(HINI_USER,
                                     (PSZ)PMINIAPP_FONTS, // "PM_Fonts",
                                     szFilename,
                                     &cb))
             && (cb)
           )
        {
            // yes, we exist:
            return TRUE;
        }
    }

    return FALSE;
}

/* ******************************************************************
 *
 *   XWPFontFile class methods
 *
 ********************************************************************/

/*
 *@@ wpclsInitData:
 *      this M_WPObject class method gets called when a class
 *      is loaded by the WPS (probably from within a
 *      somFindClass call) and allows the class to initialize
 *      itself.
 */

SOM_Scope void  SOMLINK fonfM_wpclsInitData(M_XWPFontFile *somSelf)
{
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsInitData");

    M_XWPFontFile_parent_M_XFldDataFile_wpclsInitData(somSelf);

    krnClassInitialized(G_pcszXWPFontFile);
}

/*
 *@@ wpclsCreateDefaultTemplates:
 *      this M_WPObject class method is called by the
 *      Templates folder to allow a class to
 *      create its default templates.
 *
 *      The default WPS behavior is to create new templates
 *      if the class default title is different from the
 *      existing templates.
 *
 *      Since we never want templates for font objects,
 *      we'll have to suppress this behavior.
 */

SOM_Scope BOOL  SOMLINK fonfM_wpclsCreateDefaultTemplates(M_XWPFontFile *somSelf,
                                                          WPObject* Folder)
{
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsCreateDefaultTemplates");

    return TRUE;
    // means that the Templates folder should _not_ create templates
    // by itself; we pretend that we've done this
}

/*
 *@@ wpclsQueryTitle:
 *      this M_WPObject class method tells the WPS the clear
 *      name of a class, which is shown in the third column
 *      of a Details view and also used as the default title
 *      for new objects of a class.
 */

SOM_Scope PSZ  SOMLINK fonfM_wpclsQueryTitle(M_XWPFontFile *somSelf)
{
    // PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsQueryTitle");

    return (PSZ)cmnGetString(ID_XSSI_FONTFILE);
}

/*
 *@@ wpclsQueryStyle:
 *
 *@@changed V0.9.16 (2001-11-25) [umoeller]: added nevertemplate
 *@@changed V0.9.20 (2002-07-25) [umoeller]: removed CLSSTYLE_NEVERCOPY
 */

SOM_Scope ULONG  SOMLINK fonfM_wpclsQueryStyle(M_XWPFontFile *somSelf)
{
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsQueryStyle");

    return (CLSSTYLE_NEVERTEMPLATE      // V0.9.16 (2001-11-25) [umoeller]
                // | CLSSTYLE_NEVERCOPY  bullshit V0.9.20 (2002-07-25) [umoeller]
                | CLSSTYLE_NEVERDROPON
                | CLSSTYLE_NEVERPRINT);
}

/*
 *@@ wpclsQueryDefaultHelp:
 *      this M_WPObject class method returns the default help
 *      panel for objects of this class. This gets called
 *      from WPObject::wpQueryDefaultHelp if no instance
 *      help settings (HELPLIBRARY, HELPPANEL) have been
 *      set for an individual object. It is thus recommended
 *      to override this method instead of the instance
 *      method to change the default help panel for a class
 *      in order not to break instance help settings (fixed
 *      with 0.9.20).
 *
 *      We return a new default help panel for font files
 *      because the standard data file help doesn't make all
 *      that much sense for them.
 *
 *@@added V0.9.20 (2002-07-12) [umoeller]
 */

SOM_Scope BOOL  SOMLINK fonfM_wpclsQueryDefaultHelp(M_XWPFontFile *somSelf,
                                                    PULONG pHelpPanelId,
                                                    PSZ pszHelpLibrary)
{
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsQueryDefaultHelp");

    strcpy(pszHelpLibrary, cmnQueryHelpLibrary());
    *pHelpPanelId = ID_XSH_FONTFILE;
    return TRUE;
}

/*
 *@@ wpclsQueryIconData:
 *      this M_WPObject class method must return information
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
 *      We give this class a new standard icon here.
 *
 *@@changed V0.9.16 (2001-11-25) [umoeller]: now using separate icon for font _files_
 */

SOM_Scope ULONG  SOMLINK fonfM_wpclsQueryIconData(M_XWPFontFile *somSelf,
                                                  PICONINFO pIconInfo)
{
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsQueryIconData");

    if (pIconInfo)
    {
        pIconInfo->fFormat = ICON_RESOURCE;
        pIconInfo->resid   = ID_ICONXWPFONTFILE;
                // V0.9.16 (2001-11-25) [umoeller]
        pIconInfo->hmod    = cmnQueryMainResModuleHandle();
    }

    return sizeof(ICONINFO);
}


/*
 *@@ wpclsQueryInstanceFilter:
 *      this M_WPFileSystem class method determines which file-system
 *      objects will be instances of a certain class according
 *      to a file filter.
 *
 *      We return filters for what we consider font files:
 *
 *      -- *.OFM,*.AFM: Type 1 metric descriptions (OS/2-binary and ASCII
 *         format, respectively). Only these can be installed. Note that
 *         installed AFM files will be converted to OFM by OS/2.
 *
 *      -- *.TTF, *.TTC: TrueType fonts.
 *
 *      -- *.OTF: OpenType fonts.
 *
 *      -- *.FON: bitmap font files (actually, DLLs containing font
 *         resources).
 */

SOM_Scope PSZ  SOMLINK fonfM_wpclsQueryInstanceFilter(M_XWPFontFile *somSelf)
{
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsQueryInstanceFilter");

    // return (M_XWPFontFile_parent_M_WPDataFile_wpclsQueryInstanceFilter(somSelf));

    return (PSZ)G_pcszFontFileFilter;
}

SOM_Scope PSZ  SOMLINK fonfM_wpclsQueryInstanceType(M_XWPFontFile *somSelf)
{
    /* M_XWPFontFileData *somThis = M_XWPFontFileGetData(somSelf); */
    M_XWPFontFileMethodDebug("M_XWPFontFile","fonfM_wpclsQueryInstanceType");

    return "Font file";
}
