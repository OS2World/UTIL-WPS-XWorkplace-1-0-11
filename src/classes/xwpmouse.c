
/*
 *@@sourcefile xwpmouse.c:
 *      This file contains SOM code for the following XWorkplace classes:
 *
 *      --  XWPMouse (WPMouse replacement)
 *
 *      This class replaces the WPMouse class, which implements the
 *      "Mouse" settings object, to introduce new settings pages
 *      for sliding focus and such. This is all new with V0.9.0.
 *
 *      Installation of this class is optional, but you won't
 *      be able to influence certain XWorkplace hook settings without it.
 *
 *      Starting with V0.9.0, the files in classes\ contain only
 *      the SOM interface, i.e. the methods themselves.
 *      The implementation for this class is in in filesys\hookintf.c.
 *
 *@@added V0.9.0 [umoeller]
 *
 *@@somclass XWPMouse xms_
 *@@somclass M_XWPMouse xmsM_
 */

/*
 *      Copyright (C) 1999-2000 Ulrich M�ller.
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

#ifndef SOM_Module_xwpmouse_Source
#define SOM_Module_xwpmouse_Source
#endif
#define XWPMouse_Class_Source
#define M_XWPMouse_Class_Source

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

#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES
#define INCL_WINDIALOGS
#define INCL_WINSTDBOOK
#define INCL_GPIBITMAPS
#include <os2.h>

// C library headers
#include <stdio.h>

// generic headers
#include "setup.h"                      // code generation and debugging options

// headers in /helpers

// SOM headers which don't crash with prec. header files
#include "xwpmouse.ih"

// XWorkplace implementation headers
#include "dlgids.h"                     // all the IDs that are shared with NLS
#include "shared\common.h"              // the majestic XWorkplace include file
#include "shared\helppanels.h"          // all XWorkplace help panel IDs
#include "shared\kernel.h"              // XWorkplace Kernel
#include "shared\notebook.h"            // generic XWorkplace notebook handling

#include "config\hookintf.h"            // daemon/hook interface

#ifdef __ANIMATED_MOUSE_POINTERS__
#include "pointers\mptrcnr.h"           // Animated Mouse Pointers
#include "pointers\macros.h"            // Animated Mouse Pointers
#include "pointers\mptrutil.h"          // Animated Mouse Pointers
#include "pointers\mptrset.h"           // Animated Mouse Pointers
// #include "pointers\r_wpamptr.h"         // Animated Mouse Pointers -- resources

#include "pointers\mptrpage.h"          // Animated Mouse Pointers -- entrypoints
#endif

// other SOM headers
#pragma hdrstop

/*
 *@@ xwpAddMouseMovementPage:
 *      this actually adds the "Movement" page into the
 *      "Mouse" notebook to configure the XWorkplace hook.
 *      Gets called by XWPMouse::wpAddMouseCometPage.
 *
 *@@changed V0.9.9 (2001-03-27) [umoeller]: moved "Corners" from XWPMouse to XWPScreen
 *@@changed V0.9.14 (2001-08-02) [lafaix]: added a second movement page
 */

