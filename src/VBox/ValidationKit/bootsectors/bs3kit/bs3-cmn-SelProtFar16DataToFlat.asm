; $Id$
;; @file
; BS3Kit - Bs3SelProtFar16DataToFlat.
;

;
; Copyright (C) 2007-2022 Oracle and/or its affiliates.
;
; This file is part of VirtualBox base platform packages, as
; available from https://www.virtualbox.org.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation, in version 3 of the
; License.
;
; This program is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see <https://www.gnu.org/licenses>.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
; in the VirtualBox distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;
; SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
;


;*********************************************************************************************************************************
;*      Header Files                                                                                                             *
;*********************************************************************************************************************************
%include "bs3kit-template-header.mac"


;*********************************************************************************************************************************
;*  External Symbols                                                                                                             *
;*********************************************************************************************************************************
BS3_EXTERN_CMN Bs3SelFar32ToFlat32NoClobber
TMPL_BEGIN_TEXT
%if TMPL_BITS == 16
CPU 8086
%endif


;;
; @cproto   BS3_CMN_PROTO_NOSB(uint32_t, Bs3SelProtFar16DataToFlat,(uint32_t uFar1616));
;
; @uses     Only return registers (ax:dx, eax, eax)
; @remarks  No 20h scratch area requirements.
;
BS3_PROC_BEGIN_CMN Bs3SelProtFar16DataToFlat, BS3_PBC_NEAR      ; Far stub generated by the makefile/bs3kit.h.
        push    xBP
        mov     xBP, xSP

        ;
        ; Just call Bs3SelFar32ToFlat32NoClobber to do the job.
        ;
%if TMPL_BITS == 16
        push    word [xBP + xCB + cbCurRetAddr + 2]
        xor     ax, ax
        push    ax
        push    word [xBP + xCB + cbCurRetAddr]
        call    Bs3SelFar32ToFlat32NoClobber
        add     sp, 6
%else
 %if TMPL_BITS == 32
        movzx   eax, word [xBP + xCB + cbCurRetAddr + 2]
        push    eax
        movzx   eax, word [xBP + xCB + cbCurRetAddr]
        push    eax
        call    Bs3SelFar32ToFlat32NoClobber
        add     esp, 8
 %else
        push    xDX
        push    xCX

        mov     edx, ecx                ; arg #2: selector
        shr     edx, 16
        movzx   ecx, cx                 ; arg #1: offset
        call    Bs3SelFar32ToFlat32NoClobber

        pop     xDX
        pop     xCX
 %endif
%endif

.return:
        pop     xBP
        BS3_HYBRID_RET
BS3_PROC_END_CMN   Bs3SelProtFar16DataToFlat

