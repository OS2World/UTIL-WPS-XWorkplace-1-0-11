
#
# makefile:
#       makefile for main directory.
#       For use with IBM NMAKE, which comes with the IBM compilers,
#       the Developer's Toolkit, and the DDK.
#
#       All the makefiles have been restructured with V0.9.0.
#
#       Called from:    nowhere, maybe MAKE.CMD. Main makefile.
#                       This recurses into the subdirectories.
#
#       Input:          specify the target(s) to be made, which can be:
#
#                       --  "all" (default): build XFLDR.DLL, SOUND.DLL,
#                           XWPHOOK.DLL, XWPDAEMN.EXE.
#                       --  "really_all": "all" plus external EXEs
#                           (Treesize, Netscape DDE, et al) plus NLS
#                           specified by the XWP_LANG_CODE variable,
#                           which defaults to "001" (setup.in).
#
#                       The following subtargets exist (which get called
#                       by the "all" target):
#
#                       --  nls: compile $(XWP_LANG_CODE)\ directory
#                       --  tools: compile TOOLS\ directory
#                       --  idl: update SOM headers (include\classes\*)
#                       --  cpl_main: compile *.c files for DLLs, no link
#                       --  link: link bin\*.obj to DLLs and copy to XWorkplace
#                                 install directory
#
#                       Use "nmake -a [<targets>] to _re_build the targets,
#                       even if they are up to date.
#
#                       Other special targets not used by "all" or "really_all":
#
#                       --  dlgedit: invoke dialog editor on NLS DLL
#                       --  release: create/update release tree in directory
#                           specified by XWPRELEASE; this invokes "really_all"
#                           in turn.
#
#       Output:         All XWorkplace Files code files. This calls the other
#                       makefiles. Note that this does _not_ build the NLS
#                       directories (XFLDRxxx.DLL, INF, HLP files); use
#                       MAKE.CMD for that, which calls this makefile in turn.
#
#                       Output files are first created in bin\, then copied
#                       to XWPRUNNING, which must be defined externally or
#                       thru "setup.in".
#
#       Edit "setup.in" to set up the make process (compilation flags etc.).
#

# Say hello to yourself.
!if [@echo +++++ Entering $(MAKEDIR)]
!endif

# PROJECT_BASE_DIR is used by "setup.in"
# subdirectories to identify the root of the source
# tree. This is passed to the sub-makefiles.
PROJECT_BASE_DIR = $(MAKEDIR)

# MODULESDIR is used for mapfiles and final module (DLL, EXE) output.
MODULESDIR=bin\modules

# create output directory
!if [@md $(bin) 2> NUL]
!endif
!if [@md $(MODULESDIR) 2> NUL]
!endif

# include setup (compiler options etc.)
!include setup.in

# VARIABLES
# ---------

# The OBJS macro contains all the .OBJ files which have been
# created from the files in MAIN\.
OBJS = bin\xdebug.obj \
# code from classes\
    bin\xfobj.obj bin\xfldr.obj bin\xfdesk.obj bin\xfsys.obj bin\xfwps.obj \
    bin\xfdisk.obj bin\xfdataf.obj bin\xfpgmf.obj bin\xfstart.obj \
    bin\xclslist.obj bin\xwpsound.obj bin\xtrash.obj \
    bin\xwpkeybd.obj bin\xwpmouse.obj bin\xwpsetup.obj bin\xwpscreen.obj \
# code from shared \
    bin\classes.obj bin\cnrsort.obj bin\common.obj bin\notebook.obj bin\kernel.obj bin\xsetup.obj \
# code from config\
    bin\cfgsys.obj bin\classlst.obj bin\drivdlgs.obj bin\hookintf.obj \
    bin\partitions.obj bin\sound.obj \
# code from filesys\
    bin\disk.obj bin\fdrhotky.obj bin\fdrnotebooks.obj bin\fdrsubclass.obj bin\fileops.obj bin\filesys.obj \
    bin\filetype.obj bin\folder.obj bin\menus.obj bin\object.obj bin\desktop.obj \
    bin\statbars.obj bin\trash.obj bin\xthreads.obj \
# code from startshut\
    bin\apm.obj bin\archives.obj bin\shutdown.obj