SOM_Scope ULONG  SOMLINK xms_xwpAddMouseMovementPage(XWPMouse *somSelf,
                                                     HWND hwndDlg)
{
    PCREATENOTEBOOKPAGE pcnbp;
    HMODULE             savehmod = cmnQueryNLSModuleHandle(FALSE);
    // PCGLOBALSETTINGS    pGlobalSettings = cmnQueryGlobalSettings();
    // PNLSSTRINGS         pNLSStrings = cmnQueryNLSStrings();
    ULONG               ulrc = 0;

    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_xwpAddMouseMovementPage");

    // insert "MouseHook" page if the hook has been enabled
    if (hifXWPHookReady())
    {
        // moved "corners" to XWPScreen V0.9.9 (2001-03-27) [umoeller]

        // second movement page V0.9.14 (2001-08-02) [lafaix]
#ifndef __NOMOVEMENT2FEATURES__
        pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
        memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
        pcnbp->somSelf = somSelf;
        pcnbp->hwndNotebook = hwndDlg;
        pcnbp->hmod = savehmod;
        pcnbp->ulDlgID = ID_XSD_MOUSE_MOVEMENT2;
        pcnbp->usPageStyleFlags = BKA_MINOR;
        pcnbp->pszName = cmnGetString(ID_XSSI_MOUSEHOOKPAGE);  // pszMouseHookPage
        pcnbp->fEnumerate = TRUE;
        pcnbp->ulDefaultHelpPanel  = ID_XSH_MOUSE_MOVEMENT2;
        pcnbp->ulPageID = SP_MOUSE_MOVEMENT2;
        pcnbp->pfncbInitPage    = hifMouseMovement2InitPage;
        pcnbp->pfncbItemChanged = hifMouseMovement2ItemChanged;
        ulrc = ntbInsertPage(pcnbp);
#endif

        pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
        memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
        pcnbp->somSelf = somSelf;
        pcnbp->hwndNotebook = hwndDlg;
        pcnbp->hmod = savehmod;
        pcnbp->ulDlgID = ID_XFD_EMPTYDLG; // ID_XSD_MOUSE_MOVEMENT;
        pcnbp->usPageStyleFlags = BKA_MAJOR;
        pcnbp->pszName = cmnGetString(ID_XSSI_MOUSEHOOKPAGE);  // pszMouseHookPage
        // pcnbp->fEnumerate = TRUE;
        pcnbp->fEnumerate = TRUE; // re-enabled V0.9.14 (2001-08-02) [lafaix]
        pcnbp->ulDefaultHelpPanel  = ID_XSH_MOUSE_MOVEMENT;
        pcnbp->ulPageID = SP_MOUSE_MOVEMENT;
        pcnbp->pfncbInitPage    = hifMouseMovementInitPage;
        pcnbp->pfncbItemChanged = hifMouseMovementItemChanged;
        ulrc = ntbInsertPage(pcnbp);
    }

    return (ulrc);
}

/*
 *@@ xms_xwpAddMouseMappings2Page:
 *      this adds the second "Mappings" page to the
 *      "Mouse" settings notebook after the WPS's
 *      standard "Mappings" page.
 *      Gets called by XWPMouse::wpAddMouseMappingsPage.
 *
 *@@added V0.9.1 [umoeller]
 *@@changed V0.9.3 (2000-04-01) [umoeller]: page wasn't showing "page 2/2"; fixed
 */

SOM_Scope ULONG  SOMLINK xms_xwpAddMouseMappings2Page(XWPMouse *somSelf,
                                                      HWND hwndDlg)
{
    PCREATENOTEBOOKPAGE pcnbp;
    HMODULE             savehmod = cmnQueryNLSModuleHandle(FALSE);
    // PCGLOBALSETTINGS    pGlobalSettings = cmnQueryGlobalSettings();
    // PNLSSTRINGS         pNLSStrings = cmnQueryNLSStrings();
    ULONG               ulrc = 0;

    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_xwpAddMouseMappings2Page");

    // insert "MouseHook" page if the hook has been enabled
    if (hifXWPHookReady())
    {
        pcnbp = malloc(sizeof(CREATENOTEBOOKPAGE));
        memset(pcnbp, 0, sizeof(CREATENOTEBOOKPAGE));
        pcnbp->somSelf = somSelf;
        pcnbp->hwndNotebook = hwndDlg;
        pcnbp->hmod = savehmod;
        pcnbp->ulDlgID = ID_XSD_MOUSEMAPPINGS2;
        pcnbp->usPageStyleFlags = BKA_MINOR;
        pcnbp->fEnumerate = TRUE;
        pcnbp->pszName = cmnGetString(ID_XSSI_MAPPINGSPAGE);  // pszMappingsPage
        pcnbp->ulDefaultHelpPanel  = ID_XSH_MOUSEMAPPINGS2;
        pcnbp->ulPageID = SP_MOUSE_MAPPINGS2;
        pcnbp->pfncbInitPage    = hifMouseMappings2InitPage;
        pcnbp->pfncbItemChanged = hifMouseMappings2ItemChanged;
        ulrc = ntbInsertPage(pcnbp);
    }

    return (ulrc);
}

