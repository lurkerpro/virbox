/* $Id$ */
/** @file
 * Audio testing routines.
 * Common code which is being used by the ValidationKit and the debug / ValdikationKit audio driver(s).
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <VBox/vmm/pdmaudioifs.h>
#include <VBox/vmm/pdmaudioinline.h>

#include <iprt/rand.h>

#define _USE_MATH_DEFINES
#include <math.h> /* sin, M_PI */

#include "AudioTest.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Well-known frequency selection test tones. */
static const double s_aAudioTestToneFreqsHz[] =
{
     349.2282 /*F4*/,
     440.0000 /*A4*/,
     523.2511 /*C5*/,
     698.4565 /*F5*/,
     880.0000 /*A5*/,
    1046.502  /*C6*/,
    1174.659  /*D6*/,
    1396.913  /*F6*/,
    1760.0000 /*A6*/
};

/**
 * Initializes a test tone by picking a random but well-known frequency (in Hz).
 *
 * @returns Randomly picked frequency (in Hz).
 * @param   pTone               Pointer to test tone to initialize.
 * @param   pProps              PCM properties to use for the test tone.
 */
double AudioTestToneInitRandom(PAUDIOTESTTONE pTone, PPDMAUDIOPCMPROPS pProps)
{
    /* Pick a frequency from our selection, so that every time a recording starts
     * we'll hopfully generate a different note. */
    pTone->rdFreqHz = s_aAudioTestToneFreqsHz[RTRandU32Ex(0, RT_ELEMENTS(s_aAudioTestToneFreqsHz) - 1)];
    pTone->rdFixed  = 2.0 * M_PI * pTone->rdFreqHz / PDMAudioPropsHz(pProps);
    pTone->uSample  = 0;

    memcpy(&pTone->Props, pProps, sizeof(PDMAUDIOPCMPROPS));

    return pTone->rdFreqHz;
}

/**
 * Writes (and iterates) a given test tone to an output buffer.
 *
 * @returns VBox status code.
 * @param   pTone               Pointer to test tone to write.
 * @param   pvBuf               Pointer to output buffer to write test tone to.
 * @param   cbBuf               Size (in bytes) of output buffer.
 * @param   pcbWritten          How many bytes were written on success.
 */
int AudioTestToneWrite(PAUDIOTESTTONE pTone, void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten)
{
    /*
     * Clear the buffer first so we don't need to thing about additional channels.
     */
    uint32_t cFrames = PDMAudioPropsBytesToFrames(&pTone->Props, cbBuf);
    PDMAudioPropsClearBuffer(&pTone->Props, pvBuf, cbBuf, cFrames);

    /*
     * Generate the select sin wave in the first channel:
     */
    uint32_t const cbFrame   = PDMAudioPropsFrameSize(&pTone->Props);
    double const   rdFixed   = pTone->rdFixed;
    uint64_t       iSrcFrame = pTone->uSample;
    switch (PDMAudioPropsSampleSize(&pTone->Props))
    {
        case 1:
            /* untested */
            if (PDMAudioPropsIsSigned(&pTone->Props))
            {
                int8_t *piSample = (int8_t *)pvBuf;
                while (cFrames-- > 0)
                {
                    *piSample = 126 /*Amplitude*/ * sin(rdFixed * iSrcFrame);
                    iSrcFrame++;
                    piSample += cbFrame;
                }
            }
            else
            {
                /* untested */
                uint16_t *pbSample = (uint16_t *)pvBuf;
                while (cFrames-- > 0)
                {
                    *pbSample = 126 /*Amplitude*/ * sin(rdFixed * iSrcFrame) + 0x80;
                    iSrcFrame++;
                    pbSample += cbFrame;
                }
            }
            break;

        case 2:
            if (PDMAudioPropsIsSigned(&pTone->Props))
            {
                int16_t *piSample = (int16_t *)pvBuf;
                while (cFrames-- > 0)
                {
                    *piSample = 32760 /*Amplitude*/ * sin(rdFixed * iSrcFrame);
                    iSrcFrame++;
                    piSample = (int16_t *)((uint8_t *)piSample + cbFrame);
                }
            }
            else
            {
                /* untested */
                uint16_t *puSample = (uint16_t *)pvBuf;
                while (cFrames-- > 0)
                {
                    *puSample = 32760 /*Amplitude*/ * sin(rdFixed * iSrcFrame) + 0x8000;
                    iSrcFrame++;
                    puSample = (uint16_t *)((uint8_t *)puSample + cbFrame);
                }
            }
            break;

        case 4:
            /* untested */
            if (PDMAudioPropsIsSigned(&pTone->Props))
            {
                int32_t *piSample = (int32_t *)pvBuf;
                while (cFrames-- > 0)
                {
                    *piSample = (32760 << 16) /*Amplitude*/ * sin(rdFixed * iSrcFrame);
                    iSrcFrame++;
                    piSample = (int32_t *)((uint8_t *)piSample + cbFrame);
                }
            }
            else
            {
                uint32_t *puSample = (uint32_t *)pvBuf;
                while (cFrames-- > 0)
                {
                    *puSample = (32760 << 16) /*Amplitude*/ * sin(rdFixed * iSrcFrame) + UINT32_C(0x80000000);
                    iSrcFrame++;
                    puSample = (uint32_t *)((uint8_t *)puSample + cbFrame);
                }
            }
            break;

        default:
            AssertFailedReturn(VERR_NOT_SUPPORTED);
    }

    pTone->uSample = iSrcFrame;

    if (pcbWritten)
        *pcbWritten = PDMAudioPropsFramesToBytes(&pTone->Props, cFrames);

    return VINF_SUCCESS;
}

/**
 * Initializes an audio test tone parameters struct with random values.
 * @param   pToneParams         Test tone parameters to initialize.
 * @param   pProps              PCM properties to use for the test tone.
 */
int AudioTestToneParamsInitRandom(PAUDIOTESTTONEPARMS pToneParams, PPDMAUDIOPCMPROPS pProps)
{
    AssertReturn(PDMAudioPropsAreValid(pProps), VERR_INVALID_PARAMETER);

    memcpy(&pToneParams->Props, pProps, sizeof(PDMAUDIOPCMPROPS));

    /** @todo Make this a bit more sophisticated later, e.g. muting and prequel/sequel are not very balanced. */

    pToneParams->msPrequel      = RTRandU32Ex(0, RT_MS_5SEC);
    pToneParams->msDuration     = RTRandU32Ex(0, RT_MS_30SEC); /** @todo Probably a bit too long, but let's see. */
    pToneParams->msSequel       = RTRandU32Ex(0, RT_MS_5SEC);
    pToneParams->uVolumePercent = RTRandU32Ex(0, 100);

    return VINF_SUCCESS;
}
