// C Runtime
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// OS/2 Toolkit
#define INCL_ERRORS
#define INCL_PM
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSDEVIOCTL
#define INCL_DOSMISC
#include <os2.h>

// generic headers
#include "setup.h"              // code generation and debugging options

#include "pointers\mptrcnr.h"
#include "pointers\mptrprop.h"
#include "pointers\mptrutil.h"
#include "pointers\mptrptr.h"
#include "pointers\mptrppl.h"
#include "pointers\mptrset.h"
#include "pointers\mptranim.h"
#include "pointers\wmuser.h"
#include "pointers\r_wpamptr.h"
#include "pointers\macros.h"
#include "pointers\debug.h"

#include "pointers\r_amptreng.h"

// globale Variablen

#ifdef DEBUG
HAB habMain = NULLHANDLE;

#endif

// resource file types ###
CHAR szFileTypeAll[MAX_RES_STRLEN];
CHAR szFileTypeDefault[MAX_RES_STRLEN];
CHAR szFileTypePointer[MAX_RES_STRLEN];
CHAR szFileTypePointerSet[MAX_RES_STRLEN];
CHAR szFileTypeCursor[MAX_RES_STRLEN];
CHAR szFileTypeCursorSet[MAX_RES_STRLEN];
CHAR szFileTypeAniMouse[MAX_RES_STRLEN];
CHAR szFileTypeAniMouseSet[MAX_RES_STRLEN];
CHAR szFileTypeWinAnimation[MAX_RES_STRLEN];
CHAR szFileTypeWinAnimationSet[MAX_RES_STRLEN];

static PSZ apszFileType[] =
{szFileTypeDefault,
 szFileTypePointer,
 szFileTypePointerSet,
 szFileTypeCursor,
 szFileTypeCursorSet,
 szFileTypeAniMouse,
 szFileTypeAniMouseSet,
 szFileTypeWinAnimation,
 szFileTypeWinAnimationSet};

CHAR szPointerArrow[MAX_RES_STRLEN];
CHAR szPointerText[MAX_RES_STRLEN];
CHAR szPointerWait[MAX_RES_STRLEN];
CHAR szPointerSizenwse[MAX_RES_STRLEN];
CHAR szPointerSizewe[MAX_RES_STRLEN];
CHAR szPointerMove[MAX_RES_STRLEN];
CHAR szPointerSizenesw[MAX_RES_STRLEN];
CHAR szPointerSizens[MAX_RES_STRLEN];
CHAR szPointerIllegal[MAX_RES_STRLEN];

PSZ apszIconTitle[] =
{szPointerArrow,
 szPointerText,
 szPointerWait,
 szPointerSizenwse,
 szPointerSizewe,
 szPointerMove,
 szPointerSizenesw,
 szPointerSizens,
 szPointerIllegal};

CHAR szTitleIcon[MAX_RES_STRLEN];
CHAR szTitleName[MAX_RES_STRLEN];
CHAR szTitleStatus[MAX_RES_STRLEN];
CHAR szTitleAnimationType[MAX_RES_STRLEN];
CHAR szTitlePointer[MAX_RES_STRLEN];
CHAR szTitleAnimationName[MAX_RES_STRLEN];
CHAR szTitleFrameRate[MAX_RES_STRLEN];
CHAR szTitleInfoName[MAX_RES_STRLEN];
CHAR szTitleInfoArtist[MAX_RES_STRLEN];
CHAR szStatusOn[MAX_RES_STRLEN];
CHAR szStatusOff[MAX_RES_STRLEN];


// ----------------------------------------------------------------------
// private prototypes
// ----------------------------------------------------------------------


BOOL InitPtrSetContainer(HWND hwnd, PVOID * ppvCnrData);
BOOL RefreshCnrItem(HWND hwnd, PRECORDCORE prec, PRECORDCORE pcnrrec, BOOL fResetArrowPtr);
BOOL SetContainerView(HWND hwnd, ULONG ulViewStyle);
BOOL ToggleAnimate(HWND hwnd, ULONG ulPtrIndex, PRECORDCORE prec, PRECORDCORE pcnrrrec, BOOL fChangeAll, BOOL fRefresh, PBOOL fEnable);

BOOL LoadAnimationResource(HWND hwnd, PSZ pszName, PRECORDCORE prec, PRECORDCORE pcnrrrec);

