
/*
 *@@sourcefile common.c:
 *      this file contains functions that are common to all
 *      parts of XWorkplace. This is really an unsorted
 *      collection of very miscellaneous items that only
 *      have in common that they are "common" somehow.
 *
 *      These functions mainly deal with the following features:
 *
 *      -- module handling (cmnQueryMainModuleHandle etc.);
 *
 *      -- NLS management (cmnQueryNLSModuleHandle,
 *         cmnQueryNLSStrings);
 *
 *      -- global settings (cmnQueryGlobalSettings);
 *
 *      -- extended XWorkplace message boxes (cmnMessageBox).
 *
 *      Note that the system sound functions have been exported
 *      to helpers\syssound.c (V0.9.0).
 *
 *@@header "shared\common.h"
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

#define INCL_DOSMODULEMGR
#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSMISC
#define INCL_DOSERRORS

#define INCL_WINSHELLDATA       // Prf* functions
#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR        // SC_CLOSE etc.
#define INCL_WINPOINTERS
#define INCL_WININPUT
#define INCL_WINDIALOGS
#define INCL_WINSTATICS
#define INCL_WINMENUS
#define INCL_WINBUTTONS
#define INCL_WINENTRYFIELDS
#define INCL_WINSTDFILE
#define INCL_WINSTDCNR
#define INCL_WINLISTBOXES
#define INCL_WINCOUNTRY
#define INCL_WINPROGRAMLIST
#define INCL_WINSYS

#define INCL_GPILOGCOLORTABLE
#define INCL_GPIBITMAPS
#include <os2.h>

// C library headers
#include <stdio.h>
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h
#include <io.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\apps.h"               // application helpers
#include "helpers\cnrh.h"               // container helper routines
#include "helpers\dialog.h"             // dialog helpers
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\except.h"             // exception handling
#include "helpers\gpih.h"               // GPI helper routines
#include "helpers\linklist.h"           // linked list helper routines
#include "helpers\nls.h"                // National Language Support helpers
#include "helpers\prfh.h"               // INI file helper routines
#include "helpers\standards.h"          // some standard macros
#include "helpers\stringh.h"            // string helper routines
#include "helpers\textview.h"           // PM XTextView control
#include "helpers\tree.h"               // red-black binary trees
#include "helpers\winh.h"               // PM helper routines
#include "helpers\xstring.h"            // extended string helpers

#include "helpers\tmsgfile.h"           // "text message file" handling (for cmnGetMessage)

// SOM headers which don't crash with prec. header files
#pragma hdrstop                         // VAC++ keeps crashing otherwise
#include "xtrash.ih"                    // XWPTrashCan; needed for empty trash
#include <wpdesk.h>                     // WPDesktop

// XWorkplace implementation headers
#include "bldlevel.h"                   // XWorkplace build level definitions
#include "dlgids.h"                     // all the IDs that are shared with NLS
#define INCLUDE_COMMON_PRIVATE
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\xsetup.h"              // XWPSetup implementation

#include "filesys\filedlg.h"            // replacement file dialog implementation
#include "filesys\statbars.h"           // status bar translation logic
#include "filesys\xthreads.h"           // extra XWorkplace threads

#include "media\media.h"                // XWorkplace multimedia support

// other SOM headers
#include "helpers\undoc.h"              // some undocumented stuff

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

static CHAR            G_szHelpLibrary[CCHMAXPATH] = "";
static CHAR            G_szMessageFile[CCHMAXPATH] = "";

// main module (XFLDR.DLL)
static char            G_szDLLFile[CCHMAXPATH];
static HMODULE         G_hmodDLL = NULLHANDLE;

// res module (XWPRES.DLL)
static HMODULE         G_hmodRes = NULLHANDLE;

// NLS
static HMODULE         G_hmodNLS = NULLHANDLE;
// static NLSSTRINGS      *G_pNLSStringsGlobal = NULL;
static GLOBALSETTINGS  G_GlobalSettings = {0};
            // removed the pointer here V0.9.16 (2001-10-02) [umoeller]
static BOOL            G_fGlobalSettingsLoaded = FALSE;

static HMODULE         G_hmodIconsDLL = NULLHANDLE;
static CHAR            G_szLanguageCode[20] = "";

static COUNTRYSETTINGS G_CountrySettings;                  // V0.9.6 (2000-11-12) [umoeller]
static BOOL            G_fCountrySettingsLoaded = FALSE;

static ULONG           G_ulCurHelpPanel = 0;      // holds help panel for dialog

static CHAR            G_szStatusBarFont[100];
static CHAR            G_szSBTextNoneSel[CCHMAXMNEMONICS],
                       G_szSBTextMultiSel[CCHMAXMNEMONICS];
static ULONG           G_ulStatusBarHeight;

static CHAR            G_szRunDirectory[CCHMAXPATH]; // V0.9.14

static PTMFMSGFILE     G_pXWPMsgFile = NULL;        // V0.9.16 (2001-10-08) [umoeller]

// Declare C runtime prototypes, because there are no headers
// for these:

// _CRT_init is the C run-time environment initialization function.
// It will return 0 to indicate success and -1 to indicate failure.
int _CRT_init(void);

// _CRT_term is the C run-time environment termination function.
// It only needs to be called when the C run-time functions are statically
// linked, as is the case with XFolder.
void _CRT_term(void);

/* ******************************************************************
 *
 *   Main module handling (XFLDR.DLL)
 *
 ********************************************************************/

/*
 *@@ _DLL_InitTerm:
 *      this function gets called automatically by the OS/2
 *      module manager during DosLoadModule processing, on
 *      the thread which invoked DosLoadModule.
 *
 *      Since this is a SOM DLL for the WPS, this gets called
 *      right when the WPS is starting and when the WPS process
 *      ends, e.g. due to a Desktop restart or trap. Since the WPS
 *      is the only process loading this DLL, we need not bother
 *      with details.
 *
 *      Defining this function is my preferred way of getting the
 *      DLL's module handle, instead of querying the SOM kernel
 *      for the module name, like this is done in most WPS sample
 *      classes provided by IBM. I have found this to be much
 *      easier and less error-prone when several classes are put
 *      into one DLL (as is the case with XWorkplace).
 *
 *      Besides, this is faster, since we store the module handle
 *      in a global variable which can later quickly be retrieved
 *      using cmnQueryMainModuleHandle.
 *
 *      Since OS/2 calls this function directly, it must have
 *      _System linkage.
 *
 *      Note: You must then link using the /NOE option, because
 *      the VAC++ runtimes also contain a _DLL_Initterm, and the
 *      linker gets in trouble otherwise. The XWorkplace makefile
 *      takes care of this.
 *
 *      This function must return 0 upon errors or 1 otherwise.
 *
 *@@changed V0.9.0 [umoeller]: reworked locale initialization
 *@@changed V0.9.0 [umoeller]: moved this func here from module.c
 */

unsigned long _System _DLL_InitTerm(unsigned long hModule,
                                    unsigned long ulFlag)
{
    APIRET rc;

    switch (ulFlag)
    {
        case 0:
        {
            // DLL being loaded:

            // store the DLL handle in the global variable so that
            // cmnQueryMainModuleHandle() below can return it
            G_hmodDLL = hModule;

            // now initialize the C run-time environment before we
            // call any runtime functions
            if (_CRT_init() == -1)
               return (0);  // error

            if (rc = DosQueryModuleName(hModule, CCHMAXPATH, G_szDLLFile))
                DosBeep(100, 100);
        break; }

        case 1:
            // DLL being freed: cleanup runtime
            _CRT_term();
            break;

        default:
            // other code: beep for error
            DosBeep(100, 100);
            return (0);     // error
    }

    // a non-zero value must be returned to indicate success
    return (1);
}

/*
 *@@ cmnQueryMainCodeModuleHandle:
 *      this may be used to retrieve the module handle
 *      of XFLDR.DLL, which was stored by _DLL_InitTerm.
 *
 *      Note that this returns the _main_ module handle
 *      (XFLDR.DLL). There are two more query-module
 *      functions:
 *
 *      -- To get the NLS module handle (for dialogs etc.),
 *         use cmnQueryNLSModuleHandle.
 *
 *      -- To get the main resource module handle (for icons
 *         etc.), use cmnQueryMainResModuleHandle.
 *
 *@@changed V0.9.0 [umoeller]: moved this func here from module.c
 *@@changed V0.9.7 (2000-12-13) [umoeller]: renamed from cmnQueryMainModuleHandle
 */

HMODULE cmnQueryMainCodeModuleHandle(VOID)
{
    return (G_hmodDLL);
}

/*
 *@@ cmnQueryMainModuleFilename:
 *      this may be used to retrieve the fully
 *      qualified file name of the DLL
 *      which was stored by _DLL_InitTerm.
 *
 *@@changed V0.9.0 [umoeller]: moved this func here from module.c
 */

const char* cmnQueryMainModuleFilename(VOID)
{
    return (G_szDLLFile);
}

/*
 *@@ cmnQueryMainResModuleHandle:
 *      this may be used to retrieve the module handle
 *      of XWPRES.DLL, which contains resources that
 *      are independent of language (icons, bitmaps etc.).
 *
 *      This loads the DLL on the first call.
 *
 *      This has been added with V0.9.7 to separate the
 *      resources out of the main module handle to speed
 *      up link time, which became annoyingly slow with
 *      all the resources.
 *
 *@@added V0.9.7 (2000-12-13) [umoeller]
 */

HMODULE cmnQueryMainResModuleHandle(VOID)
{
    BOOL fLocked = FALSE;
    ULONG ulNesting;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        if (fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__))
        {
            if (G_hmodRes == NULLHANDLE)
            {
                // not loaded yet:
                CHAR    szError[100],
                        szResModule[CCHMAXPATH];

                if (cmnQueryXWPBasePath(szResModule))
                {
                    APIRET arc = NO_ERROR;
                    strcat(szResModule, "\\bin\\xwpres.dll");
                    arc = DosLoadModule(szError,
                                        sizeof(szError),
                                        szResModule,
                                        &G_hmodRes);
                    if (arc != NO_ERROR)
                        cmnLog(__FILE__, __LINE__, __FUNCTION__,
                               "Error %d occured loading \"%s\".",
                               arc, szResModule);
                }
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    DosExitMustComplete(&ulNesting);

    return (G_hmodRes);
}

/* ******************************************************************
 *
 *   Error logging
 *
 ********************************************************************/

/*
 *@@ cmnLog:
 *      logs a message to the XWorkplace log file
 *      in the root directory of the boot drive.
 *
 *@@added V0.9.2 (2000-03-06) [umoeller]
 */

VOID cmnLog(const char *pcszSourceFile, // in: source file name
            ULONG ulLine,               // in: source line
            const char *pcszFunction,   // in: function name
            const char *pcszFormat,     // in: format string (like with printf)
            ...)                        // in: additional stuff (like with printf)
{
    va_list     args;
    CHAR        szLogFileName[100];
    FILE        *fileLog = 0;

    DosBeep(100, 50);

    sprintf(szLogFileName,
            "%c:\\%s",
            doshQueryBootDrive(),
            XFOLDER_LOGLOG);
    fileLog = fopen(szLogFileName, "a");  // text file, append
    if (fileLog)
    {
        DATETIME DT;
        DosGetDateTime(&DT);
        fprintf(fileLog,
                "%04d-%02d-%02d %02d:%02d:%02d "
                "%s (%s, line %d):\n    ",
                DT.year, DT.month, DT.day,
                DT.hours, DT.minutes, DT.seconds,
                pcszFunction, pcszSourceFile, ulLine);
        va_start(args, pcszFormat);
        vfprintf(fileLog, pcszFormat, args);
        va_end(args);
        fprintf(fileLog, "\n");
        fclose (fileLog);
    }
}

/* ******************************************************************
 *
 *   NLS strings
 *
 ********************************************************************/

typedef struct _XWPENTITY
{
    const char *pcszEntity,
               **ppcszString;
} XWPENTITY, *PXWPENTITY;

XWPENTITY  G_aEntities[] =
    {
        "&xwp;", &ENTITY_XWORKPLACE,
        "&os2;", &ENTITY_OS2,
        "&warpcenter;", &ENTITY_WARPCENTER,
        "&xcenter;", &ENTITY_XCENTER,
        "&xsd;", &ENTITY_XSHUTDOWN,
    };

/*
 *@@ ReplaceEntities:
 *
 *@@added V0.9.16 (2001-09-29) [umoeller]
 */

ULONG ReplaceEntities(PXSTRING pstr)
{
    ULONG ul,
          rc = 0;

    for (ul = 0;
         ul < ARRAYITEMCOUNT(G_aEntities);
         ul++)
    {
        ULONG ulOfs = 0;
        PXWPENTITY pThis = &G_aEntities[ul];
        while (xstrFindReplaceC(pstr,
                                &ulOfs,
                                pThis->pcszEntity,
                                *(pThis->ppcszString)))
            rc++;
    }

    return (rc);
}

/*
 *@@ cmnLoadString:
 *      pretty similar to WinLoadString, but allocates
 *      necessary memory as well. *ppsz is a pointer
 *      to a PSZ; if this PSZ is != NULL, whatever it
 *      points to will be free()d, so you should set this
 *      to NULL if you initially call this function.
 *      This is used at Desktop startup and when XFolder's
 *      language is changed later to load all the strings
 *      from a NLS DLL (cmnQueryNLSModuleHandle).
 *
 *@@changed V0.9.0 [umoeller]: "string not found" is now re-allocated using strdup (avoids crashes)
 *@@changed V0.9.0 (99-11-28) [umoeller]: added more meaningful error message
 *@@changed V0.9.2 (2000-02-26) [umoeller]: made temporary buffer larger
 */

void cmnLoadString(HAB habDesktop,
                   HMODULE hmodResource,
                   ULONG ulID,
                   PSZ *ppsz)
{
    CHAR szBuf[500];
    if (*ppsz)
        free(*ppsz);

    if (WinLoadString(habDesktop,
                      hmodResource,
                      ulID,
                      sizeof(szBuf),
                      szBuf))
    {
        XSTRING str;
        xstrInitCopy(&str, szBuf, 0);
        ReplaceEntities(&str);
        *ppsz = str.psz;
    }
    else
    {
        sprintf(szBuf,
                "cmnLoadString error: string resource %d not found in module 0x%lX",
                ulID,
                hmodResource);
        *ppsz = strdup(szBuf);
    }

    // do not free string
}

HMTX        G_hmtxStringsCache = NULLHANDLE;
TREE        *G_StringsCache;
ULONG       G_cStringsInCache = 0;

/*
 *@@ LockStrings:
 *
 *@@added V0.9.9 (2001-04-04) [umoeller]
 */

BOOL LockStrings(VOID)
{
    BOOL brc = FALSE;

    if (G_hmtxStringsCache == NULLHANDLE)
    {
        brc = !DosCreateMutexSem(NULL,
                                 &G_hmtxStringsCache,
                                 0,
                                 TRUE);
        treeInit(&G_StringsCache);
        G_cStringsInCache = 0;
    }
    else
        brc = !DosRequestMutexSem(G_hmtxStringsCache, SEM_INDEFINITE_WAIT);

    return (brc);
}

/*
 *@@ UnlockStrings:
 *
 *@@added V0.9.9 (2001-04-04) [umoeller]
 */

VOID UnlockStrings(VOID)
{
    DosReleaseMutexSem(G_hmtxStringsCache);
}

/*
 *@@ STRINGTREENODE:
 *      internal string node structure for cmnGetString.
 *
 *@@added V0.9.9 (2001-04-04) [umoeller]
 */

typedef struct _STRINGTREENODE
{
    TREE        Tree;               // tree node (src\helpers\tree.c)
    PSZ         pszLoaded;          // string that was loaded; malloc()'ed
} STRINGTREENODE, *PSTRINGTREENODE;

/*
 *@@ cmnGetString:
 *      returns an XWorkplace NLS string.
 *
 *      On input, specify one of the ID_XSSI_* identifiers
 *      specified in dlgids.h.
 *
 *      This function completely replaces the NLSSTRINGS array
 *      which was present in all XFolder and XWorkplace versions
 *      up to V0.9.9. This function has the following advantages:
 *
 *      -- Memory is only consumed for strings that are actually
 *         used. The NLSSTRINGS array had become terribly big,
 *         and lots of strings were loaded that were never used.
 *
 *      -- Desktop startup should be a bit faster because we don't have
 *         to load a thousand strings at startup.
 *
 *      -- The memory buffer holding the string is probably close
 *         to the rest of the heap data that the caller allocated,
 *         so this might lead to less memory page fragmentation.
 *
 *      -- To add a new NLS string, before this mechanism existed,
 *         three files had to be changed (and kept in sync): common.h
 *         to add a field to the NLSSTRINGS structure, dlgids.h to
 *         add the string ID, and xfldrXXX.rc to add the resource.
 *         With the new mechanism, there's no need to change common.h
 *         any more, so the danger of forgetting something is a bit
 *         reduced. Anyway, fewer recompiles are needed (maybe),
 *         and sending in patches to the code is a bit easier.
 *
 *      The way this works is that the function maintains a
 *      fast cache of string IDs and only loads the string
 *      resources on demand from the XWorkplace NLS DLL. If
 *      a string ID is queried for the first time, the string
 *      is loaded. Otherwise the cached copy is returned.
 *
 *      There is a slight overhead to this function compared to
 *      simply getting a static string from an array, because
 *      the cache needs to be searched for the string ID. However,
 *      this uses a binary tree (balanced according to string IDs)
 *      internally, so this is quite fast still.
 *
 *      This never releases the strings again, unless the
 *      NLS DLL is reloaded (see cmnQueryNLSModuleHandle).
 *
 *      This never returns NULL. Even if loading the string failed,
 *      a string is returned; in that case, it's a meaningful error
 *      message specifying the ID that failed.
 *
 *@@added V0.9.9 (2001-04-04) [umoeller]
 */

PSZ cmnGetString(ULONG ulStringID)
{
    BOOL    fLocked = FALSE;
    PSZ     pszReturn = "Error";

    TRY_LOUD(excpt1)
    {
        if (fLocked = LockStrings())
        {
            PSTRINGTREENODE pNode;

            if (pNode = (PSTRINGTREENODE)treeFind(G_StringsCache,
                                                  ulStringID,
                                                  treeCompareKeys))
                // already loaded:
                pszReturn = pNode->pszLoaded;
            else
            {
                // not loaded: load now
                pNode = NEW(STRINGTREENODE);
                if (!pNode)
                    pszReturn = "malloc() failed.";
                else
                {
                    if (!G_hmodNLS)
                        // NLS DLL not loaded yet:
                        cmnQueryNLSModuleHandle(FALSE);

                    pNode->Tree.ulKey = ulStringID;
                    pNode->pszLoaded = NULL;
                        // otherwise cmnLoadString frees the string
                    cmnLoadString(G_habThread1,     // kernel.c
                                  G_hmodNLS,
                                  ulStringID,
                                  &pNode->pszLoaded);
                    treeInsert(&G_StringsCache,
                               (TREE*)pNode,
                               treeCompareKeys);
                    pszReturn = pNode->pszLoaded;
                }
            }
        }
        else
            // we must always return a string, never NULL
            pszReturn = "Cannot get strings lock.";
    }
    CATCH(excpt1) {} END_CATCH();

    if (fLocked)
        UnlockStrings();

    return (pszReturn);
}

/*
 *@@ UnloadAllStrings:
 *      removes all loaded strings from memory.
 *      Called by cmnQueryNLSModuleHandle when the
 *      module handle has changed.
 *
 *@@added V0.9.9 (2001-04-04) [umoeller]
 */

VOID UnloadAllStrings(VOID)
{
    BOOL    fLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        if (fLocked = LockStrings())
        {
            // to delete all nodes, build a temporary
            // array of all string node pointers;
            // we don't want to rebalance the tree
            // for each node
            ULONG           cNodes = G_cStringsInCache;
            PSTRINGTREENODE *papNodes
                = (PSTRINGTREENODE*)treeBuildArray(G_StringsCache,
                                                   &cNodes);
            if (papNodes)
            {
                if (cNodes == G_cStringsInCache)
                {
                    // delete all nodes in array
                    ULONG ul;
                    PSTRINGTREENODE pNode;
                    for (ul = 0;
                         ul < cNodes;
                         ul++)
                    {
                        pNode = papNodes[ul];
                        if (pNode->pszLoaded)
                            free(pNode->pszLoaded);
                        free(pNode);
                    }
                }
                else
                    cmnLog(__FILE__, __LINE__, __FUNCTION__,
                           "Node count mismatch.");

                free(papNodes);
            }

            // reset the tree to "empty"
            treeInit(&G_StringsCache);
            G_cStringsInCache = 0;
        }
    }
    CATCH(excpt1) {} END_CATCH();

    if (fLocked)
        UnlockStrings();
}

