/* $Id$ */
/** @file
 * Validation Kit Audio Test (VKAT) - Self test code.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/ctype.h>
#include <iprt/errcore.h>
#include <iprt/getopt.h>
#include <iprt/message.h>
#include <iprt/test.h>

#include "Audio/AudioHlp.h"
#include "Audio/AudioTest.h"
#include "Audio/AudioTestService.h"
#include "Audio/AudioTestServiceClient.h"

#include "vkatInternal.h"


/*********************************************************************************************************************************
*   Command: play                                                                                                                *
*********************************************************************************************************************************/

/**
 * Worker for audioTestPlayOne implementing the play loop.
 */
static RTEXITCODE audioTestPlayOneInner(PAUDIOTESTDRVMIXSTREAM pMix, PAUDIOTESTWAVEFILE pWaveFile,
                                        PCPDMAUDIOSTREAMCFG pCfgAcq, const char *pszFile)
{
    uint32_t const  cbPreBuffer        = PDMAudioPropsFramesToBytes(pMix->pProps, pCfgAcq->Backend.cFramesPreBuffering);
    uint64_t const  nsStarted          = RTTimeNanoTS();
    uint64_t        nsDonePreBuffering = 0;

    /*
     * Transfer data as quickly as we're allowed.
     */
    uint8_t         abSamples[16384];
    uint32_t const  cbSamplesAligned = PDMAudioPropsFloorBytesToFrame(pMix->pProps, sizeof(abSamples));
    uint64_t        offStream        = 0;
    while (!g_fTerminate)
    {
        /* Read a chunk from the wave file. */
        size_t      cbSamples = 0;
        int rc = AudioTestWaveFileRead(pWaveFile, abSamples, cbSamplesAligned, &cbSamples);
        if (RT_SUCCESS(rc) && cbSamples > 0)
        {
            /* Pace ourselves a little. */
            if (offStream >= cbPreBuffer)
            {
                if (!nsDonePreBuffering)
                    nsDonePreBuffering = RTTimeNanoTS();
                uint64_t const cNsWritten = PDMAudioPropsBytesToNano64(pMix->pProps, offStream - cbPreBuffer);
                uint64_t const cNsElapsed = RTTimeNanoTS() - nsStarted;
                if (cNsWritten > cNsElapsed + RT_NS_10MS)
                    RTThreadSleep((cNsWritten - cNsElapsed - RT_NS_10MS / 2) / RT_NS_1MS);
            }

            /* Transfer the data to the audio stream. */
            for (uint32_t offSamples = 0; offSamples < cbSamples;)
            {
                uint32_t const cbCanWrite = AudioTestMixStreamGetWritable(pMix);
                if (cbCanWrite > 0)
                {
                    uint32_t const cbToPlay = RT_MIN(cbCanWrite, (uint32_t)cbSamples - offSamples);
                    uint32_t       cbPlayed = 0;
                    rc = AudioTestMixStreamPlay(pMix, &abSamples[offSamples], cbToPlay, &cbPlayed);
                    if (RT_SUCCESS(rc))
                    {
                        if (cbPlayed)
                        {
                            offSamples += cbPlayed;
                            offStream  += cbPlayed;
                        }
                        else
                            return RTMsgErrorExitFailure("Played zero bytes - %#x bytes reported playable!\n", cbCanWrite);
                    }
                    else
                        return RTMsgErrorExitFailure("Failed to play %#x bytes: %Rrc\n", cbToPlay, rc);
                }
                else if (AudioTestMixStreamIsOkay(pMix))
                    RTThreadSleep(RT_MIN(RT_MAX(1, pCfgAcq->Device.cMsSchedulingHint), 256));
                else
                    return RTMsgErrorExitFailure("Stream is not okay!\n");
            }
        }
        else if (RT_SUCCESS(rc) && cbSamples == 0)
            break;
        else
            return RTMsgErrorExitFailure("Error reading wav file '%s': %Rrc", pszFile, rc);
    }

    /*
     * Drain the stream.
     */
    if (g_uVerbosity > 0)
        RTMsgInfo("%'RU64 ns: Draining...\n", RTTimeNanoTS() - nsStarted);
    int rc = AudioTestMixStreamDrain(pMix, true /*fSync*/);
    if (RT_SUCCESS(rc))
    {
        if (g_uVerbosity > 0)
            RTMsgInfo("%'RU64 ns: Done\n", RTTimeNanoTS() - nsStarted);
    }
    else
        return RTMsgErrorExitFailure("Draining failed: %Rrc", rc);

    return RTEXITCODE_SUCCESS;
}