/*
 *@@ xwpAddAnimatedMousePointerPage:
 *      this inserts the animated "mouse pointer" replacement
 *      page into the "Mouse" notebook.
 */

SOM_Scope ULONG  SOMLINK xms_xwpAddAnimatedMousePointerPage(XWPMouse *somSelf,
                                                            HWND hwndDlg)
{
    #ifdef __ANIMATED_MOUSE_POINTERS__
        PAGEINFO pi;
        CHAR szTabName[MAX_RES_STRLEN];
        HAB hab = WinQueryAnchorBlock(HWND_DESKTOP);
        HMODULE hmodResource = cmnQueryNLSModuleHandle(FALSE);

        XWPMouseData *somThis = XWPMouseGetData(somSelf);
        XWPMouseMethodDebug("XWPMouse","xms_xwpAddAnimatedMousePointerPage");

        LOADSTRING(IDTAB_NBPAGE, szTabName);

        memset((PCH) & pi, 0, sizeof(PAGEINFO));
        pi.cb = sizeof(PAGEINFO);
        pi.hwndPage = NULLHANDLE;
        pi.usPageStyleFlags = BKA_MAJOR;
        pi.usPageInsertFlags = BKA_FIRST;
        pi.usSettingsFlags = SETTINGS_PAGE_NUMBERS;
        pi.pfnwp = AnimatedMousePointerPageProc;
        pi.resid = hmodResource;
        pi.dlgid = IsWARP3()
                        ? IDDLG_DLG_ANIMATEDWAITPOINTER_230
                        : IDDLG_DLG_ANIMATEDWAITPOINTER;
        pi.pszName = szTabName;
        pi.pCreateParams = somSelf;
        pi.idDefaultHelpPanel = 4; // IDPNL_USAGE_NBPAGE;
        pi.pszHelpLibraryName = (PSZ)cmnQueryHelpLibrary(); // (PSZ)cmnQueryHelpLibrary(); // V0.9.3 (2000-05-21) [umoeller]

        return _wpInsertSettingsPage(somSelf, hwndDlg, &pi);
    #else
        return (0);
    #endif
}

/*
 *@@ wpInitData:
 *      this WPObject instance method gets called when the
 *      object is being initialized (on wake-up or creation).
 *      We initialize our additional instance data here.
 *      Always call the parent method first.
 */

SOM_Scope void  SOMLINK xms_wpInitData(XWPMouse *somSelf)
{
    XWPMouseData *somThis = XWPMouseGetData(somSelf);
    XWPMouseMethodDebug("XWPMouse","xms_wpInitData");

    XWPMouse_parent_WPMouse_wpInitData(somSelf);

    _pszCurrentSettings = NULL;
    _hwndNotebookPage = NULLHANDLE;
    _pcnrrec = NULL;
}

/*
 *@@ wpUnInitData:
 *      this WPObject instance method is called when the object
 *      is destroyed as a SOM object, either because it's being
 *      made dormant or being deleted. All allocated resources
 *      should be freed here.
 *      The parent method must always be called last.
 */

SOM_Scope void  SOMLINK xms_wpUnInitData(XWPMouse *somSelf)
{
    XWPMouseData *somThis = XWPMouseGetData(somSelf);
    XWPMouseMethodDebug("XWPMouse","xms_wpUnInitData");

    if (_pszCurrentSettings)
        free(_pszCurrentSettings);

    XWPMouse_parent_WPMouse_wpUnInitData(somSelf);
}

/*
 *@@ wpSetup:
 *      this WPObject instance method is called to allow an
 *      object to set itself up according to setup strings.
 *      As opposed to wpSetupOnce, this gets called any time
 *      a setup string is invoked.
 *
 *@@changed V0.9.16 (2001-10-28) [pr]: prevent trap on null string
 */

