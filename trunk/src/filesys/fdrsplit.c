
/*
 *@@sourcefile fdrsplit.c:
 *      folder "split view" implementation.
 *
 *
 *@@added V0.9.21 (2002-08-21) [umoeller]
 *@@header "filesys\folder.h"
 */

/*
 *      Copyright (C) 2001-2002 Ulrich M�ller.
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
#define INCL_WINPOINTERS
#define INCL_WININPUT
#define INCL_WINSTDCNR
#define INCL_WINSHELLDATA
#define INCL_WINSCROLLBARS
#define INCL_WINSYS
#define INCL_WINTIMER

#define INCL_GPIBITMAPS
#define INCL_GPIREGIONS
#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\comctl.h"             // common controls (window procs)
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\cnrh.h"               // container helper routines
#include "helpers\except.h"             // exception handling
#include "helpers\gpih.h"               // GPI helper routines
#include "helpers\linklist.h"           // linked list helper routines
#include "helpers\standards.h"          // some standard macros
#include "helpers\threads.h"            // thread helpers
#include "helpers\winh.h"               // PM helper routines

// SOM headers which don't crash with prec. header files
#include "xfldr.ih"
#include "xfdisk.ih"
#include "xfobj.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\classtest.h"           // some cheap funcs for WPS class checks
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\cnrsort.h"             // container sort comparison functions
#include "shared\wpsh.h"                // some pseudo-SOM functions (WPS helper routines)

#include "filesys\folder.h"             // XFolder implementation
#include "filesys\fdrsplit.h"           // folder split views
#include "filesys\fdrviews.h"           // common code for folder views
#include "filesys\object.h"             // XFldObject implementation
#include "filesys\statbars.h"           // status bar translation logic

// other SOM headers
#pragma hdrstop                         // VAC++ keeps crashing otherwise

/* ******************************************************************
 *
 *   Private declarations
 *
 ********************************************************************/

PCSZ    WC_SPLITVIEWCLIENT  = "XWPSplitViewClient",
        WC_SPLITPOPULATE   = "XWPSplitViewPopulate";

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

PFNWP       G_pfnFrameProc = NULL;

/* ******************************************************************
 *
 *   Split Populate thread
 *
 ********************************************************************/

/*
 *@@ AddFirstChild:
 *      adds the first child record for precParent
 *      to the given container if precParent represents
 *      a folder that has subfolders.
 *
 *      This gets called for every record in the drives
 *      tree so we properly add the "+" expansion signs
 *      to each record without having to fully populate
 *      each folder. This is an imitation of the standard
 *      WPS behavior in Tree views.
 *
 *      Runs on the split populate thread (fntSplitPopulate).
 *
 *@@added V0.9.18 (2002-02-06) [umoeller]
 */

static WPObject* AddFirstChild(WPFolder *pFolder,
                               PMINIRECORDCORE precParent,     // in: folder record to insert first child for
                               HWND hwndCnr,                   // in: cnr where precParent is inserted
                               PLINKLIST pll)                  // in/out: list of objs
{
    PMINIRECORDCORE     precFirstChild;
    WPFolder            *pFirstChildFolder = NULL;

    if (!(precFirstChild = (PMINIRECORDCORE)WinSendMsg(hwndCnr,
                                                       CM_QUERYRECORD,
                                                       (MPARAM)precParent,
                                                       MPFROM2SHORT(CMA_FIRSTCHILD,
                                                                    CMA_ITEMORDER))))
    {
        // we don't have a first child already:

        // check if we have a subfolder in the folder already
        BOOL    fFolderLocked = FALSE,
                fFindLocked = FALSE;

        #ifdef DEBUG_POPULATESPLITVIEW
            _Pmpf(("  "__FUNCTION__": CM_QUERYRECORD returned NULL"));
        #endif

        TRY_LOUD(excpt1)
        {
            // request the find sem to make sure we won't have a populate
            // on the other thread; otherwise we get duplicate objects here
            if (fFindLocked = !_wpRequestFindMutexSem(pFolder, SEM_INDEFINITE_WAIT))
            {
                WPObject    *pObject;

                if (fFolderLocked = !_wpRequestFolderMutexSem(pFolder, SEM_INDEFINITE_WAIT))
                {
                    for (   pObject = _wpQueryContent(pFolder, NULL, QC_FIRST);
                            pObject;
                            pObject = *__get_pobjNext(pObject))
                    {
                        if (fdrvIsInsertable(pObject,
                                             INSERT_FOLDERSONLY,
                                             NULL))
                        {
                            pFirstChildFolder = pObject;
                            break;
                        }
                    }

                    _wpReleaseFolderMutexSem(pFolder);
                    fFolderLocked = FALSE;
                }

                #ifdef DEBUG_POPULATESPLITVIEW
                    _Pmpf(("  "__FUNCTION__": pFirstChildFolder pop is 0x%lX", pFirstChildFolder));
                #endif

                if (!pFirstChildFolder)
                {
                    // no folder awake in folder yet:
                    // do a quick DosFindFirst loop to find the
                    // first subfolder in here
                    HDIR          hdir = HDIR_CREATE;
                    FILEFINDBUF3  ffb3     = {0};
                    ULONG         ulFindCount    = 1;        // look for 1 file at a time
                    APIRET        arc            = NO_ERROR;

                    CHAR          szFolder[CCHMAXPATH],
                                  szSearchMask[CCHMAXPATH];

                    _wpQueryFilename(pFolder, szFolder, TRUE);
                    sprintf(szSearchMask, "%s\\*", szFolder);

                    #ifdef DEBUG_POPULATESPLITVIEW
                        _Pmpf(("  "__FUNCTION__": searching %s", szSearchMask));
                    #endif

                    ulFindCount = 1;
                    arc = DosFindFirst(szSearchMask,
                                       &hdir,
                                       MUST_HAVE_DIRECTORY | FILE_ARCHIVED | FILE_SYSTEM | FILE_READONLY,
                                             // but exclude hidden
                                       &ffb3,
                                       sizeof(ffb3),
                                       &ulFindCount,
                                       FIL_STANDARD);

                    while ((arc == NO_ERROR))
                    {
                        #ifdef DEBUG_POPULATESPLITVIEW
                            _Pmpf(("      "__FUNCTION__": got %s", ffb3.achName));
                        #endif

                        // do not use "." and ".."
                        if (    (strcmp(ffb3.achName, ".") != 0)
                             && (strcmp(ffb3.achName, "..") != 0)
                           )
                        {
                            // this is good:
                            CHAR szFolder2[CCHMAXPATH];
                            sprintf(szFolder2, "%s\\%s", szFolder, ffb3.achName);

                            #ifdef DEBUG_POPULATESPLITVIEW
                                _Pmpf(("      "__FUNCTION__": awaking %s", szFolder2));
                            #endif

                            pObject = _wpclsQueryFolder(_WPFolder,
                                                        szFolder2,
                                                        TRUE);
                            // exclude templates
                            if (fdrvIsInsertable(pObject,
                                                 INSERT_FOLDERSONLY,
                                                 NULL))
                            {
                                pFirstChildFolder = pObject;
                                break;
                            }
                        }

                        // search next file
                        ulFindCount = 1;
                        arc = DosFindNext(hdir,
                                         &ffb3,
                                         sizeof(ffb3),
                                         &ulFindCount);

                    } // end while (rc == NO_ERROR)

                    DosFindClose(hdir);
                }
            }
        }
        CATCH(excpt1)
        {
        } END_CATCH();

        if (fFolderLocked)
            _wpReleaseFolderMutexSem(pFolder);
        if (fFindLocked)
            _wpReleaseFindMutexSem(pFolder);

        if (pFirstChildFolder)
        {
            POINTL ptl = {0, 0};
            if (_wpCnrInsertObject(pFirstChildFolder,
                                   hwndCnr,
                                   &ptl,        // without this the func fails
                                   precParent,
                                   NULL))
                lstAppendItem(pll,
                              pFirstChildFolder);
        }
    }

    return pFirstChildFolder;
}

/*
 *@@ fnwpSplitPopulate:
 *      object window for populate thread.
 *
 *      Runs on the split populate thread (fntSplitPopulate).
 */