OBJS_ANISOM = bin\sominit.obj bin\wpwcur.obj
# bin\wpwani.obj bin\wpand.obj bin\wpoptr.obj
OBJS_ANICONVERT = bin\cursor.obj bin\pointer.obj \
    bin\script.obj bin\eas.obj bin\expire.obj
# bin\dll.obj bin\dllbin.obj
OBJS_ANIANI = bin\mptranim.obj bin\mptrcnr.obj bin\mptredit.obj bin\mptrlset.obj \
    bin\mptrpag1.obj bin\mptrppl.obj bin\mptrprop.obj bin\mptrptr.obj bin\mptrset.obj \
    bin\mptrutil.obj bin\mptrfile.obj

ANIOBJS =
# bin\wpamptr.obj $(OBJS_ANISOM) $(OBJS_ANIANI) $(OBJS_ANICONVERT)

# The HLPOBJS macro contains all the .OBJ files which have been
# created from the files in HELPERS\. You probably won't have to change this.
HLPOBJS = bin\animate.obj bin\comctl.obj bin\cnrh.obj bin\datetime.obj \
    bin\debug.obj bin\dosh.obj bin\eah.obj bin\except.obj bin\gpih.obj \
    bin\level.obj bin\linklist.obj bin\memdebug.obj bin\procstat.obj \
    bin\prfh.obj bin\shapewin.obj bin\stringh.obj bin\syssound.obj \
    bin\threads.obj bin\textview.obj bin\tmsgfile.obj \
    bin\winh.obj bin\wphandle.obj bin\wpsh.obj bin\xstring.obj

# The DMNOBJS macro contains all the .OBJ files for XWPDAEMN.EXE.
DMNOBJS = bin\exes\xwpdaemn.obj \
          bin\exes\pgmg_control.obj bin\exes\pgmg_move.obj bin\exes\pgmg_settings.obj \
                bin\exes\pgmg_winscan.obj \
          bin\exes\threads.obj

# Define the suffixes for files which NMAKE will work on.
# .SUFFIXES is a reserved NMAKE keyword ("pseudotarget") for
# defining file extensions that NMAKE will recognize in inference
# rules.

.SUFFIXES: .obj .dll .exe .h .rc .res

# The LIBS macro contains all the .LIB files, either from the compiler or
# others, which are needed for this project:
#   somtk       is the SOM toolkit lib
#   pmprintf    is for debugging
# The other OS/2 libraries are used by default.
PMPRINTF_LIB = $(HELPERS_BASE)\src\helpers\pmprintf.lib
LIBS = somtk.lib $(PMPRINTF_LIB)

# some variable strings to pass to sub-nmakes
PROJECT_BASE_DIR_STRING = "PROJECT_BASE_DIR=$(PROJECT_BASE_DIR)" "PROJECT_INCLUDE=$(PROJECT_INCLUDE)"
INCLUDE_STRING = "PROJECT_INCLUDE = $(PROJECT_INCLUDE)";

# store current directory so we can change back later
CURRENT_DIR = $(MAKEDIR)


# PSEUDOTARGETS
# -------------

all: idl cpl_main link
    @echo ----- Leaving $(MAKEDIR)

# "really_all" references "all".
really_all: tools all nls
    @echo ----- Leaving $(MAKEDIR)

# If you add a subdirectory to SRC\, add a target to
# "cpl_main" also to have automatic recompiles.
cpl_main: classes config filesys helpers shared startshut hook treesize netscdde xshutdwn
#animouse

# COMPILER PSEUDOTARGETS
# ----------------------

tools:
    @echo $(MAKEDIR)\makefile: Going for subdir tools
    @cd tools
    @nmake -nologo all "MAINMAKERUNNING=YES"
    @cd ..

idl:
    @echo $(MAKEDIR)\makefile: Going for subdir idl
    @cd idl
    @nmake -nologo all "MAINMAKERUNNING=YES"
    @cd ..

classes:
    @echo $(MAKEDIR)\makefile: Going for subdir src\classes
    @cd src\classes
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

config:
    @echo $(MAKEDIR)\makefile: Going for subdir src\config
    @cd src\config
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

filesys:
    @echo $(MAKEDIR)\makefile: Going for subdir src\filesys
    @cd src\filesys
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

