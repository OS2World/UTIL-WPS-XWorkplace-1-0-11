
/*
 *@@sourcefile kernel.h:
 *      header file for kernel.c. See remarks there.
 *
 *@@include #define INCL_DOSSEMAPHORES
 *@@include #define INCL_WINWINDOWMGR
 *@@include #define INCL_WINPOINTERS
 *@@include #include <os2.h>
 *@@include #include "helpers\linklist.h"       // for some features
 *@@include #include "helpers\threads.h"
 *@@include #include <wpobject.h>       // or any other WPS header, for KERNELGLOBALS
 *@@include #include "shared\common.h"
 *@@include #include "shared\kernel.h"
 */

/*
 *      Copyright (C) 1997-2002 Ulrich M�ller.
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, in version 2 as it comes in the COPYING
 *      file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#ifndef KERNEL_HEADER_INCLUDED
    #define KERNEL_HEADER_INCLUDED

    #ifndef INCL_DOSSEMAPHORES
        #error kernel.h requires INCL_DOSSEMAPHORES to be defined.
    #endif

    // log file names
    #ifndef __XWPLITE__
        #define XFOLDER_CRASHLOG        "xwptrap.log"
        #define XFOLDER_SHUTDOWNLOG     "xshutdwn.log"
        #define XFOLDER_LOGLOG          "xwplog.log"
        #define XFOLDER_DMNCRASHLOG     "xdmntrap.log"
    #else
        #define XFOLDER_CRASHLOG        "ewptrap.log"
        #define XFOLDER_SHUTDOWNLOG     "eshutdwn.log"
        #define XFOLDER_LOGLOG          "ewplog.log"
        #define XFOLDER_DMNCRASHLOG     "edmntrap.log"
    #endif

    /* ******************************************************************
     *
     *   Resource protection (thread safety)
     *
     ********************************************************************/

    BOOL krnLock(PCSZ pcszSourceFile,
                 ULONG ulLine,
                 PCSZ pcszFunction);

    VOID krnUnlock(VOID);

    ULONG krnQueryLock(VOID);

    /* ******************************************************************
     *
     *   Exception handlers (\helpers\except.c)
     *
     ********************************************************************/

    #ifdef INCL_DOSPROCESS
    FILE* _System krnExceptOpenLogFile(VOID);

    VOID _System krnExceptExplainXFolder(FILE *file,
                                         PTIB ptib,
                                         ULONG ulpri);  // V0.9.16 (2001-12-02) [pr]

    VOID APIENTRY krnExceptError(PCSZ pcszFile,
                                 ULONG ulLine,
                                 PCSZ pcszFunction,
                                 APIRET arc);
    #endif

    /********************************************************************
     *
     *   KERNELGLOBALS structure
     *
     ********************************************************************/

    // anchor block of WPS thread 1 (queried in initMain)
    extern HAB          G_habThread1;

    extern USHORT       G_usHiwordAbstract;
    extern USHORT       G_usHiwordFileSystem;

    #ifdef SOM_WPObject_h

        // startup flags for KERNELGLOBALS.ulStartupFlags
#ifndef __NOBOOTLOGO__
        #define SUF_SKIPBOOTLOGO            0x0001  // skip boot logo
#endif
#ifndef __NOXWPSTARTUP__
        #define SUF_SKIPXFLDSTARTUP         0x0002  // skip XFldStartup processing
#endif
#ifndef __NOQUICKOPEN__
        #define SUF_SKIPQUICKOPEN           0x0004  // skip "quick open" folder processing