/* some frequently used dialog controls
    V0.9.16 (2001-10-08) [umoeller] */

CONTROLDEF
    G_UndoButton = CONTROLDEF_PUSHBUTTON(
                            LOAD_STRING, // "~Undo",
                            DID_UNDO,
                            100,
                            30),
    G_DefaultButton = CONTROLDEF_PUSHBUTTON(
                            LOAD_STRING, // "~Default",
                            DID_DEFAULT,
                            100,
                            30),
    G_HelpButton = CONTROLDEF_HELPPUSHBUTTON(
                            LOAD_STRING, // "~Help",
                            DID_HELP,
                            100,
                            30),
    G_Spacing = CONTROLDEF_TEXT(
                            "",
                            -1,
                            20,
                            2);

/*
 *@@ cmnLoadDialogStrings:
 *
 *      Used by ntbFormatPage.
 *
 *@@added V0.9.16 (2001-10-08) [umoeller]
 */

VOID cmnLoadDialogStrings(PDLGHITEM paDlgItems,      // in: definition array
                          ULONG cDlgItems)           // in: array item count (NOT array size)
{
    // load the strings
    ULONG ul;
    for (ul = 0;
         ul < cDlgItems;
         ul++)
    {
        PDLGHITEM pThis = &paDlgItems[ul];
        PCONTROLDEF pDef;
        if (    (    (pThis->Type == TYPE_CONTROL_DEF)
                  || (pThis->Type == TYPE_START_NEW_TABLE)
                )
             && (pDef = (PCONTROLDEF)pThis->ulData)
             && (pDef->pcszText == LOAD_STRING ) // (PCSZ)-1)
           )
        {
            pDef->pcszText = cmnGetString(pDef->usID);
        }
    }
}

/*
 * G_aStringIDs:
 *      array of LOADSTRING structures specifying the
 *      NLS strings to be loaded at startup.
 *
 *      This array has been removed again... we now have
 *      the new cmnGetString function V0.9.9 (2001-04-04) [umoeller].
 *      If you need to look up the old id -> psz pairs,
 *      look at src\shared\OldStringIDs.txt.
 *
 *added V0.9.9 (2001-03-07) [umoeller]
 *removed again V0.9.9 (2001-04-04) [umoeller]
 */

/* ******************************************************************
 *
 *   XWorkplace National Language Support (NLS)
 *
 ********************************************************************/

/*
 *  The following routines are for querying the XFolder
 *  installation path and similiar routines, such as
 *  querying the current NLS module, changing it, loading
 *  strings, the help file and all that sort of stuff.
 */

/*
 *@@ cmnQueryXWPBasePath:
 *      this routine returns the path of where XFolder was installed,
 *      i.e. the parent directory of where the xfldr.dll file
 *      resides, without a trailing backslash (e.g. "C:\XFolder").
 *
 *      The buffer to copy this to is assumed to be CCHMAXPATH in size.
 *
 *      As opposed to versions before V0.81, OS2.INI is no longer
 *      needed for this to work. The path is retrieved from the
 *      DLL directly by evaluating what was passed to _DLL_InitTerm.
 *
 *@@changed V0.9.7 (2000-12-02) [umoeller]: renamed from cmnQueryXFolderBasePath
 */

BOOL cmnQueryXWPBasePath(PSZ pszPath)
{
    BOOL brc = FALSE;
    const char *pszDLL = cmnQueryMainModuleFilename();
    if (pszDLL)
    {
        // copy until last backslash minus four characters
        // (leave out "\bin\xfldr.dll")
        PSZ pszLastSlash = strrchr(pszDLL, '\\');
        #ifdef DEBUG_LANGCODES
            _Pmpf(( "cmnQueryMainModuleFilename: %s", pszDLL));
        #endif
        strncpy(pszPath, pszDLL, (pszLastSlash-pszDLL)-4);
        pszPath[(pszLastSlash-pszDLL-4)] = '\0';
        brc = TRUE;
    }
    #ifdef DEBUG_LANGCODES
        _Pmpf(( "cmnQueryXWPBasePath: %s", pszPath ));
    #endif
    return (brc);
}

/*
 *@@ cmnQueryLanguageCode:
 *      returns PSZ to three-digit language code (e.g. "001").
 *      This points to a global variable, so do NOT change.
 *
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 */

const char* cmnQueryLanguageCode(VOID)
{
    BOOL fLocked = FALSE;
    ULONG ulNesting;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            if (G_szLanguageCode[0] == '\0')
                PrfQueryProfileString(HINI_USERPROFILE,
                                      (PSZ)INIAPP_XWORKPLACE,
                                      (PSZ)INIKEY_LANGUAGECODE,
                                      (PSZ)DEFAULT_LANGUAGECODE,
                                      (PVOID)G_szLanguageCode,
                                      sizeof(G_szLanguageCode));

            G_szLanguageCode[3] = '\0';
            #ifdef DEBUG_LANGCODES
                _Pmpf(( "cmnQueryLanguageCode: %s", szLanguageCode ));
            #endif
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    DosExitMustComplete(&ulNesting);

    return (G_szLanguageCode);
}

/*
 *@@ cmnSetLanguageCode:
 *      changes XFolder's language to three-digit language code in
 *      pszLanguage (e.g. "001"). This does not reload the NLS DLL,
 *      but only change the setting.
 *
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 */

BOOL cmnSetLanguageCode(PSZ pszLanguage)
{
    BOOL brc = FALSE;

    BOOL fLocked = FALSE;
    ULONG ulNesting;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            strcpy(G_szLanguageCode, pszLanguage);
            G_szLanguageCode[3] = 0;

            brc = PrfWriteProfileString(HINI_USERPROFILE,
                                        (PSZ)INIAPP_XWORKPLACE,
                                        (PSZ)INIKEY_LANGUAGECODE,
                                        G_szLanguageCode);
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    DosExitMustComplete(&ulNesting);

    return (brc);
}

/*
 *@@ cmnQueryHelpLibrary:
 *      returns PSZ to full help library path in XFolder directory,
 *      depending on where XFolder was installed and on the current
 *      language (e.g. "C:\XFolder\help\xfldr001.hlp").
 *
 *      This PSZ points to a global variable, so you better not
 *      change it.
 *
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 */

