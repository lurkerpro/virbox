/* $Id$ */
/** @file
 * Shared Clipboard: HTTP server implementation for Shared Clipboard transfers on UNIX-y hosts.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <signal.h>

#include <iprt/http.h>
#include <iprt/http-server.h>

#include <iprt/net.h> /* To make use of IPv4Addr in RTGETOPTUNION. */

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/list.h>
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <iprt/log.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/uuid.h>
#include <iprt/vfs.h>

#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <VBox/GuestHost/SharedClipboard-transfers.h>


/*********************************************************************************************************************************
*   Definitations                                                                                                                *
*********************************************************************************************************************************/

typedef struct _SHCLHTTPSERVERTRANSFER
{
    /** The node list. */
    RTLISTNODE          Node;
    /** Pointer to associated transfer. */
    PSHCLTRANSFER       pTransfer;
    /** The virtual path of the HTTP server's root directory for this transfer. */
    char                szPathVirtual[RTPATH_MAX];
} SHCLHTTPSERVERTRANSFER;
typedef SHCLHTTPSERVERTRANSFER *PSHCLHTTPSERVERTRANSFER;


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
int ShClTransferHttpServerDestroyInternal(PSHCLHTTPSERVER pThis);


/*********************************************************************************************************************************
*   Internal functions                                                                                                           *
*********************************************************************************************************************************/
static PSHCLHTTPSERVERTRANSFER shClTransferHttpServerGetTransferById(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer)
{
    PSHCLHTTPSERVERTRANSFER pSrvTx;
    RTListForEach(&pSrv->lstTransfers, pSrvTx, SHCLHTTPSERVERTRANSFER, Node) /** @todo Slow O(n) lookup, but does it for now. */
    {
        if (pSrvTx->pTransfer->State.uID == idTransfer)
            return pSrvTx;
    }

    return NULL;
}

/**
 * Validates a given URL whether it matches a registered HTTP transfer.
 *
 * @returns VBox status code.
 * @param   pThis               HTTP server instance data.
 * @param   pszUrl              URL to validate.
 */
static int shClTransferHttpPathValidate(PSHCLHTTPSERVER pThis, const char *pszUrl)
{
    PSHCLHTTPSERVERTRANSFER pSrvTx;
    RTListForEach(&pThis->lstTransfers, pSrvTx, SHCLHTTPSERVERTRANSFER, Node)
    {
        AssertPtr(pSrvTx->pTransfer);
        /* Be picky here, do a case sensitive comparison. */
        if (RTStrStartsWith(pszUrl, pSrvTx->szPathVirtual))
            return VINF_SUCCESS;
    }

    return VERR_PATH_NOT_FOUND;
}

static DECLCALLBACK(int) shClTransferHttpOpen(PRTHTTPCALLBACKDATA pData, PRTHTTPSERVERREQ pReq, void **ppvHandle)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    int rc = shClTransferHttpPathValidate(pThis, pReq->pszUrl);
    if (RT_FAILURE(rc))
        return rc;

    RT_NOREF(ppvHandle);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static DECLCALLBACK(int) shClTransferHttpRead(PRTHTTPCALLBACKDATA pData, void *pvHandle, void *pvBuf, size_t cbBuf, size_t *pcbRead)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    RT_NOREF(pThis, pvHandle, pvBuf, cbBuf, pcbRead);

    int rc = 0;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static DECLCALLBACK(int) shClTransferHttpClose(PRTHTTPCALLBACKDATA pData, void *pvHandle)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    RT_NOREF(pThis, pvHandle);

    int rc = 0;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static DECLCALLBACK(int) shClTransferHttpQueryInfo(PRTHTTPCALLBACKDATA pData,
                                                   PRTHTTPSERVERREQ pReq, PRTFSOBJINFO pObjInfo, char **ppszMIMEHint)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    int rc = shClTransferHttpPathValidate(pThis, pReq->pszUrl);
    if (RT_FAILURE(rc))
        return rc;

    RT_NOREF(pObjInfo, ppszMIMEHint);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

static DECLCALLBACK(int) shClTransferHttpDestroy(PRTHTTPCALLBACKDATA pData)
{
    PSHCLHTTPSERVER pThis = (PSHCLHTTPSERVER)pData->pvUser;
    Assert(pData->cbUser == sizeof(SHCLHTTPSERVER));

    return ShClTransferHttpServerDestroyInternal(pThis);
}

