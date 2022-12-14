/* $Id$ */
/** @file
 * IPRT - No-CRT - Common Windows startup code.
 */

/*
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/nocrt.h"
#include "internal/process.h"

#include <iprt/nt/nt-and-windows.h>
#ifndef IPRT_NOCRT_WITHOUT_FATAL_WRITE
# include <iprt/assert.h>
#endif
#include <iprt/getopt.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/utf16.h>

#include "internal/compiler-vcc.h"
#include "internal/process.h"



void rtVccWinInitProcExecPath(void)
{
    WCHAR wszPath[RTPATH_MAX];
    UINT cwcPath = GetModuleFileNameW(NULL, wszPath, RT_ELEMENTS(wszPath));
    if (cwcPath)
    {
        char *pszDst = g_szrtProcExePath;
        int rc = RTUtf16ToUtf8Ex(wszPath, cwcPath, &pszDst, sizeof(g_szrtProcExePath), &g_cchrtProcExePath);
        if (RT_SUCCESS(rc))
        {
            g_cchrtProcExeDir = g_offrtProcName = RTPathFilename(pszDst) - g_szrtProcExePath;
            while (   g_cchrtProcExeDir >= 2
                   && RTPATH_IS_SLASH(g_szrtProcExePath[g_cchrtProcExeDir - 1])
                   && g_szrtProcExePath[g_cchrtProcExeDir - 2] != ':')
                g_cchrtProcExeDir--;
        }
        else
        {
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
            RTMsgError("initProcExecPath: RTUtf16ToUtf8Ex failed: %Rrc\n", rc);
#else
            rtNoCrtFatalMsgWithRc(RT_STR_TUPLE("initProcExecPath: RTUtf16ToUtf8Ex failed: "), rc);
#endif
        }
    }
    else
    {
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
        RTMsgError("initProcExecPath: GetModuleFileNameW failed: %Rhrc\n", GetLastError());
#else
        rtNoCrtFatalWriteBegin(RT_STR_TUPLE("initProcExecPath: GetModuleFileNameW failed: "));
        rtNoCrtFatalWriteWinRc(GetLastError());
        rtNoCrtFatalWrite(RT_STR_TUPLE("\r\n"));
#endif
    }
}