/**
 * Worker for audioTestCmdPlayHandler that plays one file.
 */
RTEXITCODE audioTestPlayOne(const char *pszFile, PCPDMDRVREG pDrvReg, const char *pszDevId, uint32_t cMsBufferSize,
                            uint32_t cMsPreBuffer, uint32_t cMsSchedulingHint,
                            uint8_t cChannels, uint8_t cbSample, uint32_t uHz,
                            bool fWithDrvAudio, bool fWithMixer)
{
    char szTmp[128];

    /*
     * First we must open the file and determin the format.
     */
    RTERRINFOSTATIC ErrInfo;
    AUDIOTESTWAVEFILE WaveFile;
    int rc = AudioTestWaveFileOpen(pszFile, &WaveFile, RTErrInfoInitStatic(&ErrInfo));
    if (RT_FAILURE(rc))
        return RTMsgErrorExitFailure("Failed to open '%s': %Rrc%#RTeim", pszFile, rc, &ErrInfo.Core);

    if (g_uVerbosity > 0)
    {
        RTMsgInfo("Opened '%s' for playing\n", pszFile);
        RTMsgInfo("Format: %s\n", PDMAudioPropsToString(&WaveFile.Props, szTmp, sizeof(szTmp)));
        RTMsgInfo("Size:   %'RU32 bytes / %#RX32 / %'RU32 frames / %'RU64 ns\n",
                  WaveFile.cbSamples, WaveFile.cbSamples,
                  PDMAudioPropsBytesToFrames(&WaveFile.Props, WaveFile.cbSamples),
                  PDMAudioPropsBytesToNano(&WaveFile.Props, WaveFile.cbSamples));
    }

    /*
     * Construct the driver stack.
     */
    RTEXITCODE          rcExit = RTEXITCODE_FAILURE;
    AUDIOTESTDRVSTACK   DrvStack;
    rc = audioTestDriverStackInit(&DrvStack, pDrvReg, fWithDrvAudio);
    if (RT_SUCCESS(rc))
    {
        /*
         * Set the output device if one is specified.
         */
        rc = audioTestDriverStackSetDevice(&DrvStack, PDMAUDIODIR_OUT, pszDevId);
        if (RT_SUCCESS(rc))
        {
            /*
             * Open a stream for the output.
             */
            PDMAUDIOPCMPROPS ReqProps = WaveFile.Props;
            if (cChannels != 0 && PDMAudioPropsChannels(&ReqProps) != cChannels)
                PDMAudioPropsSetChannels(&ReqProps, cChannels);
            if (cbSample != 0)
                PDMAudioPropsSetSampleSize(&ReqProps, cbSample);
            if (uHz != 0)
                ReqProps.uHz = uHz;

            PDMAUDIOSTREAMCFG CfgAcq;
            PPDMAUDIOSTREAM   pStream  = NULL;
            rc = audioTestDriverStackStreamCreateOutput(&DrvStack, &ReqProps, cMsBufferSize,
                                                        cMsPreBuffer, cMsSchedulingHint, &pStream, &CfgAcq);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Automatically enable the mixer if the wave file and the
                 * output parameters doesn't match.
                 */
                if (   !fWithMixer
                    && !PDMAudioPropsAreEqual(&WaveFile.Props, &pStream->Cfg.Props))
                {
                    RTMsgInfo("Enabling the mixer buffer.\n");
                    fWithMixer = true;
                }

                /*
                 * Create a mixer wrapper.  This is just a thin wrapper if fWithMixer
                 * is false, otherwise it's doing mixing, resampling and recoding.
                 */
                AUDIOTESTDRVMIXSTREAM Mix;
                rc = AudioTestMixStreamInit(&Mix, &DrvStack, pStream, fWithMixer ? &WaveFile.Props : NULL, 100 /*ms*/);
                if (RT_SUCCESS(rc))
                {
                    if (g_uVerbosity > 0)
                        RTMsgInfo("Stream: %s cbBackend=%#RX32%s\n",
                                  PDMAudioPropsToString(&pStream->Cfg.Props, szTmp, sizeof(szTmp)),
                                  pStream->cbBackend, fWithMixer ? " mixed" : "");

                    /*
                     * Enable the stream and start playing.
                     */
                    rc = AudioTestMixStreamEnable(&Mix);
                    if (RT_SUCCESS(rc))
                        rcExit = audioTestPlayOneInner(&Mix, &WaveFile, &CfgAcq, pszFile);
                    else
                        rcExit = RTMsgErrorExitFailure("Enabling the output stream failed: %Rrc", rc);

                    /*
                     * Clean up.
                     */
                    AudioTestMixStreamTerm(&Mix);
                }
                audioTestDriverStackStreamDestroy(&DrvStack, pStream);
            }
            else
                rcExit = RTMsgErrorExitFailure("Creating output stream failed: %Rrc", rc);
        }
        else
            rcExit = RTMsgErrorExitFailure("Failed to set output device to '%s': %Rrc", pszDevId, rc);
        audioTestDriverStackDelete(&DrvStack);
    }
    else
        rcExit = RTMsgErrorExitFailure("Driver stack construction failed: %Rrc", rc);
    AudioTestWaveFileClose(&WaveFile);
    return rcExit;
}


