/* $Id$ */
/** @file
 * Shared Clipboard: Some helper function for converting between the various eol.
 */

/*
 * Includes contributions from François Revol
 *
 * Copyright (C) 2006-2022 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD

#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/semaphore.h>
#include <iprt/path.h>
#include <iprt/rand.h>
#include <iprt/utf16.h>

#include <iprt/formats/bmp.h>

#include <iprt/errcore.h>
#include <VBox/log.h>
#include <VBox/GuestHost/clipboard-helper.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
static void shClEventSourceResetInternal(PSHCLEVENTSOURCE pSource);

static void shClEventDestroy(PSHCLEVENT pEvent);
DECLINLINE(PSHCLEVENT) shclEventGet(PSHCLEVENTSOURCE pSource, SHCLEVENTID idEvent);


/*********************************************************************************************************************************
*   Implementation                                                                                                               *
*********************************************************************************************************************************/

/**
 * Allocates a new event payload.
 *
 * @returns VBox status code.
 * @param   uID                 Payload ID to set for this payload. Useful for consequtive payloads.
 * @param   pvData              Data block to associate to this payload.
 * @param   cbData              Size (in bytes) of data block to associate.
 * @param   ppPayload           Where to store the allocated event payload on success.
 */
int ShClPayloadAlloc(uint32_t uID, const void *pvData, uint32_t cbData,
                     PSHCLEVENTPAYLOAD *ppPayload)
{
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData > 0, VERR_INVALID_PARAMETER);

    PSHCLEVENTPAYLOAD pPayload = (PSHCLEVENTPAYLOAD)RTMemAlloc(sizeof(SHCLEVENTPAYLOAD));
    if (pPayload)
    {
        pPayload->pvData = RTMemDup(pvData, cbData);
        if (pPayload->pvData)
        {
            pPayload->cbData = cbData;
            pPayload->uID    = uID;

            *ppPayload = pPayload;
            return VINF_SUCCESS;
        }

        RTMemFree(pPayload);
    }
    return VERR_NO_MEMORY;
}

/**
 * Frees an event payload.
 *
 * @returns VBox status code.
 * @param   pPayload            Event payload to free.
 */
void ShClPayloadFree(PSHCLEVENTPAYLOAD pPayload)
{
    if (!pPayload)
        return;

    if (pPayload->pvData)
    {
        Assert(pPayload->cbData);
        RTMemFree(pPayload->pvData);
        pPayload->pvData = NULL;
    }

    pPayload->cbData = 0;
    pPayload->uID = UINT32_MAX;

    RTMemFree(pPayload);
}

/**
 * Creates a new event source.
 *
 * @returns VBox status code.
 * @param   pSource             Event source to create.
 * @param   uID                 ID to use for event source.
 */
int ShClEventSourceCreate(PSHCLEVENTSOURCE pSource, SHCLEVENTSOURCEID uID)
{
    LogFlowFunc(("pSource=%p, uID=%RU16\n", pSource, uID));
    AssertPtrReturn(pSource, VERR_INVALID_POINTER);

    int rc = RTCritSectInit(&pSource->CritSect);
    AssertRCReturn(rc, rc);

    RTListInit(&pSource->lstEvents);

    pSource->uID          = uID;
    /* Choose a random event ID starting point. */
    pSource->idNextEvent  = RTRandU32Ex(1, VBOX_SHCL_MAX_EVENTS - 1);

    return VINF_SUCCESS;
}

/**
 * Destroys an event source.
 *
 * @returns VBox status code.
 * @param   pSource             Event source to destroy.
 */
int ShClEventSourceDestroy(PSHCLEVENTSOURCE pSource)
{
    if (!pSource)
        return VINF_SUCCESS;

    LogFlowFunc(("ID=%RU32\n", pSource->uID));

    int rc = RTCritSectEnter(&pSource->CritSect);
    if (RT_SUCCESS(rc))
    {
        shClEventSourceResetInternal(pSource);

        rc = RTCritSectLeave(&pSource->CritSect);
        AssertRC(rc);

        RTCritSectDelete(&pSource->CritSect);

        pSource->uID          = UINT16_MAX;
        pSource->idNextEvent  = UINT32_MAX;
    }

    return rc;
}

/**
 * Resets an event source, internal version.
 *
 * @param   pSource             Event source to reset.
 */
static void shClEventSourceResetInternal(PSHCLEVENTSOURCE pSource)
{
    LogFlowFunc(("ID=%RU32\n", pSource->uID));

    PSHCLEVENT pEvIt;
    PSHCLEVENT pEvItNext;
    RTListForEachSafe(&pSource->lstEvents, pEvIt, pEvItNext, SHCLEVENT, Node)
    {
        RTListNodeRemove(&pEvIt->Node);

        shClEventDestroy(pEvIt);

        RTMemFree(pEvIt);
        pEvIt = NULL;
    }
}

/**
 * Resets an event source.
 *
 * @param   pSource             Event source to reset.
 */