BOOL BeginEditPointer(HWND hwnd, PRECORDCORE prec, PRECORDCORE pcnrrec);
BOOL EndEditPointer(HWND hwnd, HAPP happ, ULONG ulReturncode, PRECORDCORE pcnrrec);

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : InitCnrResources                                           �
 *� Kommentar : l꼋t Strings aus Resource Lib - muss vor                   �
 *�             InitPtrSetContainer aufgerufen werden !                    �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 12.06.1996                                                 �
 *� 럑derung  : 12.06.1996                                                 �
 *� aufgerufen: InitializePointerlist                                      �
 *� ruft auf  : -                                                          �
 *� Eingabe   : HMODULE - Handle der Resource lib                          �
 *� Aufgaben  : - Strings laden                                            �
 *� R갷kgabe  : -                                                          �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

VOID _Export InitStringResources
 (
     HAB hab,
     HMODULE hmodResource
)
{

// Strings laden
    LOADSTRING(IDSTR_FILETYPE_ALL, szFileTypeAll);
    LOADSTRING(IDSTR_FILETYPE_DEFAULT, szFileTypeDefault);
    LOADSTRING(IDSTR_FILETYPE_POINTER, szFileTypePointer);
    LOADSTRING(IDSTR_FILETYPE_POINTERSET, szFileTypePointerSet);
    LOADSTRING(IDSTR_FILETYPE_CURSOR, szFileTypeCursor);
    LOADSTRING(IDSTR_FILETYPE_CURSORSET, szFileTypeCursorSet);
    LOADSTRING(IDSTR_FILETYPE_ANIMOUSE, szFileTypeAniMouse);
    LOADSTRING(IDSTR_FILETYPE_ANIMOUSESET, szFileTypeAniMouseSet);
    LOADSTRING(IDSTR_FILETYPE_WINANIMATION, szFileTypeWinAnimation);
    LOADSTRING(IDSTR_FILETYPE_WINANIMATIONSET, szFileTypeWinAnimationSet);
    LOADSTRING(IDSTR_POINTER_ARROW, szPointerArrow);
    LOADSTRING(IDSTR_POINTER_TEXT, szPointerText);
    LOADSTRING(IDSTR_POINTER_WAIT, szPointerWait);
    LOADSTRING(IDSTR_POINTER_SIZENWSE, szPointerSizenwse);
    LOADSTRING(IDSTR_POINTER_SIZEWE, szPointerSizewe);
    LOADSTRING(IDSTR_POINTER_MOVE, szPointerMove);
    LOADSTRING(IDSTR_POINTER_SIZENESW, szPointerSizenesw);
    LOADSTRING(IDSTR_POINTER_SIZENS, szPointerSizens);
    LOADSTRING(IDSTR_POINTER_ILLEGAL, szPointerIllegal);
    LOADSTRING(IDSTR_TITLE_ICON, szTitleIcon);
    LOADSTRING(IDSTR_TITLE_NAME, szTitleName);
    LOADSTRING(IDSTR_TITLE_STATUS, szTitleStatus);
    LOADSTRING(IDSTR_TITLE_ANIMATIONTYPE, szTitleAnimationType);
    LOADSTRING(IDSTR_TITLE_POINTER, szTitlePointer);
    LOADSTRING(IDSTR_TITLE_ANIMATIONNAME, szTitleAnimationName);
    LOADSTRING(IDSTR_TITLE_FRAMERATE, szTitleFrameRate);
    LOADSTRING(IDSTR_TITLE_INFONAME, szTitleInfoName);
    LOADSTRING(IDSTR_TITLE_INFOARTIST, szTitleInfoArtist);
    LOADSTRING(IDSTR_STATUS_ON, szStatusOn);
    LOADSTRING(IDSTR_STATUS_OFF, szStatusOff);

    return;
}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : InitPtrSetContainer                                        �
 *� Kommentar : ermittelt Basisname                                        �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 12.06.1996                                                 �
 *� 럑derung  : 12.06.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : HWND   - frame window handle                               �
 *�             PSZ    - Speicherbereich                                   �
 *�             ULONG  - L꼗ge des Speicherbereichs                        �
 *� Aufgaben  : - Basisname ermitteln                                      �
 *� R갷kgabe  : BOOL - Flag, ob erfolgreich                                �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

#define SPLITBAR_OFFSET   150
#define CNR_COLUMNS_COUNT 9