/*********************************************************************************************************************************
*   Command: rec                                                                                                                 *
*********************************************************************************************************************************/

/**
 * Worker for audioTestRecOne implementing the recording loop.
 */
static RTEXITCODE audioTestRecOneInner(PAUDIOTESTDRVMIXSTREAM pMix, PAUDIOTESTWAVEFILE pWaveFile,
                                       PCPDMAUDIOSTREAMCFG pCfgAcq, uint64_t cMaxFrames, const char *pszFile)
{
    int             rc;
    uint64_t const  nsStarted   = RTTimeNanoTS();

    /*
     * Transfer data as quickly as we're allowed.
     */
    uint8_t         abSamples[16384];
    uint32_t const  cbSamplesAligned     = PDMAudioPropsFloorBytesToFrame(pMix->pProps, sizeof(abSamples));
    uint64_t        cFramesCapturedTotal = 0;
    while (!g_fTerminate && cFramesCapturedTotal < cMaxFrames)
    {
        /*
         * Anything we can read?
         */
        uint32_t const cbCanRead = AudioTestMixStreamGetReadable(pMix);
        if (cbCanRead)
        {
            uint32_t const cbToRead   = RT_MIN(cbCanRead, cbSamplesAligned);
            uint32_t       cbCaptured = 0;
            rc = AudioTestMixStreamCapture(pMix, abSamples, cbToRead, &cbCaptured);
            if (RT_SUCCESS(rc))
            {
                if (cbCaptured)
                {
                    uint32_t cFramesCaptured = PDMAudioPropsBytesToFrames(pMix->pProps, cbCaptured);
                    if (cFramesCaptured + cFramesCaptured < cMaxFrames)
                    { /* likely */ }
                    else
                    {
                        cFramesCaptured = cMaxFrames - cFramesCaptured;
                        cbCaptured      = PDMAudioPropsFramesToBytes(pMix->pProps, cFramesCaptured);
                    }

                    rc = AudioTestWaveFileWrite(pWaveFile, abSamples, cbCaptured);
                    if (RT_SUCCESS(rc))
                        cFramesCapturedTotal += cFramesCaptured;
                    else
                        return RTMsgErrorExitFailure("Error writing to '%s': %Rrc", pszFile, rc);
                }
                else
                    return RTMsgErrorExitFailure("Captured zero bytes - %#x bytes reported readable!\n", cbCanRead);
            }
            else
                return RTMsgErrorExitFailure("Failed to capture %#x bytes: %Rrc (%#x available)\n", cbToRead, rc, cbCanRead);
        }
        else if (AudioTestMixStreamIsOkay(pMix))
            RTThreadSleep(RT_MIN(RT_MAX(1, pCfgAcq->Device.cMsSchedulingHint), 256));
        else
            return RTMsgErrorExitFailure("Stream is not okay!\n");
    }

    /*
     * Disable the stream.
     */
    rc = AudioTestMixStreamDisable(pMix);
    if (RT_SUCCESS(rc) && g_uVerbosity > 0)
        RTMsgInfo("%'RU64 ns: Stopped after recording %RU64 frames%s\n", RTTimeNanoTS() - nsStarted, cFramesCapturedTotal,
                  g_fTerminate ? " - Ctrl-C" : ".");
    else if (RT_FAILURE(rc))
        return RTMsgErrorExitFailure("Disabling stream failed: %Rrc", rc);

    return RTEXITCODE_SUCCESS;
}


