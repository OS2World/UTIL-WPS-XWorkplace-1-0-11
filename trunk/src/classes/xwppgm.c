
/*
 *  This file was generated by the SOM Compiler.
 *  Generated using:
 *     SOM incremental update: 2.41
 */


/*
 *@@sourcefile xwppgm.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPProgram (WPProgram replacement)
 *
 *      XFldProgram is only responsible for overriding the
 *      "Associations" page at this point.
 *
 *      Installation of this class is optional.
 *
 *@@added V0.9.9 (2001-04-02) [umoeller]
 *@@somclass XWPProgram xpgm_
 *@@somclass M_XWPProgram xpgmM_
 */

/*
 *      Copyright (C) 2001 Ulrich M�ller.
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

#ifndef SOM_Module_xwppgm_Source
#define SOM_Module_xwppgm_Source
#endif
#define XWPProgram_Class_Source
#define M_XWPProgram_Class_Source

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

#define INCL_DOSSESMGR          // DosQueryAppType
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS

#define INCL_WINPOINTERS
#define INCL_WINPROGRAMLIST     // needed for PROGDETAILS, wppgm.h
#include <os2.h>

// C library headers
#include <stdio.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\dosh.h"
#include "helpers\standards.h"          // some standard macros
#include "helpers\winh.h"

// SOM headers which don't crash with prec. header files
#include "xfobj.ih"
#include "xfdataf.ih"
#include "xwppgm.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)

#include "filesys\filesys.h"            // various file-system object implementation code
#include "filesys\filetype.h"           // extended file types implementation
#include "filesys\program.h"            // program implementation

#pragma hdrstop                         // VAC++ keeps crashing otherwise

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

const char *G_pcszXWPProgram = "XWPProgram";

/* ******************************************************************
 *
 *   XWPProgram instance methods
 *
 ********************************************************************/

/*
 *@@ xwpAddAssociationsPage:
 *      this new XWPProgram method adds our replacement
 *      "Associations" page to an executable's settings notebook.
 *
 *      Gets called from our
 *      XWPProgram::wpAddProgramAssociationPage override,
 *      if extended associations are enabled.
 */

SOM_Scope ULONG  SOMLINK xpgm_xwpAddAssociationsPage(XWPProgram *somSelf,
                                                     HWND hwndNotebook)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xwppgm_xwpAddAssociationsPage");

    return (ftypInsertAssociationsPage(somSelf,
                                       hwndNotebook));
}

/*
 *@@ xwpQuerySetup2:
 *      this XFldObject method is overridden to support
 *      setup strings for program files.
 *
 *      This uses code in filesys\filesys.c which is
 *      shared for WPProgram and WPProgramFile instances.
 *
 *      See XFldObject::xwpQuerySetup2 for details.
 */

SOM_Scope ULONG  SOMLINK xpgm_xwpQuerySetup2(XWPProgram *somSelf,
                                             PSZ pszSetupString,
                                             ULONG cbSetupString)
{
    ULONG ulReturn = 0;

    // method pointer for parent class
    somTD_XFldObject_xwpQuerySetup pfn_xwpQuerySetup2 = 0;

    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xwppgm_xwpQuerySetup2");

    // call implementation
    ulReturn = fsysQueryProgramFileSetup(somSelf, pszSetupString, cbSetupString);

    // manually resolve parent method
    pfn_xwpQuerySetup2
        = (somTD_XFldObject_xwpQuerySetup)wpshResolveFor(somSelf,
                                                         _somGetParent(_XWPProgram),
                                                         "xwpQuerySetup2");
    if (pfn_xwpQuerySetup2)
    {
        // now call XFldObject method
        if ( (pszSetupString) && (cbSetupString) )
            // string buffer already specified:
            // tell XFldObject to append to that string
            ulReturn += pfn_xwpQuerySetup2(somSelf,
                                           pszSetupString + ulReturn, // append to existing
                                           cbSetupString - ulReturn); // remaining size
        else
            // string buffer not yet specified: return length only
            ulReturn += pfn_xwpQuerySetup2(somSelf, 0, 0);
    }

    return (ulReturn);
}