BOOL InitPtrSetContainer
 (
     HWND hwnd,
     PVOID * ppvCnrData
)
{
    BOOL fSuccess = FALSE;
    APIRET rc;
    ULONG i;
    HAB hab = WinQueryAnchorBlock(hwnd);
    HWND hwndCnrPointerSet = WinWindowFromID(hwnd, IDDLG_CN_POINTERSET);
    PRECORDCORE prec = NULL;
    PMYRECORDCORE pmyrec;
    RECORDINSERT recinsert;
    BOOL fInitError = FALSE;

    PFIELDINFO pfi, pfiFirst, pfiLastLeftCol;
    FIELDINFOINSERT fii;
    CNRINFO cnri;

    BOOL fActivateDemo;

    PPOINTERLIST ppl;

    do
    {
        // Parameter pr갽en
        if (ppvCnrData == NULL)
            break;
        if (hwndCnrPointerSet == NULLHANDLE)
            break;

        // Speicher holen
        prec = (PRECORDCORE) WinSendMsg(hwndCnrPointerSet,
                                        CM_ALLOCRECORD,
                                        MPFROMLONG(RECORD_EXTRAMEMORY),
                                        MPFROMLONG(NUM_OF_SYSCURSORS));

        // aktuelle Pointer anzeigen
        for (i = 0, pmyrec = (PMYRECORDCORE) prec;
             i < NUM_OF_SYSCURSORS;
             i++, pmyrec = (PMYRECORDCORE)pmyrec->rec.preccNextRecord)
        {
            ppl = QueryPointerlist(i);

            pmyrec->rec.flRecordAttr = CRA_RECORDREADONLY;
            pmyrec->rec.pszIcon = apszIconTitle[i];
            pmyrec->rec.hptrIcon = WinQuerySysPointer(HWND_DESKTOP, QueryPointerSysId(i), FALSE);
            pmyrec->rec.hptrMiniIcon = pmyrec->rec.hptrIcon;
            pmyrec->pszAnimationName = "";
            pmyrec->pszAnimationType = "";
            pmyrec->pszFrameRate = "";
            pmyrec->pszInfoName = "";
            pmyrec->pszInfoArtist = "";
            pmyrec->ulPtrCount = 0;
            pmyrec->pszAnimate = szStatusOff;
            pmyrec->ulPointerIndex = i;
            pmyrec->ppl = QueryPointerlist(i);
        }

        // Record in Container einstellen
        recinsert.cb = sizeof(RECORDINSERT);
        recinsert.pRecordOrder = (PRECORDCORE)CMA_END;
        recinsert.pRecordParent = NULL;
        recinsert.fInvalidateRecord = TRUE;
        recinsert.zOrder = CMA_TOP;
        recinsert.cRecordsInsert = NUM_OF_SYSCURSORS;
        if (!WinSendMsg(hwndCnrPointerSet,
                        CM_INSERTRECORD,
                        MPFROMP(prec),
                        MPFROMP(&recinsert)))
        {
            rc = ERRORIDERROR(WinGetLastError(WinQueryAnchorBlock(hwnd)));
            break;
        }

        // jetzt Detailsview konfigurieren
        // Speicher holen
        if ((pfi = WinSendMsg(hwndCnrPointerSet,
                              CM_ALLOCDETAILFIELDINFO,
                              MPFROMLONG(CNR_COLUMNS_COUNT),
                              0L)) == NULL)
            break;


        pfiFirst = pfi;         // Zeiger auf erste Struktur festhalten
        // Details f걊 Symbol

        pfi->flData = CFA_BITMAPORICON | CFA_HORZSEPARATOR |
            CFA_CENTER | CFA_SEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleIcon;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, rec.hptrMiniIcon);
        pfi->cxWidth = 0;

        // Titel
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleName;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, rec.pszIcon);
        pfi->cxWidth = 0;

        pfiLastLeftCol = pfi;   // letzte linke Spalte festhalten

        // Status
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR | CFA_SEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleStatus;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, pszAnimate);
        pfi->cxWidth = 0;

        // Animationstyp
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR | CFA_SEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleAnimationType;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, pszAnimationType);
        pfi->cxWidth = 0;

        // Anzahl Pointer
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_ULONG | CFA_RIGHT | CFA_HORZSEPARATOR | CFA_SEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitlePointer;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, ulPtrCount);
        pfi->cxWidth = 0;

        // Animationsname
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR | CFA_SEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleAnimationName;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, pszAnimationName);
        pfi->cxWidth = 0;

        // Timeouts
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR | CFA_SEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleFrameRate;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, pszFrameRate);
        pfi->cxWidth = 0;

        // Info animation name
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR | CFA_SEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleInfoName;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, pszInfoName);
        pfi->cxWidth = 0;

        // Info animation artist
        pfi = pfi->pNextFieldInfo;
        pfi->flData = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR;
        pfi->flTitle = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = szTitleInfoArtist;
        pfi->offStruct = FIELDOFFSET(MYRECORDCORE, pszInfoArtist);

        // Struktur an Container senden
        memset(&fii, 0, sizeof(FIELDINFOINSERT));
        fii.cb = sizeof(FIELDINFOINSERT);
        fii.pFieldInfoOrder = (PFIELDINFO) CMA_END;
        fii.cFieldInfoInsert = (SHORT) CNR_COLUMNS_COUNT;
        fii.fInvalidateFieldInfo = TRUE;

        if (WinSendMsg(hwndCnrPointerSet,
                       CM_INSERTDETAILFIELDINFO,
                       MPFROMP(pfiFirst),
                       MPFROMP(&fii)) != (MRESULT) CNR_COLUMNS_COUNT)
            break;

        // Splitbar setzen
        memset(&cnri, 0, sizeof(CNRINFO));
        cnri.cb = sizeof(CNRINFO);
        cnri.pFieldInfoLast = pfiLastLeftCol;
        cnri.xVertSplitbar = SPLITBAR_OFFSET;

        if (!WinSendMsg(hwndCnrPointerSet,
                        CM_SETCNRINFO,
                        MPFROMP(&cnri),
                        MPFROMLONG(CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR)))
            break;

        // f걊 Setzen der Controls sorgen
        WinPostMsg(hwnd, WM_USER_ENABLECONTROLS, 0, 0);

        // ggfs. Demo anstellen
        fActivateDemo = QueryDemo();
        if (fActivateDemo)
            ToggleDemo(hwnd, prec, FALSE, &fActivateDemo);

        // aktuellen Stand aus Pointerlisten einstellen
        RefreshCnrItem(hwnd, NULL, prec, TRUE);

        // fertig
        fSuccess = TRUE;

    }
    while (FALSE);