void ShClEventSourceReset(PSHCLEVENTSOURCE pSource)
{
    int rc2 = RTCritSectEnter(&pSource->CritSect);
    if (RT_SUCCESS(rc2))
    {
        shClEventSourceResetInternal(pSource);

        rc2 = RTCritSectLeave(&pSource->CritSect);
        AssertRC(rc2);
    }
}

/**
 * Generates a new event ID for a specific event source and registers it.
 *
 * @returns VBox status code.
 * @param   pSource             Event source to generate event for.
 * @param   ppEvent             Where to return the new event generated on success.
 */
int ShClEventSourceGenerateAndRegisterEvent(PSHCLEVENTSOURCE pSource, PSHCLEVENT *ppEvent)
{
    AssertPtrReturn(pSource, VERR_INVALID_POINTER);
    AssertPtrReturn(ppEvent, VERR_INVALID_POINTER);

    PSHCLEVENT pEvent = (PSHCLEVENT)RTMemAllocZ(sizeof(SHCLEVENT));
    AssertReturn(pEvent, VERR_NO_MEMORY);
    int rc = RTSemEventMultiCreate(&pEvent->hEvtMulSem);
    if (RT_SUCCESS(rc))
    {
        rc = RTCritSectEnter(&pSource->CritSect);
        if (RT_SUCCESS(rc))
        {
            /*
             * Allocate an unique event ID.
             */
            for (uint32_t cTries = 0;; cTries++)
            {
                SHCLEVENTID idEvent = ++pSource->idNextEvent;
                if (idEvent < VBOX_SHCL_MAX_EVENTS)
                { /* likely */ }
                else
                    pSource->idNextEvent = idEvent = 1; /* zero == error, remember! */

                if (shclEventGet(pSource, idEvent) == NULL)
                {
                    pEvent->pParent = pSource;
                    pEvent->idEvent = idEvent;
                    RTListAppend(&pSource->lstEvents, &pEvent->Node);

                    rc = RTCritSectLeave(&pSource->CritSect);
                    AssertRC(rc);

                    LogFlowFunc(("uSource=%RU16: New event: %#x\n", pSource->uID, idEvent));

                    ShClEventRetain(pEvent);
                    *ppEvent = pEvent;

                    return VINF_SUCCESS;
                }

                AssertBreak(cTries < 4096);
            }

            rc = RTCritSectLeave(&pSource->CritSect);
            AssertRC(rc);
        }
    }

    AssertMsgFailed(("Unable to register a new event ID for event source %RU16\n", pSource->uID));

    RTSemEventMultiDestroy(pEvent->hEvtMulSem);
    pEvent->hEvtMulSem = NIL_RTSEMEVENTMULTI;
    RTMemFree(pEvent);
    return rc;
}

/**
 * Destroys an event.
 *
 * @param   pEvent              Event to destroy.
 */
static void shClEventDestroy(PSHCLEVENT pEvent)
{
    if (!pEvent)
        return;

    AssertMsgReturnVoid(pEvent->cRefs == 0, ("Event %RU32 still has %RU32 references\n",
                                             pEvent->idEvent, pEvent->cRefs));

    LogFlowFunc(("Event %RU32\n", pEvent->idEvent));

    if (pEvent->hEvtMulSem != NIL_RTSEMEVENT)
    {
        RTSemEventMultiDestroy(pEvent->hEvtMulSem);
        pEvent->hEvtMulSem = NIL_RTSEMEVENT;
    }

    ShClPayloadFree(pEvent->pPayload);

    pEvent->idEvent = NIL_SHCLEVENTID;
}

/**
 * Unregisters an event.
 *
 * @returns VBox status code.
 * @param   pSource             Event source to unregister event for.
 * @param   pEvent              Event to unregister. On success the pointer will be invalid.
 */
static int shClEventSourceUnregisterEventInternal(PSHCLEVENTSOURCE pSource, PSHCLEVENT pEvent)
{
    LogFlowFunc(("idEvent=%RU32, cRefs=%RU32\n", pEvent->idEvent, pEvent->cRefs));

    AssertReturn(pEvent->cRefs == 0, VERR_WRONG_ORDER);

    int rc = RTCritSectEnter(&pSource->CritSect);
    if (RT_SUCCESS(rc))
    {
        RTListNodeRemove(&pEvent->Node);

        shClEventDestroy(pEvent);

        rc = RTCritSectLeave(&pSource->CritSect);
        if (RT_SUCCESS(rc))
        {
            RTMemFree(pEvent);
            pEvent = NULL;
        }
    }

    return rc;
}

/**
 * Returns a specific event of a event source. Inlined version.
 *
 * @returns Pointer to event if found, or NULL if not found.
 * @param   pSource             Event source to get event from.
 * @param   uID                 Event ID to get.
 */
