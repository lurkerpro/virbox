/* $Id$ */
/** @file
 * VBox Console COM Class implementation, The Teleporter Part.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "ConsoleImpl.h"
#include "Global.h"
#include "Logging.h"
#include "ProgressImpl.h"

#include <iprt/err.h>
#include <iprt/rand.h>
#include <iprt/tcp.h>
#include <iprt/timer.h>

#include <VBox/vmapi.h>
#include <VBox/ssm.h>
#include <VBox/err.h>
#include <VBox/version.h>
#include <VBox/com/string.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Base class for the teleporter state.
 *
 * These classes are used as advanced structs, not as proper classes.
 */
class TeleporterState
{
public:
    ComPtr<Console>     mptrConsole;
    PVM                 mpVM;
    ComObjPtr<Progress> mptrProgress;
    Utf8Str             mstrPassword;
    bool const          mfIsSource;

    /** @name stream stuff
     * @{  */
    RTSOCKET            mhSocket;
    uint64_t            moffStream;
    uint32_t            mcbReadBlock;
    bool volatile       mfStopReading;
    bool volatile       mfEndOfStream;
    bool volatile       mfIOError;
    /** @} */

    TeleporterState(Console *pConsole, PVM pVM, Progress *pProgress, bool fIsSource)
        : mptrConsole(pConsole)
        , mpVM(pVM)
        , mptrProgress(pProgress)
        , mfIsSource(fIsSource)
        , mhSocket(NIL_RTSOCKET)
        , moffStream(UINT64_MAX / 2)
        , mcbReadBlock(0)
        , mfStopReading(false)
        , mfEndOfStream(false)
        , mfIOError(false)
    {
    }
};


/**
 * Teleporter state used by the source side.
 */
class TeleporterStateSrc : public TeleporterState
{
public:
    Utf8Str             mstrHostname;
    uint32_t            muPort;
    uint32_t            mcMsMaxDowntime;
    MachineState_T      menmOldMachineState;
    bool                mfSuspendedByUs;
    bool                mfUnlockedMedia;

    TeleporterStateSrc(Console *pConsole, PVM pVM, Progress *pProgress, MachineState_T enmOldMachineState)
        : TeleporterState(pConsole, pVM, pProgress, true /*fIsSource*/)
        , muPort(UINT32_MAX)
        , mcMsMaxDowntime(250)
        , menmOldMachineState(enmOldMachineState)
        , mfSuspendedByUs(false)
        , mfUnlockedMedia(false)
    {
    }
};


/**
 * Teleporter state used by the destiation side.
 */
class TeleporterStateTrg : public TeleporterState
{
public:
    IMachine                   *mpMachine;
    IInternalMachineControl    *mpControl;
    PRTTCPSERVER                mhServer;
    PRTTIMERLR                  mphTimerLR;
    bool                        mfLockedMedia;
    int                         mRc;

    TeleporterStateTrg(Console *pConsole, PVM pVM, Progress *pProgress,
                       IMachine *pMachine, IInternalMachineControl *pControl,
                       PRTTIMERLR phTimerLR, bool fStartPaused)
        : TeleporterState(pConsole, pVM, pProgress, false /*fIsSource*/)
        , mpMachine(pMachine)
        , mpControl(pControl)
        , mhServer(NULL)
        , mphTimerLR(phTimerLR)
        , mfLockedMedia(false)
        , mRc(VINF_SUCCESS)
    {
    }
};


/**
 * TCP stream header.
 *
 * This is an extra layer for fixing the problem with figuring out when the SSM
 * stream ends.
 */
typedef struct TELEPORTERTCPHDR
{
    /** Magic value. */
    uint32_t    u32Magic;
    /** The size of the data block following this header.
     * 0 indicates the end of the stream. */
    uint32_t    cb;
} TELEPORTERTCPHDR;
/** Magic value for TELEPORTERTCPHDR::u32Magic. (Egberto Gismonti Amin) */
#define TELEPORTERTCPHDR_MAGIC       UINT32_C(0x19471205)
/** The max block size. */
#define TELEPORTERTCPHDR_MAX_SIZE    UINT32_C(0x00fffff8)


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
static const char g_szWelcome[] = "VirtualBox-Teleporter-1.0\n";


/**
 * Reads a string from the socket.
 *
 * @returns VBox status code.
 *
 * @param   pState      The teleporter state structure.
 * @param   pszBuf      The output buffer.
 * @param   cchBuf      The size of the output buffer.
 *
 */
static int teleporterTcpReadLine(TeleporterState *pState, char *pszBuf, size_t cchBuf)
{
    char       *pszStart = pszBuf;
    RTSOCKET    Sock     = pState->mhSocket;

    AssertReturn(cchBuf > 1, VERR_INTERNAL_ERROR);
    *pszBuf = '\0';

    /* dead simple approach. */
    for (;;)
    {
        char ch;
        int rc = RTTcpRead(Sock, &ch, sizeof(ch), NULL);
        if (RT_FAILURE(rc))
        {
            LogRel(("Teleporter: RTTcpRead -> %Rrc while reading string ('%s')\n", rc, pszStart));
            return rc;
        }
        if (    ch == '\n'
            ||  ch == '\0')
            return VINF_SUCCESS;
        if (cchBuf <= 1)
        {
            LogRel(("Teleporter: String buffer overflow: '%s'\n", pszStart));
            return VERR_BUFFER_OVERFLOW;
        }
        *pszBuf++ = ch;
        *pszBuf = '\0';
        cchBuf--;
    }
}