SOM_Scope BOOL  SOMLINK xms_wpSetup(XWPMouse *somSelf, PSZ pszSetupString)
{
    BOOL fResult;

#ifdef __ANIMATED_MOUSE_POINTERS__
    ULONG ulAnimationInitDelay = 0;

    XWPMouseData *somThis = XWPMouseGetData(somSelf);
    XWPMouseMethodDebug("XWPMouse", "xms_wpSetup");

    // V0.9.16 (2001-10-28) [pr]: Prevent trap on null string
    if (pszSetupString)
    {
        // make a duplicate of the setup string, so that we may modify it
        // V0.9.16 (2001-10-28) [pr]: Prevent trap on null string
        PSZ pszSetupCopy = strdup(pszSetupString);

        // delay init until the object was initialized
        if (!IsSettingsInitialized())
        {
            // numerischen Wert f�r Animtion Init delay holen
            ulAnimationInitDelay = getAnimationInitDelay();
            setAnimationInitDelay(ulAnimationInitDelay);

            ScanSetupString(_hwndNotebookPage, _pcnrrec, pszSetupCopy, TRUE, TRUE);
        }
        else
            ScanSetupString(_hwndNotebookPage, _pcnrrec, pszSetupCopy, TRUE, FALSE);

        fResult = XWPMouse_parent_WPMouse_wpSetup(somSelf,
                                                  pszSetupCopy);
        free(pszSetupCopy);
    }
    else
#endif
        fResult = XWPMouse_parent_WPMouse_wpSetup(somSelf,
                                                  pszSetupString);

    return fResult;

    // return (XWPMouse_parent_WPMouse_wpSetup(somSelf, pszSetupString));
}

/*
 *@@ wpSaveState:
 *      this WPObject instance method saves an object's state
 *      persistently so that it can later be re-initialized
 *      with wpRestoreState. This gets called during wpClose,
 *      wpSaveImmediate or wpSaveDeferred processing.
 *      All persistent instance variables should be stored here.
 */

SOM_Scope BOOL  SOMLINK xms_wpSaveState(XWPMouse *somSelf)
{
    #ifdef __ANIMATED_MOUSE_POINTERS__
        APIRET rc = NO_ERROR;
        PSZ pszSettingsTmp = NULL;
        PSZ pszSettings = NULL;
        ULONG ulMaxLen = 0;

        XWPMouseData *somThis = XWPMouseGetData(somSelf);

        XWPMouseMethodDebug("XWPMouse","xms_wpSaveState");

        do
        {
            // Settings holen
            rc = QueryCurrentSettings(&pszSettingsTmp);
            if (rc != NO_ERROR)
                break;
            if (*pszSettingsTmp == 0)
                break;

            // SOM Speicher holen
            pszSettings = _wpAllocMem(somSelf, strlen(pszSettingsTmp) + 1, &rc);
            if (!pszSettings)
                break;

            // Settings kopieren
            strcpy(pszSettings, pszSettingsTmp);

            // alte Settings verwerfen
            if (_pszCurrentSettings != NULL)
                _wpFreeMem(somSelf, _pszCurrentSettings);
            _pszCurrentSettings = pszSettings;

            // jetzt speichern
            _wpSaveString(somSelf, G_pcszXWPMouse, 1, pszSettings);
            if (getAnimationInitDelay() != getDefaultAnimationInitDelay())
                _wpSaveLong(somSelf, G_pcszXWPMouse, 2, getAnimationInitDelay());
            else
                _wpSaveLong(somSelf, G_pcszXWPMouse, 2, -1);

        }
        while (FALSE);

        // cleanup
        if (pszSettingsTmp)
            free(pszSettingsTmp);
    #endif

    return (XWPMouse_parent_WPMouse_wpSaveState(somSelf));
}

/*
 *@@ wpRestoreState:
 *      this WPObject instance method gets called during object
 *      initialization (after wpInitData) to restore the data
 *      which was stored with wpSaveState.
 */