DECLINLINE(PSHCLEVENT) shclEventGet(PSHCLEVENTSOURCE pSource, SHCLEVENTID idEvent)
{
    PSHCLEVENT pEvent;
    RTListForEach(&pSource->lstEvents, pEvent, SHCLEVENT, Node)
    {
        if (pEvent->idEvent == idEvent)
            return pEvent;
    }

    return NULL;
}

/**
 * Returns a specific event of a event source.
 *
 * @returns Pointer to event if found, or NULL if not found.
 * @param   pSource             Event source to get event from.
 * @param   idEvent             ID of event to return.
 */
PSHCLEVENT ShClEventSourceGetFromId(PSHCLEVENTSOURCE pSource, SHCLEVENTID idEvent)
{
    AssertPtrReturn(pSource, NULL);

    int rc = RTCritSectEnter(&pSource->CritSect);
    if (RT_SUCCESS(rc))
    {
         PSHCLEVENT pEvent = shclEventGet(pSource, idEvent);

         rc = RTCritSectLeave(&pSource->CritSect);
         AssertRC(rc);

         return pEvent;
    }

    return NULL;
}

/**
 * Returns the last (newest) event ID which has been registered for an event source.
 *
 * @returns Pointer to last registered event, or NULL if not found.
 * @param   pSource             Event source to get last registered event from.
 */
PSHCLEVENT ShClEventSourceGetLast(PSHCLEVENTSOURCE pSource)
{
    AssertPtrReturn(pSource, NULL);

    int rc = RTCritSectEnter(&pSource->CritSect);
    if (RT_SUCCESS(rc))
    {
        PSHCLEVENT pEvent = RTListGetLast(&pSource->lstEvents, SHCLEVENT, Node);

        rc = RTCritSectLeave(&pSource->CritSect);
        AssertRC(rc);

        return pEvent;
    }

    return NULL;
}

/**
 * Returns the current reference count for a specific event.
 *
 * @returns Reference count.
 * @param   pSource             Event source the specific event is part of.
 * @param   idEvent             Event ID to return reference count for.
 */
uint32_t ShClEventGetRefs(PSHCLEVENT pEvent)
{
    AssertPtrReturn(pEvent, 0);

    return ASMAtomicReadU32(&pEvent->cRefs);
}

/**
 * Detaches a payload from an event, internal version.
 *
 * @returns Pointer to the detached payload. Can be NULL if the payload has no payload.
 * @param   pEvent              Event to detach payload for.
 */
static PSHCLEVENTPAYLOAD shclEventPayloadDetachInternal(PSHCLEVENT pEvent)
{
#ifdef VBOX_STRICT
    AssertPtrReturn(pEvent, NULL);
#endif

    PSHCLEVENTPAYLOAD pPayload = pEvent->pPayload;

    pEvent->pPayload = NULL;

    return pPayload;
}

/**
 * Waits for an event to get signalled.
 *
 * @returns VBox status code.
 * @param   pEvent              Event to wait for.
 * @param   uTimeoutMs          Timeout (in ms) to wait.
 * @param   ppPayload           Where to store the (allocated) event payload on success. Needs to be free'd with
 *                              SharedClipboardPayloadFree(). Optional.
 */
int ShClEventWait(PSHCLEVENT pEvent, RTMSINTERVAL uTimeoutMs, PSHCLEVENTPAYLOAD *ppPayload)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);
    AssertPtrNullReturn(ppPayload, VERR_INVALID_POINTER);
    LogFlowFuncEnter();

    int rc = RTSemEventMultiWait(pEvent->hEvtMulSem, uTimeoutMs);
    if (RT_SUCCESS(rc))
    {
        if (ppPayload)
        {
            /* Make sure to detach payload here, as the caller now owns the data. */
            *ppPayload = shclEventPayloadDetachInternal(pEvent);
        }
    }

    if (RT_FAILURE(rc))
        LogRel2(("Shared Clipboard: Waiting for event %RU32 failed, rc=%Rrc\n", pEvent->idEvent, rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Retains an event by increasing its reference count.
 *
 * @returns New reference count, or UINT32_MAX if failed.
 * @param   pEvent              Event to retain.
 */
uint32_t ShClEventRetain(PSHCLEVENT pEvent)
{
    AssertPtrReturn(pEvent, UINT32_MAX);
    AssertReturn(ASMAtomicReadU32(&pEvent->cRefs) < 64, UINT32_MAX);
    return ASMAtomicIncU32(&pEvent->cRefs);
}

/**
 * Releases an event by decreasing its reference count.
 *
 * @returns New reference count, or UINT32_MAX if failed.
 * @param   pEvent              Event to release.
 *                              If the reference count reaches 0, the event will
 *                              be destroyed and \a pEvent will be invalid.
 */
uint32_t ShClEventRelease(PSHCLEVENT pEvent)
{
    if (!pEvent)
        return 0;

    AssertReturn(ASMAtomicReadU32(&pEvent->cRefs) > 0, UINT32_MAX);

    uint32_t const cRefs = ASMAtomicDecU32(&pEvent->cRefs);
    if (cRefs == 0)
    {
        AssertPtr(pEvent->pParent);
        int rc2 = shClEventSourceUnregisterEventInternal(pEvent->pParent, pEvent);
        AssertRC(rc2);

        return RT_SUCCESS(rc2) ? 0 : UINT32_MAX;
    }

    return cRefs;
}

/**
 * Signals an event.
 *
 * @returns VBox status code.
 * @param   pEvent              Event to signal.
 * @param   pPayload            Event payload to associate. Takes ownership on
 *                              success. Optional.
 */
int ShClEventSignal(PSHCLEVENT pEvent, PSHCLEVENTPAYLOAD pPayload)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);

    Assert(pEvent->pPayload == NULL);

    pEvent->pPayload = pPayload;

    int rc = RTSemEventMultiSignal(pEvent->hEvtMulSem);
    if (RT_FAILURE(rc))
        pEvent->pPayload = NULL; /* (no race condition if consumer also enters the critical section) */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClUtf16LenUtf8(PCRTUTF16 pcwszSrc, size_t cwcSrc, size_t *pchLen)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertPtrReturn(pchLen, VERR_INVALID_POINTER);

    size_t chLen = 0;
    int rc = RTUtf16CalcUtf8LenEx(pcwszSrc, cwcSrc, &chLen);
    if (RT_SUCCESS(rc))
        *pchLen = chLen;
    return rc;
}

