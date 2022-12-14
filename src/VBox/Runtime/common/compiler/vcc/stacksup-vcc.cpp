/* $Id$ */
/** @file
 * IPRT - Visual C++ Compiler - Stack Checking C/C++ Support.
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

#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#ifndef IPRT_NOCRT_WITHOUT_FATAL_WRITE
# include <iprt/assert.h>
#endif

#include "internal/compiler-vcc.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Variable descriptor. */
typedef struct RTC_VAR_DESC_T
{
    int32_t     offFrame;
    uint32_t    cbVar;
    const char *pszName;
} RTC_VAR_DESC_T;

/** Frame descriptor. */
typedef struct RTC_FRAME_DESC_T
{
    uint32_t                cVars;
    RTC_VAR_DESC_T const   *paVars;
} RTC_FRAME_DESC_T;

#define VARIABLE_MARKER_PRE     0xcccccccc
#define VARIABLE_MARKER_POST    0xcccccccc


/**
 * Alloca allocation entry.
 * @note For whatever reason the pNext and cb members are misaligned on 64-bit
 *       targets.  32-bit targets OTOH adds padding to keep the structure size
 *       and pNext + cb offsets the same.
 */
#pragma pack(4)
typedef struct RTC_ALLOC_ENTRY
{
    uint32_t            uGuard1;
    RTC_ALLOC_ENTRY    *pNext;
#if ARCH_BITS == 32
    uint32_t            pNextPad;
#endif
    size_t              cb;
#if ARCH_BITS == 32
    uint32_t            cbPad;
#endif
    uint32_t            auGuard2[3];
} RTC_ALLOC_ENTRY;
#pragma pack()

#define ALLOCA_FILLER_BYTE      0xcc
#define ALLOCA_FILLER_32        0xcccccccc


/*********************************************************************************************************************************
*   External Symbols                                                                                                             *
*********************************************************************************************************************************/
extern "C" void __fastcall _RTC_CheckStackVars(uint8_t *pbFrame, RTC_VAR_DESC_T const *pVar); /* nocrt-stack.asm */
extern "C" uintptr_t __security_cookie;


/**
 * Initializes the security cookie value.
 *
 * This must be called as the first thing by the startup code.  We must also no
 * do anything fancy here.
 */
void rtVccInitSecurityCookie(void) RT_NOEXCEPT
{
    __security_cookie = (uintptr_t)ASMReadTSC() ^ (uintptr_t)&__security_cookie;
}


DECLASM(void) _RTC_StackVarCorrupted(uint8_t *pbFrame, RTC_VAR_DESC_T const *pVar)
{
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
    RTAssertMsg2("\n\n!!Stack corruption!!\n\n"
                 "%p LB %#x - %s\n",
                 pbFrame + pVar->offFrame, pVar->cbVar, pVar->pszName);
#else
    rtNoCrtFatalWriteBegin(RT_STR_TUPLE("\r\n\r\n!!Stack corruption!!\r\n\r\n"));
    rtNoCrtFatalWritePtr(pbFrame + pVar->offFrame);
    rtNoCrtFatalWrite(RT_STR_TUPLE(" LB "));
    rtNoCrtFatalWriteX32(pVar->cbVar);
    rtNoCrtFatalWrite(RT_STR_TUPLE(" - "));
    rtNoCrtFatalWriteStr(pVar->pszName);
    rtNoCrtFatalWriteEnd(RT_STR_TUPLE("\r\n"));
#endif
    RT_BREAKPOINT();
}


DECLASM(void) _RTC_SecurityCookieMismatch(uintptr_t uCookie)
{
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
    RTAssertMsg2("\n\n!!Stack cookie corruption!!\n\n"
                 "expected %p, found %p\n",
                 __security_cookie, uCookie);
#else
    rtNoCrtFatalWriteBegin(RT_STR_TUPLE("\r\n\r\n!!Stack cookie corruption!!\r\n\r\n"
                                        "expected"));
    rtNoCrtFatalWritePtr((void *)__security_cookie);
    rtNoCrtFatalWrite(RT_STR_TUPLE(", found "));
    rtNoCrtFatalWritePtr((void *)uCookie);
    rtNoCrtFatalWriteEnd(RT_STR_TUPLE("\r\n"));
#endif
    RT_BREAKPOINT();
}