/**
 * Reads an ACK or NACK.
 *
 * @returns S_OK on ACK, E_FAIL+setError() on failure or NACK.
 * @param   pState              The teleporter source state.
 * @param   pszWhich            Which ACK is this this?
 * @param   pszNAckMsg          Optional NACK message.
 *
 * @remarks the setError laziness forces this to be a Console member.
 */
HRESULT
Console::teleporterSrcReadACK(TeleporterStateSrc *pState, const char *pszWhich,
                             const char *pszNAckMsg /*= NULL*/)
{
    char szMsg[128];
    int vrc = teleporterTcpReadLine(pState, szMsg, sizeof(szMsg));
    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("Failed reading ACK(%s): %Rrc"), pszWhich, vrc);
    if (strcmp(szMsg, "ACK"))
    {
        if (!strncmp(szMsg, "NACK=", sizeof("NACK=") - 1))
        {
            int32_t vrc2;
            vrc = RTStrToInt32Full(&szMsg[sizeof("NACK=") - 1], 10, &vrc2);
            if (vrc == VINF_SUCCESS)
            {
                if (pszNAckMsg)
                {
                    LogRel(("Teleporter: %s: NACK=%Rrc (%d)\n", pszWhich, vrc2, vrc2));
                    return setError(E_FAIL, pszNAckMsg);
                }
                return setError(E_FAIL, "NACK(%s) - %Rrc (%d)", pszWhich, vrc2, vrc2);
            }
        }
        return setError(E_FAIL, tr("%s: Expected ACK or NACK, got '%s'"), pszWhich, szMsg);
    }
    return S_OK;
}


/**
 * Submitts a command to the destination and waits for the ACK.
 *
 * @returns S_OK on ACKed command, E_FAIL+setError() on failure.
 *
 * @param   pState              The teleporter source state.
 * @param   pszCommand          The command.
 * @param   fWaitForAck         Whether to wait for the ACK.
 *
 * @remarks the setError laziness forces this to be a Console member.
 */
HRESULT
Console::teleporterSrcSubmitCommand(TeleporterStateSrc *pState, const char *pszCommand, bool fWaitForAck /*= true*/)
{
    size_t cchCommand = strlen(pszCommand);
    int vrc = RTTcpWrite(pState->mhSocket, pszCommand, cchCommand);
    if (RT_SUCCESS(vrc))
        vrc = RTTcpWrite(pState->mhSocket, "\n", sizeof("\n") - 1);
    if (RT_SUCCESS(vrc))
        vrc = RTTcpFlush(pState->mhSocket);
    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("Failed writing command '%s': %Rrc"), pszCommand, vrc);
    if (!fWaitForAck)
        return S_OK;
    return teleporterSrcReadACK(pState, pszCommand);
}


/**
 * @copydoc SSMSTRMOPS::pfnWrite
 */
static DECLCALLBACK(int) teleporterTcpOpWrite(void *pvUser, uint64_t offStream, const void *pvBuf, size_t cbToWrite)
{
    TeleporterState *pState = (TeleporterState *)pvUser;

    AssertReturn(cbToWrite > 0, VINF_SUCCESS);
    AssertReturn(cbToWrite < UINT32_MAX, VERR_OUT_OF_RANGE);
    AssertReturn(pState->mfIsSource, VERR_INVALID_HANDLE);

    for (;;)
    {
        /* Write block header. */
        TELEPORTERTCPHDR Hdr;
        Hdr.u32Magic = TELEPORTERTCPHDR_MAGIC;
        Hdr.cb       = RT_MIN((uint32_t)cbToWrite, TELEPORTERTCPHDR_MAX_SIZE);
        int rc = RTTcpWrite(pState->mhSocket, &Hdr, sizeof(Hdr));
        if (RT_FAILURE(rc))
        {
            LogRel(("Teleporter/TCP: Header write error: %Rrc\n", rc));
            return rc;
        }

        /* Write the data. */
        rc = RTTcpWrite(pState->mhSocket, pvBuf, Hdr.cb);
        if (RT_FAILURE(rc))
        {
            LogRel(("Teleporter/TCP: Data write error: %Rrc (cb=%#x)\n", rc, Hdr.cb));
            return rc;
        }
        pState->moffStream += Hdr.cb;
        if (Hdr.cb == cbToWrite)
            return VINF_SUCCESS;

        /* advance */
        cbToWrite -= Hdr.cb;
        pvBuf = (uint8_t const *)pvBuf + Hdr.cb;
    }
}


/**
 * Selects and poll for close condition.
 *
 * We can use a relatively high poll timeout here since it's only used to get
 * us out of error paths.  In the normal cause of events, we'll get a
 * end-of-stream header.
 *
 * @returns VBox status code.
 *
 * @param   pState          The teleporter state data.
 */
static int teleporterTcpReadSelect(TeleporterState *pState)
{
    int rc;
    do
    {
        rc = RTTcpSelectOne(pState->mhSocket, 1000);
        if (RT_FAILURE(rc) && rc != VERR_TIMEOUT)
        {
            pState->mfIOError = true;
            LogRel(("Teleporter/TCP: Header select error: %Rrc\n", rc));
            break;
        }
        if (pState->mfStopReading)
        {
            rc = VERR_EOF;
            break;
        }
    } while (rc == VERR_TIMEOUT);
    return rc;
}


/**
 * @copydoc SSMSTRMOPS::pfnRead
 */