int ShClConvUtf16CRLFToUtf8LF(PCRTUTF16 pcwszSrc, size_t cwcSrc,
                              char *pszBuf, size_t cbBuf, size_t *pcbLen)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertReturn   (cwcSrc,   VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszBuf,   VERR_INVALID_POINTER);
    AssertPtrReturn(pcbLen,   VERR_INVALID_POINTER);

    int rc;

    PRTUTF16 pwszTmp = NULL;
    size_t   cchTmp  = 0;

    size_t   cbLen = 0;

    /* How long will the converted text be? */
    rc = ShClUtf16CRLFLenUtf8(pcwszSrc, cwcSrc, &cchTmp);
    if (RT_SUCCESS(rc))
    {
        cchTmp++; /* Add space for terminator. */

        pwszTmp = (PRTUTF16)RTMemAlloc(cchTmp * sizeof(RTUTF16));
        if (pwszTmp)
        {
            rc = ShClConvUtf16CRLFToLF(pcwszSrc, cwcSrc, pwszTmp, cchTmp);
            if (RT_SUCCESS(rc))
                rc = RTUtf16ToUtf8Ex(pwszTmp + 1, cchTmp - 1, &pszBuf, cbBuf, &cbLen);

            RTMemFree(reinterpret_cast<void *>(pwszTmp));
        }
        else
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        *pcbLen = cbLen;
    }

    return rc;
}

int ShClConvUtf16LFToCRLFA(PCRTUTF16 pcwszSrc, size_t cwcSrc,
                           PRTUTF16 *ppwszDst, size_t *pcwDst)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertPtrReturn(ppwszDst, VERR_INVALID_POINTER);
    AssertPtrReturn(pcwDst,   VERR_INVALID_POINTER);

    PRTUTF16 pwszDst = NULL;
    size_t   cchDst;

    int rc = ShClUtf16LFLenUtf8(pcwszSrc, cwcSrc, &cchDst);
    if (RT_SUCCESS(rc))
    {
        pwszDst = (PRTUTF16)RTMemAlloc((cchDst + 1 /* Leave space for terminator */) * sizeof(RTUTF16));
        if (pwszDst)
        {
            rc = ShClConvUtf16LFToCRLF(pcwszSrc, cwcSrc, pwszDst, cchDst + 1 /* Include terminator */);
        }
        else
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        *ppwszDst = pwszDst;
        *pcwDst   = cchDst;
    }
    else
        RTMemFree(pwszDst);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int ShClConvUtf8LFToUtf16CRLF(const char *pcszSrc, size_t cbSrc,
                              PRTUTF16 *ppwszDst, size_t *pcwDst)
{
    AssertPtrReturn(pcszSrc,  VERR_INVALID_POINTER);
    AssertReturn(cbSrc,       VERR_INVALID_PARAMETER);
    AssertPtrReturn(ppwszDst, VERR_INVALID_POINTER);
    AssertPtrReturn(pcwDst,   VERR_INVALID_POINTER);

    /* Intermediate conversion to UTF-16. */
    size_t   cwcTmp;
    PRTUTF16 pwcTmp = NULL;
    int rc = RTStrToUtf16Ex(pcszSrc, cbSrc, &pwcTmp, 0, &cwcTmp);
    if (RT_SUCCESS(rc))
    {
        rc = ShClConvUtf16LFToCRLFA(pwcTmp, cwcTmp, ppwszDst, pcwDst);
        RTUtf16Free(pwcTmp);
    }

    return rc;
}