const char* cmnQueryHelpLibrary(VOID)
{
    const char *rc = 0;

    BOOL fLocked = FALSE;
    ULONG ulNesting;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            if (cmnQueryXWPBasePath(G_szHelpLibrary))
            {
                // path found: append helpfile
                sprintf(G_szHelpLibrary + strlen(G_szHelpLibrary),
                        "\\help\\xfldr%s.hlp",
                        cmnQueryLanguageCode());
                #ifdef DEBUG_LANGCODES
                    _Pmpf(( "cmnQueryHelpLibrary: %s", szHelpLibrary ));
                #endif
                rc = G_szHelpLibrary;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    DosExitMustComplete(&ulNesting);

    return (rc);
}

/*
 *@@ cmnHelpNotFound:
 *      displays an error msg that the given help panel
 *      could not be found.
 *
 *@@added V0.9.16 (2001-10-15) [umoeller]
 */

VOID cmnHelpNotFound(ULONG ulPanelID)
{
    CHAR sz[100];
    PSZ psz = (PSZ)cmnQueryHelpLibrary();
    PSZ apsz[] =
        {  sz,
           psz };
    sprintf(sz, "%d", ulPanelID);

    cmnMessageBoxMsgExt(NULLHANDLE,
                        104,            // title
                        apsz,
                        2,
                        134,
                        MB_OK);
}

/*
 *@@ cmnDisplayHelp:
 *      displays an XWorkplace help panel,
 *      using wpDisplayHelp.
 *      If somSelf == NULL, we'll query the
 *      active desktop.
 */

BOOL cmnDisplayHelp(WPObject *somSelf,
                    ULONG ulPanelID)
{
    BOOL brc = FALSE;
    if (somSelf == NULL)
        somSelf = cmnQueryActiveDesktop();

    if (somSelf)
    {
        if (!(brc = _wpDisplayHelp(somSelf,
                                   ulPanelID,
                                   (PSZ)cmnQueryHelpLibrary())))
            // complain
            cmnHelpNotFound(ulPanelID);

    }
    return (brc);
}

/*
 *@@ cmnQueryMessageFile:
 *      returns PSZ to full message file path in XFolder directory,
 *      depending on where XFolder was installed and on the current
 *      language (e.g. "C:\XFolder\help\xfldr001.tmf").
 *
 *@@changed V0.9.0 [umoeller]: changed, this now returns the TMF file (tmsgfile.c).
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 */

const char* cmnQueryMessageFile(VOID)
{
    const char *rc = 0;

    BOOL fLocked = FALSE;
    ULONG ulNesting;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            if (cmnQueryXWPBasePath(G_szMessageFile))
            {
                // path found: append message file
                sprintf(G_szMessageFile + strlen(G_szMessageFile),
                        "\\help\\xfldr%s.tmf",
                        cmnQueryLanguageCode());
                #ifdef DEBUG_LANGCODES
                    _Pmpf(( "cmnQueryMessageFile: %s", szMessageFile));
                #endif
                rc = G_szMessageFile;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    DosExitMustComplete(&ulNesting);

    return (rc);
}

/*
 *@@ cmnQueryIconsDLL:
 *      this returns the HMODULE of XFolder ICONS.DLL
 *      (new with V0.84).
 *
 *      If this is queried for the first time, the DLL
 *      is loaded from the /BIN directory.
 *
 *      In this case, this routine also checks if
 *      ICONS.DLL exists in the /ICONS directory. If
 *      so, it is copied to /BIN before loading the
 *      DLL. This allows for replacing the DLL using
 *      the REXX script in /ICONS.
 *
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 */

HMODULE cmnQueryIconsDLL(VOID)
{
    BOOL fLocked = FALSE;
    ULONG ulNesting;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            // first query?
            if (G_hmodIconsDLL == NULLHANDLE)
            {
                CHAR    szIconsDLL[CCHMAXPATH],
                        szNewIconsDLL[CCHMAXPATH];
                cmnQueryXWPBasePath(szIconsDLL);
                strcpy(szNewIconsDLL, szIconsDLL);

                sprintf(szIconsDLL+strlen(szIconsDLL),
                        "\\bin\\icons.dll");
                sprintf(szNewIconsDLL+strlen(szNewIconsDLL),
                        "\\icons\\icons.dll");

                #ifdef DEBUG_LANGCODES
                    _Pmpf(("cmnQueryIconsDLL: checking %s", szNewIconsDLL));
                #endif
                // first check for /ICONS/ICONS.DLL
                if (access(szNewIconsDLL, 0) == 0)
                {
                    #ifdef DEBUG_LANGCODES
                        _Pmpf(("    found, copying to %s", szIconsDLL));
                    #endif
                    // exists: move to /BIN
                    // and use that one
                    DosDelete(szIconsDLL);
                    DosMove(szNewIconsDLL,      // old
                            szIconsDLL);        // new
                    DosDelete(szNewIconsDLL);
                }

                #ifdef DEBUG_LANGCODES
                    _Pmpf(("cmnQueryIconsDLL: loading %s", szIconsDLL));
                #endif
                // now load /BIN/ICONS.DLL
                if (DosLoadModule(NULL,
                                  0,
                                  szIconsDLL,
                                  &G_hmodIconsDLL)
                        != NO_ERROR)
                    G_hmodIconsDLL = NULLHANDLE;
            }

            #ifdef DEBUG_LANGCODES
                _Pmpf(("cmnQueryIconsDLL: returning %lX", hmodIconsDLL));
            #endif

        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    DosExitMustComplete(&ulNesting);

    return (G_hmodIconsDLL);
}

#ifndef __NOBOOTLOGO__

/*
 *@@ cmnQueryBootLogoFile:
 *      this returns the boot logo file as stored
 *      in OS2.INI. If it is not stored there,
 *      we return the default xfolder.bmp in
 *      the XFolder installation directories.
 *
 *      The return value of this function must
 *      be free()'d after use.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 */

PSZ cmnQueryBootLogoFile(VOID)
{
    PSZ pszReturn = 0;

    BOOL fLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            pszReturn = prfhQueryProfileData(HINI_USER,
                                             INIAPP_XWORKPLACE,
                                             INIKEY_BOOTLOGOFILE,
                                             NULL);
            if (!pszReturn)
            {
                CHAR szBootLogoFile[CCHMAXPATH];
                // INI data not found: return default file
                cmnQueryXWPBasePath(szBootLogoFile);
                strcat(szBootLogoFile,
                        "\\bootlogo\\xfolder.bmp");
                pszReturn = strdup(szBootLogoFile);
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    return (pszReturn);
}

#endif

/*
 *@@ cmnQueryNLSModuleHandle:
 *      returns the module handle of the language-dependent XFolder
 *      National Language Support DLL (XFLDRxxx.DLL).
 *
 *      This is called in two situations:
 *
 *          1) with (fEnforceReload == FALSE) everytime some part
 *             of XFolder needs the NLS resources (e.g. for dialogs);
 *             this only loads the NLS DLL on the very first call
 *             then, whose module handle is cached for subsequent calls.
 *
 *          2) with (fEnforceReload == TRUE) only when the user changes
 *             XFolder's language in the "Workplace Shell" object.
 *
 *      If the DLL is (re)loaded, this function also initializes
 *      all language-dependent XWorkplace components.
 *      This function also checks for whether the NLS DLL has a
 *      decent version level to support this XFolder version.
 *
 *@@changed V0.9.0 [umoeller]: added various NLS strings
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 *@@changed V0.9.7 (2000-12-09) [umoeller]: restructured to fix mutex hangs with load errors
 *@@changed V0.9.9 (2001-03-07) [umoeller]: now loading strings from array
 */

HMODULE cmnQueryNLSModuleHandle(BOOL fEnforceReload)
{
    HMODULE hmodReturn = NULLHANDLE,
            hmodLoaded = NULLHANDLE;

    // load resource DLL if it's not loaded yet or a reload is enforced
    if (    (G_hmodNLS == NULLHANDLE)
         || (fEnforceReload)
       )
    {
        CHAR    szResourceModuleName[CCHMAXPATH];

        // get the XFolder path first
        if (!cmnQueryXWPBasePath(szResourceModuleName))
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "cmnQueryXWPBasePath failed.");
        else
        {
            APIRET arc = NO_ERROR;
            // now compose module name from language code
            strcat(szResourceModuleName, "\\bin\\xfldr");
            strcat(szResourceModuleName, cmnQueryLanguageCode());
            strcat(szResourceModuleName, ".dll");

            // try to load the module
            arc = DosLoadModule(NULL,
                                0,
                                szResourceModuleName,
                                (PHMODULE)&hmodLoaded);
            if (arc != NO_ERROR)
            {
                // display an error string; since we don't have NLS,
                // this must be in English...
                CHAR szError[1000];
                sprintf(szError, "XWorkplace was unable to load its National "
                                 "Language Support DLL \"%s\". DosLoadModule returned "
                                 "error %d.",
                        szResourceModuleName,
                        arc);
                // log
                cmnLog(__FILE__, __LINE__, __FUNCTION__,
                       szError);
                // and display
                winhDebugBox(HWND_DESKTOP,
                             "XWorkplace: Error",
                             szError);
            }
            else
            {
                // module loaded alright!
                // hmodLoaded has the new module handle
                HAB habDesktop = G_habThread1;

                if (fEnforceReload)
                {
                    // if fEnforceReload == TRUE, we will load a test string from
                    // the module to see if it has at least the version level which
                    // this XFolder version requires. This is done using a #define
                    // in dlgids.h: XFOLDER_VERSION is compiled as a string resource
                    // into both the NLS DLL and into the main DLL (this file),
                    // so we always have the versions in there automatically.
                    // MINIMUM_NLS_VERSION (dlgids.h too) contains the minimum
                    // NLS version level that this XFolder version requires.
                    CHAR   szTest[30] = "";
                    LONG   lLength;
                    cmnSetDlgHelpPanel(-1);
                    lLength = WinLoadString(habDesktop,
                                            G_hmodNLS,
                                            ID_XSSI_XFOLDERVERSION,
                                            sizeof(szTest), szTest);
                    #ifdef DEBUG_LANGCODES
                        _Pmpf(("%s version: %s", szResourceModuleName, szTest));
                    #endif

                    if (lLength == 0)
                    {
                        // version string not found: complain
                        winhDebugBox(HWND_DESKTOP,
                                     "XWorkplace",
                                     "The requested file is not an XWorkplace National Language Support DLL.");
                    }
                    else if (memcmp(szTest, MINIMUM_NLS_VERSION, 4) < 0)
                            // szTest has NLS version (e.g. "0.81 beta"),
                            // MINIMUM_NLS_VERSION has minimum version required
                            // (e.g. "0.9.0")
                    {
                        // version level not sufficient:
                        // load dialog from _old_ NLS DLL which says
                        // that the DLL is too old; if user presses
                        // "Cancel", we abort loading the DLL
                        if (WinDlgBox(HWND_DESKTOP,
                                      HWND_DESKTOP,
                                      (PFNWP)cmn_fnwpDlgWithHelp,
                                      G_hmodNLS,        // still the old one
                                      ID_XFD_WRONGVERSION,
                                      (PVOID)NULL)
                                == DID_CANCEL)
                        {
                            winhDebugBox(HWND_DESKTOP,
                                         "XWorkplace",
                                         "The new National Language Support DLL was not loaded.");
                        }
                        else
                            // user wants outdated module:
                            hmodReturn = hmodLoaded;
                    }
                    else
                    {
                        // new module is OK:
                        hmodReturn = hmodLoaded;
                    }
                } // end if (fEnforceReload)
                else
                    // no enfore reload: that's OK always
                    hmodReturn = hmodLoaded;
            } // end else if (arc != NO_ERROR)
        } // end if (cmnQueryXWPBasePath(szResourceModuleName))
    } // end if (    (G_hmodNLS == NULLHANDLE)  || (fEnforceReload) )
    else
        // no (re)load neccessary:
        // use old module (this must be != NULLHANDLE now)
        hmodReturn = G_hmodNLS;

    // V0.9.7 (2000-12-09) [umoeller]
    // alright, now we have:
    // --  hmodLoaded: != NULLHANDLE if we loaded a new module.
    // --  hmodReturn: != NULLHANDLE if the new module is OK.

    if (hmodLoaded)                    // new module loaded here?
    {
        if (hmodReturn == NULLHANDLE)      // but error?
            DosFreeModule(hmodLoaded);
        else
        {
            // module loaded, and OK:
            // replace the global module handle for NLS,
            // and reload all NLS strings...
            // do this safely.
            HMODULE hmodOld = G_hmodNLS;
            BOOL fLocked = FALSE;
            ULONG ulNesting;
            DosEnterMustComplete(&ulNesting);

            TRY_LOUD(excpt1)
            {
                fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
                if (fLocked)
                {
                    G_hmodNLS = hmodLoaded;
                }
            }
            CATCH(excpt1) { } END_CATCH();

            if (fLocked)
                krnUnlock();

            DosExitMustComplete(&ulNesting);

            // free all NLS strings we ever used;
            // they will be dynamically re-loaded
            // with the new NLS module
            UnloadAllStrings();

            if (hmodOld)
                // after all this, unload the old resource module
                DosFreeModule(hmodOld);
        }
    }

    if (hmodReturn == NULLHANDLE)
        // error:
        cmnLog(__FILE__, __LINE__, __FUNCTION__,
               "Returning NULLHANDLE. Some error occured.");

    // return (new?) module handle
    return (hmodReturn);
}

/*
 *cmnQueryNLSStrings:
 *      returns pointer to global NLSSTRINGS structure which contains
 *      all the language-dependent XFolder strings from the resource
 *      files.
 *
 *changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 *removed V0.9.9 (2001-04-04) [umoeller]
 */

/* PNLSSTRINGS cmnQueryNLSStrings(VOID)
{
    BOOL fLocked = FALSE;
    ULONG ulNesting;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            if (G_pNLSStringsGlobal == NULL)
            {
                G_pNLSStringsGlobal = malloc(sizeof(NLSSTRINGS));
                memset(G_pNLSStringsGlobal, 0, sizeof(NLSSTRINGS));
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    DosExitMustComplete(&ulNesting);

    return (G_pNLSStringsGlobal);
}

/*
 *@@ cmnQueryThousandsSeparator:
 *      returns the global COUNTRYSETTINGS (see helpers\prfh.c)
 *      as set in the "Country" object, which are cached for speed.
 *
 *      If (fReload == TRUE), the settings are re-read.
 *
 *@@added V0.9.6 (2000-11-12) [umoeller]
 */

PCOUNTRYSETTINGS cmnQueryCountrySettings(BOOL fReload)
{
    if ((!G_fCountrySettingsLoaded) || (fReload))
    {
        nlsQueryCountrySettings(&G_CountrySettings);
        G_fCountrySettingsLoaded = TRUE;
    }

    return (&G_CountrySettings);
}

/*
 *@@ cmnQueryThousandsSeparator:
 *      returns the thousands separator from the "Country"
 *      object.
 *
 *@@added V0.9.6 (2000-11-12) [umoeller]
 */

CHAR cmnQueryThousandsSeparator(VOID)
{
    PCOUNTRYSETTINGS p = cmnQueryCountrySettings(FALSE);
    return (p->cThousands);
}

/*
 *@@ cmnIsValidHotkey:
 *      returns TRUE if the specified key combo can
 *      be used as a hotkey without endangering the
 *      system.
 *
 *@@added V0.9.4 (2000-08-03) [umoeller]
 */

BOOL cmnIsValidHotkey(USHORT usFlags,
                      USHORT usKeyCode)
{
    BOOL brc
        = (
                // must be a virtual key
                (  (  ((usFlags & KC_VIRTUALKEY) != 0)
                // or Ctrl or Alt must be pressed
                   || ((usFlags & KC_CTRL) != 0)
                   || ((usFlags & KC_ALT) != 0)
                // or one of the Win95 keys must be pressed
                   || (   ((usFlags & KC_VIRTUALKEY) == 0)
                       && (     (usKeyCode == 0xEC00)
                            ||  (usKeyCode == 0xED00)
                            ||  (usKeyCode == 0xEE00)
                          )
                   )
                )
                // OK:
                // filter out lone modifier keys
                && (    ((usFlags & KC_VIRTUALKEY) == 0)
                     || (   (usKeyCode != VK_SHIFT)     // shift
                         && (usKeyCode != VK_CTRL)     // ctrl
                         && (usKeyCode != VK_ALT)     // alt
                // and filter out the tab key too
                         && (usKeyCode != VK_TAB)     // tab
                        )
                   )
                )
           );
    return (brc);
}

/*
 *@@ cmnDescribeKey:
 *      this stores a description of a certain
 *      key into pszBuf, using the NLS DLL strings.
 *      usFlags is as in WM_CHAR.
 *      If (usFlags & KC_VIRTUALKEY), usKeyCode must
 *      be usvk of WM_CHAR (VK_* code), or usch otherwise.
 *      Returns TRUE if this was a valid key combo.
 */

BOOL cmnDescribeKey(PSZ pszBuf,
                    USHORT usFlags,
                    USHORT usKeyCode)
{
    BOOL brc = TRUE;
    ULONG ulID = 0;

    *pszBuf = 0;
    if (usFlags & KC_CTRL)
        strcpy(pszBuf, cmnGetString(ID_XSSI_KEY_CTRL)) ; // pszCtrl
    if (usFlags & KC_SHIFT)
        strcat(pszBuf, cmnGetString(ID_XSSI_KEY_SHIFT)) ; // pszShift
    if (usFlags & KC_ALT)
        strcat(pszBuf, cmnGetString(ID_XSSI_KEY_Alt)) ; // pszAlt

    if (usFlags & KC_VIRTUALKEY)
    {
        switch (usKeyCode)
        {
            case VK_BACKSPACE: ulID = ID_XSSI_KEY_BACKSPACE; break; // pszBackspace
            case VK_TAB: ulID = ID_XSSI_KEY_TAB; break; // pszTab
            case VK_BACKTAB: ulID = ID_XSSI_KEY_BACKTABTAB; break; // pszBacktab
            case VK_NEWLINE: ulID = ID_XSSI_KEY_ENTER; break; // pszEnter
            case VK_ESC: ulID = ID_XSSI_KEY_ESC; break; // pszEsc
            case VK_SPACE: ulID = ID_XSSI_KEY_SPACE; break; // pszSpace
            case VK_PAGEUP: ulID = ID_XSSI_KEY_PAGEUP; break; // pszPageup
            case VK_PAGEDOWN: ulID = ID_XSSI_KEY_PAGEDOWN; break; // pszPagedown
            case VK_END: ulID = ID_XSSI_KEY_END; break; // pszEnd
            case VK_HOME: ulID = ID_XSSI_KEY_HOME; break; // pszHome
            case VK_LEFT: ulID = ID_XSSI_KEY_LEFT; break; // pszLeft
            case VK_UP: ulID = ID_XSSI_KEY_UP; break; // pszUp
            case VK_RIGHT: ulID = ID_XSSI_KEY_RIGHT; break; // pszRight
            case VK_DOWN: ulID = ID_XSSI_KEY_DOWN; break; // pszDown
            case VK_PRINTSCRN: ulID = ID_XSSI_KEY_PRINTSCRN; break; // pszPrintscrn
            case VK_INSERT: ulID = ID_XSSI_KEY_INSERT; break; // pszInsert
            case VK_DELETE: ulID = ID_XSSI_KEY_DELETE; break; // pszDelete
            case VK_SCRLLOCK: ulID = ID_XSSI_KEY_SCRLLOCK; break; // pszScrlLock
            case VK_NUMLOCK: ulID = ID_XSSI_KEY_NUMLOCK; break; // pszNumLock
            case VK_ENTER: ulID = ID_XSSI_KEY_ENTER; break; // pszEnter
            case VK_F1: strcat(pszBuf, "F1"); break;
            case VK_F2: strcat(pszBuf, "F2"); break;
            case VK_F3: strcat(pszBuf, "F3"); break;
            case VK_F4: strcat(pszBuf, "F4"); break;
            case VK_F5: strcat(pszBuf, "F5"); break;
            case VK_F6: strcat(pszBuf, "F6"); break;
            case VK_F7: strcat(pszBuf, "F7"); break;
            case VK_F8: strcat(pszBuf, "F8"); break;
            case VK_F9: strcat(pszBuf, "F9"); break;
            case VK_F10: strcat(pszBuf, "F10"); break;
            case VK_F11: strcat(pszBuf, "F11"); break;
            case VK_F12: strcat(pszBuf, "F12"); break;
            case VK_F13: strcat(pszBuf, "F13"); break;
            case VK_F14: strcat(pszBuf, "F14"); break;
            case VK_F15: strcat(pszBuf, "F15"); break;
            case VK_F16: strcat(pszBuf, "F16"); break;
            case VK_F17: strcat(pszBuf, "F17"); break;
            case VK_F18: strcat(pszBuf, "F18"); break;
            case VK_F19: strcat(pszBuf, "F19"); break;
            case VK_F20: strcat(pszBuf, "F20"); break;
            case VK_F21: strcat(pszBuf, "F21"); break;
            case VK_F22: strcat(pszBuf, "F22"); break;
            case VK_F23: strcat(pszBuf, "F23"); break;
            case VK_F24: strcat(pszBuf, "F24"); break;
            default: brc = FALSE; break;
        }
    } // end if (usFlags & KC_VIRTUALKEY)
    else
    {
        switch (usKeyCode)
        {
            case 0xEC00: ulID = ID_XSSI_KEY_WINLEFT; break; // pszWinLeft
            case 0xED00: ulID = ID_XSSI_KEY_WINRIGHT; break; // pszWinRight
            case 0xEE00: ulID = ID_XSSI_KEY_WINMENU; break; // pszWinMenu
            default:
            {
                CHAR szTemp[2];
                if (usKeyCode >= 'a')
                    szTemp[0] = (CHAR)usKeyCode-32;
                else
                    szTemp[0] = (CHAR)usKeyCode;
                szTemp[1] = '\0';
                strcat(pszBuf, szTemp);
            }
        }
    }

    if (ulID)
        strcat(pszBuf, cmnGetString(ulID));


    #ifdef DEBUG_KEYS
        _Pmpf(("Key: %s, usKeyCode: 0x%lX, usFlags: 0x%lX", pszBuf, usKeyCode, usFlags));
    #endif

    return (brc);
}

/*
 *@@ cmnAddProductInfoMenuItem:
 *      adds the XWP product info menu item to the menu.
 *
 *@@added V0.9.9 (2001-04-05) [umoeller]
 */

BOOL cmnAddProductInfoMenuItem(HWND hwndMenu)   // in: main menu with "Help" submenu
{
    BOOL brc = FALSE;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();

    if ((pGlobalSettings->DefaultMenuItems & CTXT_HELP) == 0)
    {
        MENUITEM mi;

        #ifdef DEBUG_MENUS
            _Pmpf(("  Inserting 'Product info'"));
        #endif
        // get handle to the WPObject's "Help" submenu in the
        // the folder's popup menu
        if (winhQueryMenuItem(hwndMenu,
                              WPMENUID_HELP,
                              TRUE,
                              &mi))
        {
            // mi.hwndSubMenu now contains "Help" submenu handle,
            // which we add items to now
            winhInsertMenuSeparator(mi.hwndSubMenu,
                                    MIT_END,
                                    (pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_SEPARATOR));
            winhInsertMenuItem(mi.hwndSubMenu,
                               MIT_END,
                               (pGlobalSettings->VarMenuOffset + ID_XFMI_OFS_PRODINFO),
                               cmnGetString(ID_XSSI_PRODUCTINFO),  // pszProductInfo
                               MIS_TEXT, 0);
            brc = TRUE;
        }
        // else: "Help" menu not found, but this can
        // happen in Warp 4 folder menu bars
    }

    return (brc);
}

/*
 *@@ cmnAddCloseMenuItem:
 *      adds a "Close" menu item to the given menu.
 *
 *@@added V0.9.7 (2000-12-21) [umoeller]
 */

VOID cmnAddCloseMenuItem(HWND hwndMenu)
{
    // PNLSSTRINGS pNLSStrings = cmnQueryNLSStrings();
    // add "Close" menu item
    winhInsertMenuSeparator(hwndMenu,
                            MIT_END,
                            (G_GlobalSettings.VarMenuOffset + ID_XFMI_OFS_SEPARATOR));
    winhInsertMenuItem(hwndMenu,
                       MIT_END,
                       WPMENUID_CLOSE,
                       cmnGetString(ID_XSSI_CLOSE),  // "~Close", // pszClose
                       MIS_TEXT, 0);
}

/* ******************************************************************
 *
 *   XFolder Global Settings
 *
 ********************************************************************/

/*
 *@@ cmnQueryStatusBarSetting:
 *      returns a PSZ to a certain status bar setting, which
 *      may be:
 *      --      SBS_STATUSBARFONT       font (e.g. "9.WarpSans")
 *      --      SBS_TEXTNONESEL         mnemonics for no-object mode
 *      --      SBS_TEXTMULTISEL        mnemonics for multi-object mode
 *
 *      Note that there is no key for querying the mnemonics for
 *      one-object mode, because this is handled by the functions
 *      in statbars.c to provide different data depending on the
 *      class of the selected object.
 *
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 */

const char* cmnQueryStatusBarSetting(USHORT usSetting)
{
    const char *rc = 0;

    BOOL fLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            switch (usSetting)
            {
                case SBS_STATUSBARFONT:
                    rc = G_szStatusBarFont;
                break;

                case SBS_TEXTNONESEL:
                    rc = G_szSBTextNoneSel;
                break;

                case SBS_TEXTMULTISEL:
                    rc = G_szSBTextMultiSel;
                break;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    return (rc);
}

/*
 *@@ cmnSetStatusBarSetting:
 *      sets usSetting to pszSetting. If pszSetting == NULL, the
 *      default value will be loaded from the XFolder NLS DLL.
 *      usSetting works just like in cmnQueryStatusBarSetting.
 *
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 *@@changed V0.9.16 (2001-09-29) [umoeller]: now using XWP default font for status bars instead of 8.Helv always
 */

BOOL cmnSetStatusBarSetting(USHORT usSetting, PSZ pszSetting)
{
    BOOL    brc = FALSE;

    BOOL fLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            HAB     habDesktop = WinQueryAnchorBlock(HWND_DESKTOP);
            HMODULE hmodResource = cmnQueryNLSModuleHandle(FALSE);

            brc = TRUE;

            switch (usSetting)
            {
                case SBS_STATUSBARFONT:
                {
                    CHAR szDummy[CCHMAXMNEMONICS];
#ifndef __NOCFGSTATUSBARS__
                    if (pszSetting)
                    {
                        strcpy(G_szStatusBarFont, pszSetting);
                        PrfWriteProfileString(HINI_USERPROFILE,
                                              (PSZ)INIAPP_XWORKPLACE,
                                              (PSZ)INIKEY_STATUSBARFONT,
                                              G_szStatusBarFont);
                    }
                    else
#endif
                        strcpy(G_szStatusBarFont,
                               cmnQueryDefaultFont());      // V0.9.16 (2001-09-29) [umoeller]
                    sscanf(G_szStatusBarFont, "%d.%s", &(G_ulStatusBarHeight), &szDummy);
                    G_ulStatusBarHeight += 15;
                break; }

                case SBS_TEXTNONESEL:
                {
#ifndef __NOCFGSTATUSBARS__
                    if (pszSetting)
                    {
                        strcpy(G_szSBTextNoneSel, pszSetting);
                        PrfWriteProfileString(HINI_USERPROFILE,
                                              (PSZ)INIAPP_XWORKPLACE,
                                              (PSZ)INIKEY_SBTEXTNONESEL,
                                              G_szSBTextNoneSel);
                    }
                    else
#endif
                        WinLoadString(habDesktop,
                                      hmodResource, ID_XSSI_SBTEXTNONESEL,
                                      sizeof(G_szSBTextNoneSel), G_szSBTextNoneSel);
                break; }

                case SBS_TEXTMULTISEL:
                {
#ifndef __NOCFGSTATUSBARS__
                    if (pszSetting)
                    {
                        strcpy(G_szSBTextMultiSel, pszSetting);
                        PrfWriteProfileString(HINI_USERPROFILE,
                                              (PSZ)INIAPP_XWORKPLACE,
                                              (PSZ)INIKEY_SBTEXTMULTISEL,
                                              G_szSBTextMultiSel);
                    }
                    else
#endif
                        WinLoadString(habDesktop,
                                      hmodResource, ID_XSSI_SBTEXTMULTISEL,
                                      sizeof(G_szSBTextMultiSel), G_szSBTextMultiSel);
                break; }

                default:
                    brc = FALSE;

            } // end switch(usSetting)
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    return (brc);
}

/*
 *@@ cmnQueryStatusBarHeight:
 *      returns the height of the status bars according to the
 *      current settings in pixels. This was calculated when
 *      the status bar font was set.
 */

ULONG cmnQueryStatusBarHeight(VOID)
{
    return (G_ulStatusBarHeight);
}

/*
 *@@ cmnLoadGlobalSettings:
 *      this loads the Global Settings from the INI files; should
 *      not be called directly, because this is done automatically
 *      by cmnQueryGlobalSettings, if necessary.
 *
 *      Before loading the settings, all settings are initialized
 *      in case the settings in OS2.INI do not contain all the
 *      settings for this XWorkplace version. This allows for
 *      compatibility with older versions, including XFolder versions.
 *
 *      If (fResetDefaults == TRUE), this only resets all settings
 *      to the default values without loading them from OS2.INI.
 *      This does _not_ write the default settings back to OS2.INI;
 *      if this is desired, call cmnStoreGlobalSettings afterwards.
 *
 *@@changed V0.9.0 [umoeller]: added fResetDefaults to prototype
 *@@changed V0.9.0 [umoeller]: changed initializations for new settings pages
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally
 *@@changed V0.9.16 (2001-09-29) [umoeller]: fixed duplicate status bars code
 */

PCGLOBALSETTINGS cmnLoadGlobalSettings(BOOL fResetDefaults)
{
    BOOL fLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            ULONG       ulCopied1;

            /* if (G_pGlobalSettings == NULL)
            {
                G_pGlobalSettings = malloc(sizeof(GLOBALSETTINGS));
                memset(G_pGlobalSettings, 0, sizeof(GLOBALSETTINGS));
            } */

            // first set default settings for each settings page;
            // we only load the "real" settings from OS2.INI afterwards
            // because the user might have updated XFolder, and the
            // settings struct in the INIs might be too short
            cmnSetDefaultSettings(SP_1GENERIC              );
            cmnSetDefaultSettings(SP_2REMOVEITEMS          );
            cmnSetDefaultSettings(SP_25ADDITEMS            );
            cmnSetDefaultSettings(SP_26CONFIGITEMS         );
            cmnSetDefaultSettings(SP_27STATUSBAR           );
            cmnSetDefaultSettings(SP_3SNAPTOGRID           );
            cmnSetDefaultSettings(SP_4ACCELERATORS         );
            // cmnSetDefaultSettings(SP_5INTERNALS            );  removed V0.9.0
            // cmnSetDefaultSettings(SP_DTP2                  );  removed V0.9.0
            cmnSetDefaultSettings(SP_FLDRSORT_GLOBAL       );
            // cmnSetDefaultSettings(SP_FILEOPS               );  removed V0.9.0

            // the following are new with V0.9.0
            cmnSetDefaultSettings(SP_SETUP_INFO            );
            cmnSetDefaultSettings(SP_SETUP_FEATURES        );
            cmnSetDefaultSettings(SP_SETUP_PARANOIA        );

            cmnSetDefaultSettings(SP_DTP_MENUITEMS         );
            cmnSetDefaultSettings(SP_DTP_STARTUP           );
            cmnSetDefaultSettings(SP_DTP_SHUTDOWN          );

            cmnSetDefaultSettings(SP_STARTUPFOLDER         );

            cmnSetDefaultSettings(SP_TRASHCAN_SETTINGS);

            // cmnSetDefaultSettings(SP_MOUSE_MOVEMENT); does nothing
            // cmnSetDefaultSettings(SP_MOUSE_CORNERS);  does nothing

            // reset help panels
            G_GlobalSettings.ulIntroHelpShown = 0;

            if (fResetDefaults == FALSE)
            {
                // get global XFolder settings from OS2.INI

                // V0.9.16 (2001-09-29) [umoeller]:
                // now calling cmnSetStatusBarSetting, which will
                // load the defaults... disabled the duplicate
                // code below
                cmnSetStatusBarSetting(SBS_STATUSBARFONT, NULL);
                cmnSetStatusBarSetting(SBS_TEXTNONESEL, NULL);
                cmnSetStatusBarSetting(SBS_TEXTMULTISEL, NULL);
/*
                PrfQueryProfileString(HINI_USERPROFILE,
                                      (PSZ)INIAPP_XWORKPLACE,
                                      (PSZ)INIKEY_STATUSBARFONT,
                                      "8.Helv",
                                      &(G_szStatusBarFont),
                                      sizeof(G_szStatusBarFont));
                sscanf(G_szStatusBarFont, "%d.*%s", &(G_ulStatusBarHeight));
                G_ulStatusBarHeight += 15;

                PrfQueryProfileString(HINI_USERPROFILE,
                                      (PSZ)INIAPP_XWORKPLACE,
                                      (PSZ)INIKEY_SBTEXTNONESEL,
                                      NULL,
                                      &(G_szSBTextNoneSel),
                                      sizeof(G_szSBTextNoneSel));
                PrfQueryProfileString(HINI_USERPROFILE,
                                      (PSZ)INIAPP_XWORKPLACE,
                                      (PSZ)INIKEY_SBTEXTMULTISEL,
                                      NULL,
                                      &(G_szSBTextMultiSel),
                                      sizeof(G_szSBTextMultiSel));
    end V0.9.16 (2001-09-29) [umoeller]
   */
                ulCopied1 = sizeof(GLOBALSETTINGS);
                PrfQueryProfileData(HINI_USERPROFILE,
                                    (PSZ)INIAPP_XWORKPLACE,
                                    (PSZ)INIKEY_GLOBALSETTINGS,
                                    &G_GlobalSettings,
                                    &ulCopied1);
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    return (&G_GlobalSettings);
}

/*
 *@@ cmnQueryGlobalSettings:
 *      returns pointer to the GLOBALSETTINGS structure which
 *      contains the XWorkplace Global Settings valid for all
 *      classes. Loads the settings from the INI files if this
 *      hasn't been done yet.
 *
 *      This is used all the time throughout XWorkplace.
 *
 *      NOTE (UM 99-11-14): This now returns a const pointer
 *      to the global settings, because the settings are
 *      unprotected after this call. Never make changes to the
 *      global settings using the return value of this function;
 *      use cmnLockGlobalSettings instead.
 *
 *@@changed V0.9.0 (99-11-14) [umoeller]: made this reentrant, finally; now returning a const pointer only
 */

const GLOBALSETTINGS* cmnQueryGlobalSettings(VOID)
{
    BOOL fLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__);
        if (fLocked)
        {
            if (!G_fGlobalSettingsLoaded)
            {
                cmnLoadGlobalSettings(FALSE);       // load from INI
                        // this locks again
                G_fGlobalSettingsLoaded = TRUE;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    return (&G_GlobalSettings);
}

/*
 *@@ cmnLockGlobalSettings:
 *      this returns a non-const pointer to the global
 *      settings and locks them, using a mutex semaphore.
 *
 *      If you use this function, ALWAYS call
 *      cmnUnlockGlobalSettings afterwards, as quickly
 *      as possible, because other threads cannot
 *      access the global settings after this call.
 *
 *      Always install an exception handler ...
 *
 *@@added V0.9.0 (99-11-14) [umoeller]
 */

GLOBALSETTINGS* cmnLockGlobalSettings(const char *pcszSourceFile,
                                      ULONG ulLine,
                                      const char *pcszFunction)
{
    if (krnLock(pcszSourceFile, ulLine, pcszFunction))
        return (&G_GlobalSettings);
    else
    {
        cmnLog(__FILE__, __LINE__, __FUNCTION__,
                       "krnLock failed.");
        return (NULL);
    }
}

/*
 *@@ cmnUnlockGlobalSettings:
 *      antagonist to cmnLockGlobalSettings.
 *
 *@@added V0.9.0 (99-11-14) [umoeller]
 */

VOID cmnUnlockGlobalSettings(VOID)
{
    krnUnlock();
}

/*
 *@@ cmnStoreGlobalSettings:
 *      stores the current Global Settings back into the INI files;
 *      returns TRUE if successful.
 *
 *@@changed V0.9.4 (2000-06-16) [umoeller]: now using Worker thread instead of File thread
 */

BOOL cmnStoreGlobalSettings(VOID)
{
    xthrPostWorkerMsg(WOM_STOREGLOBALSETTINGS, 0, 0);
    return (TRUE);
}

/*
 *@@ cmnSetDefaultSettings:
 *      resets those Global Settings which correspond to usSettingsPage
 *      in the System notebook to the default values.
 *
 *      usSettingsPage must be one of the SP_* flags def'd in common.h.
 *      This approach allows to reset the settings to default values
 *      both for a single page (in the various settings notebooks)
 *      and, when this function gets called for each of the settings
 *      pages in cmnLoadGlobalSettings, globally.
 *
 *@@changed V0.9.0 [umoeller]: greatly extended for all the new settings pages
 */

BOOL cmnSetDefaultSettings(USHORT usSettingsPage)
{
    switch(usSettingsPage)
    {
        case SP_1GENERIC:
            // pGlobalSettings->ShowInternals = 1;   // removed V0.9.0
            // pGlobalSettings->ReplIcons = 1;       // removed V0.9.0
            G_GlobalSettings.FullPath = 1;
            G_GlobalSettings.KeepTitle = 1;
            G_GlobalSettings.MaxPathChars = 25;
            G_GlobalSettings.TreeViewAutoScroll = 1;

            G_GlobalSettings._fFdrDefaultDoc = 0;
            G_GlobalSettings._fFdrDefaultDocView = 0;

            G_GlobalSettings.fFdrAutoRefreshDisabled = 0;

            G_GlobalSettings.bDefaultFolderView = 0;  // V0.9.12 (2001-04-30) [umoeller]
        break;

        case SP_2REMOVEITEMS:
            G_GlobalSettings.DefaultMenuItems = 0;
            G_GlobalSettings.RemoveLockInPlaceItem = 0;
            G_GlobalSettings.RemoveCheckDiskItem = 0;
            G_GlobalSettings.RemoveFormatDiskItem = 0;
            G_GlobalSettings.RemoveViewMenu = 0;
            G_GlobalSettings.RemovePasteItem = 0;
            G_GlobalSettings.fFixLockInPlace = 0;     // V0.9.7 (2000-12-10) [umoeller]
        break;

        case SP_25ADDITEMS:
            G_GlobalSettings.FileAttribs = 1;
            G_GlobalSettings.AddCopyFilenameItem = 1;
            G_GlobalSettings.ExtendFldrViewMenu = 1;
            G_GlobalSettings.__fMoveRefreshNow = (doshIsWarp4() ? 1 : 0);
            G_GlobalSettings.AddSelectSomeItem = 1;
            G_GlobalSettings.__fAddFolderContentItem = 1;
            G_GlobalSettings.__fFolderContentShowIcons = 1;
            // G_GlobalSettings.fExtendCloseMenu = 0;
        break;

        case SP_26CONFIGITEMS:
            G_GlobalSettings.MenuCascadeMode = 0;
            G_GlobalSettings.RemoveX = 1;
            G_GlobalSettings.AppdParam = 1;
            G_GlobalSettings.TemplatesOpenSettings = BM_INDETERMINATE;
            G_GlobalSettings.TemplatesReposition = 1;
        break;

        case SP_27STATUSBAR:
            G_GlobalSettings.fDefaultStatusBarVisibility = 1;       // changed V0.9.0
            G_GlobalSettings.SBStyle = (doshIsWarp4() ? SBSTYLE_WARP4MENU : SBSTYLE_WARP3RAISED);
            G_GlobalSettings.SBForViews = SBV_ICON | SBV_DETAILS;
            G_GlobalSettings.lSBBgndColor = WinQuerySysColor(HWND_DESKTOP, SYSCLR_INACTIVEBORDER, 0);
            G_GlobalSettings.lSBTextColor = WinQuerySysColor(HWND_DESKTOP, SYSCLR_OUTPUTTEXT, 0);
            cmnSetStatusBarSetting(SBS_TEXTNONESEL, NULL);
            cmnSetStatusBarSetting(SBS_TEXTMULTISEL, NULL);
            G_GlobalSettings.bDereferenceShadows = STBF_DEREFSHADOWS_SINGLE;
        break;

        case SP_3SNAPTOGRID:
            G_GlobalSettings.fAddSnapToGridDefault = 1;
            G_GlobalSettings.GridX = 15;
            G_GlobalSettings.GridY = 10;
            G_GlobalSettings.GridCX = 20;
            G_GlobalSettings.GridCY = 35;
        break;

        case SP_4ACCELERATORS:
            G_GlobalSettings.fFolderHotkeysDefault = 1;
            G_GlobalSettings.fShowHotkeysInMenus = 1;
        break;

        case SP_FLDRSORT_GLOBAL:
            // G_GlobalSettings.ReplaceSort = 0;        removed V0.9.0
            G_GlobalSettings.lDefSortCrit = -2;        // sort by name
            G_GlobalSettings.AlwaysSort = FALSE;
            G_GlobalSettings.fFoldersFirst = FALSE;   // V0.9.12 (2001-05-18) [umoeller]
        break;

        case SP_DTP_MENUITEMS:  // extra Desktop page
            G_GlobalSettings.fDTMSort = 1;
            G_GlobalSettings.fDTMArrange = 1;
            G_GlobalSettings.fDTMSystemSetup = 1;
            G_GlobalSettings.fDTMLockup = 1;
            G_GlobalSettings.fDTMLogoffNetwork = 1; // V0.9.7 (2000-12-13) [umoeller]
            G_GlobalSettings.fDTMShutdown = 1;
            G_GlobalSettings.fDTMShutdownMenu = 1;
        break;

        case SP_DTP_STARTUP:
            G_GlobalSettings.fWriteXWPStartupLog = 0;
            G_GlobalSettings._fShowBootupStatus = 0;
            G_GlobalSettings.__fBootLogo = 0;
            G_GlobalSettings._bBootLogoStyle = 0;
            G_GlobalSettings.fNumLockStartup = 0;
        break;

        case SP_DTP_SHUTDOWN:
            G_GlobalSettings.ulXShutdownFlags = // changed V0.9.0
                XSD_WPS_CLOSEWINDOWS | XSD_CONFIRM | XSD_REBOOT | XSD_ANIMATE_SHUTDOWN;
            G_GlobalSettings._bSaveINIS = 0; // new method, V0.9.5 (2000-08-16) [umoeller]
        break;

        case SP_DTP_ARCHIVES:  // all new with V0.9.0
            // no settings here, these are set elsewhere
        break;

        case SP_SETUP_FEATURES:   // all new with V0.9.0
            G_GlobalSettings.__fIconReplacements = 0;
            G_GlobalSettings.__fResizeSettingsPages = 0;
            G_GlobalSettings.__fReplaceIconPage = 0;
            G_GlobalSettings.__fReplaceFilePage = 0;
            G_GlobalSettings.fXSystemSounds = 0;
            G_GlobalSettings.fFixClassTitles = 0;     // added V0.9.12 (2001-05-22) [umoeller]

            G_GlobalSettings.__fEnableStatusBars = 0;
            G_GlobalSettings.__fEnableSnap2Grid = 0;
            G_GlobalSettings.__fEnableFolderHotkeys = 0;
            G_GlobalSettings.ExtFolderSort = 0;

            G_GlobalSettings.fAniMouse = 0;
            G_GlobalSettings.fEnableXWPHook = 0;
            G_GlobalSettings.fEnablePageMage = 0;

            G_GlobalSettings.fReplaceArchiving = 0;
            G_GlobalSettings.fRestartWPS = 0;
            G_GlobalSettings.fXShutdown = 0;

            // G_GlobalSettings.fMonitorCDRoms = 0;

            G_GlobalSettings.fExtAssocs = 0;
            // G_GlobalSettings.CleanupINIs = 0;
                    // removed for now V0.9.12 (2001-05-12) [umoeller]

#ifdef __REPLHANDLES__
            G_GlobalSettings.fReplaceHandles = 0; // added V0.9.5 (2000-08-14) [umoeller]
#endif
            G_GlobalSettings.fReplFileExists = 0;
            G_GlobalSettings.fReplDriveNotReady = 0;
            G_GlobalSettings.fTrashDelete = 0;
            G_GlobalSettings.fReplaceTrueDelete = 0; // added V0.9.3 (2000-04-26) [umoeller]
        break;

        case SP_SETUP_PARANOIA:   // all new with V0.9.0
            G_GlobalSettings.VarMenuOffset   = 700;     // raised (V0.9.0)
            G_GlobalSettings.fNoFreakyMenus   = 0;
            G_GlobalSettings.__fNoSubclassing   = 0;
            G_GlobalSettings.NoWorkerThread  = 0;
            G_GlobalSettings.fUse8HelvFont   = (!doshIsWarp4());
            G_GlobalSettings.fNoExcptBeeps    = 0;
            G_GlobalSettings.bDefaultWorkerThreadPriority = 1;  // idle +31
            G_GlobalSettings.fWorkerPriorityBeep = 0;
        break;

        case SP_STARTUPFOLDER:        // all new with V0.9.0
            G_GlobalSettings.ShowStartupProgress = 1;
            G_GlobalSettings.ulStartupInitialDelay = 1000;
            G_GlobalSettings.ulStartupObjectDelay = 1000;
        break;

        case SP_TRASHCAN_SETTINGS:             // all new with V0.9.0
            // G_GlobalSettings.fTrashDelete = 0;  // removedV0.9.3 (2000-04-10) [umoeller]
            // G_GlobalSettings.fTrashEmptyStartup = 0;
            // G_GlobalSettings.fTrashEmptyShutdown = 0;
            G_GlobalSettings.ulTrashConfirmEmpty = TRSHCONF_DESTROYOBJ | TRSHCONF_EMPTYTRASH;
        break;
    }

    return (TRUE);
}

/*
 *@@ cmnIsFeatureEnabled:
 *      returns TRUE if the specified feature is
 *      currently enabled.
 *
 *      This is reasonably sick code but allows
 *      for maximum safety with the code when
 *      adapted XWorkplace versions are built.
 *
 *@@added V0.9.16 (2001-10-11) [umoeller]
 */

BOOL cmnIsFeatureEnabled(XWPFEATURE f)
{
    switch (f)
    {

#ifndef __NOICONREPLACEMENTS__
        case IconReplacements: return G_GlobalSettings.__fIconReplacements;
#endif

#ifndef __NOMOVEREFRESHNOW__
        case MoveRefreshNow: return G_GlobalSettings.__fMoveRefreshNow;
#endif

#ifndef __ALWAYSSUBCLASS__
        case NoSubclassing: return G_GlobalSettings.__fNoSubclassing;
#endif

#ifndef __NOFOLDERCONTENTS__
        case AddFolderContentItem: return G_GlobalSettings.__fAddFolderContentItem;
        case FolderContentShowIcons: return G_GlobalSettings.__fFolderContentShowIcons;
#endif

#ifndef __NOBOOTLOGO__
        case BootLogo: return G_GlobalSettings.__fBootLogo;
#endif

#ifndef __ALWAYSREPLACEFILEPAGE__
        case ReplaceFilePage: return G_GlobalSettings.__fReplaceFilePage;
#endif

#ifndef __NOCFGSTATUSBARS__
        case StatusBars: return G_GlobalSettings.__fEnableStatusBars;
#endif

#ifndef __NOSNAPTOGRID__
        case Snap2Grid: return G_GlobalSettings.__fEnableSnap2Grid;
#endif

#ifndef __ALWAYSFDRHOTKEYS__
        case FolderHotkeys: return G_GlobalSettings.__fEnableFolderHotkeys;
#endif

#ifndef __ALWAYSRESIZESETTINGSPAGES__
        case ResizeSettingsPages: return G_GlobalSettings.__fResizeSettingsPages;
#endif

#ifndef __ALWAYSREPLACEICONPAGE__
        case ReplaceIconPage: return G_GlobalSettings.__fReplaceIconPage;
#endif

        default:
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "Warning: Invalid feature %d queried.", f);
    }

    return FALSE;
}

/* ******************************************************************
 *
 *   Object setup sets V0.9.9 (2001-01-29) [umoeller]
 *
 ********************************************************************/

/*
 *@@ cmnSetupInitData:
 *      setup set helper to be used in a wpInitData override.
 *      See XWPSETUPENTRY for an introduction.
 *
 *      This initializes each entry with the lDefault value
 *      from the XWPSETUPENTRY.
 *
 *      This initializes STG_LONG and STG_BOOL entries with
 *      a copy of XWPSETUPENTRY.lDefault,
 *      but not STG_BITFLAG fields. For this reason, for bit
 *      fields, always define a preceding STG_LONG entry.
 *
 *      For STG_PSZ, if (lDefault != NULL), this makes a
 *      strdup() copy of that string. It is the responsibility
 *      of the caller to clean that up.
 *
 *@@added V0.9.9 (2001-01-29) [umoeller]
 */

VOID cmnSetupInitData(PXWPSETUPENTRY paSettings, // in: object's setup set
                      ULONG cSettings,       // in: array item count (NOT array size)
                      PVOID somThis)         // in: instance's somThis pointer
{
    ULONG   ul = 0;
    // CHAR    szTemp[100];

    for (ul = 0;
         ul < cSettings;
         ul++)
    {
        PXWPSETUPENTRY pSettingThis = &paSettings[ul];

        switch (pSettingThis->ulType)
        {
            case STG_LONG:
            case STG_BOOL:
            {
                PLONG plData = (PLONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                *plData = pSettingThis->lDefault;
            }
            break;

            // ignore STG_BITFIELD

            case STG_PSZ:
            case STG_PSZARRAY:
            {
                PSZ *ppszData = (PSZ*)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                if (pSettingThis->lDefault)
                    *ppszData = strdup((PSZ)pSettingThis->lDefault);
                else
                    // no default given:
                    *ppszData = NULL;
            }
            break;

            case STG_BINARY:
            {
                PVOID pData = ((PBYTE)somThis + pSettingThis->ulOfsOfData);
                memset(pData, 0, pSettingThis->ulExtra);    // struct size
            }
            break;
        }
    }
}

/*
 *@@ cmnSetupBuildString:
 *      setup set helper to be used in an xwpQuerySetup2 override.
 *      See XWPSETUPENTRY for an introduction.
 *
 *      This adds new setup strings to the specified XSTRING, which
 *      should be safely initialized. XWPSETUPENTRY's that have
 *      (pcszSetupString == NULL) are skipped.
 *
 *      -- For STG_LONG, this appends "KEYWORD=%d;".
 *
 *      -- For STG_BOOL and STG_BITFLAG, this appends "KEYWORD={YES|NO};".
 *
 *@@added V0.9.7 (2001-01-25) [umoeller]
 */

VOID cmnSetupBuildString(PXWPSETUPENTRY paSettings, // in: object's setup set
                         ULONG cSettings,       // in: array item count (NOT array size)
                         PVOID somThis,         // in: instance's somThis pointer
                         PXSTRING pstr)         // out: setup string

{
    ULONG   ul = 0;
    CHAR    szTemp[100];

    for (ul = 0;
         ul < cSettings;
         ul++)
    {
        PXWPSETUPENTRY pSettingThis = &paSettings[ul];

        // setup string supported for this?
        if (pSettingThis->pcszSetupString)
        {
            // yes:

            switch (pSettingThis->ulType)
            {
                case STG_LONG:
                {
                    PLONG plData = (PLONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                    if (*plData != pSettingThis->lDefault)
                    {
                        sprintf(szTemp,
                                "%s=%d;",
                                pSettingThis->pcszSetupString,
                                *plData);
                        xstrcat(pstr, szTemp, 0);
                    }
                break; }

                case STG_BOOL:
                {
                    PBOOL plData = (PBOOL)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                    if (*plData != (BOOL)pSettingThis->lDefault)
                    {
                        sprintf(szTemp,
                                "%s=%s;",
                                pSettingThis->pcszSetupString,
                                (*plData == TRUE)
                                    ? "YES"
                                    : "NO");
                        xstrcat(pstr, szTemp, 0);
                    }
                break; }

                case STG_BITFLAG:
                {
                    PULONG pulData = (PULONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                    if (    ((*pulData) & pSettingThis->ulExtra)            // bitmask
                         != (ULONG)pSettingThis->lDefault
                       )
                    {
                        sprintf(szTemp,
                                "%s=%s;",
                                pSettingThis->pcszSetupString,
                                ((*pulData) & pSettingThis->ulExtra)        // bitmask
                                    ? "YES"
                                    : "NO");
                        xstrcat(pstr, szTemp, 0);
                    }
                break; }

                case STG_PSZ:
                {
                    PSZ *ppszData = (PSZ*)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                    ULONG ulDataLen;
                    if (    *ppszData
                         && (ulDataLen = strlen(*ppszData))
                         && strhcmp(*ppszData, (PSZ)pSettingThis->lDefault)
                       )
                    {
                        // not default value:
                        ULONG ul1 = strlen(pSettingThis->pcszSetupString);
                        xstrReserve(pstr,   pstr->cbAllocated
                                          + ul1
                                          + ulDataLen
                                          + 5);

                        xstrcat(pstr, pSettingThis->pcszSetupString, ul1);
                        xstrcatc(pstr, '=');
                        xstrcat(pstr, *ppszData, ulDataLen);
                        xstrcatc(pstr, ';');
                    }
                break; }
            }
        } // end if (pSettingThis->pcszSetupString)
    }
}

/*
 *@@ cmnSetupScanString:
 *      setup set helper to be used in a wpSetup override.
 *      See XWPSETUPENTRY for an introduction.
 *
 *      -- For STG_LONG, this expects "KEYWORD=%d;" strings.
 *
 *      -- For STG_BOOL and STG_BITFLAG, this expects
 *         "KEYWORD={YES|NO};" strings.
 *
 *      -- for STG_PSZ, this expects "KEYWORD=STRING;" strings.
 *
 *      Returns FALSE if values were not set properly.
 *
 *@@added V0.9.7 (2001-01-25) [umoeller]
 */

BOOL cmnSetupScanString(WPObject *somSelf,
                        PXWPSETUPENTRY paSettings, // in: object's setup set
                        ULONG cSettings,         // in: array item count (NOT array size)
                        PVOID somThis,           // in: instance's somThis pointer
                        PSZ pszSetupString,      // in: setup string from wpSetup
                        PULONG pcSuccess)        // out: items successfully parsed and set
{
    BOOL    brc = TRUE;
    CHAR    szValue[500];
    ULONG   cbValue;
    ULONG   ul = 0;

    for (ul = 0;
         ul < cSettings;
         ul++)
    {
        PXWPSETUPENTRY pSettingThis = &paSettings[ul];

        // setup string supported for this?
        if (pSettingThis->pcszSetupString)
        {
            // yes:
            cbValue = sizeof(szValue);
            if (_wpScanSetupString(somSelf,
                                   pszSetupString,
                                   (PSZ)pSettingThis->pcszSetupString,
                                   szValue,
                                   &cbValue))
            {
                // setting found:
                // see what to do with it
                switch (pSettingThis->ulType)
                {
                    case STG_LONG:
                    {
                        LONG lValue = atoi(szValue);
                        if (    (lValue >= pSettingThis->lMin)
                             && (lValue <= pSettingThis->lMax)
                           )
                        {
                            PLONG plData = (PLONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                            if (*plData != lValue)
                            {
                                // data changed:
                                *plData = lValue;
                                (*pcSuccess)++;
                            }
                        }
                        else
                            brc = FALSE;
                    break; }

                    case STG_BOOL:
                    {
                        BOOL fNew;
                        if (!stricmp(szValue, "YES"))
                            fNew = TRUE;
                        else if (!stricmp(szValue, "NO"))
                            fNew = FALSE;
                        else
                            brc = FALSE;

                        if (brc)
                        {
                            PBOOL plData = (PBOOL)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                            if (*plData != fNew)
                            {
                                *plData = fNew;
                                (*pcSuccess)++;
                            }
                        }
                    break; }

                    case STG_BITFLAG:
                    {
                        ULONG   ulNew = 0;
                        if (!stricmp(szValue, "YES"))
                            ulNew = pSettingThis->ulExtra;      // bitmask
                        else if (!stricmp(szValue, "NO"))
                            ulNew = 0;
                        else
                            brc = FALSE;

                        if (brc)
                        {
                            PULONG pulData = (PULONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                            if (    ((*pulData) & pSettingThis->ulExtra)    // bitmask
                                 != ulNew
                               )
                            {
                                *pulData = (  // clear the bit first:
                                              ((*pulData) & ~pSettingThis->ulExtra) // bitmask
                                              // set it if set
                                            | ulNew);

                                (*pcSuccess)++;
                            }
                        }
                    break; }

                    case STG_PSZ:
                    {
                        PSZ *ppszData = (PSZ*)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                        if (*ppszData)
                        {
                            // we have something already:
                            free(*ppszData);
                            *ppszData = NULL;
                        }

                        *ppszData = strdup(szValue);
                        (*pcSuccess)++;
                    break; }
                }
            }

            if (!brc)
                // error occured:
                break;
        } // end if (pSettingThis->pcszSetupString)
    }

    return (brc);
}

/*
 *@@ cmnSetupSave:
 *      setup set helper to be used in a wpSaveState override.
 *      See XWPSETUPENTRY for an introduction.
 *
 *      This invokes wpSave* on each setup set entry.
 *
 *      Returns FALSE if values were not saved properly.
 *
 *@@added V0.9.9 (2001-01-29) [umoeller]
 */

BOOL cmnSetupSave(WPObject *somSelf,
                  PXWPSETUPENTRY paSettings, // in: object's setup set
                  ULONG cSettings,         // in: array item count (NOT array size)
                  const char *pcszClassName, // in: class name to be used with wpSave*
                  PVOID somThis)           // in: instance's somThis pointer
{
    BOOL    brc = TRUE;
    ULONG   ul = 0;

    for (ul = 0;
         ul < cSettings;
         ul++)
    {
        PXWPSETUPENTRY pSettingThis = &paSettings[ul];

        if (pSettingThis->ulKey)
        {
            switch (pSettingThis->ulType)
            {
                case STG_LONG:
                case STG_BOOL:
                // case STG_BITFLAG:
                {
                    PULONG pulData = (PULONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                    if (!_wpSaveLong(somSelf,
                                     (PSZ)pcszClassName,
                                     pSettingThis->ulKey,
                                     *pulData))
                    {
                        // error:
                        brc = FALSE;
                        break;
                    }
                break; }

                case STG_PSZ:
                {
                    PSZ *ppszData = (PSZ*)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                    if (!_wpSaveString(somSelf,
                                       (PSZ)pcszClassName,
                                       pSettingThis->ulKey,
                                       *ppszData))
                    {
                        // error:
                        brc = FALSE;
                        break;
                    }
                break; }
            }
        }
    }

    return (brc);
}

/*
 *@@ cmnSetupRestore:
 *      setup set helper to be used in a wpRestoreState override.
 *      See XWPSETUPENTRY for an introduction.
 *
 *      This invokes wpRestoreLong on each setup set entry.
 *
 *      For each entry, only if wpRestoreLong succeeded,
 *      the corresponding value in the instance data is
 *      overwritten. Otherwise it is undefined, so this
 *      does not replace the need to call cmnSetupInitData
 *      on wpInitData.
 *
 *@@added V0.9.9 (2001-01-29) [umoeller]
 */

BOOL cmnSetupRestore(WPObject *somSelf,
                     PXWPSETUPENTRY paSettings, // in: object's setup set
                     ULONG cSettings,         // in: array item count (NOT array size)
                     const char *pcszClassName, // in: class name to be used with wpRestore*
                     PVOID somThis)           // in: instance's somThis pointer
{
    BOOL    brc = TRUE;
    ULONG   ul = 0;

    for (ul = 0;
         ul < cSettings;
         ul++)
    {
        PXWPSETUPENTRY pSettingThis = &paSettings[ul];

        if (pSettingThis->ulKey)
        {
            switch (pSettingThis->ulType)
            {
                case STG_LONG:
                case STG_BOOL:
                // case STG_BITFLAG:
                {
                    ULONG   ulTemp = 0;
                    if (_wpRestoreLong(somSelf,
                                       (PSZ)pcszClassName,
                                       pSettingThis->ulKey,
                                       &ulTemp))
                    {
                        // only if found,
                        // replace value
                        PULONG pulData = (PULONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                        *pulData = ulTemp;
                    }
                break; }

                case STG_PSZ:
                {
                    ULONG cbValue;
                    if (    _wpRestoreString(somSelf,
                                             (PSZ)pcszClassName,
                                             pSettingThis->ulKey,
                                             NULL,      // get size
                                             &cbValue)
                         && cbValue
                       )
                    {
                        PSZ *ppszData = (PSZ*)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                        if (*ppszData)
                        {
                            // we have something already:
                            free(*ppszData);
                            *ppszData = NULL;
                        }
                        *ppszData = (PSZ)malloc(cbValue + 1);
                        _wpRestoreString(somSelf,
                                         (PSZ)pcszClassName,
                                         pSettingThis->ulKey,
                                         *ppszData,
                                         &cbValue);
                    }
                break; }
            }
        }
    }

    return (brc);
}

/*
 *@@ cmnSetupSetDefaults:
 *      resets part of an object's setup set to default
 *      values. Useful for the "Default" button on a
 *      notebook page.
 *
 *      This requires two arrays as input:
 *
 *      -- PXWPSETUPENTRY paSettings specifies the object's
 *         setup set, as with the other cmnSetup* functions.
 *         This is used to retrieve the default values.
 *
 *      -- PULONG paulOffsets specifies an array of ULONG's.
 *         Each ULONG in that array must specify the
 *         FIELDOFFSET matching one of the items in the
 *         XWPSETUPENTRY array.
 *
 *      This function goes thru the "paulOffsets" array and
 *      finds the corresponding offset in the "paSettings"
 *      setup set array. If found, the corresponding value
 *      (offset to the somThis pointer) is reset to the default.
 *
 *      Again, this ignores STG_BITFIELD entries.
 *
 *      Returns the no. of values successfully changed,
 *      which should match cOffsets.
 *
 *@@added V0.9.9 (2001-01-29) [umoeller]
 */

ULONG cmnSetupSetDefaults(PXWPSETUPENTRY paSettings, // in: object's setup set
                          ULONG cSettings,          // in: array item count (NOT array size)
                          PULONG paulOffsets,
                          ULONG cOffsets,           // in: array item count (NOT array size)
                          PVOID somThis)            // in: instance's somThis pointer
{
    ULONG   ulrc = 0,
            ulOfsThis = 0;

    // go thru the offsets array
    for (ulOfsThis = 0;
         ulOfsThis < cOffsets;
         ulOfsThis++)
    {
        PULONG pulOfsOfDataThis = &paulOffsets[ulOfsThis];

        // now go thru the setup set and find the first entry
        // which matches this offset
        ULONG ulSettingThis = 0;
        for (ulSettingThis = 0;
             ulSettingThis < cSettings;
             ulSettingThis++)
        {
            PXWPSETUPENTRY pSettingThis = &paSettings[ulSettingThis];

            if (pSettingThis->ulOfsOfData == *pulOfsOfDataThis)
            {
                // found:
                switch (pSettingThis->ulType)
                {
                    case STG_LONG:
                    case STG_BOOL:
                     // but skip STG_BITFLAG
                    {
                        // reset value
                        PLONG plData = (PLONG)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                        *plData = pSettingThis->lDefault;
                        // raise return count
                        ulrc++;
                    break; }

                    case STG_PSZ:
                    {
                        PSZ *ppszData = (PSZ*)((PBYTE)somThis + pSettingThis->ulOfsOfData);
                        if (*ppszData)
                        {
                            // we have something already:
                            free(*ppszData);
                            *ppszData = NULL;
                        }
                        if (pSettingThis->lDefault)
                            *ppszData = strdup((PSZ)pSettingThis->lDefault);
                    break; }
                }

                break;
            }

        } // for (ulSettingThis = 0;
    } // for (ulOfsThis = 0;

    return (ulrc);
}

/*
 *@@ cmnSetupRestoreBackup:
 *      resets part of an object's setup set to values
 *      that have been backed up before.
 *      Useful for the "Undo" button on a notebook page.
 *
 *      As opposed to cmnSetupSetDefaults, this only needs
 *      the "paulOffsets" array.
 *
 *      As with cmnSetupSetDefaults, "PULONG paulOffsets"
 *      specifies an array of ULONG's. Each ULONG in that
 *      array must specify the FIELDOFFSET of a setting
 *      from somThis.
 *
 *      This function goes thru the "paulOffsets" array and
 *      copies the value at each offset from pBackup to
 *      somThis. This does NO MEMORY MANAGEMENT for STG_PSZ
 *      values.
 *
 *      Returns the no. of values successfully changed,
 *      which should match cOffsets.
 *
 *@@added V0.9.9 (2001-01-29) [umoeller]
 */

ULONG cmnSetupRestoreBackup(PULONG paulOffsets,
                            ULONG cOffsets,           // in: array item count (NOT array size)
                            PVOID somThis,            // in: instance's somThis pointer
                            PVOID pBackup)            // in: backup of somThis
{
    ULONG   ulrc = 0,
            ulOfsThis = 0;

    // go thru the offsets array
    for (ulOfsThis = 0;
         ulOfsThis < cOffsets;
         ulOfsThis++)
    {
        PULONG pulOfsOfDataThis = &paulOffsets[ulOfsThis];

        // restore value
        PLONG plTarget = (PLONG)((PBYTE)somThis + *pulOfsOfDataThis);
        PLONG plSource = (PLONG)((PBYTE)pBackup + *pulOfsOfDataThis);
        *plTarget = *plSource;
        // raise return count
        ulrc++;
    } // for (ulOfsThis = 0;

    return (ulrc);
}

/* ******************************************************************
 *
 *   Trash can setup
 *
 ********************************************************************/

/*
 *@@ cmnTrashCanReady:
 *      returns TRUE if the trash can classes are
 *      installed and the default trash can exists.
 *
 *      This does not check for whether "delete to
 *      trash can" is enabled. Query
 *      GLOBALSETTINGS.fTrashDelete to find out.
 *
 *@@added V0.9.1 (2000-02-01) [umoeller]
 *@@changed V0.9.4 (2000-08-03) [umoeller]: moved this here from fileops.c
 */

BOOL cmnTrashCanReady(VOID)
{
    BOOL brc = FALSE;
    // PCKERNELGLOBALS pKernelGlobals = krnQueryGlobals();
    M_XWPTrashCan *pTrashCanClass = _XWPTrashCan;
    if (pTrashCanClass)
    {
        if (_xwpclsQueryDefaultTrashCan(pTrashCanClass))
            brc = TRUE;
    }

    return (brc);
}

/*
 *@@ cmnEnableTrashCan:
 *      enables or disables the XWorkplace trash can
 *      altogether after displaying a confirmation prompt.
 *
 *      This does all of the following:
 *      -- (de)register XWPTrashCan and XWPTrashObject;
 *      -- enable "delete into trashcan" support;
 *      -- create or destroy the default trash can.
 *
 *@@added V0.9.1 (2000-02-01) [umoeller]
 *@@changed V0.9.4 (2000-08-03) [umoeller]: moved this here from fileops.c
 *@@changed V0.9.9 (2001-04-08) [umoeller]: wrong item ID
 */

BOOL cmnEnableTrashCan(HWND hwndOwner,     // for message boxes
                       BOOL fEnable)
{
    BOOL    brc = FALSE;

    if (fEnable)
    {
        // enable:
        // M_XWPTrashCan       *pXWPTrashCanClass = _XWPTrashCan;

        BOOL    fCreateObject = FALSE;

        if (    (!winhIsClassRegistered(G_pcszXWPTrashCan))
             || (!winhIsClassRegistered(G_pcszXWPTrashObject))
           )
        {
            // classes not registered yet:
            if (cmnMessageBoxMsg(hwndOwner,
                                 148,       // XWPSetup
                                 170,       // "register trash can?"
                                 MB_YESNO)
                    == MBID_YES)
            {
                // CHAR szRegisterError[500];

                HPOINTER hptrOld = winhSetWaitPointer();

                if (WinRegisterObjectClass((PSZ)G_pcszXWPTrashCan,
                                           (PSZ)cmnQueryMainModuleFilename()))
                    if (WinRegisterObjectClass((PSZ)G_pcszXWPTrashObject,
                                               (PSZ)cmnQueryMainModuleFilename()))
                    {
                        fCreateObject = TRUE;
                        brc = TRUE;
                    }

                WinSetPointer(HWND_DESKTOP, hptrOld);

                if (!brc)
                    // error:
                    cmnMessageBoxMsg(hwndOwner,
                                     148,
                                     171, // "error"
                                     MB_CANCEL);
            }
        }
        else
            fCreateObject = TRUE;

        if (fCreateObject)
        {
            // XWPTrashCan *pDefaultTrashCan = NULL;

            if (NULLHANDLE == WinQueryObject((PSZ)XFOLDER_TRASHCANID))
            {
                brc = setCreateStandardObject(hwndOwner,
                                              220,        // XWPTrashCan
                                                    // yo, this needed to be updated
                                                    // V0.9.9 (2001-04-08) [umoeller]
                                              FALSE);     // XWP object
            }
            else
                brc = TRUE;

            if (brc)
            {
                GLOBALSETTINGS *pGlobalSettings = cmnLockGlobalSettings(__FILE__, __LINE__, __FUNCTION__);
                if (pGlobalSettings)
                {
                    pGlobalSettings->fTrashDelete = TRUE;
                    cmnUnlockGlobalSettings();
                }
            }
        }
    } // end if (fEnable)
    else
    {
        GLOBALSETTINGS *pGlobalSettings = cmnLockGlobalSettings(__FILE__, __LINE__, __FUNCTION__);
        if (pGlobalSettings)
        {
            pGlobalSettings->fTrashDelete = FALSE;
            cmnUnlockGlobalSettings();
        }

        if (krnQueryLock())
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                       "Global lock already requested.");
        else
        {
            // disable:
            if (cmnMessageBoxMsg(hwndOwner,
                                 148,       // XWPSetup
                                 172,       // "deregister trash can?"
                                 MB_YESNO | MB_DEFBUTTON2)
                    == MBID_YES)
            {
                XWPTrashCan *pDefaultTrashCan = _xwpclsQueryDefaultTrashCan(_XWPTrashCan);
                if (pDefaultTrashCan)
                    _wpFree(pDefaultTrashCan);
                WinDeregisterObjectClass((PSZ)G_pcszXWPTrashCan);
                WinDeregisterObjectClass((PSZ)G_pcszXWPTrashObject);

                cmnMessageBoxMsg(hwndOwner,
                                 148,       // XWPSetup
                                 173,       // "done, restart Desktop"
                                 MB_OK);
            }
        }
    }

    cmnStoreGlobalSettings();

    return (brc);
}

/*
 *@@ cmnDeleteIntoDefTrashCan:
 *      moves a single object to the default trash can.
 *
 *@@added V0.9.4 (2000-08-03) [umoeller]
 *@@changed V0.9.9 (2001-02-06) [umoeller]: renamed from cmnMove2DefTrashCan
 */

BOOL cmnDeleteIntoDefTrashCan(WPObject *pObject)
{
    BOOL brc = FALSE;
    XWPTrashCan *pDefaultTrashCan = _xwpclsQueryDefaultTrashCan(_XWPTrashCan);
    if (pDefaultTrashCan)
        brc = _xwpDeleteIntoTrashCan(pDefaultTrashCan,
                                     pObject);
    return (brc);
}

/*
 *@@ cmnEmptyDefTrashCan:
 *      quick interface to empty the default trash can
 *      without having to include all the trash can headers.
 *
 *      See XWPTrashCan::xwpEmptyTrashCan for the description
 *      of the parameters.
 *
 *@@added V0.9.4 (2000-08-03) [umoeller]
 *@@changed V0.9.7 (2001-01-17) [umoeller]: now returning ULONG
 */

APIRET cmnEmptyDefTrashCan(HAB hab,        // in: synchronously?
                           PULONG pulDeleted, // out: if TRUE is returned, no. of deleted objects; can be 0
                           HWND hwndConfirmOwner) // in: if != NULLHANDLE, confirm empty
{
    LONG ulrc = FALSE;
    XWPTrashCan *pDefaultTrashCan = _xwpclsQueryDefaultTrashCan(_XWPTrashCan);
    if (pDefaultTrashCan)
    {
        ulrc = _xwpEmptyTrashCan(pDefaultTrashCan,
                                 hab,
                                 pulDeleted,
                                 hwndConfirmOwner);
    }

    return (ulrc);
}

/* ******************************************************************
 *
 *   Miscellaneae
 *
 ********************************************************************/

/*
 *@@ cmnRegisterView:
 *      helper for the typical wpAddToObjUseList/wpRegisterView
 *      sequence.
 *
 *      With pUseItem, pass in a USEITEM structure which must be
 *      immediately followed by a VIEWITEM structure. The buffer
 *      pointed to by pUseItem must be valid while the view exists,
 *      so you best store this in the view's window words somewhere.
 *
 *      This function then calls wpRegisterView with the specified
 *      frame window handle and view title. Tilde chars (~) are
 *      removed from the view title so you can easily use the
 *      menu item's text.
 *
 *@@added V0.9.11 (2001-04-18) [umoeller]
 */

BOOL cmnRegisterView(WPObject *somSelf,
                     PUSEITEM pUseItem,     // in: USEITEM, immediately followed by VIEWITEM
                     ULONG ulViewID,        // in: view ID == menu item ID
                     HWND hwndFrame,        // in: frame window handle of new view (must be WC_FRAME)
                     const char *pcszViewTitle) // in: view title for wpRegisterView (tilde chars are removed)
{
    BOOL        brc = FALSE;
    PSZ         pszViewTitle = strdup(pcszViewTitle),
                p = 0;

    if (pszViewTitle)
    {
        PVIEWITEM   pViewItem = (PVIEWITEM)(((PBYTE)pUseItem) + sizeof(USEITEM));
        // add the use list item to the object's use list
        pUseItem->type    = USAGE_OPENVIEW;
        pUseItem->pNext   = NULL;
        memset(pViewItem, 0, sizeof(VIEWITEM));
        pViewItem->view   = ulViewID;
        pViewItem->handle = hwndFrame;
        if (!_wpAddToObjUseList(somSelf, pUseItem))
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "_wpAddToObjUseList failed.");
        else
        {
            // create view title: remove ~ char
            p = strchr(pszViewTitle, '~');
            if (p)
                // found: remove that
                strcpy(p, p+1);

            brc = _wpRegisterView(somSelf,
                                  hwndFrame,
                                  pszViewTitle); // view title
        }
        free(pszViewTitle);
    }

    return (brc);
}

/*
 *@@ cmnPlaySystemSound:
 *      this posts a msg to the XFolder Media thread to
 *      have it play a system sound. This does sufficient
 *      error checking and returns FALSE if playing the
 *      sound failed.
 *
 *      usIndex may be any of the MMSOUND_* values defined
 *      in helpers\syssound.h and shared\common.h.
 *
 *@@changed V0.9.3 (2000-04-10) [umoeller]: "Sounds" setting in XWPSetup wasn't respected; fixed
 */

BOOL cmnPlaySystemSound(USHORT usIndex)
{
    BOOL brc = FALSE;
    PCGLOBALSETTINGS pGlobalSettings = cmnQueryGlobalSettings();
    if (pGlobalSettings->fXSystemSounds)    // V0.9.3 (2000-04-10) [umoeller]
    {
        // PCKERNELGLOBALS pKernelGlobals = krnQueryGlobals();

        // check if the XWPMedia subsystem is working
        if (xmmQueryStatus() == MMSTAT_WORKING)
        {
            brc = xmmPostMediaMsg(XMM_PLAYSYSTEMSOUND,
                                  (MPARAM)usIndex,
                                  MPNULL);
        }
    }

    return (brc);
}

/*
 *@@ cmnIsADesktop:
 *      returns TRUE if somSelf is a WPDesktop
 *      instance.
 *
 *@@added V0.9.14 (2001-07-28) [umoeller]
 */

BOOL cmnIsADesktop(WPObject *somSelf)
{
    return (_somIsA(somSelf, _WPDesktop));
}

/*
 *@@ cmnQueryActiveDesktop:
 *      wrapper for wpclsQueryActiveDesktop. This
 *      has been implemented so that this method
 *      gets called only once (speed). Also, this
 *      saves us from including wpdesk.h in every
 *      source file.
 *
 *@@added V0.9.3 (2000-04-17) [umoeller]
 */

WPObject* cmnQueryActiveDesktop(VOID)
{
    return (_wpclsQueryActiveDesktop(_WPDesktop));
}

/*
 *@@ cmnQueryActiveDesktopHWND:
 *      wrapper for wpclsQueryActiveDesktopHWND. This
 *      has been implemented so that this method
 *      gets called only once (speed). Also, this
 *      saves us from including wpdesk.h in every
 *      source file.
 *
 *@@added V0.9.3 (2000-04-17) [umoeller]
 */

HWND cmnQueryActiveDesktopHWND(VOID)
{
    return (_wpclsQueryActiveDesktopHWND(_WPDesktop));
}

/*
 * PRODUCTINFODATA:
 *      small struct for QWL_USER in cmn_fnwpProductInfo.
 */

typedef struct _PRODUCTINFODATA
{
    HBITMAP hbm;
    POINTL  ptlBitmap;
} PRODUCTINFODATA, *PPRODUCTINFODATA;

/*
 *@@ cmn_fnwpProductInfo:
 *      dialog func which paints the XWP logo directly
 *      as a bitmap, instead of using a static control.
 *
 *@@added V0.9.5 (2000-10-07) [umoeller]
 */

MRESULT EXPENTRY cmn_fnwpProductInfo(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    BOOL fCallDefault = TRUE;

    switch (msg)
    {
        case WM_INITDLG:
        {
            PPRODUCTINFODATA ppd = (PPRODUCTINFODATA)malloc(sizeof(PRODUCTINFODATA));
            if (ppd)
            {
                HPS hpsTemp = 0;
                memset(ppd, 0, sizeof(PRODUCTINFODATA));

                hpsTemp = WinGetPS(hwndDlg);
                if (hpsTemp)
                {
                    ppd->hbm = GpiLoadBitmap(hpsTemp,
                                             cmnQueryNLSModuleHandle(FALSE),
                                             ID_XFLDRBITMAP,
                                             0, 0); // no stretch
                    if (ppd->hbm)
                    {
                        HWND    hwndStatic = WinWindowFromID(hwndDlg, ID_XFD_PRODLOGO);
                        if (hwndStatic)
                        {
                            SWP     swpFrame;
                            BITMAPINFOHEADER2 bmih2;
                            bmih2.cbFix = sizeof(bmih2);
                            if (GpiQueryBitmapInfoHeader(ppd->hbm, &bmih2))
                            {
                                if (WinQueryWindowPos(hwndStatic,
                                                      &swpFrame))
                                {
                                    ppd->ptlBitmap.x = swpFrame.x
                                                       + ((swpFrame.cx - (LONG)bmih2.cx) / 2);
                                    ppd->ptlBitmap.y = swpFrame.y
                                                       + ((swpFrame.cy - (LONG)bmih2.cy) / 2);
                                }

                                WinDestroyWindow(hwndStatic);
                            }
                        }
                    }

                    WinReleasePS(hpsTemp);
                }
            }

            WinSetWindowPtr(hwndDlg, QWL_USER, ppd);
        break; }

        case WM_PAINT:
        {
            HPS hpsPaint = 0;
            mrc = cmn_fnwpDlgWithHelp(hwndDlg, msg, mp1, mp2);

            hpsPaint = WinGetPS(hwndDlg);
            if (hpsPaint)
            {
                PPRODUCTINFODATA ppd = (PPRODUCTINFODATA)WinQueryWindowPtr(hwndDlg, QWL_USER);
                if (ppd)
                {
                    if (ppd->hbm)
                    {
                        POINTL ptlDest;
                        ptlDest.x = ppd->ptlBitmap.x;
                        ptlDest.y = ppd->ptlBitmap.y;
                        WinDrawBitmap(hpsPaint,
                                      ppd->hbm,
                                      NULL,
                                      &ptlDest,
                                      0, 0, DBM_NORMAL);
                    }
                }

                WinReleasePS(hpsPaint);
            }
        break; }

        case WM_DESTROY:
        {
            PPRODUCTINFODATA ppd = (PPRODUCTINFODATA)WinQueryWindowPtr(hwndDlg, QWL_USER);
            if (ppd)
            {
                if (ppd->hbm)
                    GpiDeleteBitmap(ppd->hbm);
                free(ppd);
            }
        break; }
    }

    if (fCallDefault)
        mrc = cmn_fnwpDlgWithHelp(hwndDlg, msg, mp1, mp2);

    return (mrc);
}

/*
 *@@ cmnShowProductInfo:
 *      shows the XWorkplace "Product info" dlg.
 *      This calls WinProcessDlg in turn.
 *
 *@@added V0.9.1 (2000-02-13) [umoeller]
 *@@changed V0.9.5 (2000-10-07) [umoeller]: now using cmn_fnwpProductInfo
 *@@changed V0.9.13 (2001-06-23) [umoeller]: added hwndOwner
 */

VOID cmnShowProductInfo(HWND hwndOwner,     // in: owner window or NULLHANDLE
                        ULONG ulSound)      // in: sound intex to play
{
    // advertise for myself
    XSTRING strGPLInfo;
    LONG    lBackClr = CLR_WHITE;
    HWND hwndInfo = WinLoadDlg(HWND_DESKTOP,
                               hwndOwner,
                               cmn_fnwpProductInfo,
                               cmnQueryNLSModuleHandle(FALSE),
                               ID_XFD_PRODINFO,
                               NULL),
         hwndTextView;

    xstrInit(&strGPLInfo, 0);

    // text view (GPL info)
    txvRegisterTextView(WinQueryAnchorBlock(hwndInfo));
    hwndTextView = txvReplaceWithTextView(hwndInfo,
                                          ID_XLDI_TEXT2,
                                          WS_VISIBLE | WS_TABSTOP,
                                          XTXF_VSCROLL,
                                          2);
    WinSendMsg(hwndTextView, TXM_SETWORDWRAP, (MPARAM)TRUE, 0);
    WinSetPresParam(hwndTextView,
                    PP_BACKGROUNDCOLOR,
                    sizeof(ULONG),
                    &lBackClr);

    cmnSetControlsFont(hwndInfo, 1, 10000);

    cmnPlaySystemSound(ulSound);

    // load and convert info text
    cmnGetMessage(NULL, 0,
                  &strGPLInfo,
                  140);
    /* pszGPLInfo = strdup(szGPLInfo);
    txvStripLinefeeds(&pszGPLInfo, 4); */
    WinSetWindowText(hwndTextView, strGPLInfo.psz);

    // version string
    winhSetWindowText(WinWindowFromID(hwndInfo, ID_XFDI_XFLDVERSION),
                      "XWorkplace V%s (%s)",
                      BLDLEVEL_VERSION,
                      __DATE__);

    cmnSetDlgHelpPanel(0);
    winhCenterWindow(hwndInfo);
    WinProcessDlg(hwndInfo);
    WinDestroyWindow(hwndInfo);

    xstrClear(&strGPLInfo);
}

const char *G_apcszExtensions[]
    = {
                "EXE",
                "COM",
                "CMD",
                "BAT"
      };

/*
 *@@ StripParams:
 *      returns a new string with the executable
 *      name only.
 *
 *      Examples:
 *
 *      --  e c:\config.sys will return "e".
 *
 *      --  "my program" param will return "my program"
 *          (without quotes).
 *
 *@@added V0.9.11 (2001-04-18) [umoeller]
 */

PSZ StripParams(PSZ pcszCommand,
                PSZ *ppParams)      // out: ptr to first char of params
{
    PSZ pszReturn = NULL;

    if (pcszCommand && strlen(pcszCommand))
    {
        // parse the command line to check if we have
        // parameters
        if (*pcszCommand == '\"')
        {
            PSZ pSecondQuote = strchr(pcszCommand + 1, '\"');
            if (pSecondQuote)
            {
                pszReturn = strhSubstr(pcszCommand + 1, pSecondQuote);
                if (ppParams)
                    *ppParams = pSecondQuote + 1;
            }
        }
        else
        {
            // no quote first:
            // find first space --> parameters
            PSZ pSpace;
            if (pSpace = strchr(pcszCommand, ' '))
            {
                pszReturn = strhSubstr(pcszCommand, pSpace);
                if (ppParams)
                    *ppParams = pSpace + 1;
            }
        }

        if (!pszReturn)
            pszReturn = strdup(pcszCommand);
    }

    return (pszReturn);
}

/*
 *@@ GetExeFromControl:
 *      returns a fully qualified executable
 *      name from the text in a control.
 *
 *@@added V0.9.14 (2001-08-23) [pr]
 */

APIRET GetExeFromControl(HWND hwnd,
                         PSZ pszExecutable,
                         USHORT usExeLength)
{
    APIRET arc = ERROR_FILE_NOT_FOUND;

    PSZ pszCommand;
    if (pszCommand = winhQueryWindowText(hwnd))
    {
        // we got a command:
        PSZ pszExec;
        if (pszExec = StripParams(pszCommand,
                                  NULL))
        {
            if (!(arc = doshFindExecutable(pszExec,
                                           pszExecutable,
                                           usExeLength,
                                           G_apcszExtensions,
                                           ARRAYITEMCOUNT(G_apcszExtensions))))
                strupr(pszExecutable);

            _Pmpf((__FUNCTION__ ": doshFindExecutable returned %d", arc));

            free(pszExec);
        }

        free(pszCommand);
    }

    return(arc);
}

/*
 *@@ LoadRunHistory:
 *      Loads the Run dialog's combo box with
 *      the history list from the INI file.
 *
 *@@added V0.9.14 (2001-08-23) [pr]
 */

BOOL LoadRunHistory(HWND hwnd)
{
    USHORT i;
    BOOL   bOK = FALSE;

    for (i = 0; i < RUN_MAXITEMS; i++)
    {
        CHAR szKey[32], szData[CCHMAXPATH];

        sprintf(szKey, "%s%02u", INIKEY_RUNHISTORY, i);
        if (PrfQueryProfileString(HINI_USER,
                                  (PSZ)INIAPP_XCENTER,
                                  szKey,
                                  NULL,
                                  szData,
                                  sizeof(szData)))
        {
            WinInsertLboxItem(hwnd, i, szData);
            if (i == 0)
            {
                WinSetWindowText(hwnd, szData);
                bOK = !GetExeFromControl(hwnd, szData, sizeof(szData));
            }
        }
    }

    return(bOK);
}

/*
 *@@ SaveRunHistory:
 *      Saves the Run dialog's combo box
 *      history list to the INI file.
 *
 *@@added V0.9.14 (2001-08-23) [pr]
 */

VOID SaveRunHistory(HWND hwnd)
{
    USHORT i;

    for (i = 0; i < RUN_MAXITEMS; i++)
    {
        CHAR szKey[32], szData[CCHMAXPATH];

        sprintf(szKey, "%s%02u", INIKEY_RUNHISTORY, i);
        if (WinQueryLboxItemText(hwnd, i, szData, sizeof(szData)))
            PrfWriteProfileString(HINI_USER, (PSZ) INIAPP_XCENTER, szKey, szData);
        else
            break;
    }
}

/*
 *@@ UpdateRunHistory:
 *      Updates the Run dialog's combo box
 *      history list and changes the saved
 *      directory for the Browse dialog.
 *
 *@@added V0.9.14 (2001-08-23) [pr]
 */

VOID UpdateRunHistory(HWND hwnd)
{
    CHAR szData[CCHMAXPATH];
    USHORT i, usCount;
    BOOL bFound = FALSE;
    PSZ pszExec;

    WinQueryWindowText(hwnd, sizeof(szData), szData);
    usCount = WinQueryLboxCount(hwnd);
    for (i = 0; i < usCount; i++)
    {
        CHAR szHistory[CCHMAXPATH];

        if (   WinQueryLboxItemText(hwnd, i, szHistory, sizeof(szHistory))
            && (!stricmp(szData, szHistory))
           )
        {
            bFound = TRUE;
            break;
        }
    }

    if (bFound)
        WinDeleteLboxItem(hwnd, i);
    else
        if (usCount == RUN_MAXITEMS)
            WinDeleteLboxItem(hwnd, RUN_MAXITEMS - 1);

    WinInsertLboxItem(hwnd, 0, szData);
    if (pszExec = StripParams(szData, NULL))
    {
        PSZ p;

        for (p = pszExec + strlen(pszExec); p >= pszExec; p--)
            if (*p != '\\' && *p != ':')
                *p = '\0';
            else
                break;

        strcpy(G_szRunDirectory, pszExec);
        free(pszExec);
    }
}

/*
 *@@ fnwpRunCommandLine:
 *      window proc for "run" dialog.
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 *@@changed V0.9.11 (2001-04-18) [umoeller]: fixed parameters
 *@@changed V0.9.11 (2001-04-25) [umoeller]: fixed fully qualified executables
 *@@changed V0.9.14 (2001-08-23) [pr]: added more options and Browse button
 */

MRESULT EXPENTRY fnwpRunCommandLine(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    switch (msg)
    {
        case WM_CONTROL:
        {
            USHORT usid = SHORT1FROMMP(mp1);
            USHORT usNotifyCode = SHORT2FROMMP(mp1);
            CHAR szExecutable[CCHMAXPATH] = "";
            ULONG ulDosAppType, ulWinAppType;

            switch (usid)
            {
                case ID_XFD_RUN_COMMAND:
                    if (usNotifyCode == CBN_EFCHANGE)
                    {
                        BOOL bOK, bIsWinProg;
                        HWND hwndOK = WinWindowFromID(hwnd, DID_OK);
                        HWND hwndCancel = WinWindowFromID(hwnd, DID_CANCEL);
                        HWND hwndCommand = (HWND)mp2;

                        // Remove leading spaces
                        WinQueryWindowText(hwndCommand, sizeof(szExecutable), szExecutable);
                        if (szExecutable[0] == ' ')
                        {
                            PSZ p;

                            for (p = szExecutable; *p == ' '; p++)
                                ;

                            WinSetWindowText(hwndCommand, p);
                            WinSendMsg(hwndCommand,
                                       WM_CHAR,
                                       MPFROM2SHORT(KC_VIRTUALKEY, 0),
                                       MPFROM2SHORT(0, VK_HOME));
                        }

                        bOK = !GetExeFromControl(hwndCommand,
                                                 szExecutable,
                                                 sizeof(szExecutable));
                        bIsWinProg = (    (bOK)
                                       && (!appQueryAppType(szExecutable,
                                                            &ulDosAppType,
                                                            &ulWinAppType))
                                       && (ulWinAppType == PROG_31_ENHSEAMLESSCOMMON)
                                     );

                        WinEnableWindow(hwndOK, bOK);
                        if (!bOK)
                        {
                            HWND hwndTmp = hwndOK;
                            hwndOK = hwndCancel;
                            hwndCancel = hwndTmp;
                            // do not display the full path
                            // if the file wasn't found
                            szExecutable[0] = '\0';
                                    // V0.9.16 (2001-10-08) [umoeller]
                        }

                        WinSetWindowULong(hwnd,
                                          QWL_DEFBUTTON,
                                          hwndOK); // V0.9.15
                        WinSetWindowBits(hwndOK, QWL_STYLE, -1, WS_GROUP | BS_DEFAULT );
                        WinSetWindowBits(hwndCancel, QWL_STYLE, 0, WS_GROUP | BS_DEFAULT);
                        WinInvalidateRect(hwndOK, NULL, FALSE);
                        WinInvalidateRect(hwndCancel, NULL, FALSE);
                        winhEnableDlgItem(hwnd,
                                          ID_XFD_RUN_WINOS2_GROUP,
                                          bIsWinProg);
                        winhEnableDlgItem(hwnd,
                                          ID_XFD_RUN_ENHANCED,
                                          bIsWinProg);
                        winhEnableDlgItem(hwnd,
                                          ID_XFD_RUN_SEPARATE,
                                          (   (bIsWinProg)
                                           && (!winhIsDlgItemChecked(hwnd,
                                                                     ID_XFD_RUN_FULLSCREEN))));
                        WinSetDlgItemText(hwnd,
                                          ID_XFD_RUN_FULLPATH,
                                          szExecutable);
                    }
                break;

                case ID_XFD_RUN_FULLSCREEN:
                    if (   (usNotifyCode == BN_CLICKED)
                        || (usNotifyCode == BN_DBLCLICKED)
                       )
                    {
                        BOOL bOK = GetExeFromControl(WinWindowFromID(hwnd, ID_XFD_RUN_COMMAND),
                                                     szExecutable,
                                                     sizeof(szExecutable));
                        BOOL bIsWinProg = (    (bOK)
                                            && (!appQueryAppType(szExecutable,
                                                                 &ulDosAppType,
                                                                 &ulWinAppType))
                                            && (ulWinAppType == PROG_31_ENHSEAMLESSCOMMON)
                                          );
                        winhEnableDlgItem(hwnd,
                                          ID_XFD_RUN_SEPARATE,
                                          (    (bIsWinProg)
                                            && (!winhIsDlgItemChecked(hwnd, usid))));
                    }

                break;
            }
        break; }

        case WM_COMMAND:
        {
            USHORT usid = SHORT1FROMMP(mp1);

            switch(usid)
            {
                case ID_XFD_RUN_BROWSE:
                {
                    FILEDLG filedlg;
                    APSZ typelist[] = { "DOS Command File",
                                        "Executable",
                                        "OS/2 Command File",        // V0.9.16 (2001-09-29) [umoeller]
                                        NULL };
                    PSZ pszFilespec = "*.COM;*.EXE;*.CMD;*.BAT";

                    memset(&filedlg, '\0', sizeof(filedlg));
                    filedlg.cbSize = sizeof(filedlg);
                    filedlg.fl = FDS_OPEN_DIALOG | FDS_CENTER;
                    if (   strlen(G_szRunDirectory) + strlen(pszFilespec)
                         < sizeof(filedlg.szFullFile)
                       )
                    {
                        strcpy(filedlg.szFullFile, G_szRunDirectory);
                        strcat(filedlg.szFullFile, pszFilespec);
                    }
                    else
                        strcpy(filedlg.szFullFile, pszFilespec);

                    filedlg.papszITypeList = typelist;
                    if (    (WinFileDlg(HWND_DESKTOP, hwnd, &filedlg))
                         && (filedlg.lReturn == DID_OK)
                       )
                    {
                        PSZ p;

                        WinSetDlgItemText(hwnd, ID_XFD_RUN_COMMAND, filedlg.szFullFile);
                        for (p = filedlg.szFullFile + strlen(filedlg.szFullFile);
                             p >= filedlg.szFullFile;
                             p--)
                            if (*p != '\\' && *p != ':')
                                *p = '\0';
                            else
                                break;

                        strcpy(G_szRunDirectory, filedlg.szFullFile);
                    }

                break; }

                default:
                    mrc = WinDefDlgProc(hwnd, msg, mp1, mp2);

                break;
            }
        break; }

        case WM_HELP:
            cmnDisplayHelp(NULL,
                           ID_XSH_RUN);
        break;

        default:
            mrc = WinDefDlgProc(hwnd, msg, mp1, mp2);
    }

    return (mrc);
}

/*
 *@@ cmnRunCommandLine:
 *      displays a prompt dialog in which the user can
 *      enter a command line and then runs that command
 *      line using winhStartApp.
 *
 *      Returns the HAPP that was started or NULLHANDLE,
 *      e.g. if an error occured or the user cancelled
 *      the dialog.
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 *@@changed V0.9.11 (2001-04-18) [umoeller]: fixed parameters
 *@@changed V0.9.11 (2001-04-18) [umoeller]: fixed entry field lengths
 *@@changed V0.9.12 (2001-05-26) [umoeller]: added return value
 *@@changed V0.9.14 (2001-07-28) [umoeller]: fixed parameter handling which was ignored
 *@@changed V0.9.14 (2001-08-07) [pr]: changed dialog handling, fixed Win-OS/2 full-screen hang
 *@@changed V0.9.14 (2001-08-23) [pr]: added more options & Browse button
 */

HAPP cmnRunCommandLine(HWND hwndOwner,              // in: owner window or NULLHANDLE for active desktop
                       const char *pcszStartupDir)  // in: startup dir or NULL
{
    static HWND hwndDlg = NULLHANDLE;
    HAPP        happ = NULLHANDLE;

    // activate the current Run dialog if user tries to open a new one V0.9.14
    if (hwndDlg)
    {
        HWND    hwnd = hwndDlg, hwndTmp;

        // find the Browse dialog if it is open
        HENUM   henum = WinBeginEnumWindows(HWND_DESKTOP);
        while (hwndTmp = WinGetNextWindow(henum))
            if (WinQueryWindow(hwndTmp, QW_OWNER) == hwndDlg)
            {
                hwnd = hwndTmp;
                break;
            }
        WinEndEnumWindows(henum);

        WinSetFocus (HWND_DESKTOP, hwnd);
        return(happ);
    }

    /* V0.9.14 This is a very bad idea as it means the desktop is disabled;
    this is one of the things that causes Win-OS/2 full screen to fail
    V0.9.14 (2001-08-03) [pr]
    if (!hwndOwner)
        hwndOwner = cmnQueryActiveDesktopHWND(); */

    if (hwndDlg = WinLoadDlg(HWND_DESKTOP,
                             hwndOwner,
                             fnwpRunCommandLine,
                             cmnQueryNLSModuleHandle(FALSE),
                             ID_XFD_RUN,
                             NULL))
    {
        HWND    hwndCommand = WinWindowFromID(hwndDlg, ID_XFD_RUN_COMMAND),
                hwndStartup = WinWindowFromID(hwndDlg, ID_XFD_RUN_STARTUPDIR);

        winhSetEntryFieldLimit(hwndCommand, CCHMAXPATH);
        if (LoadRunHistory(hwndCommand))
        {
            HWND hwndOK = WinWindowFromID(hwndDlg, DID_OK);
            HWND hwndCancel = WinWindowFromID(hwndDlg, DID_CANCEL);

            WinEnableWindow(hwndOK, TRUE);
            WinSetWindowULong(hwndDlg, QWL_DEFBUTTON, DID_OK);
            WinSetWindowBits(hwndOK, QWL_STYLE, -1, WS_GROUP | BS_DEFAULT );
            WinSetWindowBits(hwndCancel, QWL_STYLE, 0, WS_GROUP | BS_DEFAULT);
        }

        winhSetEntryFieldLimit(hwndStartup, CCHMAXPATH);
        WinSetWindowText(hwndStartup, pcszStartupDir);

        cmnSetControlsFont(hwndDlg, 1, 10000);
        winhSetDlgItemChecked(hwndDlg, ID_XFD_RUN_AUTOCLOSE, TRUE);
        winhSetDlgItemChecked(hwndDlg, ID_XFD_RUN_ENHANCED, TRUE); // V0.9.14
        winhCenterWindow(hwndDlg);

        // go!
        if (WinProcessDlg(hwndDlg) == DID_OK)
        {
            PSZ pszCommand = winhQueryWindowText(hwndCommand);
            PSZ pszStartup = winhQueryWindowText(hwndStartup);

            if (pszCommand)
            {
                APIRET  arc = NO_ERROR;
                PSZ     pszExec,
                        pParams = NULL;
                CHAR    szExecutable[CCHMAXPATH];

                UpdateRunHistory(hwndCommand);
                SaveRunHistory(hwndCommand);
                if (!pszStartup)
                {
                    pszStartup = strdup("?:\\");
                    *pszStartup = doshQueryBootDrive();
                }

                pszExec = StripParams(pszCommand,
                                      &pParams);
                if (!pszExec)
                    arc = ERROR_INVALID_PARAMETER;
                else
                {
                    arc = doshFindExecutable(pszExec,
                                             szExecutable,
                                             sizeof(szExecutable),
                                             G_apcszExtensions,
                                             ARRAYITEMCOUNT(G_apcszExtensions));
                    free(pszExec);
                }

                if (arc != NO_ERROR)
                {
                    PSZ pszError = doshQuerySysErrorMsg(arc);
                    if (pszError)
                    {
                        cmnMessageBox(hwndOwner,
                                      pszCommand,
                                      pszError,
                                      MB_CANCEL);
                        free(pszError);
                    }
                }
                else
                {
                    PROGDETAILS pd;
                    ULONG   ulDosAppType, ulFlags = 0;
                    memset(&pd, 0, sizeof(pd));

                    if (!(arc = appQueryAppType(szExecutable,
                                                &ulDosAppType,
                                                &pd.progt.progc)))
                    {
                        pd.progt.fbVisible = SHE_VISIBLE;
                        pd.pszExecutable = szExecutable;
                        strupr(szExecutable);
                        pd.pszParameters = (PSZ)pParams;
                        pd.pszStartupDir = pszStartup;

                        pd.swpInitial.hwndInsertBehind = HWND_TOP; // V0.9.14
                        if (winhIsDlgItemChecked(hwndDlg, ID_XFD_RUN_MINIMIZED))
                            pd.swpInitial.fl = SWP_MINIMIZE;
                        else
                            pd.swpInitial.fl = SWP_ACTIVATE; // V0.9.14

                        if (!winhIsDlgItemChecked(hwndDlg, ID_XFD_RUN_AUTOCLOSE))
                            pd.swpInitial.fl |= SWP_NOAUTOCLOSE; // V0.9.14

                        if (winhIsDlgItemChecked(hwndDlg, ID_XFD_RUN_FULLSCREEN))
                            ulFlags |= APP_RUN_FULLSCREEN;

                        if (winhIsDlgItemChecked(hwndDlg, ID_XFD_RUN_ENHANCED))
                            ulFlags |= APP_RUN_ENHANCED;
                        else
                            ulFlags |= APP_RUN_STANDARD;

                        if (winhIsDlgItemChecked(hwndDlg, ID_XFD_RUN_SEPARATE))
                            ulFlags |= APP_RUN_SEPARATE;

                        happ = appStartApp(NULLHANDLE,        // no notify
                                           &pd,
                                           ulFlags); //V0.9.14
                    }
                }
            }

            if (pszCommand)
                free(pszCommand);
            if (pszStartup)
                free(pszStartup);
        }
        WinDestroyWindow(hwndDlg);
        hwndDlg = NULLHANDLE; // V0.9.14
    }

    return (happ);      // V0.9.12 (2001-05-26) [umoeller]
}

/*
 *@@ cmnQueryDefaultFont:
 *      this returns the font to be used for dialogs.
 *      If the "Use 8.Helv" checkbox is enabled on
 *      the "Paranoia" page, we return "8.Helv",
 *      otherwise "9.WarpSans". The returned font
 *      string is static, so don't attempt to free it.
 *
 *@@added V0.9.0 [umoeller]
 */

const char* cmnQueryDefaultFont(VOID)
{
#ifndef __XWPLITE__
    if (G_GlobalSettings.fUse8HelvFont)
        return ("8.Helv");
    else
#endif
        return ("9.WarpSans");
}

/*
 *@@ cmnSetControlsFont:
 *      this sets the font presentation parameters for a dialog
 *      window. See winhSetControlsFont for the parameters.
 *      This calls cmnQueryDefaultFont in turn.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.7 (2000-12-13) [umoeller]: removed krnLock(), which wasn't needed here
 */

VOID cmnSetControlsFont(HWND hwnd,
                        SHORT usIDMin,
                        SHORT usIDMax)
{
    winhSetControlsFont(hwnd,
                        usIDMin,
                        usIDMax,
                        (PSZ)cmnQueryDefaultFont());
}

/*
 *@@ cmnMessageBox:
 *      this is the generic function for displaying XFolder
 *      message boxes. This is very similar to WinMessageBox,
 *      but looks a lot better, especially since IBM chose
 *      to make message boxes so small with FP13. In
 *      addition, an XFolder icon is displayed.
 *
 *      Currently the following flStyle's are supported:
 *
 *      -- MB_OK                      0x0000
 *      -- MB_OKCANCEL                0x0001
 *      -- MB_RETRYCANCEL             0x0002
 *      -- MB_ABORTRETRYIGNORE        0x0003
 *      -- MB_YESNO                   0x0004
 *      -- MB_YESNOCANCEL             0x0005
 *      -- MB_CANCEL                  0x0006
 *      -- MB_ENTER                   0x0007 (not implemented yet)
 *      -- MB_ENTERCANCEL             0x0008 (not implemented yet)
 *
 *      -- MB_YES_YES2ALL_NO          0x0009
 *          This is new: this has three buttons called "Yes"
 *          (MBID_YES), "Yes to all" (MBID_YES2ALL), "No" (MBID_NO).
 *
 *      -- MB_DEFBUTTON2            (for two-button styles)
 *      -- MB_DEFBUTTON3            (for three-button styles)
 *
 *      -- MB_ICONHAND
 *      -- MB_ICONEXCLAMATION
 *
 *      Returns MBID_* codes like WinMessageBox.
 *
 *@@changed V0.9.0 [umoeller]: added support for MB_YESNOCANCEL
 *@@changed V0.9.0 [umoeller]: fixed default button bugs
 *@@changed V0.9.0 [umoeller]: added WinAlarm sound support
 *@@changed V0.9.3 (2000-05-05) [umoeller]: extracted cmnLoadMessageBoxDlg
 *@@changed V0.9.13 (2001-06-23) [umoeller]: completely rewritten, now using dlghMessageBox
 */

ULONG cmnMessageBox(HWND hwndOwner,     // in: owner
                    const char *pcszTitle,       // in: msgbox title
                    const char *pcszMessage,     // in: msgbox text
                    ULONG flStyle)      // in: MB_* flags
{
    ULONG   ulrc = DID_CANCEL;

    // set our extended exception handler
    TRY_LOUD(excpt1)
    {
        static hptrIcon = NULLHANDLE;
        static MSGBOXSTRINGS Strings;

        if (!hptrIcon)
        {
            // first call:
            hptrIcon = WinLoadPointer(HWND_DESKTOP,
                                      cmnQueryMainResModuleHandle(),
                                      ID_ICONDLG);
            // load all the strings too
            Strings.pcszYes = cmnGetString(ID_XSSI_DLG_YES);
            Strings.pcszNo = cmnGetString(ID_XSSI_DLG_NO);
            Strings.pcszOK = cmnGetString(ID_XSSI_DLG_OK);
            Strings.pcszCancel = cmnGetString(ID_XSSI_DLG_CANCEL);
            Strings.pcszAbort = cmnGetString(ID_XSSI_DLG_ABORT);
            Strings.pcszRetry = cmnGetString(ID_XSSI_DLG_RETRY);
            Strings.pcszIgnore = cmnGetString(ID_XSSI_DLG_IGNORE);
            Strings.pcszEnter = "Enter"; // never used anyway
            Strings.pcszYesToAll = cmnGetString(ID_XSSI_DLG_YES2ALL);
        }

        // now using new dynamic dialog routines
        // V0.9.13 (2001-06-23) [umoeller]
        ulrc = dlghMessageBox(hwndOwner,
                              hptrIcon,
                              pcszTitle,
                              pcszMessage,
                              flStyle,
                              cmnQueryDefaultFont(),
                              &Strings);

    }
    CATCH(excpt1) { } END_CATCH();

    return (ulrc);
}

/*
 *@@ cmnGetMessageExt:
 *      retrieves a message string from the XWorkplace
 *      TMF message file. The message is specified
 *      using the TMF message ID string directly.
 *      This gets called from cmnGetMessage.
 *
 *      The XSTRING is assumed to be initialized.
 *
 *@@added V0.9.4 (2000-06-17) [umoeller]
 *@@changed V0.9.16 (2001-10-08) [umoeller]: now using XSTRING
 */

APIRET cmnGetMessageExt(PCHAR *pTable,     // in: replacement PSZ table or NULL
                        ULONG ulTable,     // in: size of that table or 0
                        PXSTRING pstr,     // in/out: string
                        PCSZ pcszMsgID)    // in: msg ID to retrieve
{
    APIRET  arc = NO_ERROR;
    BOOL fLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        #ifdef DEBUG_LANGCODES
            _Pmpf(("cmnGetMessage %s %s", pszMessageFile, pszMsgId));
        #endif

        if (fLocked = krnLock(__FILE__, __LINE__, __FUNCTION__))
        {
            if (!G_pXWPMsgFile)
            {
                // first call:
                // go load the XWP message file
                arc = tmfOpenMessageFile(cmnQueryMessageFile(),
                                         &G_pXWPMsgFile);
            }

            if (!arc)
            {
                arc = tmfGetMessage(G_pXWPMsgFile,
                                    pcszMsgID,
                                    pstr,
                                    pTable,
                                    ulTable);

                #ifdef DEBUG_LANGCODES
                    _Pmpf(("  tmfGetMessage rc: %d", arc));
                #endif

                if (!arc)
                    ReplaceEntities(pstr);
                else
                {
                    CHAR sz[500];
                    sprintf(sz,
                            "Message %s not found in %s, rc = %d",
                            pcszMsgID,
                            cmnQueryMessageFile(),
                            arc);
                    xstrcpy(pstr, sz, 0);
                }
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    if (fLocked)
        krnUnlock();

    return (arc);
}

/*
 *@@ cmnGetMessage:
 *      like DosGetMessage, but automatically uses the
 *      (NLS) XFolder message file.
 *      The parameters are exactly like with DosGetMessage.
 *      The message code (ulMsgNumber) is automatically
 *      converted to a TMF message ID.
 *
 *      The XSTRING is assumed to be initialized.
 *
 *      <B>Returns:</B> the error code of tmfGetMessage.
 *
 *@@changed V0.9.0 [umoeller]: changed, this now uses the TMF file format (tmsgfile.c).
 *@@changed V0.9.4 (2000-06-18) [umoeller]: extracted cmnGetMessageExt
 *@@changed V0.9.16 (2001-10-08) [umoeller]: now using XSTRING
 */

APIRET cmnGetMessage(PCHAR *pTable,     // in: replacement PSZ table or NULL
                     ULONG ulTable,     // in: size of that table or 0
                     PXSTRING pstr,     // in/out: string
                     ULONG ulMsgNumber) // in: msg number to retrieve
{
    CHAR szMessageName[40];
    // create string message identifier from ulMsgNumber
    sprintf(szMessageName, "XFL%04d", ulMsgNumber);

    return (cmnGetMessageExt(pTable, ulTable, pstr, szMessageName));
}

/*
 *@@ cmnMessageBoxMsg:
 *      calls cmnMessageBox, but this one accepts ULONG indices
 *      into the XFolder message file (XFLDRxxx.MSG) instead
 *      of real PSZs. This calls cmnGetMessage for retrieving
 *      the messages, but placeholder replacement does not work
 *      here (use cmnMessageBoxMsgExt for that).
 *
 *@@changed V0.9.16 (2001-10-08) [umoeller]: now using XSTRINGs
 */

ULONG cmnMessageBoxMsg(HWND hwndOwner,
                       ULONG ulTitle,       // in: msg index for dlg title
                       ULONG ulMessage,     // in: msg index for message
                       ULONG flStyle)       // in: like cmnMsgBox
{
    ULONG ulrc;

    XSTRING strTitle, strMessage;
    xstrInit(&strTitle, 0);
    xstrInit(&strMessage, 0);

    cmnGetMessage(NULL, 0,
                  &strTitle,
                  ulTitle);
    cmnGetMessage(NULL, 0,
                  &strMessage,
                  ulMessage);

    ulrc = cmnMessageBox(hwndOwner, strTitle.psz, strMessage.psz, flStyle);

    xstrClear(&strTitle);
    xstrClear(&strMessage);

    return (ulrc);
}

/*
 *@@ cmnMessageBoxMsgExt:
 *      like cmnMessageBoxMsg, but with string substitution
 *      (see cmnGetMessage for more); substitution only
 *      takes place for the message specified with ulMessage,
 *      not for the title.
 */

ULONG cmnMessageBoxMsgExt(HWND hwndOwner,   // in: owner window
                          ULONG ulTitle,    // in: msg number for title
                          PCHAR *pTable,    // in: replacement table for ulMessage
                          ULONG ulTable,    // in: array count in *pTable
                          ULONG ulMessage,  // in: msg number for message
                          ULONG flStyle)    // in: msg box style flags (cmnMessageBox)
{
    ULONG ulrc;

    XSTRING strTitle, strMessage;
    xstrInit(&strTitle, 0);
    xstrInit(&strMessage, 0);

    cmnGetMessage(NULL, 0,
                  &strTitle,
                  ulTitle);
    cmnGetMessage(pTable, ulTable,
                  &strMessage,
                  ulMessage);

    ulrc = cmnMessageBox(hwndOwner, strTitle.psz, strMessage.psz, flStyle);

    xstrClear(&strTitle);
    xstrClear(&strMessage);

    return (ulrc);
}

/*
 *@@ cmnDosErrorMsgBox:
 *      displays a DOS error message.
 *      This calls cmnMessageBox in turn.
 *
 *@@added V0.9.1 (2000-02-08) [umoeller]
 *@@changed V0.9.3 (2000-04-09) [umoeller]: added error explanation
 *@@changed V0.9.13 (2001-06-14) [umoeller]: reduced stack consumption
 */

ULONG cmnDosErrorMsgBox(HWND hwndOwner,     // in: owner window.
                        CHAR cDrive,        // in: drive letter
                        PSZ pszTitle,       // in: msgbox title
                        APIRET arc,         // in: DOS error code to get msg for
                        ULONG ulFlags,      // in: as in cmnMessageBox flStyle
                        BOOL fShowExplanation) // in: if TRUE, we'll retrieve an explanation as with the HELP command
{
    ULONG   mbrc = 0;
    CHAR    szMsgBuf[1000];
    XSTRING strError;
    ULONG   ulLen = 0;
    APIRET  arc2 = NO_ERROR;

    // get error message for APIRET
    CHAR    szDrive[3] = "?:";
    PSZ     pszTable = szDrive;
    szDrive[0] = cDrive;

    xstrInit(&strError, 0);

    if (!(arc2 = DosGetMessage(&pszTable, 1,
                               szMsgBuf, sizeof(szMsgBuf),
                               arc,
                               "OSO001.MSG",        // default OS/2 message file
                               &ulLen)))
    {
        szMsgBuf[ulLen] = 0;
        xstrcpy(&strError, szMsgBuf, 0);

        if (fShowExplanation)
        {
            // get help too
            if (!(arc2 = DosGetMessage(&pszTable, 1,
                                       szMsgBuf, sizeof(szMsgBuf),
                                       arc,
                                       "OSO001H.MSG",        // default OS/2 help message file
                                       &ulLen)))
            {
                szMsgBuf[ulLen] = 0;
                xstrcatc(&strError, '\n');
                xstrcat(&strError, szMsgBuf, 0);
            }
        }
    }
    else
    {
        // cannot find msg:
        CHAR szError3[20];
        PSZ apsz = szError3;
        sprintf(szError3, "%d", arc);
        cmnGetMessage(&apsz,
                      1,
                      &strError,
                      219);          // "error %d occured"
    }

    mbrc = cmnMessageBox(HWND_DESKTOP,
                         pszTitle,
                         strError.psz,
                         ulFlags);
    xstrClear(&strError);

    return (mbrc);
}

/*
 *@@ cmnTextEntryBox:
 *      wrapper around dlghTextEntryBox, which
 *      was moved to dialog.c with V0.9.15.
 *
 *@@added V0.9.13 (2001-06-19) [umoeller]
 *@@changed V0.9.15 (2001-09-14) [umoeller]: moved actual code to dialog.c
 */

PSZ cmnTextEntryBox(HWND hwndOwner,
                    const char *pcszTitle,
                    const char *pcszDescription,
                    const char *pcszDefault,
                    ULONG ulMaxLen,
                    ULONG fl)
{
    return (dlghTextEntryBox(hwndOwner,
                             pcszTitle,
                             pcszDescription,
                             pcszDefault,
                             cmnGetString(ID_XSSI_DLG_OK),
                             cmnGetString(ID_XSSI_DLG_CANCEL),
                             ulMaxLen,
                             fl,
                             cmnQueryDefaultFont()));
}

/*
 *@@ cmnSetDlgHelpPanel:
 *      sets help panel before calling fnwpDlgGeneric.
 */

VOID cmnSetDlgHelpPanel(ULONG ulHelpPanel)
{
    G_ulCurHelpPanel = ulHelpPanel;
}

/*
 *@@  cmn_fnwpDlgWithHelp:
 *          this is the dlg procedure for XFolder dlg boxes;
 *          it can process WM_HELP messages. All other messages
 *          are passed to WinDefDlgProc.
 *
 *          Use cmnSetDlgHelpPanel to set the help panel before
 *          using this dlg proc.
 *
 *@@changed V0.9.2 (2000-03-04) [umoeller]: renamed from fnwpDlgGeneric
 */

MRESULT EXPENTRY cmn_fnwpDlgWithHelp(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = NULL;

    switch (msg)
    {
        case WM_HELP:
        {
            // HMODULE hmodResource = cmnQueryNLSModuleHandle(FALSE);
            /* WM_HELP is received by this function when F1 or a "help" button
               is pressed in a dialog window. */
            // ulCurHelpPanel is set by instance methods before creating a
            // dialog box in order to link help topics to the displayed
            // dialog box. Possible values are:
            //      0: open online reference ("<XWP_REF>", INF book)
            //    > 0: open help topic in xfldr.hlp
            //     -1: ignore WM_HELP */

            if (G_ulCurHelpPanel > 0)
            {
                // replaced all the following V0.9.16 (2001-10-15) [umoeller]
                cmnDisplayHelp(NULL, G_ulCurHelpPanel);
                /* WPObject    *pHelpSomSelf = cmnQueryActiveDesktop();
                if (pHelpSomSelf)
                {
                    const char* pszHelpLibrary;
                    BOOL fProcessed = FALSE;
                    if (pszHelpLibrary = cmnQueryHelpLibrary())
                        // path found: display help panel
                        if (_wpDisplayHelp(pHelpSomSelf, G_ulCurHelpPanel, (PSZ)pszHelpLibrary))
                            fProcessed = TRUE;

                    if (!fProcessed)
                        cmnMessageBoxMsg(HWND_DESKTOP, 104, 134, MB_OK);
                } */
            }
            else if (G_ulCurHelpPanel == 0)
            {
                HOBJECT     hobjRef = 0;
                // open online reference
                // G_ulCurHelpPanel = -1; // ignore further WM_HELP messages: this one suffices
                hobjRef = WinQueryObject((PSZ)XFOLDER_USERGUIDE);
                if (hobjRef)
                    WinOpenObject(hobjRef, OPEN_DEFAULT, TRUE);
                else
                    cmnMessageBoxMsg(HWND_DESKTOP, 104, 137, MB_OK);

            } // end else; if ulCurHelpPanel is < 0, nothing happens
            mrc = NULL;
        break; } // end case WM_HELP

        default:
            mrc = WinDefDlgProc(hwnd, msg, mp1, mp2);
        break;
    }

    return (mrc);
}

/*
 *@@ cmnFileDlg:
 *      same as winhFileDlg, but uses fdlgFileDlg
 *      instead.
 *
 *@@added V0.9.9 (2001-03-10) [umoeller]
 */

BOOL cmnFileDlg(HWND hwndOwner,    // in: owner for file dlg
                PSZ pszFile,       // in: file mask; out: fully q'd filename
                                   //    (should be CCHMAXPATH in size)
                ULONG flFlags,     // in: any combination of the following:
                                   // -- WINH_FOD_SAVEDLG: save dlg; else open dlg
                                   // -- WINH_FOD_INILOADDIR: load FOD path from INI
                                   // -- WINH_FOD_INISAVEDIR: store FOD path to INI on OK
                HINI hini,         // in: INI file to load/store last path from (can be HINI_USER)
                const char *pcszApplication, // in: INI application to load/store last path from
                const char *pcszKey)        // in: INI key to load/store last path from
{
    FILEDLG fd;
    memset(&fd, 0, sizeof(FILEDLG));
    fd.cbSize = sizeof(FILEDLG);
    fd.fl = FDS_CENTER;

    if (flFlags & WINH_FOD_SAVEDLG)
        fd.fl |= FDS_SAVEAS_DIALOG;
    else
        fd.fl |= FDS_OPEN_DIALOG;

    // default: copy pszFile
    strcpy(fd.szFullFile, pszFile);

    if ( (hini) && (flFlags & WINH_FOD_INILOADDIR) )
    {
        // overwrite with initial directory for FOD from OS2.INI
        if (PrfQueryProfileString(hini,
                                  (PSZ)pcszApplication,
                                  (PSZ)pcszKey,
                                  "",      // default string V0.9.9 (2001-02-10) [umoeller]
                                  fd.szFullFile,
                                  sizeof(fd.szFullFile)-10)
                    >= 2)
        {
            // found: append "\*"
            strcat(fd.szFullFile, "\\");
            strcat(fd.szFullFile, pszFile);
        }
    }

    if (    fdlgFileDlg(hwndOwner, // owner
                        NULL,
                        &fd)
        && (fd.lReturn == DID_OK)
       )
    {
        _Pmpf((__FUNCTION__ ": got DID_OK"));

        // save path back?
        if (    (hini)
             && (flFlags & WINH_FOD_INISAVEDIR)
           )
        {
            // get the directory that was used
            PSZ p = strrchr(fd.szFullFile, '\\');
            if (p)
            {
                // contains directory:
                // copy to OS2.INI
                PSZ pszDir = strhSubstr(fd.szFullFile, p);
                if (pszDir)
                {
                    PrfWriteProfileString(hini,
                                          (PSZ)pcszApplication,
                                          (PSZ)pcszKey,
                                          pszDir);
                    free(pszDir);
                }
            }
        }

        strcpy(pszFile, fd.szFullFile);

        return (TRUE);
    }

    return (FALSE);
}