int shClTransferHttpServerDestroyInternal(PSHCLHTTPSERVER pSrv)
{
    PSHCLHTTPSERVERTRANSFER pSrvTx, pSrvTxNext;
    RTListForEachSafe(&pSrv->lstTransfers, pSrvTx, pSrvTxNext, SHCLHTTPSERVERTRANSFER, Node)
    {
        RTListNodeRemove(&pSrvTx->Node);

        RTMemFree(pSrvTx);
        pSrvTx = NULL;
    }

    RTHttpServerResponseDestroy(&pSrv->Resp);

    return RTCritSectDelete(&pSrv->CritSect);
}

DECLINLINE(void) shClTransferHttpServerLock(PSHCLHTTPSERVER pSrv)
{
    int rc2 = RTCritSectEnter(&pSrv->CritSect);
    AssertRC(rc2);
}

DECLINLINE(void) shClTransferHttpServerUnlock(PSHCLHTTPSERVER pSrv)
{
    int rc2 = RTCritSectLeave(&pSrv->CritSect);
    AssertRC(rc2);
}


/*********************************************************************************************************************************
*   Public functions                                                                                                             *
*********************************************************************************************************************************/
int ShClTransferHttpServerCreateEx(PSHCLHTTPSERVER pSrv, uint16_t uPort)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);

    RTHTTPSERVERCALLBACKS Callbacks;
    RT_ZERO(Callbacks);

    Callbacks.pfnOpen          = shClTransferHttpOpen;
    Callbacks.pfnRead          = shClTransferHttpRead;
    Callbacks.pfnClose         = shClTransferHttpClose;
    Callbacks.pfnQueryInfo     = shClTransferHttpQueryInfo;
    Callbacks.pfnDestroy       = shClTransferHttpDestroy;

    /* Note: The server always and *only* runs against the localhost interface. */
    int rc = RTHttpServerCreate(&pSrv->hHTTPServer, "localhost", uPort, &Callbacks,
                                pSrv, sizeof(SHCLHTTPSERVER));
    if (RT_SUCCESS(rc))
    {
        rc = RTHttpServerResponseInit(&pSrv->Resp);
        AssertRCReturn(rc, rc);

        rc = RTCritSectInit(&pSrv->CritSect);
        AssertRCReturn(rc, rc);

        RTListInit(&pSrv->lstTransfers);
        pSrv->cTransfers = 0;

        pSrv->uPort = uPort;

        LogRel2(("Shared Clipboard: HTTP server running at port %RU16\n", pSrv->uPort));
    }
    else
    {
        int rc2 = shClTransferHttpServerDestroyInternal(pSrv);
        AssertRC(rc2);
    }

    if (RT_FAILURE(rc))
        LogRel(("Shared Clipboard: HTTP server failed to run, rc=%Rrc\n", rc));

    return rc;
}

int ShClTransferHttpServerCreate(PSHCLHTTPSERVER pSrv, uint16_t *puPort)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);
    AssertPtrReturn(puPort, VERR_INVALID_POINTER);

    /** @todo Try favorite ports first (e.g. 8080, 8000, ...)? */

    RTRAND hRand;
    int rc = RTRandAdvCreateSystemFaster(&hRand); /* Should be good enough for this task. */
    if (RT_SUCCESS(rc))
    {
        uint16_t uPort;
        for (int i = 0; i < 32; i++)
        {
            uPort = RTRandAdvU32Ex(hRand, 1024, UINT16_MAX);
#ifdef DEBUG_andy
            uPort = 8000; /** @todo BUGBUG Remove this! */
#endif
            rc = ShClTransferHttpServerCreateEx(pSrv, (uint32_t)uPort);
            if (RT_SUCCESS(rc))
            {
                *puPort = uPort;
                break;
            }
        }

        RTRandAdvDestroy(hRand);
    }

    return rc;
}

int ShClTransferHttpServerDestroy(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);

    if (pSrv->hHTTPServer == NIL_RTHTTPSERVER)
        return VINF_SUCCESS;

    Assert(pSrv->cTransfers == 0); /* Sanity. */

    int rc = RTHttpServerDestroy(pSrv->hHTTPServer);
    if (RT_SUCCESS(rc))
        rc = shClTransferHttpServerDestroyInternal(pSrv);

    if (RT_SUCCESS(rc))
        LogRel2(("Shared Clipboard: HTTP server stopped\n"));
    else
        LogRel(("Shared Clipboard: HTTP server failed to stop, rc=%Rrc\n", rc));

    return rc;
}

