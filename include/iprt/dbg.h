/* $Id$ */
/** @file
 * IPRT - Debugging Routines.
 */

/*
 * Copyright (C) 2008-2009 Sun Microsystems, Inc.
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
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___iprt_dbg_h
#define ___iprt_dbg_h

#include <iprt/types.h>
#include <iprt/stdarg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_dbg    RTDbg - Debugging Routines
 * @ingroup grp_rt
 * @{
 */


/** Debug segment index. */
typedef uint32_t            RTDBGSEGIDX;
/** Pointer to a debug segment index. */
typedef RTDBGSEGIDX        *PRTDBGSEGIDX;
/** Pointer to a const debug segment index. */
typedef RTDBGSEGIDX const  *PCRTDBGSEGIDX;
/** NIL debug segment index. */
#define NIL_RTDBGSEGIDX             UINT32_C(0xffffffff)
/** The last normal segment index. */
#define RTDBGSEGIDX_LAST            UINT32_C(0xffffffef)
/** Special segment index that indicates that the offset is a relative
 * virtual address (RVA). I.e. an offset from the start of the module. */
#define RTDBGSEGIDX_RVA             UINT32_C(0xfffffff0)
/** Special segment index that indicates that the offset is a absolute. */
#define RTDBGSEGIDX_ABS             UINT32_C(0xfffffff1)
/** The last valid special segment index. */
#define RTDBGSEGIDX_SPECIAL_LAST    RTDBGSEGIDX_ABS
/** The last valid special segment index. */
#define RTDBGSEGIDX_SPECIAL_FIRST   (RTDBGSEGIDX_LAST + 1U)


/** Max length (including '\\0') of a segment name. */
#define RTDBG_SEGMENT_NAME_LENGTH   (128 - 8 - 8 - 8 - 4 - 4)

/**
 * Debug module segment.
 */
typedef struct RTDBGSEGMENT
{
    /** The load address.
     * RTUINTPTR_MAX if not applicable. */
    RTUINTPTR           Address;
    /** The image relative virtual address of the segment.
     * RTUINTPTR_MAX if not applicable. */
    RTUINTPTR           uRva;
    /** The segment size. */
    RTUINTPTR           cb;
    /** The segment flags. (reserved) */
    uint32_t            fFlags;
    /** The segment index. */
    RTDBGSEGIDX         iSeg;
    /** Symbol name. */
    char                szName[RTDBG_SEGMENT_NAME_LENGTH];
} RTDBGSEGMENT;
/** Pointer to a debug module segment. */
typedef RTDBGSEGMENT *PRTDBGSEGMENT;
/** Pointer to a const debug module segment. */
typedef RTDBGSEGMENT const *PCRTDBGSEGMENT;



/** Max length (including '\\0') of a symbol name. */
#define RTDBG_SYMBOL_NAME_LENGTH    (384 - 8 - 8 - 8 - 4 - 4 - 8)

/**
 * Debug symbol.
 */
typedef struct RTDBGSYMBOL
{
    /** Symbol value (address).
     * This depends a bit who you ask. It will be the same as offSeg when you
     * as RTDbgMod, but the mapping address if you ask RTDbgAs. */
    RTUINTPTR           Value;
    /** Symbol size. */
    RTUINTPTR           cb;
    /** Offset into the segment specified by iSeg. */
    RTUINTPTR           offSeg;
    /** Segment number. */
    RTDBGSEGIDX         iSeg;
    /** Symbol Flags. (reserved). */
    uint32_t            fFlags;
    /** Symbol ordinal.
     * This is set to UINT32_MAX if the ordinals aren't supported. */
    uint32_t            iOrdinal;
    /** Symbol name. */
    char                szName[RTDBG_SYMBOL_NAME_LENGTH];
} RTDBGSYMBOL;
/** Pointer to debug symbol. */
typedef RTDBGSYMBOL *PRTDBGSYMBOL;
/** Pointer to const debug symbol. */
typedef const RTDBGSYMBOL *PCRTDBGSYMBOL;

/**
 * Allocate a new symbol structure.
 *
 * @returns Pointer to a new structure on success, NULL on failure.
 */
RTDECL(PRTDBGSYMBOL)    RTDbgSymbolAlloc(void);