/**
 * Worker for audioTestCmdRecHandler that recs one file.
 */
RTEXITCODE audioTestRecOne(const char *pszFile, uint8_t cWaveChannels, uint8_t cbWaveSample, uint32_t uWaveHz,
                           PCPDMDRVREG pDrvReg, const char *pszDevId, uint32_t cMsBufferSize,
                           uint32_t cMsPreBuffer, uint32_t cMsSchedulingHint,
                           uint8_t cChannels, uint8_t cbSample, uint32_t uHz, bool fWithDrvAudio, bool fWithMixer,
                           uint64_t cMaxFrames, uint64_t cNsMaxDuration)
{
    /*
     * Construct the driver stack.
     */
    RTEXITCODE          rcExit = RTEXITCODE_FAILURE;
    AUDIOTESTDRVSTACK   DrvStack;
    int rc = audioTestDriverStackInit(&DrvStack, pDrvReg, fWithDrvAudio);
    if (RT_SUCCESS(rc))
    {
        /*
         * Set the input device if one is specified.
         */
        rc = audioTestDriverStackSetDevice(&DrvStack, PDMAUDIODIR_IN, pszDevId);
        if (RT_SUCCESS(rc))
        {
            /*
             * Create an input stream.
             */
            PDMAUDIOPCMPROPS  ReqProps;
            PDMAudioPropsInit(&ReqProps,
                              cbSample ? cbSample : cbWaveSample ? cbWaveSample : 2,
                              true /*fSigned*/,
                              cChannels ? cChannels : cWaveChannels ? cWaveChannels : 2,
                              uHz ? uHz : uWaveHz ? uWaveHz : 44100);
            PDMAUDIOSTREAMCFG CfgAcq;
            PPDMAUDIOSTREAM   pStream  = NULL;
            rc = audioTestDriverStackStreamCreateInput(&DrvStack, &ReqProps, cMsBufferSize,
                                                       cMsPreBuffer, cMsSchedulingHint, &pStream, &CfgAcq);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Determine the wave file properties.  If it differs from the stream
                 * properties, make sure the mixer is enabled.
                 */
                PDMAUDIOPCMPROPS WaveProps;
                PDMAudioPropsInit(&WaveProps,
                                  cbWaveSample ? cbWaveSample : PDMAudioPropsSampleSize(&CfgAcq.Props),
                                  true /*fSigned*/,
                                  cWaveChannels ? cWaveChannels : PDMAudioPropsChannels(&CfgAcq.Props),
                                  uWaveHz ? uWaveHz : PDMAudioPropsHz(&CfgAcq.Props));
                if (!fWithMixer && !PDMAudioPropsAreEqual(&WaveProps, &CfgAcq.Props))
                {
                    RTMsgInfo("Enabling the mixer buffer.\n");
                    fWithMixer = true;
                }

                /* Console the max duration into frames now that we've got the wave file format. */
                if (cMaxFrames != UINT64_MAX && cNsMaxDuration != UINT64_MAX)
                {
                    uint64_t cMaxFrames2 = PDMAudioPropsNanoToBytes64(&WaveProps, cNsMaxDuration);
                    cMaxFrames = RT_MAX(cMaxFrames, cMaxFrames2);
                }
                else if (cNsMaxDuration != UINT64_MAX)
                    cMaxFrames = PDMAudioPropsNanoToBytes64(&WaveProps, cNsMaxDuration);

                /*
                 * Create a mixer wrapper.  This is just a thin wrapper if fWithMixer
                 * is false, otherwise it's doing mixing, resampling and recoding.
                 */
                AUDIOTESTDRVMIXSTREAM Mix;
                rc = AudioTestMixStreamInit(&Mix, &DrvStack, pStream, fWithMixer ? &WaveProps : NULL, 100 /*ms*/);
                if (RT_SUCCESS(rc))
                {
                    char szTmp[128];
                    if (g_uVerbosity > 0)
                        RTMsgInfo("Stream: %s cbBackend=%#RX32%s\n",
                                  PDMAudioPropsToString(&pStream->Cfg.Props, szTmp, sizeof(szTmp)),
                                  pStream->cbBackend, fWithMixer ? " mixed" : "");

                    /*
                     * Open the wave output file.
                     */
                    AUDIOTESTWAVEFILE WaveFile;
                    RTERRINFOSTATIC ErrInfo;
                    rc = AudioTestWaveFileCreate(pszFile, &WaveProps, &WaveFile, RTErrInfoInitStatic(&ErrInfo));
                    if (RT_SUCCESS(rc))
                    {
                        if (g_uVerbosity > 0)
                        {
                            RTMsgInfo("Opened '%s' for playing\n", pszFile);
                            RTMsgInfo("Format: %s\n", PDMAudioPropsToString(&WaveFile.Props, szTmp, sizeof(szTmp)));
                        }

                        /*
                         * Enable the stream and start recording.
                         */
                        rc = AudioTestMixStreamEnable(&Mix);
                        if (RT_SUCCESS(rc))
                            rcExit = audioTestRecOneInner(&Mix, &WaveFile, &CfgAcq, cMaxFrames, pszFile);
                        else
                            rcExit = RTMsgErrorExitFailure("Enabling the input stream failed: %Rrc", rc);
                        if (rcExit != RTEXITCODE_SUCCESS)
                            AudioTestMixStreamDisable(&Mix);

                        /*
                         * Clean up.
                         */
                        rc = AudioTestWaveFileClose(&WaveFile);
                        if (RT_FAILURE(rc))
                            rcExit = RTMsgErrorExitFailure("Error closing '%s': %Rrc", pszFile, rc);
                    }
                    else
                        rcExit = RTMsgErrorExitFailure("Failed to open '%s': %Rrc%#RTeim", pszFile, rc, &ErrInfo.Core.pszMsg);

                    AudioTestMixStreamTerm(&Mix);
                }
                audioTestDriverStackStreamDestroy(&DrvStack, pStream);
            }
            else
                rcExit = RTMsgErrorExitFailure("Creating output stream failed: %Rrc", rc);
        }
        else
            rcExit = RTMsgErrorExitFailure("Failed to set output device to '%s': %Rrc", pszDevId, rc);
        audioTestDriverStackDelete(&DrvStack);
    }
    else
        rcExit = RTMsgErrorExitFailure("Driver stack construction failed: %Rrc", rc);
    return rcExit;
}