int ShClConvLatin1LFToUtf16CRLF(const char *pcszSrc, size_t cbSrc,
                                PRTUTF16 *ppwszDst, size_t *pcwDst)
{
    AssertPtrReturn(pcszSrc,  VERR_INVALID_POINTER);
    AssertReturn(cbSrc,       VERR_INVALID_PARAMETER);
    AssertPtrReturn(ppwszDst, VERR_INVALID_POINTER);
    AssertPtrReturn(pcwDst,   VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    PRTUTF16 pwszDst = NULL;

    /* Calculate the space needed. */
    unsigned cwDst = 0;
    for (unsigned i = 0; i < cbSrc && pcszSrc[i] != '\0'; ++i)
    {
        if (pcszSrc[i] == VBOX_SHCL_LINEFEED)
            cwDst += 2; /* Space for VBOX_SHCL_CARRIAGERETURN + VBOX_SHCL_LINEFEED. */
        else
            ++cwDst;
    }

    pwszDst = (PRTUTF16)RTMemAlloc((cwDst + 1 /* Leave space for the terminator */) * sizeof(RTUTF16));
    if (!pwszDst)
        rc = VERR_NO_MEMORY;

    /* Do the conversion, bearing in mind that Latin-1 expands "naturally" to UTF-16. */
    if (RT_SUCCESS(rc))
    {
        for (unsigned i = 0, j = 0; i < cbSrc; ++i, ++j)
        {
            if (pcszSrc[i] != VBOX_SHCL_LINEFEED)
                pwszDst[j] = pcszSrc[i];
            else
            {
                pwszDst[j]     = VBOX_SHCL_CARRIAGERETURN;
                pwszDst[j + 1] = VBOX_SHCL_LINEFEED;
                ++j;
            }
        }

        pwszDst[cwDst] = '\0';  /* Make sure we are zero-terminated. */
    }

    if (RT_SUCCESS(rc))
    {
        *ppwszDst = pwszDst;
        *pcwDst   = cwDst;
    }
    else
        RTMemFree(pwszDst);

    return rc;
}

int ShClConvUtf16ToUtf8HTML(PCRTUTF16 pcwszSrc, size_t cwcSrc, char **ppszDst, size_t *pcbDst)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertReturn   (cwcSrc,   VERR_INVALID_PARAMETER);
    AssertPtrReturn(ppszDst,  VERR_INVALID_POINTER);
    AssertPtrReturn(pcbDst,   VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    size_t    cwTmp = cwcSrc;
    PCRTUTF16 pwTmp = pcwszSrc;

    char  *pchDst = NULL;
    size_t cbDst  = 0;

    size_t i = 0;
    while (i < cwTmp)
    {
        /* Find  zero symbol (end of string). */
        for (; i < cwTmp && pcwszSrc[i] != 0; i++)
            ;

        /* Convert found string. */
        char  *psz = NULL;
        size_t cch = 0;
        rc = RTUtf16ToUtf8Ex(pwTmp, cwTmp, &psz, pwTmp - pcwszSrc, &cch);
        if (RT_FAILURE(rc))
            break;

        /* Append new substring. */
        char *pchNew = (char *)RTMemRealloc(pchDst, cbDst + cch + 1);
        if (!pchNew)
        {
            RTStrFree(psz);
            rc = VERR_NO_MEMORY;
            break;
        }

        pchDst = pchNew;
        memcpy(pchDst + cbDst, psz, cch + 1);

        RTStrFree(psz);

        cbDst += cch + 1;

        /* Skip zero symbols. */
        for (; i < cwTmp && pcwszSrc[i] == 0; i++)
            ;

        /* Remember start of string. */
        pwTmp += i;
    }

    if (RT_SUCCESS(rc))
    {
        *ppszDst = pchDst;
        *pcbDst  = cbDst;

        return VINF_SUCCESS;
    }

    RTMemFree(pchDst);

    return rc;
}

int ShClUtf16LFLenUtf8(PCRTUTF16 pcwszSrc, size_t cwSrc, size_t *pchLen)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertPtrReturn(pchLen, VERR_INVALID_POINTER);

    AssertMsgReturn(pcwszSrc[0] != VBOX_SHCL_UTF16BEMARKER,
                    ("Big endian UTF-16 not supported yet\n"), VERR_NOT_SUPPORTED);

    size_t cLen = 0;

    /* Don't copy the endian marker. */
    size_t i = pcwszSrc[0] == VBOX_SHCL_UTF16LEMARKER ? 1 : 0;

    /* Calculate the size of the destination text string. */
    /* Is this Utf16 or Utf16-LE? */
    for (; i < cwSrc; ++i, ++cLen)
    {
        /* Check for a single line feed */
        if (pcwszSrc[i] == VBOX_SHCL_LINEFEED)
            ++cLen;
#ifdef RT_OS_DARWIN
        /* Check for a single carriage return (MacOS) */
        if (pcwszSrc[i] == VBOX_SHCL_CARRIAGERETURN)
            ++cLen;
#endif
        if (pcwszSrc[i] == 0)
        {
            /* Don't count this, as we do so below. */
            break;
        }
    }

    *pchLen = cLen;

    return VINF_SUCCESS;
}