static MRESULT EXPENTRY fnwpSplitPopulate(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    switch (msg)
    {
        case WM_CREATE:
            WinSetWindowPtr(hwnd, QWL_USER, mp1);       // PFDRSPLITVIEW
        break;

        /*
         *@@ FM2_POPULATE:
         *      posted by fnwpMainControl when
         *      FM_FILLFOLDER comes in to offload
         *      populate to this second thread.
         *
         *      After populate is done, we post the
         *      following back to fnwpMainControl:
         *
         *      --  FM_POPULATED_FILLTREE always so
         *          that the drives tree can get
         *          updated;
         *
         *      --  FM_POPULATED_SCROLLTO, if the
         *          FFL_SCROLLTO flag was set;
         *
         *      --  FM_POPULATED_FILLFILES, if the
         *          FFL_FOLDERSONLY flag was _not_
         *          set.
         *
         *      This processing is all new with V0.9.18
         *      to finally synchronize the populate with
         *      the main thread better.
         *
         *      Parameters:
         *
         *      --  PMINIRECORDCORE mp1
         *
         *      --  ULONG mp2: flags, as with FM_FILLFOLDER.
         *
         *@@added V0.9.18 (2002-02-06) [umoeller]
         */

        case FM2_POPULATE:
        {
            PFDRSPLITVIEW   psv = WinQueryWindowPtr(hwnd, QWL_USER);
            WPFolder        *pFolder;
            PMINIRECORDCORE prec = (PMINIRECORDCORE)mp1;
            ULONG           fl = (ULONG)mp2;

            if (pFolder = fdrvGetFSFromRecord(prec, TRUE))
            {
                BOOL fFoldersOnly = ((fl & FFL_FOLDERSONLY) != 0);

                // set wait pointer
                (psv->cThreadsRunning)++;
                WinPostMsg(psv->hwndMainControl,
                           FM_UPDATEPOINTER,
                           0,
                           0);

                #ifdef DEBUG_POPULATESPLITVIEW
                    _PmpfF(("populating %s", _wpQueryTitle(pFolder)));
                #endif

                if (fdrCheckIfPopulated(pFolder,
                                        fFoldersOnly))
                {
                    // in any case, refresh the tree
                    WinPostMsg(psv->hwndMainControl,
                               FM_POPULATED_FILLTREE,
                               (MPARAM)prec,
                               (MPARAM)fl);
                            // fnwpMainControl will check fl again and
                            // fire "add first child" msgs accordingly

                    if (fl & FFL_SCROLLTO)
                        WinPostMsg(psv->hwndMainControl,
                                   FM_POPULATED_SCROLLTO,
                                   (MPARAM)prec,
                                   0);

                    if (!fFoldersOnly)
                        // refresh the files only if we are not
                        // in folders-only mode
                        WinPostMsg(psv->hwndMainControl,
                                   FM_POPULATED_FILLFILES,
                                   (MPARAM)prec,
                                   (MPARAM)pFolder);
                }

                // clear wait pointer
                (psv->cThreadsRunning)--;
                WinPostMsg(psv->hwndMainControl,
                           FM_UPDATEPOINTER,
                           0,
                           0);
            }
        }
        break;

        /*
         *@@ FM2_ADDFIRSTCHILD_BEGIN:
         *      posted by InsertContents before the first
         *      FM2_ADDFIRSTCHILD_NEXT is posted so we
         *      can update the "wait" ptr accordingly.
         *
         *@@added V0.9.18 (2002-02-06) [umoeller]
         */

        case FM2_ADDFIRSTCHILD_BEGIN:
        {
            PFDRSPLITVIEW   psv = WinQueryWindowPtr(hwnd, QWL_USER);
            (psv->cThreadsRunning)++;
            WinPostMsg(psv->hwndMainControl,
                       FM_UPDATEPOINTER,
                       0, 0);
        }
        break;

        /*
         *@@ FM2_ADDFIRSTCHILD_NEXT:
         *      fired by InsertContents for every folder that
         *      is added to the drives tree.
         *
         *      Parameters:
         *
         *      --  WPFolder* mp1: folder to add first child for.
         *          This better be in the tree.
         *
         *@@added V0.9.18 (2002-02-06) [umoeller]
         */

        case FM2_ADDFIRSTCHILD_NEXT:
            if (mp1)
            {
                PFDRSPLITVIEW       psv = WinQueryWindowPtr(hwnd, QWL_USER);
                HWND                hwndCnr = psv->hwndTreeCnr;
                WPFolder            *pFolder = (WPObject*)mp1;
                PMINIRECORDCORE     precParent = _wpQueryCoreRecord(pFolder);

                #ifdef DEBUG_POPULATESPLITVIEW
                    _PmpfF(("CM_ADDFIRSTCHILD %s", _wpQueryTitle(mp1)));
                #endif

                AddFirstChild(pFolder,
                              precParent,
                              hwndCnr,
                              &psv->llTreeObjectsInserted);
            }
        break;

        /*
         *@@ FM2_ADDFIRSTCHILD_DONE:
         *      posted by InsertContents after the last
         *      FM2_ADDFIRSTCHILD_NEXT was posted so we
         *      can reset the "wait" ptr.
         *
         *@@added V0.9.18 (2002-02-06) [umoeller]
         */

        case FM2_ADDFIRSTCHILD_DONE:
        {
            PFDRSPLITVIEW   psv = WinQueryWindowPtr(hwnd, QWL_USER);
            (psv->cThreadsRunning)--;
            WinPostMsg(psv->hwndMainControl,
                       FM_UPDATEPOINTER,
                       0,
                       0);
        }
        break;

        default:
            mrc = WinDefWindowProc(hwnd, msg, mp1, mp2);
    }

    return mrc;
}

/*
 *@@ fntSplitPopulate:
 *      "split populate" thread. This creates an object window
 *      so that we can easily serialize the order in which
 *      folders are populate and such.
 *
 *      This is responsible for both populating folders _and_
 *      doing the "add first child" processing. This was all
 *      new with V0.9.18's file dialog and was my second attempt
 *      at getting the thread synchronization right, which
 *      turned out to work pretty well.
 *
 *      We _need_ a second thread for "add first child" too
 *      because even adding the first child can take quite a
 *      while. For example, if a folder has 1,000 files in it
 *      and the 999th is a directory, the file system has to
 *      scan the entire contents first.
 */

static VOID _Optlink fntSplitPopulate(PTHREADINFO ptiMyself)
{
    TRY_LOUD(excpt1)
    {
        QMSG qmsg;
        PFDRSPLITVIEW psv = (PFDRSPLITVIEW)ptiMyself->ulData;

        #ifdef DEBUG_POPULATESPLITVIEW
            _PmpfF(("thread starting"));
        #endif

        WinRegisterClass(ptiMyself->hab,
                         (PSZ)WC_SPLITPOPULATE,
                         fnwpSplitPopulate,
                         0,
                         sizeof(PVOID));
        if (!(psv->hwndSplitPopulate = winhCreateObjectWindow(WC_SPLITPOPULATE,
                                                              psv)))
            cmnLog(__FILE__, __LINE__, __FUNCTION__,
                   "Cannot create split populate object window.");

        // thread 1 is waiting for obj window to be created
        DosPostEventSem(ptiMyself->hevRunning);

        while (WinGetMsg(ptiMyself->hab, &qmsg, NULLHANDLE, 0, 0))
            WinDispatchMsg(ptiMyself->hab, &qmsg);

        WinDestroyWindow(psv->hwndSplitPopulate);

        #ifdef DEBUG_POPULATESPLITVIEW
            _PmpfF(("thread ending"));
        #endif

    }
    CATCH(excpt1) {} END_CATCH();

}

/*
 *@@ fdrSplitPopulate:
 *      posts FM2_POPULATE to fnwpSplitPopulate to
 *      populate the folder represented by the
 *      given MINIRECORDCORE according to fl.
 *
 *      fnwpSplitPopulate does the following:
 *
 *      1)  Before populating, raise psv->cThreadsRunning
 *          and post FM_UPDATEPOINTER to hwndMainControl.
 *          to have it display the "wait" pointer.
 *
 *      2)  Run _wpPopulate on the folder represented
 *          by prec.
 *
 *      3)  Post FM_POPULATED_FILLTREE back to hwndMainControl
 *          in any case to fill the tree under prec with the
 *          subfolders that were found.
 *
 *      4)  If the FFL_SCROLLTO flag is set, post
 *          FM_POPULATED_SCROLLTO back to hwndMainControl
 *          so that the tree can be scrolled properly.
 *
 *      5)  If the FFL_FOLDERSONLY flag was _not_ set,
 *          post FM_POPULATED_FILLFILES to hwndMainControl
 *          so it can insert all objects into the files
 *          container.
 *
 *      6)  Decrement psv->cThreadsRunning and post
 *          FM_UPDATEPOINTER again to reset the wait
 *          pointer.
 */

VOID fdrSplitPopulate(PFDRSPLITVIEW psv,
                      PMINIRECORDCORE prec,
                      ULONG fl)
{
    #ifdef DEBUG_POPULATESPLITVIEW
        _PmpfF(("psv->hwndSplitPopulate: 0x%lX", psv->hwndSplitPopulate));
    #endif

    WinPostMsg(psv->hwndSplitPopulate,
               FM2_POPULATE,
               (MPARAM)prec,
               (MPARAM)fl);
}

#ifdef DEBUG_POPULATESPLITVIEW
VOID DumpFlags(ULONG fl)
{
    _Pmpf(("  fl %s %s %s",
                (fl & FFL_FOLDERSONLY) ? "FFL_FOLDERSONLY " : "",
                (fl & FFL_SCROLLTO) ? "FFL_SCROLLTO " : "",
                (fl & FFL_EXPAND) ? "FFL_EXPAND " : ""));
}
#else
    #define DumpFlags(fl)
#endif

/*
 *@@ fdrPostFillFolder:
 *      posts FM_FILLFOLDER to the main control
 *      window with the given parameters.
 *
 *      This gets called from the tree frame
 *      to fire populate when things get
 *      selected in the tree.
 *
 *      The main control window will then call
 *      fdrSplitPopulate to have FM2_POPULATE
 *      posted to the split populate thread.
 */

VOID fdrPostFillFolder(PFDRSPLITVIEW psv,
                       PMINIRECORDCORE prec,       // in: record with folder to populate
                       ULONG fl)                   // in: FFL_* flags
{
    WinPostMsg(psv->hwndMainControl,
               FM_FILLFOLDER,
               (MPARAM)prec,
               (MPARAM)fl);
}

/*
 *@@ fdrSplitQueryPointer:
 *      returns the HPOINTER that should be used
 *      according to the present thread state.
 *
 *      Returns a HPOINTER for either the wait or
 *      arrow pointer.
 */

HPOINTER fdrSplitQueryPointer(PFDRSPLITVIEW psv)
{
    ULONG           idPtr = SPTR_ARROW;

    if (    (psv)
         && (psv->cThreadsRunning)
       )
        idPtr = SPTR_WAIT;

    return WinQuerySysPointer(HWND_DESKTOP,
                              idPtr,
                              FALSE);
}