/*
 *@@ xwpNukePhysical:
 *      override of XFldObject::xwpNukePhysical, which must
 *      remove the physical representation of an object
 *      when it gets physically deleted.
 *
 *      xwpNukePhysical gets called by name from
 *      XFldObject::wpFree. The default XFldObject::xwpNukePhysical
 *      calls WPObject::wpDestroyObject.
 *
 *      We override this in order to prevent the original
 *      WPProgram::wpDestroyObject to be called, which messes
 *      wrongly with our association data. Instead,
 *      we destroy any association data in the OS2.INI
 *      file ourselves and them jump directly to
 *      WPAbstract::wpDestroyObject.
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_xwpNukePhysical(XWPProgram *somSelf)
{
    static xfTD_wpDestroyObject pWPAbstract_wpDestroyObject = NULL;

    BOOL brc = FALSE;
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_xwpNukePhysical");

    if (!pWPAbstract_wpDestroyObject)
    {
        // first call:
        // resolve WPAbstract::wpDestroyObject
        // (skip WPProgram parent call!)
        pWPAbstract_wpDestroyObject
            = (xfTD_wpDestroyObject)wpshResolveFor(somSelf,
                                                   _WPAbstract,
                                                   "wpDestroyObject");
    }

    if (pWPAbstract_wpDestroyObject)
    {
        // clean up program resources in INI file;
        // there's no way to avoid running through
        // the entire handles cache, unfortunately...
        // _wpQueryHandle is safe, since each WPProgram
        // must have one per definition
        ftypAssocObjectDeleted(_wpQueryHandle(somSelf));

        // call WPAbstract::wpDestroyObject explicitly,
        // skipping WPProgram
        brc = pWPAbstract_wpDestroyObject(somSelf);
    }
    else
        cmnLog(__FILE__, __LINE__, __FUNCTION__,
               "Cannot resolve WPAbstract::wpDestroyObject.");

    return (brc);
}

/*
 *@@ wpInitData:
 *      this WPObject instance method gets called when the
 *      object is being initialized (on wake-up or creation).
 *      We initialize our additional instance data here.
 *      Always call the parent method first.
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope void  SOMLINK xpgm_wpInitData(XWPProgram *somSelf)
{
    XWPProgramData *somThis = XWPProgramGetData(somSelf);
    XWPProgramMethodDebug("XWPProgram","xpgm_wpInitData");

    XWPProgram_parent_WPProgram_wpInitData(somSelf);

    // clear all the pointers
    memset(somThis, 0, sizeof(*somThis));
}

/*
 *@@ wpUnInitData:
 *      this WPObject instance method is called when the object
 *      is destroyed as a SOM object, either because it's being
 *      made dormant or being deleted. All allocated resources
 *      should be freed here.
 *      The parent method must always be called last.
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope void  SOMLINK xpgm_wpUnInitData(XWPProgram *somSelf)
{
    XWPProgramData *somThis = XWPProgramGetData(somSelf);
    XWPProgramMethodDebug("XWPProgram","xpgm_wpUnInitData");

    if (_pvStringArray)
    {
        free(_pvStringArray);
        _pvStringArray = NULL;
    }

    XWPProgram_parent_WPProgram_wpUnInitData(somSelf);
}

/*
 *@@ wpSaveState:
 *      this WPObject instance method saves an object's state
 *      persistently so that it can later be re-initialized
 *      with wpRestoreState. This gets called during wpClose,
 *      wpSaveImmediate or wpSaveDeferred processing.
 *      All persistent instance variables should be stored here.
 *
 *@@added V0.9.12 (2001-05-26) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_wpSaveState(XWPProgram *somSelf)
{
    BOOL brc;
    XWPProgramData *somThis = XWPProgramGetData(somSelf);
    XWPProgramMethodDebug("XWPProgram","xpgm_wpSaveState");

    brc = XWPProgram_parent_WPProgram_wpSaveState(somSelf);

    return (brc);
}

/*
 *@@ wpRestoreState:
 *      this WPObject instance method gets called during object
 *      initialization (after wpInitData) to restore the data
 *      which was stored with wpSaveState.
 *
 *@@added V0.9.12 (2001-05-26) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_wpRestoreState(XWPProgram *somSelf,
                                            ULONG ulReserved)
{
    BOOL brc;
    XWPProgramData *somThis = XWPProgramGetData(somSelf);
    XWPProgramMethodDebug("XWPProgram","xpgm_wpRestoreState");

    brc = XWPProgram_parent_WPProgram_wpRestoreState(somSelf,
                                                     ulReserved);

    return (brc);
}

/*
 *@@ wpRestoreData:
 *      this WPObject instance method restores a chunk
 *      of binary instance data which was previously
 *      saved by WPObject::wpSaveData.
 *
 *      This may only be used while WPObject::wpRestoreState
 *      is being processed.
 *
 *      This method normally isn't designed to be overridden.
 *      However, since this gets called by WPProgram::wpRestoreState,
 *      we override this method to be able to intercept pointers
 *      to the WPProgram instance data, which we cannot access
 *      otherwise. We can store these pointers in the XWPProgram
 *      instance data then and read/write the WPProgram instance
 *      data this way.
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_wpRestoreData(XWPProgram *somSelf,
                                           PSZ pszClass,
                                           ULONG ulKey,
                                           PBYTE pValue,
                                           PULONG pcbValue)
{
    BOOL brc,
         fMakeCopy = FALSE;
    XWPProgramData *somThis = XWPProgramGetData(somSelf);
    XWPProgramMethodDebug("XWPProgram","xpgm_wpRestoreData");

    // intercept pointer to internal WPProgram data
    if (!strcmp(pszClass, "WPProgramRef"))
    {
        switch (ulKey)
        {
            case 6:             // internal WPProgram key for environment
                                // string array
                // note, this comes in twice, first call has
                // pValue == NULL to query the size of the data
                if (pValue)
                    _pvpszEnvironment = pValue;
            break;

            case 7:             // internal WPProgram key for SWP
                _pSWPInitial = pValue;
            break;

            case 10:        // internal WPProgram key for array of strings
                // this is tricky, because in this case we won't have
                // a pointer to WPProgram data... seems like a temporary
                // stack pointer. Restore the chunk and make a copy
                // of the entire data in our own instance data instead.
                fMakeCopy = TRUE;
            break;

            case 11:        // internal WPProgram key for array of LONG values
                _pvProgramLongs = pValue;

                // apparently, this points to SIX LONG values then:
                // 1) executable fsh
                // 2) startup dir fsh
                // 3) the other four are unknown to me
            break;
       }
    }

    brc = XWPProgram_parent_WPProgram_wpRestoreData(somSelf,
                                                    pszClass,
                                                    ulKey,
                                                    pValue,
                                                    pcbValue);

    if ((brc) && (fMakeCopy) && (pcbValue) && (*pcbValue))
    {
        if (_pvStringArray)
            free(_pvStringArray);
        _cbStringArray = *pcbValue;
        _pvStringArray = malloc(_cbStringArray);
        memcpy(_pvStringArray, pValue, _cbStringArray);
    }

    return (brc);
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
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope HWND  SOMLINK xpgm_wpOpen(XWPProgram *somSelf, HWND hwndCnr,
                                    ULONG ulView, ULONG param)
{
    XWPProgramData *somThis = XWPProgramGetData(somSelf);
    XWPProgramMethodDebug("XWPProgram","xpgm_wpOpen");

    _Pmpf((__FUNCTION__ ": _pvProgramLongs is 0x%lX", _pvProgramLongs));
    if (_pvProgramLongs)
    {
        CHAR sz[CCHMAXPATH] = "unknown";
        HOBJECT hobj = *(PULONG)_pvProgramLongs;
        if (hobj)
        {
            WPObject *pobj = _wpclsQueryObject(_WPObject,
                                               hobj | 0x30000);
            if (pobj)
                _wpQueryFilename(pobj, sz, TRUE);
        }
        _Pmpf(("  executable: hobj = 0x%lX, \"%s\"", hobj, sz));

    }

    _Pmpf((__FUNCTION__ ": _pvStringArray is 0x%lX, cb %d",
                        _pvStringArray, _cbStringArray));
    if (_pvStringArray && _cbStringArray)
    {
        // WPS uses a very strange format for the string array:
        // each string starts with a USHORT string _index_,
        // followed by the null-terminated string; the last
        // string is marked with a USHORT 0xFFFF index
        PBYTE pThis = _pvStringArray;
        USHORT usIndexThis;

        while (0xFFFF != (usIndexThis = *(PUSHORT)pThis))
        {
            pThis += sizeof(USHORT);

            _Pmpf(("  string %d: \"%s\"", usIndexThis, pThis));

            pThis += strlen(pThis) + 1;
        }
    }

    _Pmpf((__FUNCTION__ ": _pvpszEnvironment is 0x%lX",
                        _pvpszEnvironment));
    if (_pvpszEnvironment)
    {
        PSZ pszThis = _pvpszEnvironment;
        while (*pszThis != 0)
        {
            _Pmpf(("  \"%s\"", pszThis));
            pszThis += strlen(pszThis) + 1;
        }
    }

    /* if (ulView == OPEN_RUNNING)
        return (progOpenProgram(somSelf,
                                NULL,           // no data file argument
                                0));            // no menu ID
       */

    return (XWPProgram_parent_WPProgram_wpOpen(somSelf, hwndCnr,
                                               ulView, param));
}