/**
 * Duplicates a symbol structure.
 *
 * @returns Pointer to duplicate on success, NULL on failure.
 *
 * @param   pSymbol         The symbol to duplicate.
 */
RTDECL(PRTDBGSYMBOL)    RTDbgSymbolDup(PCRTDBGSYMBOL pSymbol);

/**
 * Free a symbol structure previously allocated by a RTDbg method.
 *
 * @param   pSymbol         The symbol to free. NULL is ignored.
 */
RTDECL(void)            RTDbgSymbolFree(PRTDBGSYMBOL pSymbol);


/** Max length (including '\\0') of a debug info file name. */
#define RTDBG_FILE_NAME_LENGTH      (260)


/**
 * Debug line number information.
 */
typedef struct RTDBGLINE
{
    /** Address.
     * This depends a bit who you ask. It will be the same as offSeg when you
     * as RTDbgMod, but the mapping address if you ask RTDbgAs. */
    RTUINTPTR           Address;
    /** Offset into the segment specified by iSeg. */
    RTUINTPTR           offSeg;
    /** Segment number. */
    RTDBGSEGIDX         iSeg;
    /** Line number. */
    uint32_t            uLineNo;
    /** Symbol ordinal.
     * This is set to UINT32_MAX if the ordinals aren't supported. */
    uint32_t            iOrdinal;
    /** Filename. */
    char                szFilename[RTDBG_FILE_NAME_LENGTH];
} RTDBGLINE;
/** Pointer to debug line number. */
typedef RTDBGLINE *PRTDBGLINE;
/** Pointer to const debug line number. */
typedef const RTDBGLINE *PCRTDBGLINE;

/**
 * Allocate a new line number structure.
 *
 * @returns Pointer to a new structure on success, NULL on failure.
 */
RTDECL(PRTDBGLINE)      RTDbgLineAlloc(void);

/**
 * Duplicates a line number structure.
 *
 * @returns Pointer to duplicate on success, NULL on failure.
 *
 * @param   pLine           The line number to duplicate.
 */
RTDECL(PRTDBGLINE)      RTDbgLineDup(PCRTDBGLINE pLine);

/**
 * Free a line number structure previously allocated by a RTDbg method.
 *
 * @param   pLine           The line number to free. NULL is ignored.
 */
RTDECL(void)            RTDbgLineFree(PRTDBGLINE pLine);


/** @defgroup grp_rt_dbgas      RTDbgAs - Debug Address Space
 * @{
 */

/**
 * Creates an empty address space.
 *
 * @returns IPRT status code.
 *
 * @param   phDbgAs         Where to store the address space handle on success.
 * @param   FirstAddr       The first address in the address space.
 * @param   LastAddr        The last address in the address space.
 * @param   pszName         The name of the address space.
 */
RTDECL(int) RTDbgAsCreate(PRTDBGAS phDbgAs, RTUINTPTR FirstAddr, RTUINTPTR LastAddr, const char *pszName);

/**
 * Variant of RTDbgAsCreate that takes a name format string.
 *
 * @returns IPRT status code.
 *
 * @param   phDbgAs         Where to store the address space handle on success.
 * @param   FirstAddr       The first address in the address space.
 * @param   LastAddr        The last address in the address space.
 * @param   pszNameFmt      The name format of the address space.
 * @param   va              Format arguments.
 */
RTDECL(int) RTDbgAsCreateV(PRTDBGAS phDbgAs, RTUINTPTR FirstAddr, RTUINTPTR LastAddr, const char *pszNameFmt, va_list va);

/**
 * Variant of RTDbgAsCreate that takes a name format string.
 *
 * @returns IPRT status code.
 *
 * @param   phDbgAs         Where to store the address space handle on success.
 * @param   FirstAddr       The first address in the address space.
 * @param   LastAddr        The last address in the address space.
 * @param   pszNameFmt      The name format of the address space.
 * @param   ...             Format arguments.
 */
RTDECL(int) RTDbgAsCreateF(PRTDBGAS phDbgAs, RTUINTPTR FirstAddr, RTUINTPTR LastAddr, const char *pszNameFmt, ...);

/**
 * Retains a reference to the address space.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hDbgAs          The address space handle.
 *
 * @remarks Will not take any locks.
 */