helpers:
# this branches over to the WarpIN source tree,
# which is prepared for this
    @echo $(MAKEDIR)\makefile: Going for subdir src\helpers (from WarpIN source tree)
    @echo $(MAKEDIR)\makefile: HELPERS_BASE is defined from setup.in as: $(HELPERS_BASE)
    @cd $(HELPERS_BASE)\src\helpers
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd $(CURRENT_DIR)

shared:
    @echo $(MAKEDIR)\makefile: Going for subdir src\shared
    @cd src\shared
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

startshut:
    @echo $(MAKEDIR)\makefile: Going for subdir src\startshut
    @cd src\startshut
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

animouse:
    @echo $(MAKEDIR)\makefile: Going for subdir src\animouse
    @cd src\animouse
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

hook:
    @echo $(MAKEDIR)\makefile: Going for subdir src\hook
    @cd src\hook
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

treesize:
    @echo $(MAKEDIR)\makefile: Going for subdir src\treesize
    @cd src\treesize
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

netscdde:
    @echo $(MAKEDIR)\makefile: Going for subdir src\NetscapeDDE
    @cd src\NetscapeDDE
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

xshutdwn:
    @echo $(MAKEDIR)\makefile: Going for subdir src\xshutdwn
    @cd src\xshutdwn
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..

nls:
    @echo $(MAKEDIR)\makefile: Going for subdir $(XWP_LANG_CODE)\dll
    @cd $(XWP_LANG_CODE)\dll
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..
    @echo $(MAKEDIR)\makefile: Going for subdir $(XWP_LANG_CODE)\inf.$(XWP_LANG_CODE)
    @cd inf.$(XWP_LANG_CODE)
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..
    @echo $(MAKEDIR)\makefile: Going for subdir $(XWP_LANG_CODE)\help.$(XWP_LANG_CODE)
    @cd help.$(XWP_LANG_CODE)
    @nmake -nologo all "MAINMAKERUNNING=YES" $(PROJECT_BASE_DIR_STRING)
    @cd ..\..


# LINKER PSEUDOTARGETS
# --------------------

link: $(XWPRUNNING)\bin\xfldr.dll \
      $(XWPRUNNING)\bin\xwphook.dll \
      $(XWPRUNNING)\bin\xwpdaemn.exe \
      $(XWPRUNNING)\bin\sound.dll \
      $(XWPRUNNING)\bin\xdebug.dll

# Finally, define rules for linking the target DLLs and EXEs
# This uses the $OBJS and $HLPOBJS macros defined at the top.
#
# The actual targets are the DLLs and EXEs in the XWorkplace
# installation directory. We create the target in bin\ first
# and then copy it thereto.

#
# XFLDR.DLL
#
$(XWPRUNNING)\bin\xfldr.dll: $(MODULESDIR)\$(@B).dll
        unlock $(XWPRUNNING)\bin\$(@B).dll
        cmd.exe /c copy $(MODULESDIR)\$(@B).dll $(XWPRUNNING)\bin
!ifndef DEBUG
# copy symbol file, which is only needed if debug code is disabled
        cmd.exe /c copy $(MODULESDIR)\$(@B).sym $(XWPRUNNING)\bin
!endif
!ifdef DYNAMIC_TRACE
        @echo $(MAKEDIR)\makefile: Creating TRACE files for $(@B).dll
        maptsf $(@B).map /MAJOR=255 /LOGSTACK=32 /LOGRETURN > $(@B).tsf
        trcust $(@B).tsf /I /L=bin\$(@B).dll /node /M=$(@B).map
        @echo $(MAKEDIR)\makefile: Done creating TRACE files for $(@B).dll
        cmd.exe /c copy $(@B).tdf $(XWPRUNNING)\bin
        cmd.exe /c del $(@B).tdf
        cmd.exe /c copy TRC00FF.TFF $(DYNAMIC_TRACE):\OS2\SYSTEM\TRACE
        cmd.exe /c del TRC00FF.TFF
!endif

# update DEF file if buildlevel has changed
src\shared\xwp.def: include\bldlevel.h
        cmd.exe /c BuildLevel.cmd src\shared\$(@B).def include\bldlevel.h "XWorkplace Main WPS Classes Module"