#ifdef RT_ARCH_X86
DECLASM(void) _RTC_CheckEspFailed(uintptr_t uEip, uintptr_t uEsp, uintptr_t uEbp)
{
# ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
    RTAssertMsg2("\n\n!!ESP check failed!!\n\n"
                 "eip=%p esp=%p ebp=%p\n",
                 uEip, uEsp, uEbp);
# else
    rtNoCrtFatalWriteBegin(RT_STR_TUPLE("\r\n\r\n!!ESP check failed!!\r\n\r\n"
                                       "eip="));
    rtNoCrtFatalWritePtr((void *)uEip);
    rtNoCrtFatalWrite(RT_STR_TUPLE(" esp="));
    rtNoCrtFatalWritePtr((void *)uEsp);
    rtNoCrtFatalWrite(RT_STR_TUPLE(" ebp="));
    rtNoCrtFatalWritePtr((void *)uEbp);
    rtNoCrtFatalWriteEnd(RT_STR_TUPLE("\r\n"));
# endif
    RT_BREAKPOINT();
}
#endif


extern "C" void __cdecl _RTC_UninitUse(const char *pszVar)
{
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
    RTAssertMsg2("\n\n!!Used uninitialized variable %s at %p!!\n\n",
                 pszVar ? pszVar : "", ASMReturnAddress());
#else
    rtNoCrtFatalWriteBegin(RT_STR_TUPLE("\r\n\r\n!!Used uninitialized variable "));
    rtNoCrtFatalWriteStr(pszVar);
    rtNoCrtFatalWrite(RT_STR_TUPLE(" at "));
    rtNoCrtFatalWritePtr(ASMReturnAddress());
    rtNoCrtFatalWriteEnd(RT_STR_TUPLE("!!\r\n\r\n"));
#endif
    RT_BREAKPOINT();
}


/** @todo reimplement in assembly (feeling too lazy right now). */
extern "C" void __fastcall _RTC_CheckStackVars2(uint8_t *pbFrame, RTC_VAR_DESC_T const *pVar, RTC_ALLOC_ENTRY *pHead)
{
    while (pHead)
    {
        if (   pHead->uGuard1     == ALLOCA_FILLER_32
#if 1 && ARCH_BITS == 32
            && pHead->pNextPad    == ALLOCA_FILLER_32
            && pHead->cbPad       == ALLOCA_FILLER_32
#endif
            && pHead->auGuard2[0] == ALLOCA_FILLER_32
            && pHead->auGuard2[1] == ALLOCA_FILLER_32
            && pHead->auGuard2[2] == ALLOCA_FILLER_32
            && *(uint32_t const *)((uint8_t const *)pHead + pHead->cb - sizeof(uint32_t)) == ALLOCA_FILLER_32)
        { /* likely */ }
        else
        {
#ifdef IPRT_NOCRT_WITHOUT_FATAL_WRITE
            RTAssertMsg2("\n\n!!Stack corruption (alloca)!!\n\n"
                         "%p LB %#x\n",
                         pHead, pHead->cb);
#else
            rtNoCrtFatalWriteBegin(RT_STR_TUPLE("\r\n\r\n!!Stack corruption (alloca)!!\r\n\r\n"));
            rtNoCrtFatalWritePtr(pHead);
            rtNoCrtFatalWrite(RT_STR_TUPLE(" LB "));
            rtNoCrtFatalWriteX64(pHead->cb);
            rtNoCrtFatalWriteEnd(RT_STR_TUPLE("\r\n"));
#endif
            RT_BREAKPOINT();
        }
        pHead = pHead->pNext;
    }

    _RTC_CheckStackVars(pbFrame, pVar);
}