int ShClUtf16CRLFLenUtf8(PCRTUTF16 pcwszSrc, size_t cwSrc, size_t *pchLen)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertReturn(cwSrc, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pchLen, VERR_INVALID_POINTER);

    AssertMsgReturn(pcwszSrc[0] != VBOX_SHCL_UTF16BEMARKER,
                    ("Big endian UTF-16 not supported yet\n"), VERR_NOT_SUPPORTED);

    size_t cLen = 0;

    /* Calculate the size of the destination text string. */
    /* Is this Utf16 or Utf16-LE? */
    if (pcwszSrc[0] == VBOX_SHCL_UTF16LEMARKER)
        cLen = 0;
    else
        cLen = 1;

    for (size_t i = 0; i < cwSrc; ++i, ++cLen)
    {
        if (   (i + 1 < cwSrc)
            && (pcwszSrc[i]     == VBOX_SHCL_CARRIAGERETURN)
            && (pcwszSrc[i + 1] == VBOX_SHCL_LINEFEED))
        {
            ++i;
        }
        if (pcwszSrc[i] == 0)
            break;
    }

    *pchLen = cLen;

    return VINF_SUCCESS;
}

int ShClConvUtf16LFToCRLF(PCRTUTF16 pcwszSrc, size_t cwcSrc, PRTUTF16 pu16Dst, size_t cwDst)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertPtrReturn(pu16Dst, VERR_INVALID_POINTER);
    AssertReturn(cwDst, VERR_INVALID_PARAMETER);

    AssertMsgReturn(pcwszSrc[0] != VBOX_SHCL_UTF16BEMARKER,
                    ("Big endian UTF-16 not supported yet\n"), VERR_NOT_SUPPORTED);

    int rc = VINF_SUCCESS;

    /* Don't copy the endian marker. */
    size_t i = pcwszSrc[0] == VBOX_SHCL_UTF16LEMARKER ? 1 : 0;
    size_t j = 0;

    for (; i < cwcSrc; ++i, ++j)
    {
        /* Don't copy the null byte, as we add it below. */
        if (pcwszSrc[i] == 0)
            break;

        /* Not enough space in destination? */
        if (j == cwDst)
        {
            rc = VERR_BUFFER_OVERFLOW;
            break;
        }

        if (pcwszSrc[i] == VBOX_SHCL_LINEFEED)
        {
            pu16Dst[j] = VBOX_SHCL_CARRIAGERETURN;
            ++j;

            /* Not enough space in destination? */
            if (j == cwDst)
            {
                rc = VERR_BUFFER_OVERFLOW;
                break;
            }
        }
#ifdef RT_OS_DARWIN
        /* Check for a single carriage return (MacOS) */
        else if (pcwszSrc[i] == VBOX_SHCL_CARRIAGERETURN)
        {
            /* Set CR.r */
            pu16Dst[j] = VBOX_SHCL_CARRIAGERETURN;
            ++j;

            /* Not enough space in destination? */
            if (j == cwDst)
            {
                rc = VERR_BUFFER_OVERFLOW;
                break;
            }

            /* Add line feed. */
            pu16Dst[j] = VBOX_SHCL_LINEFEED;
            continue;
        }
#endif
        pu16Dst[j] = pcwszSrc[i];
    }

    if (j == cwDst)
        rc = VERR_BUFFER_OVERFLOW;

    if (RT_SUCCESS(rc))
    {
        /* Add terminator. */
        pu16Dst[j] = 0;
    }

    return rc;
}

int ShClConvUtf16CRLFToLF(PCRTUTF16 pcwszSrc, size_t cwcSrc, PRTUTF16 pu16Dst, size_t cwDst)
{
    AssertPtrReturn(pcwszSrc, VERR_INVALID_POINTER);
    AssertReturn(cwcSrc,      VERR_INVALID_PARAMETER);
    AssertPtrReturn(pu16Dst,  VERR_INVALID_POINTER);
    AssertReturn(cwDst,       VERR_INVALID_PARAMETER);

    AssertMsgReturn(pcwszSrc[0] != VBOX_SHCL_UTF16BEMARKER,
                    ("Big endian UTF-16 not supported yet\n"), VERR_NOT_SUPPORTED);

    /* Prepend the Utf16 byte order marker if it is missing. */
    size_t cwDstPos;
    if (pcwszSrc[0] == VBOX_SHCL_UTF16LEMARKER)
    {
        cwDstPos = 0;
    }
    else
    {
        pu16Dst[0] = VBOX_SHCL_UTF16LEMARKER;
        cwDstPos = 1;
    }

    for (size_t i = 0; i < cwcSrc; ++i, ++cwDstPos)
    {
        if (pcwszSrc[i] == 0)
            break;

        if (cwDstPos == cwDst)
            return VERR_BUFFER_OVERFLOW;

        if (   (i + 1 < cwcSrc)
            && (pcwszSrc[i]     == VBOX_SHCL_CARRIAGERETURN)
            && (pcwszSrc[i + 1] == VBOX_SHCL_LINEFEED))
        {
            ++i;
        }

        pu16Dst[cwDstPos] = pcwszSrc[i];
    }

    if (cwDstPos == cwDst)
        return VERR_BUFFER_OVERFLOW;

    /* Add terminating zero. */
    pu16Dst[cwDstPos] = 0;

    return VINF_SUCCESS;
}