/*
 *@@ wpQueryIcon:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope HPOINTER  SOMLINK xpgm_wpQueryIcon(XWPProgram *somSelf)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpQueryIcon");

    return (XWPProgram_parent_WPProgram_wpQueryIcon(somSelf));
}

/*
 *@@ wpSetProgIcon:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_wpSetProgIcon(XWPProgram *somSelf,
                                           PFEA2LIST pfeal)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpSetProgIcon");

    return (XWPProgram_parent_WPProgram_wpSetProgIcon(somSelf,
                                                      pfeal));
}

/*
 *@@ wpQueryIconData:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope ULONG  SOMLINK xpgm_wpQueryIconData(XWPProgram *somSelf,
                                              PICONINFO pIconInfo)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpQueryIconData");

    return (XWPProgram_parent_WPProgram_wpQueryIconData(somSelf,
                                                        pIconInfo));
}

/*
 *@@ wpSetIconData:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_wpSetIconData(XWPProgram *somSelf,
                                           PICONINFO pIconInfo)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpSetIconData");

    return (XWPProgram_parent_WPProgram_wpSetIconData(somSelf,
                                                      pIconInfo));
}

/*
 *@@ wpSetAssociationType:
 *      this WPProgram(File) method sets the types
 *      this object is associated with.
 *
 *      We must invalidate the caches if the WPS
 *      messes with this.
 *
 *@@added V0.9.9 (2001-04-02) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_wpSetAssociationType(XWPProgram *somSelf,
                                                  PSZ pszType)
{
    BOOL brc = FALSE;

    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpSetAssociationType");

    brc = XWPProgram_parent_WPProgram_wpSetAssociationType(somSelf,
                                                           pszType);

    ftypInvalidateCaches();

    return (brc);
}

/*
 *@@ wpMoveObject:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope BOOL  SOMLINK xpgm_wpMoveObject(XWPProgram *somSelf,
                                          WPFolder* Folder)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpMoveObject");

    return (XWPProgram_parent_WPProgram_wpMoveObject(somSelf,
                                                     Folder));
}

/*
 *@@ wpCopyObject:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope WPObject*  SOMLINK xpgm_wpCopyObject(XWPProgram *somSelf,
                                               WPFolder* Folder,
                                               BOOL fLock)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpCopyObject");

    return (XWPProgram_parent_WPProgram_wpCopyObject(somSelf,
                                                     Folder,
                                                     fLock));
}

/*
 *@@ wpAddProgramPage:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope ULONG  SOMLINK xpgm_wpAddProgramPage(XWPProgram *somSelf,
                                               HWND hwndNotebook)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpAddProgramPage");

    return (XWPProgram_parent_WPProgram_wpAddProgramPage(somSelf,
                                                         hwndNotebook));
}

/*
 *@@ wpAddProgramSessionPage:
 *
 *@@added V0.9.12 (2001-05-22) [umoeller]
 */

