
/*
 *@@sourcefile callb_move.c:
 *      SES kernel hook code.
 */

/*
 *      Copyright (C) 2000 Ulrich M�ller.
 *      Based on the MWDD32.SYS example sources,
 *      Copyright (C) 1995, 1996, 1997  Matthieu Willm (willm@ibm.net).
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, in version 2 as it comes in the COPYING
 *      file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_NOPMAPI
#include <os2.h>
#include <secure.h>

#include <string.h>

#include "xwpsec32.sys\types.h"
#include "xwpsec32.sys\StackToFlat.h"
#include "xwpsec32.sys\devhlp32.h"

#include "security\ring0api.h"

#include "xwpsec32.sys\xwpsec_types.h"
#include "xwpsec32.sys\xwpsec_callbacks.h"

/*
 *@@ MOVE_PRE:
 *      SES kernel hook for MOVE_PRE (move or rename).
 *      This gets called from the OS/2 kernel to give
 *      the ISS a chance to authorize this event.
 *
 *      As with DosMove, this will only get called when
 *      source and dest are on same volume
 *
 *      This callback is stored in SecurityImports in
 *      sec32_callbacks.c to hook the kernel.
 *
 *@@added V0.9.2 (2000-03-13) [umoeller]
 */

ULONG CallType MOVE_PRE(PSZ pszNewPath,
                        PSZ pszOldPath)
{
    int rc = NO_ERROR;

    if (utilNeedsVerify())
    {
        // access control enabled, and not call from daemon itself:
        if (    (rc = utilSemRequest(&G_hmtxBufferLocked, -1))
                    == NO_ERROR)
        {
            // daemon buffers locked
            // (we have exclusive access):

            PXWPSECEVENTDATA_MOVE_PRE pMovePre
                = &((PSECIOSHARED)G_pSecIOShared)->EventData.MovePre;

            utilWriteLog("MOVE_PRE for \"%s\" -> \"%s\"\r\n", pszOldPath, pszNewPath);
            utilWriteLogInfo();

            // prepare data for daemon notify
            strcpy( pMovePre->szNewPath,
                    pszNewPath);
                      // sizeof(EventData.Directory.szPath));
            strcpy( pMovePre->szOldPath,
                    pszOldPath);
                      // sizeof(EventData.Directory.szPath));

            // have this request authorized by daemon
            rc = utilDaemonRequest(SECEVENT_MOVE_PRE);
            // utilDaemonRequest properly serializes all requests
            // to the daemon;
            // utilDaemonRequest blocks until the daemon has either
            // authorized or turned down this request.

            // Return code is either an error in the driver
            // or NO_ERROR if daemon has authorized the request
            // or ERROR_ACCESS_DENIED or some other error code
            // if the daemon denied the request.

            // now release buffers mutex;
            // this unblocks other application threads
            // which are waiting on an access verification
            // in this function (after we return from ring-0,
            // I guess)
            utilSemRelease(&G_hmtxBufferLocked);
        }
    }

    if (    (rc != NO_ERROR)
         && (rc != ERROR_ACCESS_DENIED)
       )
    {
        // kernel panic
        // _sprintf("XWPSEC32.SYS: OPEN_PRE returned %d.", rc);
        // DevHlp32_InternalError(G_szScratchBuf, strlen(G_szScratchBuf) + 1);
        utilWriteLog("      ------ WARNING rc is %d\r\n", rc);
        rc = ERROR_ACCESS_DENIED;
    }

    return (rc);
}

/*
 *@@ MOVE_POST:
 *      SES kernel hook for MOVE_POST.
 *      This gets called from the OS/2 kernel to notify
 *      the ISS of this event. We need this so that
 *      entries in the ACLDB can be updated.
 *
 *      This callback is stored in SecurityImports in
 *      sec32_callbacks.c to hook the kernel.
 *
 *      Currently disabled. ###
 *
 *@@added V0.9.2 (2000-03-13) [umoeller]
 */

VOID CallType MOVE_POST(PSZ pszNewPath,
                        PSZ pszOldPath,
                        ULONG RC)
{
}