/*
 *@@ fdrInsertContents:
 *      inserts the contents of the given folder into
 *      the given container.
 *
 *      It is assumed that the folder is already populated.
 *
 *      If (precParent != NULL), the contents are inserted
 *      as child records below that record. Of course that
 *      will work in Tree view only.
 *
 *      In addition, if (hwndAddFirstChild != NULLHANDLE),
 *      this will fire an CM_ADDFIRSTCHILD msg to that
 *      window for every record that was inserted.
 *
 */

VOID fdrInsertContents(WPFolder *pFolder,              // in: populated folder
                       HWND hwndCnr,                   // in: cnr to insert records to
                       PMINIRECORDCORE precParent,     // in: parent record or NULL
                       ULONG ulFoldersOnly,            // in: as with fdrIsInsertable
                       HWND hwndAddFirstChild,         // in: if != 0, we post CM_ADDFIRSTCHILD for each item too
                       PCSZ pcszFileMask,              // in: file mask filter or NULL
                       PLINKLIST pllObjects)           // in/out: linked list of objs that were inserted
{
    BOOL        fFolderLocked = FALSE;

    TRY_LOUD(excpt1)
    {
        // lock the folder contents for wpQueryContent
        if (fFolderLocked = !_wpRequestFolderMutexSem(pFolder, SEM_INDEFINITE_WAIT))
        {
            // build an array of objects that should be inserted
            // (we use an array because we want to use
            // _wpclsInsertMultipleObjects);
            // we run through the folder and check if the object
            // is insertable and, if so, add it to the array,
            // which is increased in size in chunks of 1000 pointers
            WPObject    *pObject;
            WPObject    **papObjects = NULL;
            ULONG       cObjects = 0,
                        cArray = 0,
                        cAddFirstChilds = 0;

            for (   pObject = _wpQueryContent(pFolder, NULL, QC_FIRST);
                    pObject;
                    pObject = *__get_pobjNext(pObject)
                )
            {
                if (fdrvIsInsertable(pObject,
                                     ulFoldersOnly,
                                     pcszFileMask))
                {
                    // _wpclsInsertMultipleObjects fails on the
                    // entire array if only one is already in the
                    // cnr, so make sure it isn't
                    if (!fdrvIsObjectInCnr(pObject, hwndCnr))
                    {
                        // create/expand array if necessary
                        if (cObjects >= cArray)     // on the first iteration,
                                                    // both are null
                        {
                            cArray += 1000;
                            papObjects = (WPObject**)realloc(papObjects, // NULL on first call
                                                             cArray * sizeof(WPObject*));
                        }

                        // store in array
                        papObjects[cObjects++] = pObject;

                        // if caller wants a list, add that to
                        if (pllObjects)
                            lstAppendItem(pllObjects, pObject);
                    }

                    // even if the object is already in the
                    // cnr, if we are in "add first child"
                    // mode, add the first child later;
                    // this works because hwndAddFirstChild
                    // is on the same thread
                    if (    (hwndAddFirstChild)
                         && (objIsAFolder(pObject))
                       )
                    {
                        if (!cAddFirstChilds)
                        {
                            // first post: tell thread to update
                            // the wait pointer
                            WinPostMsg(hwndAddFirstChild,
                                       FM2_ADDFIRSTCHILD_BEGIN,
                                       0,
                                       0);
                            cAddFirstChilds++;
                        }

                        WinPostMsg(hwndAddFirstChild,
                                   FM2_ADDFIRSTCHILD_NEXT,
                                   (MPARAM)pObject,
                                   NULL);
                    }
                }
            }

            #ifdef DEBUG_POPULATESPLITVIEW
                _PmpfF(("--> got %d objects to insert, %d to add first child",
                        cObjects, cAddFirstChilds));
            #endif

            if (cObjects)
                _wpclsInsertMultipleObjects(_somGetClass(pFolder),
                                            hwndCnr,
                                            NULL,
                                            (PVOID*)papObjects,
                                            precParent,
                                            cObjects);

            if (papObjects)
                free(papObjects);

            if (cAddFirstChilds)
            {
                // we had any "add-first-child" posts:
                // post another msg which will get processed
                // after all the "add-first-child" things
                // so that the wait ptr can be reset
                WinPostMsg(hwndAddFirstChild,
                           FM2_ADDFIRSTCHILD_DONE,
                           0, 0);
            }
        }
    }
    CATCH(excpt1)
    {
    } END_CATCH();

    if (fFolderLocked)
        _wpReleaseFolderMutexSem(pFolder);
}

/* ******************************************************************
 *
 *   Split view main control (main frame's client)
 *
 ********************************************************************/

static MRESULT EXPENTRY fnwpSubclassedFilesFrame(HWND hwndFrame, ULONG msg, MPARAM mp1, MPARAM mp2);

/*
 *@@ fdrCreateFrameWithCnr:
 *      creates a new WC_FRAME with a WC_CONTAINER as its
 *      client window, with hwndParentOwner being the parent
 *      and owner of the frame.
 *
 *      With flCnrStyle, specify the cnr style to use. The
 *      following may optionally be set:
 *
 *      --  CCS_MINIICONS (optionally)
 *
 *      --  CCS_EXTENDSEL: allow zero, one, many icons to be
 *          selected (default WPS style).
 *
 *      --  CCS_SINGLESEL: allow only exactly one icon to be
 *          selected at a time.
 *
 *      --  CCS_MULTIPLESEL: allow zero, one, or more icons
 *          to be selected, and toggle selections (totally
 *          unusable).
 *
 *      WS_VISIBLE, WS_SYNCPAINT, CCS_AUTOPOSITION, and
 *      CCS_MINIRECORDCORE will always be set.
 *
 *      Returns the frame.
 *
 */

HWND fdrCreateFrameWithCnr(ULONG ulFrameID,
                           HWND hwndParentOwner,     // in: main client window
                           ULONG flCnrStyle,         // in: cnr style
                           HWND *phwndClient)        // out: client window (cnr)
{
    HWND    hwndFrame;
    ULONG   ws =   WS_VISIBLE
                 | WS_SYNCPAINT
                 | CCS_AUTOPOSITION
                 | CCS_MINIRECORDCORE
                 | flCnrStyle;

    if (hwndFrame = winhCreateStdWindow(hwndParentOwner, // parent
                                        NULL,          // pswpFrame
                                        FCF_NOBYTEALIGN,
                                        WS_VISIBLE,
                                        "",
                                        0,             // resources ID
                                        WC_CONTAINER,  // client
                                        ws,            // client style
                                        ulFrameID,
                                        NULL,
                                        phwndClient))
    {
        // set client as owner
        WinSetOwner(hwndFrame, hwndParentOwner);
    }

    return hwndFrame;
}

/*
 *@@ fdrSetupSplitView:
 *      creates all the subcontrols of the main controller
 *      window, that is, the split window with the two
 *      subframes and containers.
 *
 *      Returns NULL if no error occured. As a result,
 *      the return value can be returned from WM_CREATE,
 *      which stops window creation if != 0 is returned.
 */

MPARAM fdrSetupSplitView(HWND hwnd,
                         PFDRSPLITVIEW psv)
{
    MPARAM mrc = (MPARAM)FALSE;         // return value of WM_CREATE: 0 == OK

    SPLITBARCDATA sbcd;
    HAB hab = WinQueryAnchorBlock(hwnd);

    lstInit(&psv->llTreeObjectsInserted, FALSE);
    lstInit(&psv->llFileObjectsInserted, FALSE);

    // set the window font for the main client...
    // all controls will inherit this
    winhSetWindowFont(hwnd,
                      cmnQueryDefaultFont());

    /*
     *  split window with two containers
     *
     */

    // create two subframes to be linked in split window

    // 1) left: drives tree
    psv->hwndTreeFrame = fdrCreateFrameWithCnr(ID_TREEFRAME,
                                               hwnd,    // main client
                                               CCS_MINIICONS | CCS_SINGLESEL,
                                               &psv->hwndTreeCnr);
    BEGIN_CNRINFO()
    {
        cnrhSetView(   CV_TREE | CA_TREELINE | CV_ICON
                     | CV_MINI);
        cnrhSetTreeIndent(20);
        cnrhSetSortFunc(fnCompareName);             // shared/cnrsort.c
    } END_CNRINFO(psv->hwndTreeCnr);

    // 2) right: files
    psv->hwndFilesFrame = fdrCreateFrameWithCnr(ID_FILESFRAME,
                                                hwnd,    // main client
                                                (psv->flSplit & SPLIT_MULTIPLESEL)
                                                   ? CCS_MINIICONS | CCS_EXTENDSEL
                                                   : CCS_MINIICONS | CCS_SINGLESEL,
                                                &psv->hwndFilesCnr);
    BEGIN_CNRINFO()
    {
        cnrhSetView(   CV_NAME | CV_FLOW
                     | CV_MINI);
        cnrhSetTreeIndent(30);
        cnrhSetSortFunc(fnCompareNameFoldersFirst);     // shared/cnrsort.c
    } END_CNRINFO(psv->hwndFilesCnr);

    // 3) fonts
    winhSetWindowFont(psv->hwndTreeCnr,
                      cmnQueryDefaultFont());
    winhSetWindowFont(psv->hwndFilesCnr,
                      cmnQueryDefaultFont());

    // create split window
    sbcd.ulSplitWindowID = 1;
        // split window becomes client of main frame
    sbcd.ulCreateFlags =   SBCF_VERTICAL
                         | SBCF_PERCENTAGE
                         | SBCF_3DEXPLORERSTYLE
                         | SBCF_MOVEABLE;
    sbcd.lPos = psv->lSplitBarPos;   // in percent
    sbcd.ulLeftOrBottomLimit = 100;
    sbcd.ulRightOrTopLimit = 100;
    sbcd.hwndParentAndOwner = hwnd;         // client

    if (!(psv->hwndSplitWindow = ctlCreateSplitWindow(hab,
                                                      &sbcd)))
    {
        cmnLog(__FILE__, __LINE__, __FUNCTION__,
                "Cannot create split window.");
        // stop window creation!
        mrc = (MPARAM)TRUE;
    }
    else
    {
        // link left and right container
        WinSendMsg(psv->hwndSplitWindow,
                   SPLM_SETLINKS,
                   (MPARAM)psv->hwndTreeFrame,      // left
                   (MPARAM)psv->hwndFilesFrame);    // right
    }

    // create the "populate" thread
    thrCreate(&psv->tiSplitPopulate,
              fntSplitPopulate,
              &psv->tidSplitPopulate,
              "SplitPopulate",
              THRF_PMMSGQUEUE | THRF_WAIT_EXPLICIT,
                        // we MUST wait until the thread
                        // is ready; populate posts event
                        // sem when it has created its obj wnd
              (ULONG)psv);
    // this will wait until the object window has been created

    return mrc;
}