#endif

        /*
         *@@ KERNELGLOBALS:
         *      this structure is stored in a static global
         *      variable in kernel.c, whose address can
         *      always be obtained thru t1QueryGlobals.
         *
         *      This structure is used to store information
         *      which is of importance to all parts of XWorkplace.
         *      This includes the old XFolder code as well as
         *      possible new parts.
         *
         *      Here we store information about the classes
         *      which have been successfully initialized and
         *      lots of thread data.
         *
         *      You may extend this structure if you need to
         *      store global data, but please do not modify
         *      the existing fields. Also, for lucidity, use
         *      this structure only if you need to access
         *      data across several code files. Otherwise,
         *      please use global variables.
         *
         *@@changed V0.9.9 (2001-03-07) [umoeller]: removed all THREADINFO's
         *@@changed V0.9.9 (2001-03-10) [umoeller]: added fDesktopPopulated
         */

        typedef struct _KERNELGLOBALS
        {
            // WPS process ID V0.9.9 (2001-01-31) [umoeller]
            PID                 pidWPS;
            // Workplace thread ID (PMSHELL.EXE); should be 1
            TID                 tidWorkplaceThread;

            // Desktop startup date and time (initMain)
            DATETIME            StartupDateTime;

            // PM error windows queried by initMain
            HWND                hwndHardError,
                                hwndSysError;

            ULONG               ulPanicFlags;
                    // flags set by the "panic" dialog if "Shift"
                    // was pressed during startup.
                    // Per default, this field is set to zero,
                    // but if the user disables something in the
                    // "Panic" dialog, this may have any of the
                    // following flags:
                    // -- SUF_SKIPBOOTLOGO: skip boot logo
                    // -- SUF_SKIPXFLDSTARTUP: skip XFldStartup processing
                    // -- SUF_SKIPQUICKOPEN: skip "quick open" folder processing

            /*
             * XWorkplace class objects (new with V0.9.0):
             *      if any of these is FALSE, this means that
             *      the class has not been installed. All
             *      these things are set to TRUE by the respective
             *      wpclsInitData class methods.
             *
             *      If you introduce a new class to the XWorkplace
             *      class setup, please add a BOOL to this list and
             *      set that field to TRUE in the wpclsInitData of
             *      your class.
             */

            // class replacements
            /* BOOL                fXFldObject,
                                fXFolder,
                                fXFldDisk,
                                fXFldDesktop,
                                fXFldDataFile,
                                fXWPProgramFile,
                                fXWPSound,
                                fXWPMouse,
                                fXWPKeyboard,
                                fXWPProgram,        // V0.9.9 (2001-04-02) [umoeller]

            // new classes
                                fXWPSetup,
                                fXFldSystem,
                                fXFldWPS,
                                fXWPScreen,
                                fXFldStartup,
                                fXFldShutdown,
                                fXWPClassList,
                                fXWPTrashCan,
                                fXWPTrashObject,
                                fXWPString,
                                fXWPMedia,
                                fXMMVolume,     // V0.9.6 (2000-11-09) [umoeller]
                                fXMMCDPlayer,   // V0.9.7 (2000-12-20) [umoeller]
                                fXCenter,       // V0.9.7 (2000-12-20) [umoeller]
                                fXWPFontFile,   // V0.9.7 (2001-01-12) [umoeller]
                                fXWPFontFolder, // V0.9.7 (2001-01-12) [umoeller]
                                fXWPFontObject; // V0.9.7 (2001-01-12) [umoeller]
            */

            /*
             * XWorkplace daemon
             */

            HAPP                happDaemon;
                    // != NULLHANDLE if daemon was started

            PVOID               pXwpGlobalShared;
                    // ptr to XWPGLOBALSHARED structure

            PVOID               pXWPShellShared;
                    // ptr to XWPSHELLSHARED structure
                    // (if XWPSHELL.EXE is running; NULL otherwise)

            /*
             * Thread-1 object window:
             *      additional object window on thread 1.
             *      This is always created.
             */

            HWND                hwndThread1Object;

            /*
             * API object window:
             *      additional object window on thread 1.
             *      This is always created.
             */

            HWND                hwndAPIObject;  // V0.9.9 (2001-03-23) [umoeller]

            /*
             * Worker thread:
             *      this thread is always running.
             */

            // HWND                hwndWorkerObject;

            // here comes the linked list to remember all objects which
            // have been awakened by the WPS; this list is maintained
            // by the Worker thread and evaluated during XShutdown

            // root of this linked list; this holds plain
            // WPObject* pointers and is created in initMain
            // with lstCreate(FALSE)
            // PVOID               pllAwakeObjects;
                    // this has changed with V0.90; this is actually a PLINKLIST,
                    // but since not all source files have #include'd linklist.h,
                    // we declare this as PVOID to avoid compilation errors.

                    // V0.9.9 (2001-04-04) [umoeller]: moved this to xthreads.c

            // mutex semaphore for access to this list
            // HMTX                hmtxAwakeObjects;
                    // V0.9.9 (2001-04-04) [umoeller]: moved this to xthreads.c

            // count of currently awake objects
            // LONG                lAwakeObjectsCount;
                    // V0.9.9 (2001-04-04) [umoeller]: moved this to xthreads.c

            // address of awake WarpCenter; stored by Worker
            // thread, read by Shutdown thread
            WPObject            *pAwakeWarpCenter;

            /*
             * Bush thread:
             *      this thread is always running.
             */

            HWND                hwndBushObject;

            /*
             * File thread:
             *      this thread is always running also,
             *      but with regular priority.
             */

            HWND                hwndFileObject;

            /*
             * Sentinel thread:
             *      replacement thread for WPS "WheelWatcher".
             *      This does not have a PM message queue.
             */

            BOOL                fAutoRefreshReplaced;
                                    // this is set to TRUE if, on Desktop startup,
                                    // the WPS WheelWatcher was successfully
                                    // stopped and the sentinel was started.
                                    // Always use this setting instead of
                                    // calling krnReplaceRefreshEnabled.

            /*
             * Shutdown threads:
             *      the following threads are only running
             *      while XShutdown is in progress.
             */

            // BOOL                fShutdownRunning;
                    // removbed V0.9.16 (2001-11-22) [umoeller]

            // desktop already populated?
            BOOL                fDesktopPopulated;

        } KERNELGLOBALS, *PKERNELGLOBALS;

        typedef const KERNELGLOBALS* PCKERNELGLOBALS;

        PCKERNELGLOBALS krnQueryGlobals(VOID);

        PKERNELGLOBALS krnLockGlobals(PCSZ pcszSourceFile,
                                      ULONG ulLine,
                                      PCSZ pcszFunction);

        VOID krnUnlockGlobals(VOID);

    #endif

    /* ******************************************************************
     *
     *   Class maintanance
     *
     ********************************************************************/

    BOOL krnClassInitialized(PCSZ pcszClassName);

    BOOL krnIsClassReady(PCSZ pcszClassName);

    /* ******************************************************************
     *
     *   Startup/Daemon interface
     *
     ********************************************************************/

    BOOL krnReplaceRefreshEnabled(VOID);

    VOID krnEnableReplaceRefresh(BOOL fEnable);

    VOID krnSetProcessStartupFolder(BOOL fReuse);

    BOOL krnNeed2ProcessStartupFolder(VOID);

    HWND APIENTRY krnQueryDaemonObject(VOID);
    typedef HWND APIENTRY KRNQUERYDAEMONOBJECT(VOID);
    typedef KRNQUERYDAEMONOBJECT *PKRNQUERYDAEMONOBJECT;

    HAPP krnStartDaemon(VOID);

    BOOL krnPostDaemonMsg(ULONG msg, MPARAM mp1, MPARAM mp2);
    typedef BOOL KRNPOSTDAEMONMSG(ULONG msg, MPARAM mp1, MPARAM mp2);
    typedef KRNPOSTDAEMONMSG *PKRNPOSTDAEMONMSG;

    MRESULT krnSendDaemonMsg(ULONG msg, MPARAM mp1, MPARAM mp2);
    typedef MRESULT KRNSENDDAEMONMSG(ULONG msg, MPARAM mp1, MPARAM mp2);
    typedef KRNSENDDAEMONMSG *PKRNSENDDAEMONMSG;

    /* ******************************************************************
     *
     *   Thread-1 object window
     *
     ********************************************************************/

    // #define T1M_BEGINSTARTUP            (WM_USER+270)
    // #define T1M_STARTCONTENT            (WM_USER+271)
    // #define T1M_POCCALLBACK             (WM_USER+272)
    // #define T1M_BEGINQUICKOPEN          (WM_USER+273)
    // #define T1M_NEXTQUICKOPEN           (WM_USER+274)
            // all removed V0.9.12 (2001-04-29) [umoeller]

    #define T1M_LIMITREACHED            (WM_USER+275)

    #define T1M_EXCEPTIONCAUGHT         (WM_USER+276)

    // #define T1M_EXTERNALSHUTDOWN        (WM_USER+277)
            // removed V0.9.18 (2002-03-27) [umoeller]

    #define T1M_DESTROYARCHIVESTATUS    (WM_USER+278)    // added V0.9.0

    #define T1M_OPENOBJECTFROMHANDLE    (WM_USER+280)    // added V0.9.0

    #define T1M_DAEMONREADY             (WM_USER+281)    // added V0.9.0

    #define T1M_PAGERCLOSED             (WM_USER+282)    // added V0.9.2 (2000-02-23) [umoeller]

    #define T1M_QUERYXFOLDERVERSION     (WM_USER+283)
                // V0.9.2 (2000-02-26) [umoeller]:
                // msg value changed to break compatibility with V0.8x