RTDECL(uint32_t) RTDbgAsRetain(RTDBGAS hDbgAs);

/**
 * Release a reference to the address space.
 *
 * When the reference count reaches zero, the address space is destroyed.
 * That means unlinking all the modules it currently contains, potentially
 * causing some or all of them to be destroyed as they are managed by
 * reference counting.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hDbgAs          The address space handle. The NIL handle is quietly
 *                          ignored and 0 is returned.
 *
 * @remarks Will not take any locks.
 */
RTDECL(uint32_t) RTDbgAsRelease(RTDBGAS hDbgAs);

/**
 * Gets the name of an address space.
 *
 * @returns read only address space name.
 *          NULL if hDbgAs is invalid.
 *
 * @param   hDbgAs          The address space handle.
 *
 * @remarks Will not take any locks.
 */
RTDECL(const char *) RTDbgAsName(RTDBGAS hDbgAs);

/**
 * Gets the first address in an address space.
 *
 * @returns The address.
 *          0 if hDbgAs is invalid.
 *
 * @param   hDbgAs          The address space handle.
 *
 * @remarks Will not take any locks.
 */
RTDECL(RTUINTPTR) RTDbgAsFirstAddr(RTDBGAS hDbgAs);

/**
 * Gets the last address in an address space.
 *
 * @returns The address.
 *          0 if hDbgAs is invalid.
 *
 * @param   hDbgAs          The address space handle.
 *
 * @remarks Will not take any locks.
 */
RTDECL(RTUINTPTR) RTDbgAsLastAddr(RTDBGAS hDbgAs);

/**
 * Gets the number of modules in the address space.
 *
 * This can be used together with RTDbgAsModuleByIndex
 * to enumerate the modules.
 *
 * @returns The number of modules.
 *
 * @param   hDbgAs          The address space handle.
 *
 * @remarks Will not take any locks.
 */
RTDECL(uint32_t) RTDbgAsModuleCount(RTDBGAS hDbgAs);

/** @name Flags for RTDbgAsModuleLink and RTDbgAsModuleLinkSeg
 * @{ */
/** Replace all conflicting module.
 * (The conflicting modules will be removed the address space and their
 * references released.) */
#define RTDBGASLINK_FLAGS_REPLACE       RT_BIT_32(0)
/** Mask containing the valid flags. */
#define RTDBGASLINK_FLAGS_VALID_MASK    UINT32_C(0x00000001)
/** @} */

/**
 * Links a module into the address space at the give address.
 *
 * The size of the mapping is determined using RTDbgModImageSize().
 *
 * @returns IPRT status code.
 * @retval  VERR_OUT_OF_RANGE if the specified address will put the module
 *          outside the address space.
 * @retval  VERR_ADDRESS_CONFLICT if the mapping clashes with existing mappings.
 *
 * @param   hDbgAs          The address space handle.
 * @param   hDbgMod         The module handle of the module to be linked in.
 * @param   ImageAddr       The address to link the module at.
 * @param   fFlags          See RTDBGASLINK_FLAGS_*.
 */
RTDECL(int) RTDbgAsModuleLink(RTDBGAS hDbgAs, RTDBGMOD hDbgMod, RTUINTPTR ImageAddr, uint32_t fFlags);

/**
 * Links a segment into the address space at the give address.
 *
 * The size of the mapping is determined using RTDbgModSegmentSize().
 *
 * @returns IPRT status code.
 * @retval  VERR_OUT_OF_RANGE if the specified address will put the module
 *          outside the address space.
 * @retval  VERR_ADDRESS_CONFLICT if the mapping clashes with existing mappings.
 *
 * @param   hDbgAs          The address space handle.
 * @param   hDbgMod         The module handle.
 * @param   iSeg            The segment number (0-based) of the segment to be
 *                          linked in.
 * @param   SegAddr         The address to link the segment at.
 * @param   fFlags          See RTDBGASLINK_FLAGS_*.
 */
RTDECL(int) RTDbgAsModuleLinkSeg(RTDBGAS hDbgAs, RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg, RTUINTPTR SegAddr, uint32_t fFlags);

/**
 * Unlinks all the mappings of a module from the address space.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if the module wasn't found.
 *
 * @param   hDbgAs          The address space handle.
 * @param   hDbgMod         The module handle of the module to be unlinked.
 */