// Struktur zur갷kgeben
    if (fSuccess)
        *ppvCnrData = prec;
    else if (prec)
        WinSendMsg(hwndCnrPointerSet,
                   CM_ALLOCRECORD,
                   MPFROMP(prec),
                   MPFROMLONG(NUM_OF_SYSCURSORS));

    return fSuccess;

}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : RefreshCnrItem                                             �
 *� Kommentar : f갿rt einen Refresh f걊 ein Item durch                     �
 *�             Setzt ausserdem alle Mauszeiger auf den statischen         �
 *�             oder auf den Default-Zeiger zur갷k.                        �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 24.07.1996                                                 �
 *� 럑derung  : 24.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : HWND        - frame window                                 �
 *�             PRECORDCORE - Zeiger auf Containeritem oder NULL           �
 *�             PRECORDCORE - Zeiger auf Gesamt-Record                     �
 *�             BOOL        - Flag, ob Arrow-Pointer refreshed werden soll �
 *� Aufgaben  : - Refresh durchf갿ren                                      �
 *� R갷kgabe  : BOOL - Flag, ob erfolgreich                                �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL RefreshCnrItem
 (
     HWND hwnd,
     PRECORDCORE prec,
     PRECORDCORE pcnrrec,
     BOOL fResetArrowPtr
)
{
    BOOL fSuccess = FALSE;
    APIRET rc;
    HAB hab = WinQueryAnchorBlock(hwnd);

    HWND hwndCnrPointerSet = WinWindowFromID(hwnd, IDDLG_CN_POINTERSET);

    PMYRECORDCORE pmyrec = (PMYRECORDCORE) prec;
    ULONG ulPointerIndex = (pmyrec) ? pmyrec->ulPointerIndex : 0;
    ULONG ulViewStyle;
    PICONINFO piconinfo;

    ULONG i;
    PPOINTERLIST ppl;
    BOOL fLoadError = FALSE;
    HPOINTER hptrItem;

    do
    {

        // ersten Pointer der jeweiligen Pointerliste setzen
        for (i = 0; i < NUM_OF_SYSCURSORS; i++)
        {

            ppl = QueryPointerlist(i);

            if (ppl->ulPtrCount > 0)
            {

                // Pointer auf n꼊hsten Wert setzen
                // SYS-Pointer setzen
                if (ppl->hptrStatic == 0)
                {
                    // Default setzen
                    fSuccess = WinSetSysPointerData(HWND_DESKTOP, QueryPointerSysId(i), NULL);
                    rc = (fSuccess) ? NO_ERROR : ERRORIDERROR(hab);

                    // Arrow Ptr aktualisieren
                    if ((i == 0) && (fResetArrowPtr))
                    {
                        HPOINTER hptrOrg = WinQuerySysPointer(HWND_DESKTOP, QueryPointerSysId(i), TRUE);

                        if (!WinSetPointer(HWND_DESKTOP, hptrOrg))
                            fLoadError = TRUE;
                        WinDestroyPointer(hptrOrg);
                    }
                }
                else
                {
                    // neuen Pointer setzen
                    if (ppl->fAnimate)
                        piconinfo = &ppl->iconinfo[0];
                    else
                        piconinfo = &ppl->iconinfoStatic;
                    fSuccess = WinSetSysPointerData(HWND_DESKTOP,
                                                    QueryPointerSysId(i),
                                                    piconinfo);

                    rc = (fSuccess) ? NO_ERROR : ERRORIDERROR(hab);

                    if (!fSuccess)
                        fLoadError = TRUE;

                    // Arrow Ptr aktualisieren
                    if ((i == 0) && (fResetArrowPtr))
                    {
                        if (!WinSetPointer(HWND_DESKTOP,
                                           ppl->hptrStatic))
                            fLoadError = TRUE;
                    }
                }
            }
        }

        // Der Rest wird nur gebraucht, wenn der Frame und der Container da ist
        if ((hwnd == NULLHANDLE) || (pcnrrec == NULL))
        {
            fSuccess = TRUE;
            break;
        }

        // ersten Pointer der Liste setzen
        // ### noch zu optimieren: ggfs. nur betroffenes Objekt updaten
        for (i = 0, pmyrec = (PMYRECORDCORE) pcnrrec;
             i < NUM_OF_SYSCURSORS;
             i++, pmyrec = (PMYRECORDCORE)pmyrec->rec.preccNextRecord)
        {
            ULONG ulMaxLen;
            PSZ pszList;
            ULONG s;

            ppl = QueryPointerlist(i);

            // alte Resourcen freigeben
            if (strlen(pmyrec->pszFrameRate) > 0)
                free(pmyrec->pszFrameRate);

            // Container Item neu einstellen
            if (ppl->ulPtrCount > 0)
            {

                if (ppl->ulPtrCount == 1)
                    pszList = "";
                else
                {
                    // framerates ermitteln
                    ulMaxLen = (ppl->ulPtrCount * 7) + 1;   // "(nnnn) "

                    pszList = malloc(ulMaxLen);

                    if (pszList == NULL)
                        pszList = "???";
                    else
                    {
                        BOOL fOverrideTimeout = getOverrideTimeout();

                        memset(pszList, 0, ulMaxLen);

                        for (s = 0; s < ppl->ulPtrCount; s++)
                        {
                            if ((fOverrideTimeout) || (ppl->aulTimer[s] == 0) || (ppl->aulTimer[s] == 0))
                                sprintf(ENDSTRING(pszList), "(%u) ", getDefaultTimeout());
                            else
                                sprintf(ENDSTRING(pszList), "%u ", ppl->aulTimer[s]);

                        }       // end for (s < ppl->ulPtrCount)

                    }
                }

                // Werte f걊 Container-Item neu einstellen
                pmyrec->pszAnimationName = ppl->szAnimationName;
                pmyrec->pszAnimationType = apszFileType[ppl->ulResourceType];
                pmyrec->pszFrameRate = pszList;
                pmyrec->pszInfoName = ppl->pszInfoName;
                pmyrec->pszInfoArtist = ppl->pszInfoArtist;
                pmyrec->ulPtrCount = ppl->ulPtrCount;
                pmyrec->pszAnimate = (ppl->fAnimate) ? szStatusOn : szStatusOff;
            }                   // end if (ppl->ulPtrCount > 0)

            else
            {
                // Werte f걊 Container-Item neu einstellen
                pmyrec->pszAnimationName = "";
                pmyrec->pszAnimationType = "";
                pmyrec->pszFrameRate = "";
                pmyrec->ulPtrCount = 0;
                pmyrec->pszInfoName = "";
                pmyrec->pszInfoArtist = "";
                pmyrec->pszAnimate = szStatusOff;
            }
        }

        // ersten Pointer der Liste setzen
        // optimieren: ggfs. nur betroffenes Objekt updaten
        for (i = 0, pmyrec = (PMYRECORDCORE) pcnrrec;
             i < NUM_OF_SYSCURSORS;
             i++, pmyrec = (PMYRECORDCORE)pmyrec->rec.preccNextRecord)
        {
            // Pointer f걊 Item ermitteln
            ppl = QueryPointerlist(i);
            if (ppl->hptr[ppl->ulPtrIndexCnr] == 0)
                hptrItem = WinQuerySysPointer(HWND_DESKTOP, QueryPointerSysId(i), FALSE);
            else
            {
                if (ppl->fAnimate)
                    hptrItem = ppl->hptr[ppl->ulPtrIndexCnr];
                else
                    hptrItem = ppl->hptrStatic;
            }

            pmyrec->rec.hptrIcon = hptrItem;
            pmyrec->rec.hptrMiniIcon = hptrItem;
        }

        // Container refreshen
        // - Mini-Icons werden im Details-View nicht updated, daher
        //   wird hierf걊 kurz der View gewechselt
        //   dabei wird Window Update ausgeschaltet, damit es auf langsamen
        //   Maschinen nicht zu sehr flackert

        // - ggfs, auf iconview schalten
        QueryContainerView(hwnd, &ulViewStyle);
        if (ulViewStyle & CV_DETAIL)
        {
            WinEnableWindowUpdate(hwndCnrPointerSet, FALSE);
            SetContainerView(hwnd, CV_ICON);
        }

        // - Update der Werte
        // pcnrrec = (fResetArrowPtr) ? NULL : pcnrrec;
        // i       = (fResetArrowPtr) ? 0    : 1;

        pcnrrec = NULL;
        i = 0;

        WinSendMsg(hwndCnrPointerSet, CM_INVALIDATERECORD,
                   MPFROMP(pcnrrec),
                   MPFROM2SHORT(i, CMA_NOREPOSITION));

#ifdef DEBUG
        rc = ERRORIDERROR(WinGetLastError(WinQueryAnchorBlock(hwnd)));
#endif


        // - Update f걊 die Spaltenbreiten
        if (ulViewStyle & CV_DETAIL)
            WinSendMsg(hwndCnrPointerSet, CM_INVALIDATEDETAILFIELDINFO, 0L, 0L);

        // - ggfs, auf detailview zur갷kschalten
        if (ulViewStyle & CV_DETAIL)
        {
            SetContainerView(hwnd, CV_DETAIL);
            WinEnableWindowUpdate(hwndCnrPointerSet, TRUE);
        }

        // alles ok
        fSuccess = TRUE;

        // f걊 Setzen der Controls sorgen
        WinPostMsg(hwnd, WM_USER_ENABLECONTROLS, 0L, 0L);

    }
    while (FALSE);

    return fSuccess;

}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : SetContainerView                                           �
 *� Kommentar :                                                            �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 24.07.1996                                                 �
 *� 럑derung  : 24.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : HWND   - frame window handle                               �
 *�             ULONG  - neuer View des Containers                         �
 *� Aufgaben  : - View 꼗dern                                              �
 *� R갷kgabe  : BOOL - Flag, ob erfolgreich                                �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL SetContainerView
 (
     HWND hwnd,
     ULONG ulViewStyle
)

