/* $Id$ */
/** @file
 * VBoxManage - The 'updatecheck' command.
 */

/*
 * Copyright (C) 2020-2022 Oracle Corporation
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
#include <VBox/com/com.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>
#include <VBox/com/array.h>

#include <iprt/buildconfig.h>
#include <VBox/version.h>

#include <VBox/log.h>
#include <iprt/getopt.h>
#include <iprt/stream.h>
#include <iprt/ctype.h>
#include <iprt/message.h>

#include "VBoxManage.h"

DECLARE_TRANSLATION_CONTEXT(UpdateCheck);

using namespace com;    // SafeArray

static RTEXITCODE doUpdateList(int argc, char **argv, ComPtr<IUpdateAgent> pUpdateAgent)
{
    /*
     * Parse options.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--machine-readable", 'm', RTGETOPT_REQ_NOTHING }
    };
    RTGETOPTSTATE GetState;
    int vrc = RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 0 /* First */, 0);
    AssertRCReturn(vrc, RTEXITCODE_INIT);

    bool fMachineReadable = false;

    int c;
    RTGETOPTUNION ValueUnion;
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case 'm':
                fMachineReadable = true;
                break;

            default:
                return errorGetOpt(c, &ValueUnion);
        }
    }

    /*
     * Do the work.
     */
    BOOL fEnabled;
    CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(Enabled)(&fEnabled), RTEXITCODE_FAILURE);
    if (fMachineReadable)
        outputMachineReadableBool("enabled", &fEnabled);
    else
        RTPrintf(UpdateCheck::tr("Enabled:                %s\n"),
                 fEnabled ? UpdateCheck::tr("yes") : UpdateCheck::tr("no"));

    ULONG cCheckCount;
    CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(CheckCount)(&cCheckCount), RTEXITCODE_FAILURE);
    if (fMachineReadable)
        outputMachineReadableULong("count", &cCheckCount);
    else
        RTPrintf(UpdateCheck::tr("Count:                  %u\n"), cCheckCount);

    ULONG uCheckFreqSeconds;
    CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(CheckFrequency)(&uCheckFreqSeconds), RTEXITCODE_FAILURE);

    ULONG const uCheckFreqDays = uCheckFreqSeconds / RT_SEC_1DAY;

    if (fMachineReadable)
        outputMachineReadableULong("frequency", &uCheckFreqSeconds);
    else if (uCheckFreqDays == 0)
        RTPrintf(UpdateCheck::tr("Frequency:              never\n")); /** @todo r=bird: Two inconsistencies here. HostUpdateImpl.cpp code will indicate the need for updating if no last-check-date.  modifysettings cannot set it to zero (I added the error message, you just skipped setting it originally). */
    else if (uCheckFreqDays == 1)
        RTPrintf(UpdateCheck::tr("Frequency:              every day\n"));
    else
        RTPrintf(UpdateCheck::tr("Frequency:              every %u days\n"), uCheckFreqDays);

    UpdateChannel_T enmUpdateChannel;
    CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(Channel)(&enmUpdateChannel), RTEXITCODE_FAILURE);
    const char *psz;
    const char *pszMachine;
    switch (enmUpdateChannel)
    {
        case UpdateChannel_Stable:
            psz = UpdateCheck::tr("Stable - new minor and maintenance releases");
            pszMachine = "stable";
            break;
        case UpdateChannel_All:
            psz = UpdateCheck::tr("All releases - new minor, maintenance, and major releases");
            pszMachine = "all-releases";
            break;
        case UpdateChannel_WithBetas:
            psz = UpdateCheck::tr("With Betas - new minor, maintenance, major, and beta releases");
            pszMachine = "with-betas";
            break;
        default:
            AssertFailed();
            psz = UpdateCheck::tr("Unset");
            pszMachine = "invalid";
            break;
    }
    if (fMachineReadable)
        outputMachineReadableString("channel", pszMachine);
    else
        RTPrintf(UpdateCheck::tr("Channel:                %s\n"), psz);

    Bstr bstrLastCheckDate;
    CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(LastCheckDate)(bstrLastCheckDate.asOutParam()),
                      RTEXITCODE_FAILURE);
    if (fMachineReadable)
        outputMachineReadableString("last-check-date", &bstrLastCheckDate);
    else if (bstrLastCheckDate.isNotEmpty())
        RTPrintf(UpdateCheck::tr("Last Check Date:        %ls\n"), bstrLastCheckDate.raw());

    return RTEXITCODE_SUCCESS;
}