int ShClTransferHttpServerRegisterTransfer(PSHCLHTTPSERVER pSrv, PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    shClTransferHttpServerLock(pSrv);

    PSHCLHTTPSERVERTRANSFER pSrvTx = (PSHCLHTTPSERVERTRANSFER)RTMemAllocZ(sizeof(SHCLHTTPSERVERTRANSFER));
    AssertPtrReturn(pSrvTx, VERR_NO_MEMORY);

    RTUUID Uuid;
    int rc = RTUuidCreate(&Uuid);
    if (RT_SUCCESS(rc))
    {
        char szUuid[64];
        rc = RTUuidToStr(&Uuid, szUuid, sizeof(szUuid));
        if (RT_SUCCESS(rc))
        {
            AssertReturn(pTransfer->State.uID, VERR_INVALID_PARAMETER); /* Paranoia. */

            /* Create the virtual HTTP path for the transfer.
             * Every transfer has a dedicated HTTP path. */
            ssize_t cch = RTStrPrintf2(pSrvTx->szPathVirtual, sizeof(pSrvTx->szPathVirtual), "%s/%RU16/", szUuid, pTransfer->State.uID);
            AssertReturn(cch, VERR_BUFFER_OVERFLOW);

            RTListAppend(&pSrv->lstTransfers, &pSrvTx->Node);

            pSrv->cTransfers++;
        }
    }

    if (RT_FAILURE(rc))
        RTMemFree(pSrvTx);

    shClTransferHttpServerUnlock(pSrv);

    return rc;
}

int ShClTransferHttpServerUnregisterTransfer(PSHCLHTTPSERVER pSrv, PSHCLTRANSFER pTransfer)
{
    AssertPtrReturn(pSrv, VERR_INVALID_POINTER);
    AssertPtrReturn(pTransfer, VERR_INVALID_POINTER);

    shClTransferHttpServerLock(pSrv);

    AssertReturn(pSrv->cTransfers, VERR_WRONG_ORDER);

    PSHCLHTTPSERVERTRANSFER pSrvTx;
    RTListForEach(&pSrv->lstTransfers, pSrvTx, SHCLHTTPSERVERTRANSFER, Node)
    {
        AssertPtr(pSrvTx->pTransfer);
        if (pSrvTx->pTransfer->State.uID == pTransfer->State.uID)
        {
            RTListNodeRemove(&pSrvTx->Node);

            RTMemFree(pSrvTx);
            pSrvTx = NULL;

            pSrv->cTransfers--;

            shClTransferHttpServerUnlock(pSrv);
            return VINF_SUCCESS;
        }
    }

    shClTransferHttpServerUnlock(pSrv);
    return VERR_NOT_FOUND;
}

bool ShClTransferHttpServerHasTransfer(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer)
{
    AssertPtrReturn(pSrv, false);

    shClTransferHttpServerLock(pSrv);

    const bool fRc = shClTransferHttpServerGetTransferById(pSrv, idTransfer) != NULL;

    shClTransferHttpServerUnlock(pSrv);

    return fRc;
}

uint16_t ShClTransferHttpServerGetPort(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, 0);

    shClTransferHttpServerLock(pSrv);

    const uint16_t uPort = pSrv->uPort;

    shClTransferHttpServerUnlock(pSrv);

    return uPort;
}

uint32_t ShClTransferHttpServerGetTransferCount(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, 0);

    shClTransferHttpServerLock(pSrv);

    const uint32_t cTransfers = pSrv->cTransfers;

    shClTransferHttpServerUnlock(pSrv);

    return cTransfers;
}

static const char *shClTransferHttpServerGetHost(PSHCLHTTPSERVER pSrv)
{
    RT_NOREF(pSrv);
    return "http://localhost"; /* Hardcoded for now. */
}

char *ShClTransferHttpServerGetAddressA(PSHCLHTTPSERVER pSrv)
{
    AssertPtrReturn(pSrv, NULL);

    shClTransferHttpServerLock(pSrv);

    char *pszAddress = RTStrAPrintf2("%s:%RU16", shClTransferHttpServerGetHost(pSrv), pSrv->uPort);

    shClTransferHttpServerUnlock(pSrv);

    return pszAddress;
}

char *ShClTransferHttpServerGetUrlA(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer)
{
    AssertPtrReturn(pSrv, NULL);
    AssertReturn(idTransfer != NIL_SHCLTRANSFERID, NULL);

    shClTransferHttpServerLock(pSrv);

    PSHCLHTTPSERVERTRANSFER pSrvTx = shClTransferHttpServerGetTransferById(pSrv, idTransfer);
    if (!pSrvTx)
    {
        AssertFailed();
        shClTransferHttpServerUnlock(pSrv);
        return NULL;
    }

    AssertReturn(RTStrNLen(pSrvTx->szPathVirtual, RTPATH_MAX), NULL);
    char *pszUrl = RTStrAPrintf2("%s:%RU16/%s", shClTransferHttpServerGetHost(pSrv), pSrv->uPort, pSrvTx->szPathVirtual);

    shClTransferHttpServerUnlock(pSrv);

    return pszUrl;
}