/*
 *@@ fdrCleanupSplitView:
 *
 *      Does NOT free psv because we can't know how it was
 *      allocated.
 *
 *@@added V0.9.21 (2002-08-21) [umoeller]
 */

VOID fdrCleanupSplitView(PFDRSPLITVIEW psv)
{
    // remove use item for right view
    if (psv->pobjUseList)
        _wpDeleteFromObjUseList(psv->pobjUseList,
                                &psv->uiDisplaying);

    // stop threads; we crash if we exit
    // before these are stopped
    WinPostMsg(psv->hwndSplitPopulate,
               WM_QUIT,
               0,
               0);

    psv->tiSplitPopulate.fExit = TRUE;
    DosSleep(0);
    while (psv->tidSplitPopulate)
        winhSleep(50);

    // prevent dialog updates
    psv->fSplitViewReady = FALSE;
    fdrvClearContainer(psv->hwndTreeCnr,
                       &psv->llTreeObjectsInserted);
    fdrvClearContainer(psv->hwndFilesCnr,
                       &psv->llFileObjectsInserted);

    if (psv->pRootFolder)
        _wpUnlockObject(psv->pRootFolder);

    // clean up
    if (psv->hwndSplitWindow)
        WinDestroyWindow(psv->hwndSplitWindow);

    if (psv->hwndTreeFrame)
        WinDestroyWindow(psv->hwndTreeFrame);
    if (psv->hwndFilesFrame)
        WinDestroyWindow(psv->hwndFilesFrame);
    if (psv->hwndMainFrame)
        WinDestroyWindow(psv->hwndMainFrame);
}

/*
 *@@ fnwpSplitController:
 *
 */