$(MODULESDIR)\xfldr.dll: $(OBJS) $(HLPOBJS) $(ANIOBJS) src\shared\xwp.def bin\xwp.res
        @echo $(MAKEDIR)\makefile: Linking $(MODULESDIR)\$(@B).dll
        $(LINK) /OUT:$(MODULESDIR)\$(@B).dll src\shared\xwp.def @<<link.tmp
$(OBJS) $(HLPOBJS) $(ANIOBJS) $(LIBS)
<<
        @cd $(MODULESDIR)
        $(RC) ..\xwp.res $(@B).dll
!ifndef DEBUG
# create symbol file, which is only needed if debug code is disabled
        mapsym /n $(@B).map > NUL
!endif
        @cd $(CURRENT_DIR)

#
# XWPDAEMN.EXE
#
$(XWPRUNNING)\bin\xwpdaemn.exe: $(MODULESDIR)\$(@B).exe
        cmd.exe /c copy $(MODULESDIR)\$(@B).exe $(XWPRUNNING)\bin
!ifndef DEBUG
# copy symbol file, which is only needed if debug code is disabled
        cmd.exe /c copy $(MODULESDIR)\$(@B).sym $(XWPRUNNING)\bin
!endif

# update DEF file if buildlevel has changed
src\hook\xwpdaemn.def: include\bldlevel.h
        cmd.exe /c BuildLevel.cmd src\hook\$(@B).def include\bldlevel.h "XWorkplace PM Daemon"

# create import library from XWPHOOK.DLL
bin\xwphook.lib: $(MODULESDIR)\$(@B).dll src\hook\$(@B).def
        implib /nologo bin\$(@B).lib $(MODULESDIR)\$(@B).dll

$(MODULESDIR)\xwpdaemn.exe: src\hook\$(@B).def bin\xwphook.lib $(DMNOBJS) bin\exes\$(@B).res
        @echo $(MAKEDIR)\makefile: Linking $(MODULESDIR)\$(@B).exe
        $(LINK) /OUT:$(MODULESDIR)\$(@B).exe src\hook\$(@B).def $(DMNOBJS) bin\xwphook.lib $(PMPRINTF_LIB)
        @cd $(MODULESDIR)
        $(RC) ..\exes\$(@B).res $(@B).exe
!ifndef DEBUG
# create symbol file, which is only needed if debug code is disabled
        mapsym /n $(@B).map > NUL
!endif
        @cd $(CURRENT_DIR)

#
# XWPHOOK.DLL
#
$(XWPRUNNING)\bin\xwphook.dll: $(MODULESDIR)\$(@B).dll
# no unlock, this is a hook        unlock $(XWPRUNNING)\bin\$(@B).dll
        cmd.exe /c copy $(MODULESDIR)\$(@B).dll $(XWPRUNNING)\bin
!ifndef DEBUG
# copy symbol file, which is only needed if debug code is disabled
        cmd.exe /c copy $(MODULESDIR)\$(@B).sym $(XWPRUNNING)\bin
!endif
!ifdef DYNAMIC_TRACE
        @echo $(MAKEDIR)\makefile: Creating TRACE files for $(@B).dll
        maptsf $(@B).map /MAJOR=253 /LOGSTACK=32 /LOGRETURN > $(@B).tsf
        trcust $(@B).tsf /I /L=bin\$(@B).dll /node /M=$(@B).map
        @echo $(MAKEDIR)\makefile: Done creating TRACE files for $(@B).dll
        cmd.exe /c copy $(@B).tdf $(XWPRUNNING)\bin
        cmd.exe /c del $(@B).tdf
        cmd.exe /c copy TRC00FD.TFF $(DYNAMIC_TRACE):\OS2\SYSTEM\TRACE
        cmd.exe /c del TRC00FD.TFF
!endif

# update DEF file if buildlevel has changed
src\hook\xwphook.def: include\bldlevel.h
        cmd.exe /c BuildLevel.cmd src\hook\$(@B).def include\bldlevel.h "XWorkplace PM Hook Module"