SOM_Scope BOOL  SOMLINK xms_wpRestoreState(XWPMouse *somSelf,
                                           ULONG ulReserved)
{
    #ifdef __ANIMATED_MOUSE_POINTERS__
        APIRET rc = NO_ERROR;
        PSZ pszSettings = NULL;
        ULONG ulMaxLen = 0;
        ULONG ulAnimationInitDelay = 0;

        XWPMouseData *somThis = XWPMouseGetData(somSelf);
        XWPMouseMethodDebug("XWPMouse", "xms_wpRestoreState");

        do
        {
            // numerischen Wert f�r Animtion Init delay holen
            if (    (!_wpRestoreLong(somSelf, G_pcszXWPMouse, 2, &ulAnimationInitDelay))
                 || (ulAnimationInitDelay == -1)
               )
                ulAnimationInitDelay = getDefaultAnimationInitDelay();

            setAnimationInitDelay(ulAnimationInitDelay);

            // ben�tigte L�nge abfragen
            if (!_wpRestoreString(somSelf, G_pcszXWPMouse, 1, NULL, &ulMaxLen))
                break;

            if (ulMaxLen == 0)
                break;

            // Speicher holen
            pszSettings = _wpAllocMem(somSelf, ulMaxLen, &rc);
            if (rc != NO_ERROR)
                break;

            // Settings holen
            _wpRestoreString(somSelf, G_pcszXWPMouse, 1, pszSettings, &ulMaxLen);
            // DEBUGMSG("SOM: restored settings" NEWLINE, 0);
            // DEBUGMSG("SOM: %s" NEWLINE, pszSettings);

            // alte Settings verwerfen
            if (_pszCurrentSettings != NULL)
                _wpFreeMem(somSelf, _pszCurrentSettings);
            _pszCurrentSettings = pszSettings;

        }
        while (FALSE);

        // ggfs. leeren Settings-String anlegen, damit ScanSetupString
        // auf jeden fall die Notebook Page notifiziert
        if (_pszCurrentSettings == NULL)
        {
            _pszCurrentSettings = _wpAllocMem(somSelf, 1, &rc);
            *_pszCurrentSettings = 0;
        }

        // Settings verwenden
        ScanSetupString(_hwndNotebookPage, _pcnrrec, _pszCurrentSettings, FALSE, TRUE);
    #endif

    return (XWPMouse_parent_WPMouse_wpRestoreState(somSelf, ulReserved));
}

/*
 *@@ wpFilterPopupMenu:
 *      this WPObject instance method allows the object to
 *      filter out unwanted menu items from the context menu.
 *      This gets called before wpModifyPopupMenu.
 *
 *      We remove the "Create another" menu item.
 *
 *@@added V0.9.2 (2000-02-26) [umoeller]
 */

SOM_Scope ULONG  SOMLINK xms_wpFilterPopupMenu(XWPMouse *somSelf,
                                               ULONG ulFlags,
                                               HWND hwndCnr,
                                               BOOL fMultiSelect)
{
    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_wpFilterPopupMenu");

    return (XWPMouse_parent_WPMouse_wpFilterPopupMenu(somSelf,
                                                      ulFlags,
                                                      hwndCnr,
                                                      fMultiSelect)
            & ~CTXT_NEW
           );
}

/*
 *@@ wpAddMouseCometPage:
 *      this WPMouse instance method inserts the "Comet"
 *      page into the mouse object's settings notebook.
 *      We override this to get an opportunity to insert our
 *      own pages behind that page by calling
 *      XWPMouse::xwpAddMouseMovementPage to configure the hook.
 */

SOM_Scope ULONG  SOMLINK xms_wpAddMouseCometPage(XWPMouse *somSelf,
                                                 HWND hwndNotebook)
{
    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_wpAddMouseCometPage");

    _xwpAddMouseMovementPage(somSelf, hwndNotebook);

    return (XWPMouse_parent_WPMouse_wpAddMouseCometPage(somSelf,
                                                        hwndNotebook));
}


/*
 *@@ wpAddMousePtrPage:
 *      this WPMouse instance method inserts the "Pointers"
 *      page in the "Mouse" object's settings notebook.
 *      We override this to insert the animated mouse pointers
 *      page instead.
 *
 *@@added V0.9.1 [umoeller]
 */