static RTEXITCODE doUpdateModify(int argc, char **argv, ComPtr<IUpdateAgent> pUpdateAgent)
{
    /*
     * Parse options.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--enable",        'e', RTGETOPT_REQ_NOTHING },
        { "--disable",       'd', RTGETOPT_REQ_NOTHING },
        { "--channel",       'c', RTGETOPT_REQ_STRING },
        { "--frequency",     'f', RTGETOPT_REQ_UINT32 },
    };

    RTGETOPTSTATE GetState;
    int vrc = RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 0 /* First */, 0);
    AssertRCReturn(vrc, RTEXITCODE_INIT);

    int                         fEnabled       = -1; /* tristate: -1 (not modified), false, true */
    UpdateChannel_T const       enmChannelNil  = (UpdateChannel_T)-999;
    UpdateChannel_T             enmChannel     = enmChannelNil;
    uint32_t                    cFrequencyDays = 0;

    int c;
    RTGETOPTUNION ValueUnion;
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case 'e':
                fEnabled = true;
                break;

            case 'd':
                fEnabled = false;
                break;

            case 'c':
                if (!RTStrICmp(ValueUnion.psz, "stable"))
                    enmChannel = UpdateChannel_Stable;
                else if (!RTStrICmp(ValueUnion.psz, "withbetas"))
                    enmChannel = UpdateChannel_WithBetas;
                else if (!RTStrICmp(ValueUnion.psz, "all"))
                    enmChannel = UpdateChannel_All;
                else
                    return errorArgument(UpdateCheck::tr("Unknown channel specified: '%s'"), ValueUnion.psz);
                break;

            case 'f':
                cFrequencyDays = ValueUnion.u32;
                if (cFrequencyDays == 0)
                    return errorArgument(UpdateCheck::tr("The update frequency cannot be zero"));
                break;

            default:
                return errorGetOpt(c, &ValueUnion);
        }
    }

    if (   fEnabled == -1
        && enmChannel != enmChannelNil
        && cFrequencyDays == 0)
        return errorSyntax(UpdateCheck::tr("No change requested"));

    /*
     * Make the changes.
     */
    if (enmChannel != enmChannelNil)
    {
        CHECK_ERROR2I_RET(pUpdateAgent, COMSETTER(Channel)(enmChannel), RTEXITCODE_FAILURE);
    }
    if (fEnabled != -1)
    {
        CHECK_ERROR2I_RET(pUpdateAgent, COMSETTER(Enabled)((BOOL)fEnabled), RTEXITCODE_FAILURE);
    }
    if (cFrequencyDays)
    {
        CHECK_ERROR2I_RET(pUpdateAgent, COMSETTER(CheckFrequency)(cFrequencyDays * RT_SEC_1DAY), RTEXITCODE_FAILURE);
    }

    return RTEXITCODE_SUCCESS;
}