$(MODULESDIR)\xwphook.dll: src\hook\$(@B).def bin\$(@B).obj
        @echo $(MAKEDIR)\makefile: Linking $(MODULESDIR)\$(@B).dll
        $(LINK) /OUT:$(MODULESDIR)\$(@B).dll src\hook\$(@B).def bin\$(@B).obj $(PMPRINTF_LIB)
        @cd $(MODULESDIR)
!ifndef DEBUG
# create symbol file, which is only needed if debug code is disabled
        mapsym /n $(@B).map > NUL
!endif
        @cd $(CURRENT_DIR)

#
# SOUND.DLL
#
$(XWPRUNNING)\bin\sound.dll: $(MODULESDIR)\$(@B).dll
        unlock $(XWPRUNNING)\bin\$(@B).dll
        cmd.exe /c copy $(MODULESDIR)\$(@B).dll $(XWPRUNNING)\bin
!ifndef DEBUG
# copy symbol file, which is only needed if debug code is disabled
        cmd.exe /c copy $(MODULESDIR)\$(@B).sym $(XWPRUNNING)\bin
!endif

# update DEF file if buildlevel has changed
src\shared\sounddll.def: include\bldlevel.h
        cmd.exe /c BuildLevel.cmd src\shared\$(@B).def include\bldlevel.h "XWorkplace Sound Support Module"

$(MODULESDIR)\sound.dll: src\shared\sounddll.def bin\sounddll.obj
        @echo $(MAKEDIR)\makefile: Linking $(MODULESDIR)\$(@B).dll
        $(LINK) /OUT:$(MODULESDIR)\$(@B).dll bin\sounddll.obj src\shared\sounddll.def mmpm2.lib $(PMPRINTF_LIB)
        @cd $(MODULESDIR)
!ifndef DEBUG
# create symbol file, which is only needed if debug code is disabled
        mapsym /n $(@B).map > NUL
!endif
        @cd $(CURRENT_DIR)

#
# XDEBUG.DLL
#
$(XWPRUNNING)\bin\xdebug.dll: $(MODULESDIR)\$(@B).dll
        unlock $(XWPRUNNING)\bin\$(@B).dll
        cmd.exe /c copy $(MODULESDIR)\$(@B).dll $(XWPRUNNING)\bin

$(MODULESDIR)\xdebug.dll: src\shared\$(@B).def bin\$(@B).obj $(HLPOBJS)
        $(LINK) /OUT:$(MODULESDIR)\$(@B).dll src\shared\$(@B).def bin\$(@B).obj $(HLPOBJS) $(LIBS)

#
# Special target "dlgedit": this is not called by "all",
# but must be set on the NMAKE command line.
# Note that you need DLGEDIT.EXE from the Toolkit for this.
#

dlgedit:
# added (UM 99-10-24)
    @echo $(MAKEDIR)\makefile: Calling DLGEDIT.EXE
    @cd $(XWP_LANG_CODE)\dll
# rebuild RES file in bin
    @nmake -nologo all "MAINMAKERUNNING=YES"
# copy RES file to frontend.res so dlgedit finds it
    @cmd.exe /c copy ..\..\bin\xfldr$(XWP_LANG_CODE).res
    @cmd.exe /c copy ..\..\include\dlgids.h
# invoke DLGEDIT
    dlgedit xfldr$(XWP_LANG_CODE).res
# move newly created RES file back to \bin
    @cmd.exe /c copy xfldr$(XWP_LANG_CODE).res ..\bin
    @cmd.exe /c del xfldr$(XWP_LANG_CODE).res
    @cmd.exe /c del dlgids.h
    @nmake -nologo all "MAINMAKERUNNING=YES"
    @cd ..\..

dlgedit049:
# added V0.9.1 (99-12-19) [umoeller]
    @echo $(MAKEDIR)\makefile: Calling DLGEDIT.EXE
    @cd 049_de\dll
# rebuild RES file in bin
    @nmake -nologo all "MAINMAKERUNNING=YES"
# copy RES file to frontend.res so dlgedit finds it
    @cmd.exe /c copy ..\..\bin\xfldr049.res
    @cmd.exe /c copy ..\..\include\dlgids.h
# invoke DLGEDIT
    dlgedit xfldr049.res