{
    BOOL fSuccess = FALSE;
    CNRINFO cnri;
    HWND hwndCnrPointerSet = WinWindowFromID(hwnd, IDDLG_CN_POINTERSET);

    do
    {
        // Parameter pr갽en
        if ((ulViewStyle != CV_ICON) &&
            (ulViewStyle != CV_DETAIL) &&
            (ulViewStyle != 0))
            break;

        // Struktur holen
        if ((ULONG) WinSendMsg(hwndCnrPointerSet,
                               CM_QUERYCNRINFO,
                               MPFROMP(&cnri),
                               MPFROMLONG(sizeof(CNRINFO))) != sizeof(CNRINFO))
            break;

        if (ulViewStyle != NULL)
            cnri.flWindowAttr = CA_DRAWICON | CA_DETAILSVIEWTITLES | ulViewStyle;

        // zus꼝zliche Einstellungen
        switch (ulViewStyle)
        {
            case CV_ICON:
                cnri.slBitmapOrIcon.cx = 0;
                cnri.slBitmapOrIcon.cy = 0;
                break;

            case CV_DETAIL:
                cnri.slBitmapOrIcon.cx = WinQuerySysValue(HWND_DESKTOP, SV_CXICON) / 2;
                cnri.slBitmapOrIcon.cy = WinQuerySysValue(HWND_DESKTOP, SV_CYICON) / 2;
                break;
        }


        if (!WinSendMsg(hwndCnrPointerSet,
                        CM_SETCNRINFO,
                        MPFROMP(&cnri),
                        MPFROMLONG(CMA_FLWINDOWATTR | CMA_SLBITMAPORICON)))
            break;

        // alles ok
        fSuccess = TRUE;
    }
    while (FALSE);

    return fSuccess;

}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : QueryContainerView                                         �
 *� Kommentar :                                                            �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 24.07.1996                                                 �
 *� 럑derung  : 24.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : HWND   - frame window handle                               �
 *�             PULONG - Variable f걊 aktuellen View                       �
 *� Aufgaben  : - View holen                                               �
 *� R갷kgabe  : BOOL - Flag, ob erfolgreich                                �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL QueryContainerView
 (
     HWND hwnd,
     PULONG pulViewStyle
)
{
    BOOL fSuccess = FALSE;
    CNRINFO cnri;
    HWND hwndCnrPointerSet = WinWindowFromID(hwnd, IDDLG_CN_POINTERSET);

    do
    {
        // Parameter pr갽en
        if (pulViewStyle == NULL)
            break;

        // Struktur holen
        if ((ULONG) WinSendMsg(hwndCnrPointerSet,
                               CM_QUERYCNRINFO,
                               MPFROMP(&cnri),
                               MPFROMLONG(sizeof(CNRINFO))) != sizeof(CNRINFO))
            break;

        *pulViewStyle = cnri.flWindowAttr;

        // alles ok
        fSuccess = TRUE;
    }
    while (FALSE);

    return fSuccess;

}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : QueryItemSet                                               �
 *� Kommentar : fragt ab, ob eine Animation geladen wurde                  �
 *�             Wenn eine Animation geladen ist, wird TRUE zur갷kgegeben   �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 24.07.1996                                                 �
 *� 럑derung  : 24.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : PRECORDCORE - Zeiger auf Containeritem oder NULL           �
 *� Aufgaben  : - Set Abfrage                                              �
 *� R갷kgabe  : BOOL - Flag, ob ein Set geladen wurde                      �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL QueryItemSet
 (
     PRECORDCORE prec
)
{
    BOOL fSuccess = FALSE;
    APIRET rc;
    ULONG i;
    BOOL fIsSet = FALSE;

    PMYRECORDCORE pmyrec = (PMYRECORDCORE) prec;
    PPOINTERLIST ppl;
    ULONG ulPointerIndex = (pmyrec) ? pmyrec->ulPointerIndex : 0;
    BOOL fLoadSet = (prec == NULL);

    ULONG ulFirstPtr, ulLastPtr;

    do
    {
        // range f걊 Verarbeitung festlegen
        ulFirstPtr = ulPointerIndex;

        if (fLoadSet)
            ulLastPtr = NUM_OF_SYSCURSORS;
        else
            ulLastPtr = ulPointerIndex + 1;

        // entsprechende Pointerlisten 갶erpr갽en
        for (i = ulFirstPtr, ppl = QueryPointerlist(ulPointerIndex);
             i < ulLastPtr;
             i++, ppl++)
        {
            // ist es ein Pointerset ?
            if (ppl->ulPtrCount > 1)
                fIsSet = TRUE;
        }

    }
    while (FALSE);

    return fIsSet;
}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : QueryItemLoaded                                            �
 *� Kommentar : fragt ab, ob mindestens ein Zeigerbild geladen wurde       �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 24.07.1996                                                 �
 *� 럑derung  : 24.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : PRECORDCORE - Zeiger auf Containeritem                     �
 *� Aufgaben  : - Pointerabfrage                                           �
 *� R갷kgabe  : BOOL - Flag, ob mindestens ein Zeigerbild geladen wurde    �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL QueryItemLoaded
 (
     PRECORDCORE prec
)
{
    BOOL fSuccess = FALSE;
    APIRET rc;
    ULONG i;
    BOOL fIsSet = FALSE;

    PMYRECORDCORE pmyrec = (PMYRECORDCORE) prec;
    PPOINTERLIST ppl;
    ULONG ulPointerIndex = (pmyrec) ? pmyrec->ulPointerIndex : 0;
    BOOL fItemLoaded = FALSE;
    BOOL fLoadSet = (prec == NULL);

    ULONG ulFirstPtr, ulLastPtr;

    do
    {
        // range f걊 Verarbeitung festlegen
        ulFirstPtr = ulPointerIndex;

        if (fLoadSet)
            ulLastPtr = NUM_OF_SYSCURSORS;
        else
            ulLastPtr = ulPointerIndex + 1;

        // entsprechende Pointerlisten 갶erpr갽en
        for (i = ulFirstPtr, ppl = QueryPointerlist(ulPointerIndex);
             i < ulLastPtr;
             i++, ppl++)
        {
            // ist es ein Pointerset ?
            if (ppl->hptrStatic != NULL)
                fItemLoaded = TRUE;
        }

    }
    while (FALSE);

    return fItemLoaded;
}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : QueryItemAnimate                                           �
 *� Kommentar : fragt das Animation Flag f걊 einen oder alle Ptr ab        �
 *�             Wenn eine Animation aktiv ist, wird TRUE zur갷kgegeben     �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 24.07.1996                                                 �
 *� 럑derung  : 24.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : PRECORDCORE - Zeiger auf Containeritem oder NULL           �
 *� Aufgaben  : - Animate abfragen                                         �
 *� R갷kgabe  : BOOL - Flag, ob eine Animation aktiv ist                   �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL QueryItemAnimate
 (
     PRECORDCORE prec
)
{
    PMYRECORDCORE pmyrec = (PMYRECORDCORE) prec;
    ULONG ulPointerIndex = (pmyrec) ? pmyrec->ulPointerIndex : 0;
    BOOL fQueryAll = (prec == NULL);

    return QueryAnimate(ulPointerIndex, fQueryAll);
}