int ShClDibToBmp(const void *pvSrc, size_t cbSrc, void **ppvDest, size_t *pcbDest)
{
    AssertPtrReturn(pvSrc,   VERR_INVALID_POINTER);
    AssertReturn(cbSrc,      VERR_INVALID_PARAMETER);
    AssertPtrReturn(ppvDest, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbDest, VERR_INVALID_POINTER);

    PBMPWIN3XINFOHDR coreHdr = (PBMPWIN3XINFOHDR)pvSrc;
    /** @todo Support all the many versions of the DIB headers. */
    if (   cbSrc < sizeof(BMPWIN3XINFOHDR)
        || RT_LE2H_U32(coreHdr->cbSize) < sizeof(BMPWIN3XINFOHDR)
        || RT_LE2H_U32(coreHdr->cbSize) != sizeof(BMPWIN3XINFOHDR))
    {
        return VERR_INVALID_PARAMETER;
    }

    size_t offPixel = sizeof(BMPFILEHDR)
                    + RT_LE2H_U32(coreHdr->cbSize)
                    + RT_LE2H_U32(coreHdr->cClrUsed) * sizeof(uint32_t);
    if (cbSrc < offPixel)
        return VERR_INVALID_PARAMETER;

    size_t cbDst = sizeof(BMPFILEHDR) + cbSrc;

    void *pvDest = RTMemAlloc(cbDst);
    if (!pvDest)
        return VERR_NO_MEMORY;

    PBMPFILEHDR fileHdr = (PBMPFILEHDR)pvDest;

    fileHdr->uType       = BMP_HDR_MAGIC;
    fileHdr->cbFileSize  = (uint32_t)RT_H2LE_U32(cbDst);
    fileHdr->Reserved1   = 0;
    fileHdr->Reserved2   = 0;
    fileHdr->offBits     = (uint32_t)RT_H2LE_U32(offPixel);

    memcpy((uint8_t *)pvDest + sizeof(BMPFILEHDR), pvSrc, cbSrc);

    *ppvDest = pvDest;
    *pcbDest = cbDst;

    return VINF_SUCCESS;
}

int ShClBmpGetDib(const void *pvSrc, size_t cbSrc, const void **ppvDest, size_t *pcbDest)
{
    AssertPtrReturn(pvSrc,   VERR_INVALID_POINTER);
    AssertReturn(cbSrc,      VERR_INVALID_PARAMETER);
    AssertPtrReturn(ppvDest, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbDest, VERR_INVALID_POINTER);

    PBMPFILEHDR pBmpHdr = (PBMPFILEHDR)pvSrc;
    if (   cbSrc < sizeof(BMPFILEHDR)
        || pBmpHdr->uType != BMP_HDR_MAGIC
        || RT_LE2H_U32(pBmpHdr->cbFileSize) != cbSrc)
    {
        return VERR_INVALID_PARAMETER;
    }

    *ppvDest = ((uint8_t *)pvSrc) + sizeof(BMPFILEHDR);
    *pcbDest = cbSrc - sizeof(BMPFILEHDR);

    return VINF_SUCCESS;
}

#ifdef LOG_ENABLED

int ShClDbgDumpHtml(const char *pcszSrc, size_t cbSrc)
{
    int rc = VINF_SUCCESS;
    char *pszBuf = (char *)RTMemTmpAllocZ(cbSrc + 1);
    if (pszBuf)
    {
        memcpy(pszBuf, pcszSrc, cbSrc);
        pszBuf[cbSrc] = '\0';
        for (size_t off = 0; off < cbSrc; ++off)
            if (pszBuf[off] == '\n' || pszBuf[off] == '\r')
                pszBuf[off] = ' ';
        LogFunc(("Removed \\r\\n: %s\n", pszBuf));
        RTMemTmpFree(pszBuf);
    }
    else
        rc = VERR_NO_MEMORY;
    return rc;
}

void ShClDbgDumpData(const void *pv, size_t cb, SHCLFORMAT uFormat)
{
    if (LogIsEnabled())
    {
        if (uFormat & VBOX_SHCL_FMT_UNICODETEXT)
        {
            LogFunc(("VBOX_SHCL_FMT_UNICODETEXT:\n"));
            if (pv && cb)
                LogFunc(("%ls\n", pv));
            else
                LogFunc(("%p %zu\n", pv, cb));
        }
        else if (uFormat & VBOX_SHCL_FMT_BITMAP)
            LogFunc(("VBOX_SHCL_FMT_BITMAP\n"));
        else if (uFormat & VBOX_SHCL_FMT_HTML)
        {
            LogFunc(("VBOX_SHCL_FMT_HTML:\n"));
            if (pv && cb)
            {
                LogFunc(("%s\n", pv));
                ShClDbgDumpHtml((const char *)pv, cb);
            }
            else
                LogFunc(("%p %zu\n", pv, cb));
        }
        else
            LogFunc(("Invalid format %02X\n", uFormat));
    }
}