# move newly created RES file back to \bin
    @cmd.exe /c copy xfldr049.res ..\bin
    @cmd.exe /c del xfldr049.res
    @cmd.exe /c del dlgids.h
    @nmake -nologo all "MAINMAKERUNNING=YES"
    @cd ..\..

#
# Special target "release": this is not called by "all",
# but must be set on the NMAKE command line.
#

release: really_all
# 1) main dir
!ifndef XWPRELEASE
!error XWPRELEASE must be set before calling "make release". Terminating.
!endif
# create directories
!if [@md $(XWPRELEASE) 2> NUL]
!endif
!if [@md $(XWPRELEASE_MAIN) 2> NUL]
!endif
!if [@md $(XWPRELEASE_NLS) 2> NUL]
!endif
    @echo $(MAKEDIR)\makefile: Now copying files to $(XWPRELEASE).
    $(COPY) release\* $(XWPRELEASE_MAIN)
    $(COPY) $(XWP_LANG_CODE)\readme $(XWPRELEASE_NLS)
    $(COPY) $(XWP_LANG_CODE)\inf.$(XWP_LANG_CODE)\xfldr$(XWP_LANG_CODE).inf $(XWPRELEASE_NLS)
    $(COPY) BUGS $(XWPRELEASE_MAIN)
    $(COPY) FEATURES $(XWPRELEASE_MAIN)
#
# 2) bin
#    a) kernel
!if [@md $(XWPRELEASE_MAIN)\bin 2> NUL]
!endif
!if [@md $(XWPRELEASE_NLS)\bin 2> NUL]
!endif
    $(COPY) release\bin\* $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\sound.dll $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\sound.sym $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\xfldr.dll $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\xfldr.sym $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\xwphook.dll $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\xwphook.sym $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\*.exe $(XWPRELEASE_MAIN)\bin
    $(COPY) $(MODULESDIR)\*.sym $(XWPRELEASE_MAIN)\bin
    $(COPY) tools\repclass.exe $(XWPRELEASE_MAIN)\bin
    $(COPY) tools\wpsreset.exe $(XWPRELEASE_MAIN)\bin
#    b) NLS
    $(COPY) bin\xfldr$(XWP_LANG_CODE).dll $(XWPRELEASE_NLS)\bin
    $(COPY) $(XWP_LANG_CODE)\misc\*.sgs $(XWPRELEASE_NLS)\bin
#
# 3) bootlogo
!if [@md $(XWPRELEASE_MAIN)\bootlogo 2> NUL]
!endif
    $(COPY) release\bootlogo\* $(XWPRELEASE_MAIN)\bootlogo
#
# 4) help
!if [@md $(XWPRELEASE_NLS)\help 2> NUL]
!endif
    $(COPY) $(XWP_LANG_CODE)\misc\xfldr$(XWP_LANG_CODE).tmf $(XWPRELEASE_NLS)\help
    $(COPY) $(XWP_LANG_CODE)\misc\drvrs$(XWP_LANG_CODE).txt $(XWPRELEASE_NLS)\help
    $(COPY) $(XWP_LANG_CODE)\misc\xfcls$(XWP_LANG_CODE).txt $(XWPRELEASE_NLS)\help
    $(COPY) $(XWP_LANG_CODE)\help.$(XWP_LANG_CODE)\xfldr$(XWP_LANG_CODE).hlp $(XWPRELEASE_NLS)\help
# 5) icons
!if [@md $(XWPRELEASE_MAIN)\icons 2> NUL]
!endif
    $(COPY) release\icons\* $(XWPRELEASE_MAIN)\icons
# 6) install
!if [@md $(XWPRELEASE_MAIN)\install 2> NUL]
!endif
!if [@md $(XWPRELEASE_NLS)\install 2> NUL]
!endif
    $(COPY) release\install\* $(XWPRELEASE_MAIN)\install
    $(COPY) $(XWP_LANG_CODE)\misc\*.cmd $(XWPRELEASE_NLS)\install
    $(COPY) $(XWP_LANG_CODE)\misc\*.msg $(XWPRELEASE_NLS)\install
# 7) wav
!if [@md $(XWPRELEASE_MAIN)\wav 2> NUL]
!endif
    $(COPY) release\wav\* $(XWPRELEASE_MAIN)\wav
    @echo $(MAKEDIR)\makefile: Done copying files.