static DECLCALLBACK(int) teleporterTcpOpRead(void *pvUser, uint64_t offStream, void *pvBuf, size_t cbToRead, size_t *pcbRead)
{
    TeleporterState *pState = (TeleporterState *)pvUser;
    AssertReturn(!pState->mfIsSource, VERR_INVALID_HANDLE);

    for (;;)
    {
        int rc;

        /*
         * Check for various conditions and may have been signalled.
         */
        if (pState->mfEndOfStream)
            return VERR_EOF;
        if (pState->mfStopReading)
            return VERR_EOF;
        if (pState->mfIOError)
            return VERR_IO_GEN_FAILURE;

        /*
         * If there is no more data in the current block, read the next
         * block header.
         */
        if (!pState->mcbReadBlock)
        {
            rc = teleporterTcpReadSelect(pState);
            if (RT_FAILURE(rc))
                return rc;
            TELEPORTERTCPHDR Hdr;
            rc = RTTcpRead(pState->mhSocket, &Hdr, sizeof(Hdr), NULL);
            if (RT_FAILURE(rc))
            {
                pState->mfIOError = true;
                LogRel(("Teleporter/TCP: Header read error: %Rrc\n", rc));
                return rc;
            }
            if (   Hdr.u32Magic != TELEPORTERTCPHDR_MAGIC
                || Hdr.cb > TELEPORTERTCPHDR_MAX_SIZE)
            {
                pState->mfIOError = true;
                LogRel(("Teleporter/TCP: Invalid block: u32Magic=%#x cb=%#x\n", Hdr.u32Magic, Hdr.cb));
                return VERR_IO_GEN_FAILURE;
            }

            pState->mcbReadBlock = Hdr.cb;
            if (!Hdr.cb)
            {
                pState->mfEndOfStream = true;
                return VERR_EOF;
            }

            if (pState->mfStopReading)
                return VERR_EOF;
        }

        /*
         * Read more data.
         */
        rc = teleporterTcpReadSelect(pState);
        if (RT_FAILURE(rc))
            return rc;
        uint32_t cb = (uint32_t)RT_MIN(pState->mcbReadBlock, cbToRead);
        rc = RTTcpRead(pState->mhSocket, pvBuf, cb, pcbRead);
        if (RT_FAILURE(rc))
        {
            pState->mfIOError = true;
            LogRel(("Teleporter/TCP: Data read error: %Rrc (cb=%#x)\n", rc, cb));
            return rc;
        }
        if (pcbRead)
        {
            cb = (uint32_t)*pcbRead;
            pState->moffStream   += cb;
            pState->mcbReadBlock -= cb;
            return VINF_SUCCESS;
        }
        pState->moffStream   += cb;
        pState->mcbReadBlock -= cb;
        if (cbToRead == cb)
            return VINF_SUCCESS;

        /* Advance to the next block. */
        cbToRead -= cb;
        pvBuf = (uint8_t *)pvBuf + cb;
    }
}


/**
 * @copydoc SSMSTRMOPS::pfnSeek
 */
static DECLCALLBACK(int) teleporterTcpOpSeek(void *pvUser, int64_t offSeek, unsigned uMethod, uint64_t *poffActual)
{
    return VERR_NOT_SUPPORTED;
}


/**
 * @copydoc SSMSTRMOPS::pfnTell
 */
static DECLCALLBACK(uint64_t) teleporterTcpOpTell(void *pvUser)
{
    TeleporterState *pState = (TeleporterState *)pvUser;
    return pState->moffStream;
}


/**
 * @copydoc SSMSTRMOPS::pfnSize
 */
static DECLCALLBACK(int) teleporterTcpOpSize(void *pvUser, uint64_t *pcb)
{
    return VERR_NOT_SUPPORTED;
}


/**
 * @copydoc SSMSTRMOPS::pfnIsOk
 */