#endif /* LOG_ENABLED */

/**
 * Translates a Shared Clipboard host function number to a string.
 *
 * @returns Function ID string name.
 * @param   uFn                 The function to translate.
 */
const char *ShClHostFunctionToStr(uint32_t uFn)
{
    switch (uFn)
    {
        RT_CASE_RET_STR(VBOX_SHCL_HOST_FN_SET_MODE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_FN_SET_TRANSFER_MODE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_FN_SET_HEADLESS);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_FN_CANCEL);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_FN_ERROR);
    }
    return "Unknown";
}

/**
 * Translates a Shared Clipboard host message enum to a string.
 *
 * @returns Message ID string name.
 * @param   uMsg                The message to translate.
 */
const char *ShClHostMsgToStr(uint32_t uMsg)
{
    switch (uMsg)
    {
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_QUIT);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_READ_DATA);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_FORMATS_REPORT);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_CANCELED);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_READ_DATA_CID);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_STATUS);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_ROOT_LIST_HDR_READ);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_ROOT_LIST_HDR_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_ROOT_LIST_ENTRY_READ);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_ROOT_LIST_ENTRY_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_LIST_OPEN);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_LIST_CLOSE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_LIST_HDR_READ);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_LIST_HDR_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_LIST_ENTRY_READ);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_LIST_ENTRY_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_OBJ_OPEN);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_OBJ_CLOSE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_OBJ_READ);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_OBJ_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_CANCEL);
        RT_CASE_RET_STR(VBOX_SHCL_HOST_MSG_TRANSFER_ERROR);
    }
    return "Unknown";
}

/**
 * Translates a Shared Clipboard guest message enum to a string.
 *
 * @returns Message ID string name.
 * @param   uMsg                The message to translate.
 */
const char *ShClGuestMsgToStr(uint32_t uMsg)
{
    switch (uMsg)
    {
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_MSG_OLD_GET_WAIT);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_REPORT_FORMATS);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_DATA_READ);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_DATA_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_CONNECT);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_REPORT_FEATURES);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_QUERY_FEATURES);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_MSG_PEEK_NOWAIT);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_MSG_PEEK_WAIT);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_MSG_GET);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_MSG_CANCEL);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_REPLY);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_ROOT_LIST_HDR_READ);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_ROOT_LIST_HDR_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_ROOT_LIST_ENTRY_READ);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_ROOT_LIST_ENTRY_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_LIST_OPEN);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_LIST_CLOSE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_LIST_HDR_READ);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_LIST_HDR_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_LIST_ENTRY_READ);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_LIST_ENTRY_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_OBJ_OPEN);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_OBJ_CLOSE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_OBJ_READ);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_OBJ_WRITE);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_ERROR);
        RT_CASE_RET_STR(VBOX_SHCL_GUEST_FN_NEGOTIATE_CHUNK_SIZE);
    }
    return "Unknown";
}

/**
 * Converts Shared Clipboard formats to a string.
 *
 * @returns Stringified Shared Clipboard formats, or NULL on failure. Must be free'd with RTStrFree().
 * @param   fFormats            Shared Clipboard formats to convert.
 *
 */
char *ShClFormatsToStrA(SHCLFORMATS fFormats)
{
#define APPEND_FMT_TO_STR(_aFmt)                \
    if (fFormats & VBOX_SHCL_FMT_##_aFmt)       \
    {                                           \
        if (pszFmts)                            \
        {                                       \
            rc2 = RTStrAAppend(&pszFmts, ", "); \
            if (RT_FAILURE(rc2))                \
                break;                          \
        }                                       \
                                                \
        rc2 = RTStrAAppend(&pszFmts, #_aFmt);   \
        if (RT_FAILURE(rc2))                    \
            break;                              \
    }

    char *pszFmts = NULL;
    int rc2 = VINF_SUCCESS;

    do
    {
        APPEND_FMT_TO_STR(UNICODETEXT);
        APPEND_FMT_TO_STR(BITMAP);
        APPEND_FMT_TO_STR(HTML);
# ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS
        APPEND_FMT_TO_STR(URI_LIST);
# endif

    } while (0);

    if (!pszFmts)
        rc2 = RTStrAAppend(&pszFmts, "NONE");

    if (   RT_FAILURE(rc2)
        && pszFmts)
    {
        RTStrFree(pszFmts);
        pszFmts = NULL;
    }

#undef APPEND_FMT_TO_STR

    return pszFmts;
}