RTDECL(int) RTDbgAsModuleUnlink(RTDBGAS hDbgAs, RTDBGMOD hDbgMod);

/**
 * Unlinks the mapping at the specified address.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if no module or segment is mapped at that address.
 *
 * @param   hDbgAs          The address space handle.
 * @param   Addr            The address within the mapping to be unlinked.
 */
RTDECL(int) RTDbgAsModuleUnlinkByAddr(RTDBGAS hDbgAs, RTUINTPTR Addr);

/**
 * Get a the handle of a module in the address space by is index.
 *
 * @returns A retained handle to the specified module. The caller must release
 *          the returned reference.
 *          NIL_RTDBGMOD if invalid index or handle.
 *
 * @param   hDbgAs          The address space handle.
 * @param   iModule         The index of the module to get.
 *
 * @remarks The module indexes may change after calls to RTDbgAsModuleLink,
 *          RTDbgAsModuleLinkSeg, RTDbgAsModuleUnlink and
 *          RTDbgAsModuleUnlinkByAddr.
 */
RTDECL(RTDBGMOD) RTDbgAsModuleByIndex(RTDBGAS hDbgAs, uint32_t iModule);

/**
 * Queries mapping module information by handle.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if no mapping was found at the specified address.
 *
 * @param   hDbgAs          The address space handle.
 * @param   Addr            Address within the mapping of the module or segment.
 * @param   phMod           Where to the return the retained module handle.
 *                          Optional.
 * @param   pAddr           Where to return the base address of the mapping.
 *                          Optional.
 * @param   piSeg           Where to return the segment index. This is set to
 *                          NIL if the entire module is mapped as a single
 *                          mapping. Optional.
 */
RTDECL(int) RTDbgAsModuleByAddr(RTDBGAS hDbgAs, RTUINTPTR Addr, PRTDBGMOD phMod, PRTUINTPTR pAddr, PRTDBGSEGIDX piSeg);

/**
 * Queries mapping module information by name.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if no mapping was found at the specified address.
 * @retval  VERR_OUT_OF_RANGE if the name index was out of range.
 *
 * @param   hDbgAs          The address space handle.
 * @param   pszName         The module name.
 * @param   iName           There can be more than one module by the same name
 *                          in an address space. This argument indicates which
 *                          is ment. (0 based)
 * @param   phMod           Where to the return the retained module handle.
 */
RTDECL(int) RTDbgAsModuleByName(RTDBGAS hDbgAs, const char *pszName, uint32_t iName, PRTDBGMOD phMod);

/**
 * Adds a symbol to a module in the address space.
 *
 * @returns IPRT status code. See RTDbgModSymbolAdd for more specific ones.
 * @retval  VERR_INVALID_HANDLE if hDbgAs is invalid.
 * @retval  VERR_NOT_FOUND if no module was found at the specified address.
 * @retval  VERR_NOT_SUPPORTED if the module interpret doesn't support adding
 *          custom symbols.
 *
 * @param   hDbgAs          The address space handle.
 * @param   pszSymbol       The symbol name.
 * @param   Addr            The address of the symbol.
 * @param   cb              The size of the symbol.
 * @param   fFlags          Symbol flags.
 * @param   piOrdinal       Where to return the symbol ordinal on success. If
 *                          the interpreter doesn't do ordinals, this will be set to
 *                          UINT32_MAX. Optional
 */
RTDECL(int) RTDbgAsSymbolAdd(RTDBGAS hDbgAs, const char *pszSymbol, RTUINTPTR Addr, RTUINTPTR cb, uint32_t fFlags, uint32_t *piOrdinal);

/**
 * Query a symbol by address.
 *
 * @returns IPRT status code. See RTDbgModSymbolAddr for more specific ones.
 * @retval  VERR_INVALID_HANDLE if hDbgAs is invalid.
 * @retval  VERR_NOT_FOUND if the address couldn't be mapped to a module.
 *
 * @param   hDbgAs          The address space handle.
 * @param   Addr            The address which closest symbol is requested.
 * @param   poffDisp        Where to return the distance between the symbol
 *                          and address. Optional.
 * @param   pSymbol         Where to return the symbol info.
 */