static DECLCALLBACK(int) teleporterTcpOpIsOk(void *pvUser)
{
    TeleporterState *pState = (TeleporterState *)pvUser;

    if (pState->mfIsSource)
    {
        /* Poll for incoming NACKs and errors from the other side */
        int rc = RTTcpSelectOne(pState->mhSocket, 0);
        if (rc != VERR_TIMEOUT)
        {
            if (RT_SUCCESS(rc))
            {
                LogRel(("Teleporter/TCP: Incoming data detect by IsOk, assuming it is a cancellation NACK.\n"));
                rc = VERR_SSM_CANCELLED;
            }
            else
                LogRel(("Teleporter/TCP: RTTcpSelectOne -> %Rrc (IsOk).\n", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


/**
 * @copydoc SSMSTRMOPS::pfnClose
 */
static DECLCALLBACK(int) teleporterTcpOpClose(void *pvUser)
{
    TeleporterState *pState = (TeleporterState *)pvUser;

    if (pState->mfIsSource)
    {
        TELEPORTERTCPHDR EofHdr = { TELEPORTERTCPHDR_MAGIC, 0 };
        int rc = RTTcpWrite(pState->mhSocket, &EofHdr, sizeof(EofHdr));
        if (RT_SUCCESS(rc))
            rc = RTTcpFlush(pState->mhSocket);
        if (RT_FAILURE(rc))
        {
            LogRel(("Teleporter/TCP: EOF Header write error: %Rrc\n", rc));
            return rc;
        }
    }
    else
    {
        ASMAtomicWriteBool(&pState->mfStopReading, true);
        RTTcpFlush(pState->mhSocket);
    }

    return VINF_SUCCESS;
}


/**
 * Method table for a TCP based stream.
 */
static SSMSTRMOPS const g_teleporterTcpOps =
{
    SSMSTRMOPS_VERSION,
    teleporterTcpOpWrite,
    teleporterTcpOpRead,
    teleporterTcpOpSeek,
    teleporterTcpOpTell,
    teleporterTcpOpSize,
    teleporterTcpOpIsOk,
    teleporterTcpOpClose,
    SSMSTRMOPS_VERSION
};


/**
 * Progress cancelation callback.
 */
static void teleporterProgressCancelCallback(void *pvUser)
{
    TeleporterState *pState = (TeleporterState *)pvUser;
    SSMR3Cancel(pState->mpVM);
    if (!pState->mfIsSource)
    {
        TeleporterStateTrg *pStateTrg = (TeleporterStateTrg *)pState;
        RTTcpServerShutdown(pStateTrg->mhServer);
    }
}

/**
 * @copydoc PFNVMPROGRESS
 */
static DECLCALLBACK(int) teleporterProgressCallback(PVM pVM, unsigned uPercent, void *pvUser)
{
    TeleporterState *pState = (TeleporterState *)pvUser;
    if (pState->mptrProgress)
    {
        HRESULT hrc = pState->mptrProgress->SetCurrentOperationProgress(uPercent);
        if (FAILED(hrc))
        {
            /* check if the failure was caused by cancellation. */
            BOOL fCancelled;
            hrc = pState->mptrProgress->COMGETTER(Canceled)(&fCancelled);
            if (SUCCEEDED(hrc) && fCancelled)
            {
                SSMR3Cancel(pState->mpVM);
                return VERR_SSM_CANCELLED;
            }
        }
    }

    return VINF_SUCCESS;
}


/**
 * @copydoc FNRTTIMERLR
 */
static DECLCALLBACK(void) teleporterDstTimeout(RTTIMERLR hTimerLR, void *pvUser, uint64_t iTick)
{
    /* This is harmless for any open connections. */
    RTTcpServerShutdown((PRTTCPSERVER)pvUser);
}


/**
 * Do the teleporter.
 *
 * @returns VBox status code.
 * @param   pState              The teleporter state.
 */
HRESULT
Console::teleporterSrc(TeleporterStateSrc *pState)
{
    AutoCaller autoCaller(this);
    CheckComRCReturnRC(autoCaller.rc());

    /*
     * Wait for Console::Teleport to change the state.
     */
    { AutoWriteLock autoLock(this); }

    BOOL fCancelled = TRUE;
    HRESULT hrc = pState->mptrProgress->COMGETTER(Canceled)(&fCancelled);
    if (FAILED(hrc))
        return hrc;
    if (fCancelled)
        return setError(E_FAIL, tr("cancelled"));

    /*
     * Try connect to the destination machine.
     * (Note. The caller cleans up mhSocket, so we can return without worries.)
     */
    int vrc = RTTcpClientConnect(pState->mstrHostname.c_str(), pState->muPort, &pState->mhSocket);
    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("Failed to connect to port %u on '%s': %Rrc"),
                        pState->muPort, pState->mstrHostname.c_str(), vrc);

    /* Read and check the welcome message. */
    char szLine[RT_MAX(128, sizeof(g_szWelcome))];
    RT_ZERO(szLine);
    vrc = RTTcpRead(pState->mhSocket, szLine, sizeof(g_szWelcome) - 1, NULL);
    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("Failed to read welcome message: %Rrc"), vrc);
    if (strcmp(szLine, g_szWelcome))
        return setError(E_FAIL, tr("Unexpected welcome %.*Rhxs"), sizeof(g_szWelcome) - 1, szLine);

    /* password */
    pState->mstrPassword.append('\n');
    vrc = RTTcpWrite(pState->mhSocket, pState->mstrPassword.c_str(), pState->mstrPassword.length());
    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("Failed to send password: %Rrc"), vrc);

    /* ACK */
    hrc = teleporterSrcReadACK(pState, "password", tr("Invalid password"));
    if (FAILED(hrc))
        return hrc;

    /*
     * Start loading the state.
     *
     * Note! The saved state includes vital configuration data which will be
     *       verified against the VM config on the other end.  This is all done
     *       in the first pass, so we should fail pretty promptly on misconfig.
     */
    hrc = teleporterSrcSubmitCommand(pState, "load");
    if (FAILED(hrc))
        return hrc;

    void *pvUser = static_cast<void *>(static_cast<TeleporterState *>(pState));
    vrc = VMR3Teleport(pState->mpVM, pState->mcMsMaxDowntime,
                       &g_teleporterTcpOps, pvUser,
                       teleporterProgressCallback, pvUser,
                       &pState->mfSuspendedByUs);
    if (RT_FAILURE(vrc))
        return setError(E_FAIL, tr("VMR3Teleport -> %Rrc"), vrc);

    hrc = teleporterSrcReadACK(pState, "load-complete");
    if (FAILED(hrc))
        return hrc;

    /*
     * We're at the point of no return.
     */
    if (!pState->mptrProgress->notifyPointOfNoReturn())
    {
        teleporterSrcSubmitCommand(pState, "cancel", false /*fWaitForAck*/);
        return E_FAIL;
    }

    /*
     * Hand over any media which we might be sharing.
     *
     * Note! This is only important on localhost teleportations.
     */
    /** @todo Maybe we should only do this if it's a local teleportation... */
    hrc = mControl->UnlockMedia();
    if (FAILED(hrc))
        return hrc;
    pState->mfUnlockedMedia = true;

    hrc = teleporterSrcSubmitCommand(pState, "lock-media");
    if (FAILED(hrc))
        return hrc;

    /*
     * The FINAL step is giving the target instructions how to proceed with the VM.
     */
    if (    vrc == VINF_SSM_LIVE_SUSPENDED
        ||  pState->menmOldMachineState == MachineState_Paused)
        hrc = teleporterSrcSubmitCommand(pState, "hand-over-paused");
    else
        hrc = teleporterSrcSubmitCommand(pState, "hand-over-resume");
    if (FAILED(hrc))
        return hrc;

    /*
     * teleporterSrcThreadWrapper will do the automatic power off because it
     * has to release the AutoVMCaller.
     */
    return S_OK;
}


/**
 * Static thread method wrapper.
 *
 * @returns VINF_SUCCESS (ignored).
 * @param   hThread             The thread.
 * @param   pvUser              Pointer to a TeleporterStateSrc instance.
 */
/*static*/ DECLCALLBACK(int)
Console::teleporterSrcThreadWrapper(RTTHREAD hThread, void *pvUser)
{
    TeleporterStateSrc *pState = (TeleporterStateSrc *)pvUser;

    /*
     * Console::teleporterSrc does the work, we just grab onto the VM handle
     * and do the cleanups afterwards.
     */
    AutoVMCaller autoVMCaller(pState->mptrConsole);
    HRESULT hrc = autoVMCaller.rc();

    if (SUCCEEDED(hrc))
        hrc = pState->mptrConsole->teleporterSrc(pState);

    /* Close the connection ASAP on so that the other side can complete. */
    if (pState->mhSocket != NIL_RTSOCKET)
    {
        RTTcpClientClose(pState->mhSocket);
        pState->mhSocket = NIL_RTSOCKET;
    }

    /* Aaarg! setMachineState trashes error info on Windows, so we have to
       complete things here on failure instead of right before cleanup. */
    if (FAILED(hrc))
        pState->mptrProgress->notifyComplete(hrc);

    /* We can no longer be cancelled (success), or it doesn't matter any longer (failure). */
    pState->mptrProgress->setCancelCallback(NULL, NULL);

    /*
     * Write lock the console before resetting mptrCancelableProgress and
     * fixing the state.
     */
    AutoWriteLock autoLock(pState->mptrConsole);
    pState->mptrConsole->mptrCancelableProgress.setNull();

    VMSTATE const        enmVMState      = VMR3GetState(pState->mpVM);
    MachineState_T const enmMachineState = pState->mptrConsole->mMachineState;
    if (SUCCEEDED(hrc))
    {
        /*
         * Automatically shut down the VM on success.
         *
         * Note! We have to release the VM caller object or we'll deadlock in
         *       powerDown.
         */
        AssertLogRelMsg(enmVMState == VMSTATE_SUSPENDED, ("%s\n", VMR3GetStateName(enmVMState)));
        AssertLogRelMsg(enmMachineState == MachineState_TeleportingPausedVM, ("%s\n", Global::stringifyMachineState(enmMachineState)));

        autoVMCaller.release();
        hrc = pState->mptrConsole->powerDown();
        pState->mptrProgress->notifyComplete(hrc);
    }
    else
    {
        /*
         * Work the state machinery on failure.
         *
         * If the state is no longer 'Teleporting*', some other operation has
         * canceled us and there is nothing we need to do here.  In all other
         * cases, we've failed one way or another.
         */
        if (   enmMachineState == MachineState_Teleporting
            || enmMachineState == MachineState_TeleportingPausedVM
           )
        {
            if (pState->mfUnlockedMedia)
            {
                ErrorInfoKeeper Oak;
                HRESULT hrc2 = pState->mptrConsole->mControl->LockMedia();
                if (FAILED(hrc2))
                {
                    uint64_t StartMS = RTTimeMilliTS();
                    do
                    {
                        RTThreadSleep(2);
                        hrc2 = pState->mptrConsole->mControl->LockMedia();
                    } while (   FAILED(hrc2)
                             && RTTimeMilliTS() - StartMS < 2000);
                }
                if (SUCCEEDED(hrc2))
                    pState->mfUnlockedMedia = true;
                else
                    LogRel(("FATAL ERROR: Failed to re-take the media locks. hrc2=%Rhrc\n", hrc2));
            }

            switch (enmVMState)
            {
                case VMSTATE_RUNNING:
                case VMSTATE_RUNNING_LS:
                case VMSTATE_DEBUGGING:
                case VMSTATE_DEBUGGING_LS:
                case VMSTATE_POWERING_OFF:
                case VMSTATE_POWERING_OFF_LS:
                case VMSTATE_RESETTING:
                case VMSTATE_RESETTING_LS:
                    Assert(!pState->mfSuspendedByUs);
                    Assert(!pState->mfUnlockedMedia);
                    pState->mptrConsole->setMachineState(MachineState_Running);
                    break;

                case VMSTATE_GURU_MEDITATION:
                case VMSTATE_GURU_MEDITATION_LS:
                    pState->mptrConsole->setMachineState(MachineState_Stuck);
                    break;

                case VMSTATE_FATAL_ERROR:
                case VMSTATE_FATAL_ERROR_LS:
                    pState->mptrConsole->setMachineState(MachineState_Paused);
                    break;

                default:
                    AssertMsgFailed(("%s\n", VMR3GetStateName(enmVMState)));
                case VMSTATE_SUSPENDED:
                case VMSTATE_SUSPENDED_LS:
                case VMSTATE_SUSPENDING:
                case VMSTATE_SUSPENDING_LS:
                case VMSTATE_SUSPENDING_EXT_LS:
                    if (!pState->mfUnlockedMedia)
                    {
                        pState->mptrConsole->setMachineState(MachineState_Paused);
                        if (pState->mfSuspendedByUs)
                        {
                            autoLock.leave();
                            int rc = VMR3Resume(pState->mpVM);
                            AssertLogRelMsgRC(rc, ("VMR3Resume -> %Rrc\n", rc));
                            autoLock.enter();
                        }
                    }
                    else
                    {
                        /* Faking a guru meditation is the best I can think of doing here... */
                        pState->mptrConsole->setMachineState(MachineState_Stuck);
                    }
                    break;
            }
        }
    }
    autoLock.leave();

    /*
     * Cleanup.
     */
    Assert(pState->mhSocket == NIL_RTSOCKET);
    delete pState;

    return VINF_SUCCESS; /* ignored */
}


/**
 * Start teleporter to the specified target.
 *
 * @returns COM status code.
 *
 * @param   aHostname       The name of the target host.
 * @param   aPort           The TCP port number.
 * @param   aPassword       The password.
 * @param   aMaxDowntime    Max allowed "downtime" in milliseconds.
 * @param   aProgress       Where to return the progress object.
 */
STDMETHODIMP
Console::Teleport(IN_BSTR aHostname, ULONG aPort, IN_BSTR aPassword, ULONG aMaxDowntime, IProgress **aProgress)
{
    /*
     * Validate parameters, check+hold object status, write lock the object
     * and validate the state.
     */
    CheckComArgOutPointerValid(aProgress);
    CheckComArgStrNotEmptyOrNull(aHostname);
    CheckComArgNotNull(aHostname);
    CheckComArgExprMsg(aPort, aPort > 0 && aPort <= 65535, ("is %u", aPort));
    CheckComArgExprMsg(aMaxDowntime, aMaxDowntime > 0, ("is %u", aMaxDowntime));

    AutoCaller autoCaller(this);
    CheckComRCReturnRC(autoCaller.rc());

    AutoWriteLock autoLock(this);
    LogFlowThisFunc(("mMachineState=%d\n", mMachineState));

    switch (mMachineState)
    {
        case MachineState_Running:
        case MachineState_Paused:
            break;

        default:
            return setError(VBOX_E_INVALID_VM_STATE,
                tr("Invalid machine state: %s (must be Running or Paused)"),
                Global::stringifyMachineState(mMachineState));
    }


    /*
     * Create a progress object, spawn a worker thread and change the state.
     * Note! The thread won't start working until we release the lock.
     */
    LogFlowThisFunc(("Initiating TELEPORT request...\n"));

    ComObjPtr<Progress> ptrProgress;
    HRESULT hrc = ptrProgress.createObject();
    CheckComRCReturnRC(hrc);
    hrc = ptrProgress->init(static_cast<IConsole *>(this), Bstr(tr("Teleporter")), TRUE /*aCancelable*/);
    CheckComRCReturnRC(hrc);

    TeleporterStateSrc *pState = new TeleporterStateSrc(this, mpVM, ptrProgress, mMachineState);
    pState->mstrPassword    = aPassword;
    pState->mstrHostname    = aHostname;
    pState->muPort          = aPort;
    pState->mcMsMaxDowntime = aMaxDowntime;

    void *pvUser = static_cast<void *>(static_cast<TeleporterState *>(pState));
    ptrProgress->setCancelCallback(teleporterProgressCancelCallback, pvUser);

    int vrc = RTThreadCreate(NULL, Console::teleporterSrcThreadWrapper, (void *)pState, 0 /*cbStack*/,
                             RTTHREADTYPE_EMULATION, 0 /*fFlags*/, "Teleport");
    if (RT_SUCCESS(vrc))
    {
        if (mMachineState == MachineState_Running)
            hrc = setMachineState(MachineState_Teleporting);
        else
            hrc = setMachineState(MachineState_TeleportingPausedVM);
        if (SUCCEEDED(hrc))
        {
            ptrProgress.queryInterfaceTo(aProgress);
            mptrCancelableProgress = ptrProgress;
        }
        else
            ptrProgress->Cancel();
    }
    else
    {
        ptrProgress->setCancelCallback(NULL, NULL);
        delete pState;
        hrc = setError(E_FAIL, tr("RTThreadCreate -> %Rrc"), vrc);
    }

    return hrc;
}


/**
 * Creates a TCP server that listens for the source machine and passes control
 * over to Console::teleporterTrgServeConnection().
 *
 * @returns VBox status code.
 * @param   pVM                 The VM handle
 * @param   pMachine            The IMachine for the virtual machine.
 * @param   fStartPaused        Whether to start it in the Paused (true) or
 *                              Running (false) state,
 * @param   pProgress           Pointer to the progress object.
 *
 * @remarks The caller expects error information to be set on failure.
 * @todo    Check that all the possible failure paths sets error info...
 */
int
Console::teleporterTrg(PVM pVM, IMachine *pMachine, bool fStartPaused, Progress *pProgress)
{
    /*
     * Get the config.
     */
    ULONG uPort;
    HRESULT hrc = pMachine->COMGETTER(TeleporterPort)(&uPort);
    if (FAILED(hrc))
        return VERR_GENERAL_FAILURE;
    ULONG const uPortOrg = uPort;

    Bstr bstrAddress;
    hrc = pMachine->COMGETTER(TeleporterAddress)(bstrAddress.asOutParam());
    if (FAILED(hrc))
        return VERR_GENERAL_FAILURE;
    Utf8Str strAddress(bstrAddress);
    const char *pszAddress = strAddress.isEmpty() ? NULL : strAddress.c_str();

    Bstr bstrPassword;
    hrc = pMachine->COMGETTER(TeleporterPassword)(bstrPassword.asOutParam());
    if (FAILED(hrc))
        return VERR_GENERAL_FAILURE;
    Utf8Str strPassword(bstrPassword);
    strPassword.append('\n');           /* To simplify password checking. */

    /*
     * Create the TCP server.
     */
    int vrc;
    PRTTCPSERVER hServer;
    if (uPort)
        vrc = RTTcpServerCreateEx(pszAddress, uPort, &hServer);
    else
    {
        for (int cTries = 10240; cTries > 0; cTries--)
        {
            uPort = RTRandU32Ex(cTries >= 8192 ? 49152 : 1024, 65534);
            vrc = RTTcpServerCreateEx(pszAddress, uPort, &hServer);
            if (vrc != VERR_NET_ADDRESS_IN_USE)
                break;
        }
        if (RT_SUCCESS(vrc))
        {
            hrc = pMachine->COMSETTER(TeleporterPort)(uPort);
            if (FAILED(hrc))
            {
                RTTcpServerDestroy(hServer);
                return VERR_GENERAL_FAILURE;
            }
        }
    }
    if (RT_FAILURE(vrc))
        return vrc;

    /*
     * Create a one-shot timer for timing out after 5 mins.
     */
    RTTIMERLR hTimerLR;
    vrc = RTTimerLRCreateEx(&hTimerLR, 0 /*ns*/, RTTIMER_FLAGS_CPU_ANY, teleporterDstTimeout, hServer);
    if (RT_SUCCESS(vrc))
    {
        vrc = RTTimerLRStart(hTimerLR, 5*60*UINT64_C(1000000000) /*ns*/);
        if (RT_SUCCESS(vrc))
        {
            /*
             * Do the job, when it returns we're done.
             */
            TeleporterStateTrg State(this, pVM, pProgress, pMachine, mControl, &hTimerLR, fStartPaused);
            State.mstrPassword      = strPassword;
            State.mhServer          = hServer;

            void *pvUser = static_cast<void *>(static_cast<TeleporterState *>(&State));
            if (pProgress->setCancelCallback(teleporterProgressCancelCallback, pvUser))
            {
                LogRel(("Teleporter: Waiting for incoming VM...\n"));
                vrc = RTTcpServerListen(hServer, Console::teleporterTrgServeConnection, &State);
                pProgress->setCancelCallback(NULL, NULL);

                bool fPowerOff = false;
                if (vrc == VERR_TCP_SERVER_STOP)
                {
                    vrc = State.mRc;
                    /* Power off the VM on failure unless the state callback
                       already did that. */
                    if (RT_FAILURE(vrc))
                    {
                        VMSTATE enmVMState = VMR3GetState(pVM);
                        if (    enmVMState != VMSTATE_OFF
                            &&  enmVMState != VMSTATE_POWERING_OFF)
                            fPowerOff = true;
                    }
                }
                else if (vrc == VERR_TCP_SERVER_SHUTDOWN)
                {
                    BOOL fCancelled = TRUE;
                    hrc = pProgress->COMGETTER(Canceled)(&fCancelled);
                    if (FAILED(hrc) || fCancelled)
                    {
                        setError(E_FAIL, tr("Teleporting canceled"));
                        vrc = VERR_SSM_CANCELLED;
                    }
                    else
                    {
                        setError(E_FAIL, tr("Teleporter timed out waiting for incoming connection"));
                        vrc = VERR_TIMEOUT;
                    }
                    LogRel(("Teleporter: RTTcpServerListen aborted - %Rrc\n", vrc));
                    fPowerOff = true;
                }
                else
                {
                    LogRel(("Teleporter: Unexpected RTTcpServerListen rc: %Rrc\n", vrc));
                    vrc = VERR_IPE_UNEXPECTED_STATUS;
                    fPowerOff = true;
                }

                if (fPowerOff)
                {
                    int vrc2 = VMR3PowerOff(pVM);
                    AssertRC(vrc2);
                }
            }
            else
                vrc = VERR_SSM_CANCELLED;
        }

        RTTimerLRDestroy(hTimerLR);
    }
    RTTcpServerDestroy(hServer);

    /*
     * If we change TeleporterPort above, set it back to it's original
     * value before returning.
     */
    if (uPortOrg != uPort)
        pMachine->COMSETTER(TeleporterPort)(uPortOrg);

    return vrc;
}


/**
 * Unlock the media.
 *
 * This is used in error paths.
 *
 * @param  pState           The teleporter state.
 */
static void teleporterTrgUnlockMedia(TeleporterStateTrg *pState)
{
    if (pState->mfLockedMedia)
    {
        pState->mpControl->UnlockMedia();
        pState->mfLockedMedia = false;
    }
}


static int teleporterTcpWriteACK(TeleporterStateTrg *pState, bool fAutomaticUnlock = true)
{
    int rc = RTTcpWrite(pState->mhSocket, "ACK\n", sizeof("ACK\n") - 1);
    if (RT_FAILURE(rc))
    {
        LogRel(("Teleporter: RTTcpWrite(,ACK,) -> %Rrc\n", rc));
        if (fAutomaticUnlock)
            teleporterTrgUnlockMedia(pState);
    }
    RTTcpFlush(pState->mhSocket);
    return rc;
}


static int teleporterTcpWriteNACK(TeleporterStateTrg *pState, int32_t rc2)
{
    /*
     * Unlock media sending the NACK. That way the other doesn't have to spin
     * waiting to regain the locks.
     */
    teleporterTrgUnlockMedia(pState);

    char    szMsg[64];
    size_t  cch = RTStrPrintf(szMsg, sizeof(szMsg), "NACK=%d\n", rc2);
    int rc = RTTcpWrite(pState->mhSocket, szMsg, cch);
    if (RT_FAILURE(rc))
        LogRel(("Teleporter: RTTcpWrite(,%s,%zu) -> %Rrc\n", szMsg, cch, rc));
    RTTcpFlush(pState->mhSocket);
    return rc;
}


/**
 * @copydoc FNRTTCPSERVE
 *
 * @returns VINF_SUCCESS or VERR_TCP_SERVER_STOP.
 */
/*static*/ DECLCALLBACK(int)
Console::teleporterTrgServeConnection(RTSOCKET Sock, void *pvUser)
{
    TeleporterStateTrg *pState = (TeleporterStateTrg *)pvUser;
    pState->mhSocket = Sock;

    /*
     * Say hello.
     */
    int vrc = RTTcpWrite(Sock, g_szWelcome, sizeof(g_szWelcome) - 1);
    if (RT_FAILURE(vrc))
    {
        LogRel(("Teleporter: Failed to write welcome message: %Rrc\n", vrc));
        return VINF_SUCCESS;
    }

    /*
     * Password (includes '\n', see teleporterTrg).
     */
    const char *pszPassword = pState->mstrPassword.c_str();
    unsigned    off = 0;
    while (pszPassword[off])
    {
        char ch;
        vrc = RTTcpRead(Sock, &ch, sizeof(ch), NULL);
        if (    RT_FAILURE(vrc)
            ||  pszPassword[off] != ch)
        {
            if (RT_FAILURE(vrc))
                LogRel(("Teleporter: Password read failure (off=%u): %Rrc\n", off, vrc));
            else
                LogRel(("Teleporter: Invalid password (off=%u)\n", off));
            teleporterTcpWriteNACK(pState, VERR_AUTHENTICATION_FAILURE);
            return VINF_SUCCESS;
        }
        off++;
    }
    vrc = teleporterTcpWriteACK(pState);
    if (RT_FAILURE(vrc))
        return VINF_SUCCESS;
    LogRel(("Teleporter: Incoming VM!\n"));

    /*
     * Stop the server and cancel the timeout timer.
     *
     * Note! After this point we must return VERR_TCP_SERVER_STOP, while prior
     *       to it we must not return that value!
     */
    RTTcpServerShutdown(pState->mhServer);
    RTTimerLRDestroy(*pState->mphTimerLR);
    *pState->mphTimerLR = NIL_RTTIMERLR;

    /*
     * Command processing loop.
     */
    bool fDone = false;
    for (;;)
    {
        char szCmd[128];
        vrc = teleporterTcpReadLine(pState, szCmd, sizeof(szCmd));
        if (RT_FAILURE(vrc))
            break;

        if (!strcmp(szCmd, "load"))
        {
            vrc = teleporterTcpWriteACK(pState);
            if (RT_FAILURE(vrc))
                break;

            pState->moffStream = 0;
            void *pvUser = static_cast<void *>(static_cast<TeleporterState *>(pState));
            vrc = VMR3LoadFromStream(pState->mpVM, &g_teleporterTcpOps, pvUser,
                                     teleporterProgressCallback, pvUser);
            if (RT_FAILURE(vrc))
            {
                LogRel(("Teleporter: VMR3LoadFromStream -> %Rrc\n", vrc));
                teleporterTcpWriteNACK(pState, vrc);
                break;
            }

            /* The EOS might not have been read, make sure it is. */
            pState->mfStopReading = false;
            size_t cbRead;
            vrc = teleporterTcpOpRead(pvUser, pState->moffStream, szCmd, 1, &cbRead);
            if (vrc != VERR_EOF)
            {
                LogRel(("Teleporter: Draining teleporterTcpOpRead -> %Rrc\n", vrc));
                teleporterTcpWriteNACK(pState, vrc);
                break;
            }

            vrc = teleporterTcpWriteACK(pState);
        }
        else if (!strcmp(szCmd, "cancel"))
        {
            /* Don't ACK this. */
            LogRel(("Teleporter: Received cancel command.\n"));
            vrc = VERR_SSM_CANCELLED;
        }
        else if (!strcmp(szCmd, "lock-media"))
        {
            HRESULT hrc = pState->mpControl->LockMedia();
            if (SUCCEEDED(hrc))
            {
                pState->mfLockedMedia = true;
                vrc = teleporterTcpWriteACK(pState);
            }
            else
            {
                vrc = VERR_FILE_LOCK_FAILED;
                teleporterTcpWriteNACK(pState, vrc);
            }
        }
        else if (   !strcmp(szCmd, "hand-over-resume")
                 || !strcmp(szCmd, "hand-over-paused"))
        {
            /*
             * Point of no return.
             *
             * Note! Since we cannot tell whether a VMR3Resume failure is
             *       destructive for the source or not, we have little choice
             *       but to ACK it first and take any failures locally.
             *
             *       Ideally, we should try resume it first and then ACK (or
             *       NACK) the request since this would reduce latency and
             *       make it possible to recover from some VMR3Resume failures.
             */
            if (   pState->mptrProgress->notifyPointOfNoReturn()
                && pState->mfLockedMedia)
            {
                vrc = teleporterTcpWriteACK(pState);
                if (RT_SUCCESS(vrc))
                {
                    if (!strcmp(szCmd, "hand-over-resume"))
                        vrc = VMR3Resume(pState->mpVM);
                    else
                        pState->mptrConsole->setMachineState(MachineState_Paused);
                    fDone = true;
                    break;
                }
            }
            else
            {
                vrc = pState->mfLockedMedia ? VERR_WRONG_ORDER : VERR_SSM_CANCELLED;
                teleporterTcpWriteNACK(pState, vrc);
            }
        }
        else
        {
            LogRel(("Teleporter: Unknown command '%s' (%.*Rhxs)\n", szCmd, strlen(szCmd), szCmd));
            vrc = VERR_NOT_IMPLEMENTED;
            teleporterTcpWriteNACK(pState, vrc);
        }

        if (RT_FAILURE(vrc))
            break;
    }

    if (RT_SUCCESS(vrc) && !fDone)
        vrc = VERR_WRONG_ORDER;
    if (RT_FAILURE(vrc))
        teleporterTrgUnlockMedia(pState);

    pState->mRc = vrc;
    pState->mhSocket = NIL_RTSOCKET;
    LogFlowFunc(("returns mRc=%Rrc\n", vrc));
    return VERR_TCP_SERVER_STOP;
}