MRESULT EXPENTRY fnwpSplitController(HWND hwndClient, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    TRY_LOUD(excpt1)
    {
        switch (msg)
        {
            /*
             * WM_CREATE:
             *      we get PFDRSPLITVIEW in mp1.
             */

            case WM_CREATE:
            {
                PFDRSPLITVIEW psv = (PFDRSPLITVIEW)mp1;
                WinSetWindowPtr(hwndClient, QWL_USER, mp1);

                mrc = fdrSetupSplitView(hwndClient,
                                        psv);
            }
            break;

            /*
             * WM_WINDOWPOSCHANGED:
             *
             */

            case WM_WINDOWPOSCHANGED:
            {
                // this msg is passed two SWP structs:
                // one for the old, one for the new data
                // (from PM docs)
                PSWP pswpNew = PVOIDFROMMP(mp1);
                // PSWP pswpOld = pswpNew + 1;

                // resizing?
                if (pswpNew->fl & SWP_SIZE)
                {
                    PFDRSPLITVIEW  psv;
                    if (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                    {
                        // adjust size of "split window",
                        // which will rearrange all the linked
                        // windows (comctl.c)
                        WinSetWindowPos(psv->hwndSplitWindow,
                                        HWND_TOP,
                                        0,
                                        0,
                                        pswpNew->cx,
                                        pswpNew->cy,
                                        SWP_SIZE);
                    }
                }

                // return default NULL
            }
            break;

            /*
             * WM_MINMAXFRAME:
             *      when minimizing, we hide the "split window",
             *      because otherwise the child dialogs will
             *      display garbage
             */

            case WM_MINMAXFRAME:
            {
                PFDRSPLITVIEW  psv;
                if (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                {
                    PSWP pswp = (PSWP)mp1;
                    if (pswp->fl & SWP_MINIMIZE)
                        WinShowWindow(psv->hwndSplitWindow, FALSE);
                    else if (pswp->fl & SWP_RESTORE)
                        WinShowWindow(psv->hwndSplitWindow, TRUE);
                }
            }
            break;

            /*
             *@@ FM_FILLFOLDER:
             *      posted to the main control from the tree
             *      frame to fill the dialog when a new folder
             *      has been selected in the tree.
             *
             *      Use fdrPostFillFolder for posting this
             *      message, which is type-safe.
             *
             *      This automatically offloads populate
             *      to fntSplitPopulate, which will then post
             *      a bunch of messages back to us so we
             *      can update the dialog properly.
             *
             *      Parameters:
             *
             *      --  PMINIRECORDCODE mp1: record of folder
             *          (or disk or whatever) to fill with.
             *
             *      --  ULONG mp2: dialog flags.
             *
             *      mp2 can be any combination of the following:
             *
             *      --  If FFL_FOLDERSONLY is set, this operates
             *          in "folders only" mode. We will then
             *          populate the folder with subfolders only
             *          and expand the folder on the left. The
             *          files list is not changed.
             *
             *          If the flag is not set, the folder is
             *          fully populated and the files list is
             *          updated as well.
             *
             *      --  If FFL_SCROLLTO is set, we will scroll
             *          the drives tree so that the given record
             *          becomes visible.
             *
             *      --  If FFL_EXPAND is set, we will also expand
             *          the record in the drives tree after
             *          populate and run "add first child" for
             *          each subrecord that was inserted.
             *
             *      --  If FFL_SETBACKGROUND is set, we will
             *          revamp the files container to use the
             *          view settings of the given folder.
             *
             *@@added V0.9.18 (2002-02-06) [umoeller]
             */

            case FM_FILLFOLDER:
            {
                PFDRSPLITVIEW  psv;
                if (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                {
                    PMINIRECORDCORE prec = (PMINIRECORDCORE)mp1;

                    #ifdef DEBUG_POPULATESPLITVIEW
                        _PmpfF(("FM_FILLFOLDER %s", prec->pszIcon));
                    #endif

                    DumpFlags((ULONG)mp2);

                    if (!((ULONG)mp2 & FFL_FOLDERSONLY))
                    {
                        // not folders-only: then we need to
                        // refresh the files list

                        WPFolder    *pFolder;

                        fdrvClearContainer(psv->hwndFilesCnr,
                                           &psv->llFileObjectsInserted);

                        // if we had a previous view item for the
                        // files cnr, remove it... since the entire
                        // psv structure was initially zeroed, this
                        // check is safe
                        if (psv->pobjUseList)
                            _wpDeleteFromObjUseList(psv->pobjUseList,
                                                    &psv->uiDisplaying);

                        // disable the whitespace context menu
                        psv->precFilesShowing = NULL;

                        // register a view item for the object
                        // that was selected in the tree so that
                        // it is marked as "open" and the refresh
                        // thread can handle updates for it too;
                        // use the OBJECT, not the folder derived
                        // from it, since for disk objects, the
                        // disks have the views, not the root folder
                        if (psv->pobjUseList = OBJECT_FROM_PREC(prec))
                        {
                            psv->uiDisplaying.type = USAGE_OPENVIEW;
                            memset(&psv->viDisplaying, 0, sizeof(VIEWITEM));
                            psv->viDisplaying.view =
                                    *G_pulVarMenuOfs + ID_XFMI_OFS_SPLITVIEW_SHOWING;
                            psv->viDisplaying.handle = psv->hwndFilesFrame;
                                    // do not change this! XFolder::wpUnInitData
                                    // relies on this!
                            // set this flag so that we can disable
                            // _wpAddToContent for this view while we're
                            // populating; the flag is cleared once we're done
                            psv->viDisplaying.ulViewState = VIEWSTATE_OPENING;

                            _wpAddToObjUseList(psv->pobjUseList,
                                               &psv->uiDisplaying);
                        }

                        // change files container background NOW
                        // to give user immediate feedback
                        if (    (psv->flSplit & SPLIT_FDRSTYLES)
                             && ((ULONG)mp2 & FFL_SETBACKGROUND)
                             && (pFolder = fdrvGetFSFromRecord(prec,
                                                               TRUE)) // folders only
                           )
                        {
                            fdrvSetCnrLayout(psv->hwndFilesCnr,
                                             pFolder,
                                             OPEN_CONTENTS);
                        }
                    }

                    // mark this folder as "populating"
                    psv->precFolderPopulating = prec;

                    // post FM2_POPULATE
                    fdrSplitPopulate(psv,
                                     prec,
                                     (ULONG)mp2);
                }
            }
            break;

            /*
             *@@ FM_POPULATED_FILLTREE:
             *      posted by fntSplitPopulate after populate has been
             *      done for a folder. This gets posted in any case,
             *      if the folder was populated in folders-only mode
             *      or not.
             *
             *      Parameters:
             *
             *      --  PMINIRECORDCODE mp1: record of folder
             *          (or disk or whatever) to fill with.
             *
             *      --  ULONG mp2: FFL_* flags for whether to
             *          expand.
             *
             *      This then calls fdrInsertContents to insert
             *      the subrecords into the tree. If FFL_EXPAND
             *      was set, the tree is expanded, and we pass
             *      the controller window handle to fdrInsertContents
             *      so that we can "add first child" messages back.
             *
             *@@added V0.9.18 (2002-02-06) [umoeller]
             */

            case FM_POPULATED_FILLTREE:
            {
                PFDRSPLITVIEW  psv;
                PMINIRECORDCORE prec;

                #ifdef DEBUG_POPULATESPLITVIEW
                    _PmpfF(("FM_POPULATED_FILLTREE %s",
                            mp1
                                ? ((PMINIRECORDCORE)mp1)->pszIcon
                                : "NULL"));
                #endif

                if (    (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                     && (prec = (PMINIRECORDCORE)mp1)
                   )
                {
                    WPFolder    *pFolder = fdrvGetFSFromRecord(mp1, TRUE);
                    PLISTNODE   pNode;
                    HWND        hwndAddFirstChild = NULLHANDLE;

                    // we're done populating
                    psv->precFolderPopulating = NULL;

                    if ((ULONG)mp2 & FFL_EXPAND)
                    {
                        BOOL        fOld = psv->fSplitViewReady;
                        // stop control notifications from messing with this
                        psv->fSplitViewReady = FALSE;
                        cnrhExpandFromRoot(psv->hwndTreeCnr,
                                           (PRECORDCORE)prec);
                        // then fire CM_ADDFIRSTCHILD too
                        hwndAddFirstChild = psv->hwndSplitPopulate;

                        // re-enable control notifications
                        psv->fSplitViewReady = fOld;
                    }

                    // insert subfolders into tree on the left
                    fdrInsertContents(pFolder,
                                      psv->hwndTreeCnr,
                                      (PMINIRECORDCORE)mp1,
                                      INSERT_FOLDERSANDDISKS,
                                      hwndAddFirstChild,
                                      NULL,       // file mask
                                      &psv->llTreeObjectsInserted);
                }
            }
            break;

            /*
             *@@ FM_POPULATED_SCROLLTO:
             *
             *      Parameters:
             *
             *      --  PMINIRECORDCODE mp1: record of folder
             *          (or disk or whatever) that was populated
             *          and should now be scrolled to.
             *
             *@@added V0.9.18 (2002-02-06) [umoeller]
             */

            case FM_POPULATED_SCROLLTO:
            {
                PFDRSPLITVIEW  psv;
                if (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                {
                    BOOL    fOld = psv->fSplitViewReady;
                    ULONG   ul;

                    #ifdef DEBUG_POPULATESPLITVIEW
                        _PmpfF(("FM_POPULATED_SCROLLTO %s",
                                mp1
                                    ? ((PMINIRECORDCORE)mp1)->pszIcon
                                    : "NULL"));
                    #endif

                    // stop control notifications from messing with this
                    psv->fSplitViewReady = FALSE;

                    ul = cnrhScrollToRecord(psv->hwndTreeCnr,
                                            (PRECORDCORE)mp1,
                                            CMA_ICON | CMA_TEXT | CMA_TREEICON,
                                            TRUE);       // keep parent
                    cnrhSelectRecord(psv->hwndTreeCnr,
                                     (PRECORDCORE)mp1,
                                     TRUE);
                    if (ul && ul != 3)
                        cmnLog(__FILE__, __LINE__, __FUNCTION__,
                                "Error: cnrhScrollToRecord returned %d", ul);

                    // re-enable control notifications
                    psv->fSplitViewReady = fOld;
                }
            }
            break;

            /*
             *@@ FM_POPULATED_FILLFILES:
             *      posted by fntSplitPopulate after populate has been
             *      done for the newly selected folder, if this
             *      was not in folders-only mode. We must then fill
             *      the right half of the dialog with all the objects.
             *
             *      Parameters:
             *
             *      --  PMINIRECORDCODE mp1: record of folder
             *          (or disk or whatever) to fill with.
             *
             *      --  WPFolder* mp2: folder that was populated
             *          for that record.
             *
             *@@added V0.9.18 (2002-02-06) [umoeller]
             */

            case FM_POPULATED_FILLFILES:
            {
                PFDRSPLITVIEW  psv;
                if (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                {

                    #ifdef DEBUG_POPULATESPLITVIEW
                        _PmpfF(("FM_POPULATED_FILLFILES %s",
                                mp1
                                    ? ((PMINIRECORDCORE)mp1)->pszIcon
                                    : "NULL"));
                    #endif

                    if ((mp1) && (mp2))
                    {
                        WPFolder    *pFolder = (WPFolder*)mp2;
                        CHAR        szPathName[2*CCHMAXPATH];

                        // insert all contents into list on the right
                        fdrInsertContents(pFolder,
                                          psv->hwndFilesCnr,
                                          NULL,        // parent
                                          INSERT_ALL,
                                          NULLHANDLE,  // no add first child
                                          NULL,        // no file mask
                                          &psv->llFileObjectsInserted);

                        // clear the "opening" flag in the VIEWITEM
                        // so that XFolder::wpAddToContent will start
                        // giving us new objects
                        psv->viDisplaying.ulViewState &= ~VIEWSTATE_OPENING;

                        // re-enable the whitespace context menu
                        psv->precFilesShowing = (PMINIRECORDCORE)mp1;

                        // update the folder pointers in the SFV
                        psv->psfvFiles->somSelf = pFolder;
                        psv->psfvFiles->pRealObject = pFolder;
                    }
                }
            }
            break;

            /*
             * CM_UPDATEPOINTER:
             *      posted when threads exit etc. to update
             *      the current pointer.
             */

            case FM_UPDATEPOINTER:
            {
                PFDRSPLITVIEW  psv;
                if (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                {
                    WinSetPointer(HWND_DESKTOP,
                                  fdrSplitQueryPointer(psv));
                }
            }
            break;

            /*
             * WM_CHAR:
             *
             */

            case WM_CHAR:
            {
                USHORT usFlags    = SHORT1FROMMP(mp1);
                UCHAR  ucScanCode = CHAR4FROMMP(mp1);
                USHORT usvk       = SHORT2FROMMP(mp2);
                PFDRSPLITVIEW  psv;

                if (    (usFlags & KC_VIRTUALKEY)
                     && (usvk == VK_TAB)
                     && (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                   )
                {
                    // process only key-down messages
                    if  ((usFlags & KC_KEYUP) == 0)
                    {
                        HWND hwndFocus = WinQueryFocus(HWND_DESKTOP);
                        if (hwndFocus == psv->hwndTreeCnr)
                            hwndFocus = psv->hwndFilesCnr;
                        else
                            hwndFocus = psv->hwndTreeCnr;
                        WinSetFocus(HWND_DESKTOP, hwndFocus);
                    }

                    mrc = (MRESULT)TRUE;
                }
                else
                    mrc = WinDefWindowProc(hwndClient, msg, mp1, mp2);
            }
            break;

            /*
             * WM_CLOSE:
             *      clean up
             */

            case WM_CLOSE:
            {
                PFDRSPLITVIEW  psv;
                #ifdef DEBUG_POPULATESPLITVIEW
                    _PmpfF(("WM_CLOSE"));
                #endif

                if (psv = WinQueryWindowPtr(hwndClient, QWL_USER))
                {
                    // clear all containers, stop populate thread etc.
                    fdrCleanupSplitView(psv);

                    // destroy the frame window (which destroys us too)
                    WinDestroyWindow(psv->hwndMainFrame);
                }

                // return default NULL
            }
            break;

            default:
                mrc = WinDefWindowProc(hwndClient, msg, mp1, mp2);
        }
    }
    CATCH(excpt1) {} END_CATCH();

    return mrc;
}

/* ******************************************************************
 *
 *   Left tree frame and client
 *
 ********************************************************************/

/*
 *@@ TreeFrameControl:
 *      implementation for WM_CONTROL for FID_CLIENT
 *      in fnwpSubclassedTreeFrame.
 *
 *      Set *pfCallDefault to TRUE if you want the
 *      parent window proc to be called.
 *
 *@@added V0.9.21 (2002-08-26) [umoeller]
 */

MRESULT TreeFrameControl(HWND hwndFrame,
                         MPARAM mp1,
                         MPARAM mp2,
                         PBOOL pfCallDefault)
{
    MRESULT mrc = 0;
    HWND                hwndMainControl;
    PFDRSPLITVIEW       psv;
    PMINIRECORDCORE     prec;

    switch (SHORT2FROMMP(mp1))
    {
        /*
         * CN_EMPHASIS:
         *      selection changed:
         */

        case CN_EMPHASIS:
        {
            PNOTIFYRECORDEMPHASIS pnre = (PNOTIFYRECORDEMPHASIS)mp2;

            if (    (pnre->pRecord)
                 && (pnre->fEmphasisMask & CRA_SELECTED)
                 && (prec = (PMINIRECORDCORE)pnre->pRecord)
                 && (prec->flRecordAttr & CRA_SELECTED)
                 && (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                 && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                 // notifications not disabled?
                 && (psv->fSplitViewReady)
               )
            {
                #ifdef DEBUG_POPULATESPLITVIEW
                    _PmpfF(("CN_EMPHASIS %s",
                        prec->pszIcon));
                #endif

                // record changed?
                if (prec != psv->precTreeSelected)
                {
                    // then go refresh the files container
                    // with a little delay; see WM_TIMER below
                    // if the user goes thru a lot of records
                    // in the tree, this timer gets restarted
                    psv->precFolderToPopulate
                        = psv->precTreeSelected
                        = prec;
                    WinStartTimer(psv->habGUI,
                                  hwndFrame,        // post to tree frame
                                  1,
                                  200);
                }

                if (psv->hwndStatusBar)
                {
                    #ifdef DEBUG_POPULATESPLITVIEW
                        _Pmpf(( "CN_EMPHASIS: posting STBM_UPDATESTATUSBAR to hwnd %lX",
                                psv->hwndStatusBar ));
                    #endif

                    // have the status bar updated and make
                    // sure the status bar retrieves its info
                    // from the _left_ cnr
                    WinPostMsg(psv->hwndStatusBar,
                               STBM_UPDATESTATUSBAR,
                               (MPARAM)psv->hwndTreeCnr,
                               MPNULL);
                }
            }
        }
        break;

        /*
         * CN_EXPANDTREE:
         *      user clicked on "+" sign next to
         *      tree item; expand that, but start
         *      "add first child" thread again
         */

        case CN_EXPANDTREE:
            if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                 && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                 // notifications not disabled?
                 && (psv->fSplitViewReady)
                 && (prec = (PMINIRECORDCORE)mp2)
               )
            {
                #ifdef DEBUG_POPULATESPLITVIEW
                    _PmpfF(("CN_EXPANDTREE %s",
                        prec->pszIcon));
                #endif

                fdrPostFillFolder(psv,
                                  prec,
                                  FFL_FOLDERSONLY | FFL_EXPAND);

                // and call default because xfolder
                // handles auto-scroll
                *pfCallDefault = TRUE;
            }
        break;

        /*
         * CN_ENTER:
         *      intercept this so that we won't open
         *      a folder view.
         *
         *      Before this, we should have gotten
         *      CN_EMPHASIS so the files list has
         *      been updated already.
         *
         *      Instead, check whether the record has
         *      been expanded or collapsed and do
         *      the reverse.
         */

        case CN_ENTER:
        {
            PNOTIFYRECORDENTER pnre;

            if (    (pnre = (PNOTIFYRECORDENTER)mp2)
                 && (prec = (PMINIRECORDCORE)pnre->pRecord)
                            // can be null for whitespace!
               )
            {
                ULONG ulmsg = CM_EXPANDTREE;
                if (prec->flRecordAttr & CRA_EXPANDED)
                    ulmsg = CM_COLLAPSETREE;

                WinPostMsg(pnre->hwndCnr,
                           ulmsg,
                           (MPARAM)prec,
                           0);
            }
        }
        break;

        /*
         * CN_CONTEXTMENU:
         *      we need to intercept this for context menus
         *      on whitespace, because the WPS won't do it.
         *      We pass all other cases on because the WPS
         *      does do things correctly for object menus.
         */

        case CN_CONTEXTMENU:
        {
            *pfCallDefault = TRUE;

            if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                 && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                 && (!mp2)      // whitespace:
                    // display the menu for the root folder
               )
            {
#if 1
                POINTL  ptl;
                WinQueryPointerPos(HWND_DESKTOP, &ptl);
                // convert to cnr coordinates
                WinMapWindowPoints(HWND_DESKTOP,        // from
                                   psv->hwndTreeCnr,   // to
                                   &ptl,
                                   1);
                _wpDisplayMenu(psv->pRootFolder,
                               psv->hwndTreeFrame, // owner
                               psv->hwndTreeCnr,   // parent
                               &ptl,
                               MENU_OPENVIEWPOPUP,
                               0);
#endif

                *pfCallDefault = FALSE;
            }
        }
        break;

        default:
            *pfCallDefault = TRUE;
    }

    return mrc;
}

/*
 *@@ fnwpSubclassedTreeFrame:
 *      subclassed frame window on the right for the
 *      "Files" container. This has the files cnr
 *      as its FID_CLIENT.
 *
 *      We use the XFolder subclassed window proc for
 *      most messages. In addition, we intercept a
 *      couple more for extra features.
 */

MRESULT EXPENTRY fnwpSubclassedTreeFrame(HWND hwndFrame, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT             mrc = 0;

    TRY_LOUD(excpt1)
    {
        BOOL                fCallDefault = FALSE;
        PSUBCLFOLDERVIEW    psfv = WinQueryWindowPtr(hwndFrame, QWL_USER);
        HWND                hwndMainControl;
        PFDRSPLITVIEW       psv;
        PMINIRECORDCORE     prec;

        switch (msg)
        {
            case WM_CONTROL:
                if (SHORT1FROMMP(mp1) == FID_CLIENT)     // that's the container
                    mrc = TreeFrameControl(hwndFrame,
                                           mp1,
                                           mp2,
                                           &fCallDefault);
                else
                    fCallDefault = TRUE;
            break;

            case WM_TIMER:
                if (    ((ULONG)mp1 == 1)
                     && (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                     && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                   )
                {
                    // timer 1 gets (re)started every time the user
                    // selects a record in three (see CN_EMPHASIS above);
                    // we use this to avoid crowding the populate thread
                    // with populate messages if the user is using the
                    // keyboard to run down the tree; CN_EMPHASIS only
                    // sets psv->precFolderToPopulate and starts this
                    // timer.

                    #ifdef DEBUG_POPULATESPLITVIEW
                        _PmpfF(("WM_TIMER 1, precFolderPopulating is 0x%lX (%s)",
                                psv->precFolderPopulating,
                                (psv->precFolderPopulating)
                                    ? psv->precFolderPopulating->pszIcon
                                    : "NULL"));
                    #endif

                    // If we're still busy populating something,
                    // keep the timer running and do nothing.
                    if (!psv->precFolderPopulating)
                    {
                        // not currently populating:
                        // then stop the timer
                        WinStopTimer(psv->habGUI,
                                     hwndFrame,
                                     1);

                        #ifdef DEBUG_POPULATESPLITVIEW
                            _PmpfF(("posting FM_FILLFOLDER"));
                        #endif

                        // and fire populate
                        fdrPostFillFolder(psv,
                                          psv->precFolderToPopulate,
                                          FFL_SETBACKGROUND);

                        psv->precFolderToPopulate = NULL;
                    }
                }
            break;

            case WM_SYSCOMMAND:
                // forward to main frame
                WinPostMsg(WinQueryWindow(WinQueryWindow(hwndFrame, QW_OWNER),
                                          QW_OWNER),
                           msg,
                           mp1,
                           mp2);
            break;

            /*
             * WM_QUERYOBJECTPTR:
             *      we receive this message from the WPS somewhere
             *      when it tries to process menu items for the
             *      whitespace context menu. I guess normally this
             *      message is taken care of when a frame is registered
             *      as a folder view, but this frame is not. So we must
             *      answer by returning the folder that this frame
             *      represents.
             *
             *      Answering this message will enable all WPS whitespace
             *      magic: both displaying the whitespace context menu
             *      and making messages work for the whitespace menu items.
             */

            case WM_QUERYOBJECTPTR:
                _PmpfF(("WM_QUERYOBJECTPTR"));
                if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                     && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                   )
                    mrc = (MRESULT)psv->pRootFolder;
            break;

            /*
             * WM_CONTROLPOINTER:
             *      show wait pointer if we're busy.
             */

            case WM_CONTROLPOINTER:
                if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                     && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                   )
                    mrc = (MPARAM)fdrSplitQueryPointer(psv);
            break;

            default:
                fCallDefault = TRUE;
        }

        if (fCallDefault)
            mrc = fdrProcessFolderMsgs(hwndFrame,
                                       msg,
                                       mp1,
                                       mp2,
                                       psfv,
                                       psfv->pfnwpOriginal);
    }
    CATCH(excpt1) {} END_CATCH();

    return mrc;
}

/* ******************************************************************
 *
 *   Right files frame and client
 *
 ********************************************************************/

/*
 *@@ FilesFrameControl:
 *      implementation for WM_CONTROL for FID_CLIENT
 *      in fnwpSubclassedFilesFrame.
 *
 *      Set *pfCallDefault to TRUE if you want the
 *      parent window proc to be called.
 *
 *@@added V0.9.21 (2002-08-26) [umoeller]
 */

MRESULT FilesFrameControl(HWND hwndFrame,
                          MPARAM mp1,
                          MPARAM mp2,
                          PBOOL pfCallDefault)
{
    MRESULT mrc = 0;
    HWND                hwndMainControl;
    PFDRSPLITVIEW       psv;

    switch (SHORT2FROMMP(mp1))
    {
        /*
         * CN_EMPHASIS:
         *      selection changed: refresh
         *      the status bar.
         */

        case CN_EMPHASIS:
        {
            PNOTIFYRECORDEMPHASIS pnre = (PNOTIFYRECORDEMPHASIS)mp2;
            PMINIRECORDCORE prec;

            if (    // (pnre->pRecord)
                 // && (pnre->fEmphasisMask & CRA_SELECTED)
                 // && (prec = (PMINIRECORDCORE)pnre->pRecord)
                 // && (prec->flRecordAttr & CRA_SELECTED)
                    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                 && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                 // notifications not disabled?
                 && (psv->fSplitViewReady)
               )
            {
                if (psv->hwndStatusBar)
                {
                    #ifdef DEBUG_POPULATESPLITVIEW
                        _Pmpf(( "CN_EMPHASIS: posting STBM_UPDATESTATUSBAR to hwnd %lX",
                                psv->hwndStatusBar ));
                    #endif

                    // have the status bar updated and make
                    // sure the status bar retrieves its info
                    // from the _left_ cnr
                    WinPostMsg(psv->hwndStatusBar,
                               STBM_UPDATESTATUSBAR,
                               (MPARAM)psv->hwndFilesCnr,
                               MPNULL);
                }
            }
        }
        break;

        /*
         * CN_ENTER:
         *      double-click on tree record: intercept
         *      folders so we can influence the tree
         *      view on the right.
         */

        case CN_ENTER:
        {
            PNOTIFYRECORDENTER pnre;
            PMINIRECORDCORE prec;
            WPObject *pobj;

            if (    (pnre = (PNOTIFYRECORDENTER)mp2)
                 && (prec = (PMINIRECORDCORE)pnre->pRecord)
                            // can be null for whitespace!
                 && (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                 && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                 && (pobj = fdrvGetFSFromRecord(prec,
                                                TRUE))       // folders only:
               )
            {
                // double click on folder:

            }
            else
                *pfCallDefault = TRUE;
        }
        break;

        /*
         * CN_CONTEXTMENU:
         *      we need to intercept this for context menus
         *      on whitespace, because the WPS won't do it.
         *      We pass all other cases on because the WPS
         *      does do things correctly for object menus.
         */

        case CN_CONTEXTMENU:
        {
            *pfCallDefault = TRUE;

            if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                 && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                    // whitespace?
                 && (!mp2)
               )
            {
                // ok, this is a whitespace menu: if the view is
                // NOT populated, then just swallow for safety
                if (psv->precFilesShowing)
                {
                    POINTL  ptl;
                    WinQueryPointerPos(HWND_DESKTOP, &ptl);
                    // convert to cnr coordinates
                    WinMapWindowPoints(HWND_DESKTOP,        // from
                                       psv->hwndFilesCnr,   // to
                                       &ptl,
                                       1);
                    _wpDisplayMenu(OBJECT_FROM_PREC(psv->precFilesShowing),
                                   psv->hwndFilesFrame, // owner
                                   psv->hwndFilesCnr,   // parent
                                   &ptl,
                                   MENU_OPENVIEWPOPUP,
                                   0);
                }

                *pfCallDefault = FALSE;
            }
        }
        break;

        default:
            *pfCallDefault = TRUE;
    }

    return mrc;
}

/*
 *@@ fnwpSubclassedFilesFrame:
 *      subclassed frame window on the right for the
 *      "Files" container. This has the tree cnr
 *      as its FID_CLIENT.
 *
 *      We use the XFolder subclassed window proc for
 *      most messages. In addition, we intercept a
 *      couple more for extra features.
 */

static MRESULT EXPENTRY fnwpSubclassedFilesFrame(HWND hwndFrame, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT             mrc = 0;

    TRY_LOUD(excpt1)
    {
        BOOL                fCallDefault = FALSE;
        PSUBCLFOLDERVIEW    psfv = WinQueryWindowPtr(hwndFrame, QWL_USER);
        HWND                hwndMainControl;
        PFDRSPLITVIEW       psv;

        switch (msg)
        {
            case WM_CONTROL:
            {
                if (SHORT1FROMMP(mp1) == FID_CLIENT)     // that's the container
                    mrc = FilesFrameControl(hwndFrame,
                                            mp1,
                                            mp2,
                                            &fCallDefault);
                else
                    fCallDefault = TRUE;
            }
            break;

            case WM_SYSCOMMAND:
                // forward to main frame
                WinPostMsg(WinQueryWindow(WinQueryWindow(hwndFrame,
                                                         QW_OWNER),
                                          QW_OWNER),
                           msg,
                           mp1,
                           mp2);
            break;

            /*
             * WM_QUERYOBJECTPTR:
             *      we receive this message from the WPS somewhere
             *      when it tries to process menu items for the
             *      whitespace context menu. I guess normally this
             *      message is taken care of when a frame is registered
             *      as a folder view, but this frame is not. So we must
             *      answer by returning the folder that this frame
             *      represents.
             *
             *      Answering this message will enable all WPS whitespace
             *      magic: both displaying the whitespace context menu
             *      and making messages work for the whitespace menu items.
             */

            case WM_QUERYOBJECTPTR:
                _PmpfF(("WM_QUERYOBJECTPTR"));
                if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                     && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                     // return the showing folder only if it is done
                     // populating
                     && (psv->precFilesShowing)
                   )
                    mrc = (MRESULT)OBJECT_FROM_PREC(psv->precFilesShowing);
                // else return default null
            break;

            /*
             * WM_CONTROLPOINTER:
             *      show wait pointer if we're busy.
             */

            case WM_CONTROLPOINTER:
                if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                     && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                   )
                    mrc = (MPARAM)fdrSplitQueryPointer(psv);
            break;

            /*
             *@@ FM_DELETINGFDR:
             *      this message is sent from XFolder::wpUnInitData
             *      if it finds the ID_XFMI_OFS_SPLITVIEW_SHOWING
             *      useitem in the folder. In other words, the
             *      folder whose contents are currently showing
             *      in the files cnr is about to go dormant.
             *      We must null the pointers in the splitviewdata
             *      that point to ourselves, or we'll have endless
             *      problems.
             *
             *@@added V0.9.21 (2002-08-28) [umoeller]
             */

            case FM_DELETINGFDR:
                _PmpfF(("FM_DELETINGFDR"));
                if (    (hwndMainControl = WinQueryWindow(hwndFrame, QW_OWNER))
                     && (psv = WinQueryWindowPtr(hwndMainControl, QWL_USER))
                   )
                {
                    psv->pobjUseList = NULL;
                    psv->precFilesShowing = NULL;
                }
            break;

            default:
                fCallDefault = TRUE;
        }

        if (fCallDefault)
            mrc = fdrProcessFolderMsgs(hwndFrame,
                                       msg,
                                       mp1,
                                       mp2,
                                       psfv,
                                       psfv->pfnwpOriginal);
    }
    CATCH(excpt1) {} END_CATCH();

    return mrc;
}

/*
 *@@ fdrSplitCreateFrame:
 *      creates a WC_FRAME with the split view controller
 *      (fnwpSplitController) as its FID_CLIENT.
 *
 *      The caller is responsible for allocating a FDRSPLITVIEW
 *      structure and passing it in. It is assumed that
 *      the structure is zeroed completely.
 *
 *      The caller must also subclass the frame that is
 *      returned and free that structure on WM_DESTROY.
 *
 *      The frame is created invisible with an undefined
 *      position. The handle of the frame window is
 *      stored in psv->hwndMainFrame, if TRUE is returned.
 *
 *      With flSplit, pass in any combination of the
 *      following:
 *
 *      --  SPLIT_ANIMATE: give the frame the WS_ANIMATE
 *          style.
 *
 *      --  SPLIT_FDRSTYLES: make the two containers
 *          replicate the exact styles (backgrounds,
 *          fonts, colors) of the folders they are
 *          currently displaying. This is CPU-intensive,
 *          but pretty.
 *
 *@@added V0.9.21 (2002-08-28) [umoeller]
 */

BOOL fdrSplitCreateFrame(WPFolder *pRootFolder,
                         PFDRSPLITVIEW psv,
                         ULONG flFrame,
                         PCSZ pcszTitle,
                         ULONG flSplit,
                         PCSZ pcszFileMask,
                         LONG lSplitBarPos)
{
    ZERO(psv);

    psv->cbStruct = sizeof(*psv);
    psv->lSplitBarPos = lSplitBarPos;
    psv->pRootFolder = pRootFolder;
    psv->flSplit = flSplit;

    psv->pcszFileMask = pcszFileMask;

    if (psv->hwndMainFrame = winhCreateStdWindow(HWND_DESKTOP, // parent
                                                 NULL,         // pswp
                                                 flFrame,
                                                 (flSplit & SPLIT_ANIMATE)
                                                    ? WS_ANIMATE   // frame style, not yet visible
                                                    : 0,
                                                 "Split view",
                                                 0,            // resids
                                                 WC_SPLITVIEWCLIENT,
                                                 WS_VISIBLE,   // client style
                                                 0,            // frame ID
                                                 psv,
                                                 &psv->hwndMainControl))
    {
        PMINIRECORDCORE pRootRec;
        POINTL  ptlIcon = {0, 0};

        psv->habGUI = WinQueryAnchorBlock(psv->hwndMainFrame);

        if (flSplit & SPLIT_STATUSBAR)
            psv->hwndStatusBar = stbCreateBar(pRootFolder,
                                              pRootFolder,
                                              psv->hwndMainFrame,
                                              psv->hwndTreeCnr);


        // insert somSelf as the root of the tree
        pRootRec = _wpCnrInsertObject(pRootFolder,
                                      psv->hwndTreeCnr,
                                      &ptlIcon,
                                      NULL,       // parent record
                                      NULL);      // RECORDINSERT

        // _wpCnrInsertObject subclasses the container owner,
        // so subclass this with the XFolder subclass
        // proc again; otherwise the new menu items
        // won't work
        psv->psfvTree = fdrCreateSFV(psv->hwndTreeFrame,
                                     psv->hwndTreeCnr,
                                     QWL_USER,
                                     pRootFolder,
                                     pRootFolder);
        psv->psfvTree->pfnwpOriginal = WinSubclassWindow(psv->hwndTreeFrame,
                                                         fnwpSubclassedTreeFrame);

        // same thing for files frame; however we need to
        // insert a temp object first to let the WPS subclass
        // the cnr owner first
        _wpCnrInsertObject(pRootFolder,
                           psv->hwndFilesCnr,
                           &ptlIcon,
                           NULL,
                           NULL);
        psv->psfvFiles = fdrCreateSFV(psv->hwndFilesFrame,
                                      psv->hwndFilesCnr,
                                      QWL_USER,
                                      pRootFolder,
                                      pRootFolder);
        psv->psfvFiles->pfnwpOriginal = WinSubclassWindow(psv->hwndFilesFrame,
                                                          fnwpSubclassedFilesFrame);

        // remove the temp object again
        _wpCnrRemoveObject(pRootFolder,
                           psv->hwndFilesCnr);

        // and populate this once we're running
        fdrPostFillFolder(psv,
                          pRootRec,
                          // full populate, and expand tree on the left,
                          // and set background
                          FFL_SETBACKGROUND | FFL_EXPAND);

        return TRUE;
    }

    return FALSE;
}

/* ******************************************************************
 *
 *   Folder split view
 *
 ********************************************************************/

/*
 *@@ SPLITVIEWPOS:
 *
 *@@added V0.9.21 (2002-08-21) [umoeller]
 */

typedef struct _SPLITVIEWPOS
{
    LONG        x,
                y,
                cx,
                cy;

    LONG        lSplitBarPos;

} SPLITVIEWPOS, *PSPLITVIEWPOS;

/*
 *@@ SPLITVIEWDATA:
 *
 */

typedef struct _SPLITVIEWDATA
{
    USHORT          cb;

    USEITEM         ui;
    VIEWITEM        vi;

    CHAR            szFolderPosKey[10];

    FDRSPLITVIEW    sv;         // pRootFolder == somSelf

} SPLITVIEWDATA, *PSPLITVIEWDATA;

/*
 *@@ fnwpSplitViewFrame:
 *
 *@@added V0.9.21 (2002-08-21) [umoeller]
 */

MRESULT EXPENTRY fnwpSplitViewFrame(HWND hwndFrame, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    switch (msg)
    {
        /*
         * WM_SYSCOMMAND:
         *
         */

        case WM_SYSCOMMAND:
        {
            PSPLITVIEWDATA psvd;
            if (    (SHORT1FROMMP(mp1) == SC_CLOSE)
                 && (psvd = (PSPLITVIEWDATA)WinQueryWindowPtr(hwndFrame,
                                                              QWL_USER))
               )
            {
                // save window position
                SWP swp;
                SPLITVIEWPOS pos;
                WinQueryWindowPos(hwndFrame,
                                  &swp);
                pos.x = swp.x;
                pos.y = swp.y;
                pos.cx = swp.cx;
                pos.cy = swp.cy;

                pos.lSplitBarPos = ctlQuerySplitPos(psvd->sv.hwndSplitWindow);

                PrfWriteProfileData(HINI_USER,
                                    (PSZ)INIAPP_FDRSPLITVIEWPOS,
                                    psvd->szFolderPosKey,
                                    &pos,
                                    sizeof(pos));
            }

            mrc = G_pfnFrameProc(hwndFrame, msg, mp1, mp2);
        }
        break;

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
            ULONG ulCount = (ULONG)G_pfnFrameProc(hwndFrame, msg, mp1, mp2);

            PSPLITVIEWDATA psvd;
            if (    (psvd = (PSPLITVIEWDATA)WinQueryWindowPtr(hwndFrame,
                                                              QWL_USER))
                 && (psvd->sv.hwndStatusBar)
               )
                ulCount++;

            mrc = (MRESULT)ulCount;
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
            ULONG ulCount = (ULONG)G_pfnFrameProc(hwndFrame, msg, mp1, mp2);
            PSPLITVIEWDATA psvd;
            if (    (psvd = (PSPLITVIEWDATA)WinQueryWindowPtr(hwndFrame,
                                                              QWL_USER))
                 && (psvd->sv.hwndStatusBar)
               )
            {
                // we have a status bar:
                // format the frame
                fdrFormatFrame(hwndFrame,
                               psvd->sv.hwndStatusBar,
                               mp1,
                               ulCount,
                               NULL);

                // increment the number of frame controls
                // to include our status bar
                ulCount++;
            }

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
        {
            PSPLITVIEWDATA psvd;
            mrc = G_pfnFrameProc(hwndFrame, msg, mp1, mp2);
            if (    (psvd = (PSPLITVIEWDATA)WinQueryWindowPtr(hwndFrame,
                                                              QWL_USER))
                 && (psvd->sv.hwndStatusBar)
               )
            {
                fdrCalcFrameRect(mp1, mp2);
            }
        }
        break;

        /*
         * WM_DESTROY:
         *
         */

        case WM_DESTROY:
        {
            PSPLITVIEWDATA psvd;

            if (psvd = (PSPLITVIEWDATA)WinQueryWindowPtr(hwndFrame,
                                                         QWL_USER))
            {
                #ifdef DEBUG_POPULATESPLITVIEW
                    _PmpfF(("PSPLITVIEWDATA is 0x%lX", psvd));
                #endif

                _wpDeleteFromObjUseList(psvd->sv.pRootFolder,  // somSelf
                                        &psvd->ui);

                if (psvd->sv.hwndStatusBar)
                    WinDestroyWindow(psvd->sv.hwndStatusBar);

                _wpFreeMem(psvd->sv.pRootFolder,  // somSelf,
                           (PBYTE)psvd);
            }

            mrc = G_pfnFrameProc(hwndFrame, msg, mp1, mp2);
        }
        break;

        default:
            mrc = G_pfnFrameProc(hwndFrame, msg, mp1, mp2);
    }

    return mrc;
}

/*
 *@@ fdrCreateSplitView:
 *      creates a frame window with a split window and
 *      does the usual "register view and pass a zillion
 *      QWL_USER pointers everywhere" stuff.
 *
 *      Returns the frame window or NULLHANDLE on errors.
 *
 */

HWND fdrCreateSplitView(WPFolder *somSelf,
                        ULONG ulView)
{
    static  s_fRegistered = FALSE;

    HWND    hwndReturn = NULLHANDLE;

    TRY_LOUD(excpt1)
    {
        PSPLITVIEWDATA  psvd;
        ULONG           rc;

        if (!s_fRegistered)
        {
            s_fRegistered = TRUE;

            WinRegisterClass(winhMyAnchorBlock(),
                             (PSZ)WC_SPLITVIEWCLIENT,
                             fnwpSplitController,
                             CS_SIZEREDRAW,
                             sizeof(PSPLITVIEWDATA));
        }

        // allocate our SPLITVIEWDATA, which contains the
        // FDRSPLITVIEW for the split view engine, plus
        // the useitem for this view
        if (psvd = (PSPLITVIEWDATA)_wpAllocMem(somSelf,
                                               sizeof(SPLITVIEWDATA),
                                               &rc))
        {
            SPLITVIEWPOS pos;
            ULONG   cbPos;
            ULONG   flFrame = FCF_NOBYTEALIGN
                                  | FCF_TITLEBAR
                                  | FCF_SYSMENU
                                  | FCF_SIZEBORDER
                                  | FCF_AUTOICON;
            ULONG   ulButton = _wpQueryButtonAppearance(somSelf);
            ULONG   flSplit;

            ZERO(psvd);

            if (ulButton == DEFAULTBUTTON)
                ulButton = PrfQueryProfileInt(HINI_USER,
                                              "PM_ControlPanel",
                                              "MinButtonType",
                                              HIDEBUTTON);

            if (ulButton == HIDEBUTTON)
                flFrame |= FCF_HIDEMAX;     // hide and maximize
            else
                flFrame |= FCF_MINMAX;      // minimize and maximize

            psvd->cb = sizeof(SPLITVIEWDATA);

            // try to restore window position, if present;
            // we put these in a separate XWorkplace app
            // because we're using a special format to
            // allow for saving the split position
            sprintf(psvd->szFolderPosKey,
                    "%lX",
                    _wpQueryHandle(somSelf));
            cbPos = sizeof(pos);
            if (    (!(PrfQueryProfileData(HINI_USER,
                                           (PSZ)INIAPP_FDRSPLITVIEWPOS,
                                           psvd->szFolderPosKey,
                                           &pos,
                                           &cbPos)))
                 || (cbPos != sizeof(SPLITVIEWPOS))
               )
            {
                // no position stored yet:
                pos.x = (winhQueryScreenCX() - 600) / 2;
                pos.y = (winhQueryScreenCY() - 400) / 2;
                pos.cx = 600;
                pos.cy = 400;
                pos.lSplitBarPos = 30;
            }

            flSplit = SPLIT_ANIMATE | SPLIT_FDRSTYLES | SPLIT_MULTIPLESEL;

            if (stbFolderWantsStatusBars(somSelf))
                flSplit |= SPLIT_STATUSBAR;

            // create the frame and the client;
            // the client gets psvd in mp1 with WM_CREATE
            // and creates the split window and everything else
            if (fdrSplitCreateFrame(somSelf,
                                    &psvd->sv,
                                    flFrame,
                                    "",
                                    flSplit,
                                    NULL,       // no file mask
                                    pos.lSplitBarPos))
            {
                // view-specific stuff

                WinSetWindowPtr(psvd->sv.hwndMainFrame,
                                QWL_USER,
                                psvd);
                G_pfnFrameProc = WinSubclassWindow(psvd->sv.hwndMainFrame,
                                                   fnwpSplitViewFrame);

                // register the view
                cmnRegisterView(somSelf,
                                &psvd->ui,
                                ulView,
                                psvd->sv.hwndMainFrame,
                                cmnGetString(ID_XFSI_FDR_SPLITVIEW));

                // subclass the cnrs to let us paint the bitmaps
                fdrvMakeCnrPaint(psvd->sv.hwndTreeCnr);
                fdrvMakeCnrPaint(psvd->sv.hwndFilesCnr);

                // set tree view colors
                fdrvSetCnrLayout(psvd->sv.hwndTreeCnr,
                                 somSelf,
                                 OPEN_TREE);

                // now go show the damn thing
                WinSetWindowPos(psvd->sv.hwndMainFrame,
                                HWND_TOP,
                                pos.x,
                                pos.y,
                                pos.cx,
                                pos.cy,
                                SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER);

                // and set focus to the tree view
                WinSetFocus(HWND_DESKTOP,
                            psvd->sv.hwndTreeCnr);

                psvd->sv.fSplitViewReady = TRUE;

                hwndReturn = psvd->sv.hwndMainFrame;
            }
        }
    }
    CATCH(excpt1)
    {
    } END_CATCH();

    return hwndReturn;
}