static RTEXITCODE doUpdateCheck(int argc, char **argv, ComPtr<IUpdateAgent> pUpdateAgent)
{
    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--machine-readable", 'm', RTGETOPT_REQ_NOTHING }
    };
    RTGETOPTSTATE GetState;
    int vrc = RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 0 /* First */, 0);
    AssertRCReturn(vrc, RTEXITCODE_INIT);

    bool fMachineReadable = false;

    int c;
    RTGETOPTUNION ValueUnion;
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case 'm':
                fMachineReadable = true;
                break;

            default:
                return errorGetOpt(c, &ValueUnion);
        }
    }

    /*
     * Do the work.
     */
    Bstr bstrName;
    CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(Name)(bstrName.asOutParam()), RTEXITCODE_FAILURE);

    if (!fMachineReadable)
        RTPrintf(UpdateCheck::tr("Checking for a new %ls version...\n"), bstrName.raw());

    /*
     * We don't call CHECK_ERROR2I_RET(pHostUpdate, VBoxUpdate(updateCheckType, ...); here so we can check for a specific
     * return value indicating update checks are disabled.
     */
    ComPtr<IProgress> pProgress;
    HRESULT rc = pUpdateAgent->Check(pProgress.asOutParam());
    if (FAILED(rc))
    {
        if (pProgress.isNull())
            RTStrmPrintf(g_pStdErr, UpdateCheck::tr("Failed to create update progress object: %Rhrc\n"), rc);
        else
            com::GlueHandleComError(pUpdateAgent, "HostUpdate(UpdateChannel_Stable, pProgress.asOutParam())",
                                    rc, __FILE__, __LINE__);
        return RTEXITCODE_FAILURE;
    }

    /* HRESULT hrc = */ showProgress(pProgress, fMachineReadable ? SHOW_PROGRESS_NONE : SHOW_PROGRESS);
    CHECK_PROGRESS_ERROR_RET(pProgress, (UpdateCheck::tr("Checking for update failed.")), RTEXITCODE_FAILURE);

    UpdateState_T updateState;
    CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(State)(&updateState), RTEXITCODE_FAILURE);

    BOOL const fUpdateNeeded = updateState == UpdateState_Available;
    if (fMachineReadable)
        outputMachineReadableBool("update-needed", &fUpdateNeeded);

    switch (updateState)
    {
        case UpdateState_Available:
        {
            Bstr bstrUpdateVersion;
            CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(Version)(bstrUpdateVersion.asOutParam()), RTEXITCODE_FAILURE);
            Bstr bstrUpdateURL;
            CHECK_ERROR2I_RET(pUpdateAgent, COMGETTER(DownloadUrl)(bstrUpdateURL.asOutParam()), RTEXITCODE_FAILURE);

            if (!fMachineReadable)
                RTPrintf(UpdateCheck::tr(
                            "A new version of %ls has been released! Version %ls is available at virtualbox.org.\n"
                            "You can download this version here: %ls\n"),
                         bstrName.raw(), bstrUpdateVersion.raw(), bstrUpdateURL.raw());
            else
            {
                outputMachineReadableString("update-version", &bstrUpdateVersion);
                outputMachineReadableString("update-url", &bstrUpdateURL);
            }

            break;
        }

        case UpdateState_NotAvailable:
        {
            if (!fMachineReadable)
                RTPrintf(UpdateCheck::tr("You are already running the most recent version of %ls.\n"), bstrName.raw());
            break;
        }

        case UpdateState_Canceled:
            break;

        case UpdateState_Error:
            RT_FALL_THROUGH();
        default:
        {
            if (!fMachineReadable)
                RTPrintf(UpdateCheck::tr("Something went wrong while checking for updates!\n"
                                         "Please check network connection and try again later.\n"));
            break;
        }
    }

    return RTEXITCODE_SUCCESS;
}

/**
 * Handles the 'updatecheck' command.
 *
 * @returns Appropriate exit code.
 * @param   a                   Handler argument.
 */
RTEXITCODE handleUpdateCheck(HandlerArg *a)
{
    ComPtr<IHost> pHost;
    CHECK_ERROR2I_RET(a->virtualBox, COMGETTER(Host)(pHost.asOutParam()), RTEXITCODE_FAILURE);

    ComPtr<IUpdateAgent> pUpdate;
    CHECK_ERROR2I_RET(pHost, COMGETTER(UpdateHost)(pUpdate.asOutParam()), RTEXITCODE_FAILURE);
    /** @todo Add other update agents here. */

    if (a->argc < 1)
        return errorNoSubcommand();
    if (!RTStrICmp(a->argv[0], "perform"))
    {
        setCurrentSubcommand(HELP_SCOPE_UPDATECHECK_PERFORM);
        return doUpdateCheck(a->argc - 1, &a->argv[1], pUpdate);
    }
    if (!RTStrICmp(a->argv[0], "list"))
    {
        setCurrentSubcommand(HELP_SCOPE_UPDATECHECK_LIST);
        return doUpdateList(a->argc - 1, &a->argv[1], pUpdate);
    }
    if (!RTStrICmp(a->argv[0], "modify"))
    {
        setCurrentSubcommand(HELP_SCOPE_UPDATECHECK_MODIFY);
        return doUpdateModify(a->argc - 1, &a->argv[1], pUpdate);
    }
    return errorUnknownSubcommand(a->argv[0]);
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