RTDECL(int) RTDbgAsSymbolByAddr(RTDBGAS hDbgAs, RTUINTPTR Addr, PRTINTPTR poffDisp, PRTDBGSYMBOL pSymbol);

/**
 * Query a symbol by address.
 *
 * @returns IPRT status code. See RTDbgModSymbolAddrA for more specific ones.
 * @retval  VERR_INVALID_HANDLE if hDbgAs is invalid.
 * @retval  VERR_NOT_FOUND if the address couldn't be mapped to a module.
 *
 * @param   hDbgAs          The address space handle.
 * @param   Addr            The address which closest symbol is requested.
 * @param   poffDisp        Where to return the distance between the symbol
 *                          and address. Optional.
 * @param   ppSymbol        Where to return the pointer to the allocated
 *                          symbol info. Always set. Free with RTDbgSymbolFree.
 */
RTDECL(int) RTDbgAsSymbolByAddrA(RTDBGAS hDbgAs, RTUINTPTR Addr, PRTINTPTR poffDisp, PRTDBGSYMBOL *ppSymbol);

/**
 * Query a symbol by name.
 *
 * @returns IPRT status code.
 * @retval  VERR_SYMBOL_NOT_FOUND if not found.
 *
 * @param   hDbgAs          The address space handle.
 * @param   pszSymbol       The symbol name.
 * @param   pSymbol         Where to return the symbol info.
 */
RTDECL(int) RTDbgAsSymbolByName(RTDBGAS hDbgAs, const char *pszSymbol, PRTDBGSYMBOL pSymbol);

/**
 * Query a symbol by name.
 *
 * @returns IPRT status code.
 * @retval  VERR_SYMBOL_NOT_FOUND if not found.
 *
 * @param   hDbgAs          The address space handle.
 * @param   pszSymbol       The symbol name.
 * @param   ppSymbol        Where to return the pointer to the allocated
 *                          symbol info. Always set. Free with RTDbgSymbolFree.
 */
RTDECL(int) RTDbgAsSymbolByNameA(RTDBGAS hDbgAs, const char *pszSymbol, PRTDBGSYMBOL *ppSymbol);

/**
 * Query a line number by address.
 *
 * @returns IPRT status code. See RTDbgModSymbolAddrA for more specific ones.
 * @retval  VERR_INVALID_HANDLE if hDbgAs is invalid.
 * @retval  VERR_NOT_FOUND if the address couldn't be mapped to a module.
 *
 * @param   hDbgAs          The address space handle.
 * @param   Addr            The address which closest symbol is requested.
 * @param   poffDisp        Where to return the distance between the line
 *                          number and address.
 * @param   pLine           Where to return the line number information.
 */
RTDECL(int) RTDbgAs(RTDBGAS hDbgAs, RTUINTPTR Addr, PRTINTPTR poffDisp, PRTDBGLINE pLine);

/**
 * Adds a line number to a module in the address space.
 *
 * @returns IPRT status code. See RTDbgModSymbolAdd for more specific ones.
 * @retval  VERR_INVALID_HANDLE if hDbgAs is invalid.
 * @retval  VERR_NOT_FOUND if no module was found at the specified address.
 * @retval  VERR_NOT_SUPPORTED if the module interpret doesn't support adding
 *          custom symbols.
 *
 * @param   hDbgAs          The address space handle.
 * @param   pszFile         The file name.
 * @param   uLineNo         The line number.
 * @param   Addr            The address of the symbol.
 * @param   piOrdinal       Where to return the line number ordinal on success.
 *                          If the interpreter doesn't do ordinals, this will be
 *                          set to UINT32_MAX. Optional.
 */
RTDECL(int) RTDbgAsLineAdd(RTDBGAS hDbgAs, const char *pszFile, uint32_t uLineNo, RTUINTPTR Addr, uint32_t *piOrdinal);

/**
 * Query a line number by address.
 *
 * @returns IPRT status code. See RTDbgModSymbolAddrA for more specific ones.
 * @retval  VERR_INVALID_HANDLE if hDbgAs is invalid.
 * @retval  VERR_NOT_FOUND if the address couldn't be mapped to a module.
 *
 * @param   hDbgAs          The address space handle.
 * @param   Addr            The address which closest symbol is requested.
 * @param   poffDisp        Where to return the distance between the line
 *                          number and address.
 * @param   ppLine          Where to return the pointer to the allocated line
 *                          number info. Always set. Free with RTDbgLineFree.
 */