/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : ToggleAnimate                                              �
 *� Kommentar : setzt das Animation Flag f걊 einen oder alle Ptr um        �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 24.07.1996                                                 �
 *� 럑derung  : 24.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : HWND        - frame window                                 �
 *�             ULONG       - PointerIndex, falls Item = NULL              �
 *�             PRECORDCORE - Zeiger auf Containeritem oder NULL           �
 *�             PRECORDCORE - Zeiger auf Gesamt-Record                     �
 *�             BOOL        - Refreshflag                                  �
 *�             PBOOL       - Zeiger auf ForceFlag                         �
 *� Aufgaben  : - Animate umstellen                                        �
 *� R갷kgabe  : BOOL - Flag, ob erfolgreich                                �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL ToggleAnimate
 (
     HWND hwnd,
     ULONG ulPtrIndex,
     PRECORDCORE prec,
     PRECORDCORE pcnrrec,
     BOOL fChangeAll,
     BOOL fRefresh,
     PBOOL pfEnable
)

{
    BOOL fSuccess = FALSE;
    APIRET rc;
    ULONG i;
    BOOL fAnimate;

    PMYRECORDCORE pmyrec = (PMYRECORDCORE) prec;
    PPOINTERLIST ppl, pplndx;
    ULONG ulPointerIndex = (pmyrec) ? pmyrec->ulPointerIndex : ulPtrIndex;

    ULONG ulFirstPtr, ulLastPtr;

    do
    {

        // ggfs. Flag overrulen
        if (pfEnable)
            fAnimate = !(*pfEnable);    // wird sp꼝er nochmal negiert !

        else
            fAnimate = QueryItemAnimate(prec);

        // range f걊 Verarbeitung festlegen
        ulFirstPtr = ulPointerIndex;

        if (fChangeAll)
        {
            ulFirstPtr = 0;
            ulLastPtr = NUM_OF_SYSCURSORS;
        }
        else
            ulLastPtr = ulPointerIndex + 1;

        ppl = QueryPointerlist(ulPointerIndex);

        for (i = ulFirstPtr, pplndx = ppl; i < ulLastPtr; i++, pplndx++)
        {
            // alle m봥lichen Animationen ein- bzw. Ausschalten
            if (pplndx->ulPtrCount > 1)
                EnableAnimation(pplndx, !fAnimate);
        }

        // Refresh durchf갿ren
        if ((pcnrrec) && (fRefresh))
            RefreshCnrItem(hwnd, prec, pcnrrec, FALSE);

        // alles ok
        fSuccess = TRUE;
    }
    while (FALSE);

    return fSuccess;

}