SOM_Scope ULONG  SOMLINK xpgm_wpAddProgramSessionPage(XWPProgram *somSelf,
                                                      HWND hwndNotebook)
{
    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xpgm_wpAddProgramSessionPage");

    return (XWPProgram_parent_WPProgram_wpAddProgramSessionPage(somSelf,
                                                                hwndNotebook));
}

/*
 *@@ wpAddProgramAssociationPage:
 *      this WPProgram method adds the "Associations" page
 *      to an executable's settings notebook.
 *
 *      If extended associations are enabled, we replace the
 *      standard "Associations" page with a new page which
 *      lists the extended file types.
 */

SOM_Scope ULONG  SOMLINK xpgm_wpAddProgramAssociationPage(XWPProgram *somSelf,
                                                          HWND hwndNotebook)
{
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

    /* XWPProgramData *somThis = XWPProgramGetData(somSelf); */
    XWPProgramMethodDebug("XWPProgram","xwppgm_wpAddProgramAssociationPage");

    if (pGlobalSettings->fExtAssocs)
        return (_xwpAddAssociationsPage(somSelf, hwndNotebook));
    else
        return (XWPProgram_parent_WPProgram_wpAddProgramAssociationPage(somSelf,
                                                                        hwndNotebook));
}

/* ******************************************************************
 *
 *   XWPProgram class methods
 *
 ********************************************************************/

/*
 *@@ wpclsInitData:
 *      initialize XWPProgram class data.
 *
 */

SOM_Scope void  SOMLINK xpgmM_wpclsInitData(M_XWPProgram *somSelf)
{
    /* M_XWPProgramData *somThis = M_XWPProgramGetData(somSelf); */
    M_XWPProgramMethodDebug("M_XWPProgram","xwppgmM_wpclsInitData");

    M_XWPProgram_parent_M_WPProgram_wpclsInitData(somSelf);

    {
        // store the class object in KERNELGLOBALS
        PKERNELGLOBALS   pKernelGlobals = krnLockGlobals(__FILE__, __LINE__, __FUNCTION__);
        if (pKernelGlobals)
        {
            pKernelGlobals->fXWPProgram = TRUE;
            krnUnlockGlobals();
        }
    }
}