#ifndef __NOPAGER__
    #define T1M_PAGERCONFIGDELAYED      (WM_USER+284)
#endif

    /*
     *@@ T1M_FOPS_TASK_DONE:
     *      posted by file thread every time a file
     *      operation has completed. This message does
     *      nothing, but is only checked for in fopsStartTask
     *      for modal operations.
     *
     *      Parameters:
     *      -- HFILETASKLIST mp1: file task list handle.
     *      -- FOPSRET mp2: file-operations return code.
     *
     *@@added V0.9.4 (2000-08-03) [umoeller]
     */

    #define T1M_FOPS_TASK_DONE          (WM_USER+285)

#ifndef __XWPLITE__
    #define T1M_WELCOME                 (WM_USER+286)
#endif

    #define T1M_OPENOBJECTFROMPTR       (WM_USER+287)    // added V0.9.9 (2001-02-06) [umoeller]

    #define T1M_MENUITEMSELECTED        (WM_USER+289)    // added V0.9.11 (2001-04-18) [umoeller]

    #define T1M_PAGERCTXTMENU           (WM_USER+290)    // V0.9.11 (2001-04-25) [umoeller]

    #define T1M_INITIATEXSHUTDOWN       (WM_USER+291)    // V0.9.12 (2001-04-28) [umoeller]

    #define T1M_OPENRUNDIALOG           (WM_USER+292)    // V0.9.14 (2001-08-07) [pr]

    // #define T1M_PROGOPENPROGRAM         (WM_USER+293)    // V0.9.16 (2001-12-02) [umoeller]
            // removed again V0.9.19 (2002-03-28) [umoeller]

    BOOL krnPostThread1ObjectMsg(ULONG msg, MPARAM mp1, MPARAM mp2);

    MRESULT krnSendThread1ObjectMsg(ULONG msg, MPARAM mp1, MPARAM mp2);

#endif