SOM_Scope ULONG  SOMLINK xms_wpAddMousePtrPage(XWPMouse *somSelf,
                                               HWND hwndNotebook)
{
    // PCGLOBALSETTINGS    pGlobalSettings = cmnQueryGlobalSettings();
    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_wpAddMousePtrPage");

    #ifdef __ANIMATED_MOUSE_POINTERS__
        if (pGlobalSettings->fAniMouse)
            return _xwpAddAnimatedMousePointerPage(somSelf, hwndNotebook);
        else
    #endif
        return (XWPMouse_parent_WPMouse_wpAddMousePtrPage(somSelf,
                                                          hwndNotebook));
}

/*
 *@@ xms_wpAddMouseMappingsPage:
 *      this WPMouse instance method inserts the "Mappings"
 *      page in the "Mouse" object's settings notebook.
 *      We override this to insert a second "Mappings" page
 *      for configuring more XWorkplace hook features.
 *
 *@@added V0.9.1 [umoeller]
 */

SOM_Scope ULONG  SOMLINK xms_wpAddMouseMappingsPage(XWPMouse *somSelf,
                                                    HWND hwndNotebook)
{
    /* XWPMouseData *somThis = XWPMouseGetData(somSelf); */
    XWPMouseMethodDebug("XWPMouse","xms_wpAddMouseMappingsPage");

    _xwpAddMouseMappings2Page(somSelf, hwndNotebook);

    return (XWPMouse_parent_WPMouse_wpAddMouseMappingsPage(somSelf,
                                                           hwndNotebook));
}


/*
 *@@ wpclsInitData:
 *      this WPObject class method gets called when a class
 *      is loaded by the WPS (probably from within a
 *      somFindClass call) and allows the class to initialize
 *      itself.
 */

SOM_Scope void  SOMLINK xmsM_wpclsInitData(M_XWPMouse *somSelf)
{
    /* M_XWPMouseData *somThis = M_XWPMouseGetData(somSelf); */
    M_XWPMouseMethodDebug("M_XWPMouse","xmsM_wpclsInitData");

    M_XWPMouse_parent_M_WPMouse_wpclsInitData(somSelf);

    krnClassInitialized(G_pcszXWPMouse);
}

/*
 *@@ wpclsUnInitData:
 *
 *@@added V0.9.3 (2000-05-21) [umoeller]
 */

SOM_Scope void  SOMLINK xmsM_wpclsUnInitData(M_XWPMouse *somSelf)
{
    /* M_XWPMouseData *somThis = M_XWPMouseGetData(somSelf); */
    M_XWPMouseMethodDebug("M_XWPMouse","xmsM_wpclsUnInitData");

    M_XWPMouse_parent_M_WPMouse_wpclsUnInitData(somSelf);
}

/*
 *@@ wpclsQuerySettingsPageSize:
 *      this WPObject class method should return the
 *      size of the largest settings page in dialog
 *      units; if a settings notebook is initially
 *      opened, i.e. no window pos has been stored
 *      yet, the WPS will use this size, to avoid
 *      truncated settings pages.
 */

SOM_Scope BOOL  SOMLINK xmsM_wpclsQuerySettingsPageSize(M_XWPMouse *somSelf,
                                                        PSIZEL pSizl)
{
    BOOL brc;
    /* M_XWPMouseData *somThis = M_XWPMouseGetData(somSelf); */
    M_XWPMouseMethodDebug("M_XWPMouse","xmsM_wpclsQuerySettingsPageSize");

    brc = M_XWPMouse_parent_M_WPMouse_wpclsQuerySettingsPageSize(somSelf,
                                                                 pSizl);

    if (brc)
    {
        LONG lCompCY = 164 - WARP4_NOTEBOOK_OFFSET;

                            // this is the height of the "Movement" page,
                            // which is pretty large
        /* if (doshIsWarp4())
            // on Warp 4, reduce again, because we're moving
            // the notebook buttons to the bottom
            lCompCY -= WARP4_NOTEBOOK_OFFSET; */

        if (pSizl->cy < lCompCY)
            pSizl->cy = lCompCY;
    }

    return (brc);
}