/*旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
 *� Name      : LoadAnimationResource                                      �
 *� Kommentar : l꼋t Resourcen f걊 einen Pointer oder f걊 alle Pointer     �
 *� Autor     : C.Langanke                                                 �
 *� Datum     : 20.07.1996                                                 �
 *� 럑derung  : 20.07.1996                                                 �
 *� aufgerufen: Window Procedure                                           �
 *� ruft auf  : -                                                          �
 *� Eingabe   : HWND        - frame window                                 �
 *�             PSZ         - Name  Pointerdatei / Verzeichnis             �
 *�             PRECORDCORE - Zeiger auf Containeritem oder NULL           �
 *�             PRECORDCORE - Zeiger auf Gesamt-Record                     �
 *� Aufgaben  : - Basisname ermitteln                                      �
 *� R갷kgabe  : BOOL - Flag, ob erfolgreich                                �
 *읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 */

BOOL LoadAnimationResource
 (
     HWND hwnd,
     PSZ pszName,
     PRECORDCORE prec,
     PRECORDCORE pcnrrec
)

{
    BOOL fSuccess = FALSE;
    APIRET rc;

    HWND hwndCnrPointerSet = WinWindowFromID(hwnd, IDDLG_CN_POINTERSET);

    PMYRECORDCORE pmyrec = (PMYRECORDCORE) prec;
    ULONG ulPointerIndex = (pmyrec) ? pmyrec->ulPointerIndex : 0;

    do
    {

#ifdef DEBUG
        habMain = WinQueryAnchorBlock(hwnd);
#endif

        // Parameter pr갽en
        if (pcnrrec == NULL)
            break;

        // Animation laden - Fehler ignorieren
        LoadPointerAnimation(ulPointerIndex,
                             QueryPointerlist(ulPointerIndex),
                             pszName,
                             (prec == NULL),
                             TRUE,
                             TRUE);

        // Refresh durchf갿ren
        RefreshCnrItem(hwnd, prec, pcnrrec, TRUE);

        // alles ok
        fSuccess = TRUE;
    }
    while (FALSE);

    return fSuccess;

}