RTDECL(int) RTDbgAsLineByAddrA(RTDBGAS hDbgAs, RTUINTPTR Addr, PRTINTPTR poffDisp, PRTDBGLINE *ppLine);

/** @todo Missing some bits here. */

/** @} */


/** @defgroup grp_rt_dbgmod     RTDbgMod - Debug Module Interpreter
 * @{
 */
RTDECL(int)         RTDbgModCreate(PRTDBGMOD phDbgMod, const char *pszName, RTUINTPTR cb, uint32_t fFlags);
RTDECL(int)         RTDbgModCreateDeferred(PRTDBGMOD phDbgMod, const char *pszFilename, const char *pszName, RTUINTPTR cb, uint32_t fFlags);
RTDECL(int)         RTDbgModCreateFromImage(PRTDBGMOD phDbgMod, const char *pszFilename, const char *pszName, uint32_t fFlags);
RTDECL(int)         RTDbgModCreateFromMap(PRTDBGMOD phDbgMod, const char *pszFilename, const char *pszName, RTUINTPTR uSubtrahend, uint32_t fFlags);
RTDECL(uint32_t)    RTDbgModRetain(RTDBGMOD hDbgMod);
RTDECL(uint32_t)    RTDbgModRelease(RTDBGMOD hDbgMod);
RTDECL(const char *) RTDbgModName(RTDBGMOD hDbgMod);
RTDECL(RTUINTPTR)   RTDbgModImageSize(RTDBGMOD hDbgMod);

RTDECL(int)         RTDbgModSegmentAdd(RTDBGMOD hDbgMod, RTUINTPTR uRva, RTUINTPTR cb, const char *pszName, size_t cchName, uint32_t fFlags, PRTDBGSEGIDX piSeg);
RTDECL(RTDBGSEGIDX) RTDbgModSegmentCount(RTDBGMOD hDbgMod);
RTDECL(int)         RTDbgModSegmentByIndex(RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg, PRTDBGSEGMENT pSegInfo);
RTDECL(RTUINTPTR)   RTDbgModSegmentSize(RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg);
RTDECL(RTUINTPTR)   RTDbgModSegmentRva(RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg);

RTDECL(int)         RTDbgModSymbolAdd(RTDBGMOD hDbgMod, const char *pszSymbol, RTDBGSEGIDX iSeg, RTUINTPTR off, RTUINTPTR cb, uint32_t fFlags, uint32_t *piOrdinal);
RTDECL(uint32_t)    RTDbgModSymbolCount(RTDBGMOD hDbgMod);
RTDECL(int)         RTDbgModSymbolByIndex(RTDBGMOD hDbgMod, uint32_t iSymbol, PRTDBGSYMBOL pSymbol);
RTDECL(int)         RTDbgModSymbolByAddr(RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg, RTUINTPTR off, PRTINTPTR poffDisp, PRTDBGSYMBOL pSymbol);
RTDECL(int)         RTDbgModSymbolByAddrA(RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg, RTUINTPTR off, PRTINTPTR poffDisp, PRTDBGSYMBOL *ppSymbol);
RTDECL(int)         RTDbgModSymbolByName(RTDBGMOD hDbgMod, const char *pszSymbol, PRTDBGSYMBOL pSymbol);
RTDECL(int)         RTDbgModSymbolByNameA(RTDBGMOD hDbgMod, const char *pszSymbol, PRTDBGSYMBOL *ppSymbol);

RTDECL(int)         RTDbgModLineAdd(RTDBGMOD hDbgMod, const char *pszFile, uint32_t uLineNo, RTDBGSEGIDX iSeg, RTUINTPTR off, uint32_t *piOrdinal);
RTDECL(int)         RTDbgModLineByAddr(RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg, RTUINTPTR off, PRTINTPTR poffDisp, PRTDBGLINE pLine);
RTDECL(int)         RTDbgModLineByAddrA(RTDBGMOD hDbgMod, RTDBGSEGIDX iSeg, RTUINTPTR off, PRTINTPTR poffDisp, PRTDBGLINE *ppLine);
/** @} */

/** @} */

RT_C_DECLS_END

#endif

