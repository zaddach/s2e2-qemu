/*
 * tcg-plugin-api.h
 *
 *  Created on: Oct 31, 2014
 *      Author: zaddach
 */

#ifndef TCG_PLUGIN_API_H_
#define TCG_PLUGIN_API_H_

#include "tcg.h"

/**
 * Get the architecture name that Qemu is compiled for.
 * @return Architecture name (i.e., i386)
 */
const char *plgapi_get_arch_name(void);

/**
 * Register a helper function with Qemu.
 * This function can then be used with tcg_gen_callN to insert callbacks into TCG code.
 * @param s TCG Context
 * @param func Helper function to register
 * @param name Name of the helper function
 * @param flags Flags specifying side effects of this function (@see)
 */
void plgapi_register_helper(TCGContext *s, void *func, const char *name, unsigned flags, unsigned sizemask);

/**
 * Get a helper's name from its function pointer.
 * @param s TCG Context
 * @param helper The helper's function pointer
 * @return The helper's name
 */
const char *plgapi_get_helper_name(TCGContext *s, void *helper);

/**
 * Load a value from Qemu's guest's memory.
 * @param env CPU state.
 * @param virt_addr Virtual address of the value to load.
 * @param mmu_idx Index of the MMU to load from.
 * @param memop Value size specification.
 */
uint64_t plgapi_qemu_ld(CPUArchState *env, uint64_t virt_addr, uint32_t mmu_index, uint32_t memop);

/**
 * Store a value to Qemu's guest's memory.
 * @param env CPU state.
 * @param virt_addr Virtual address where to store the value.
 * @param mmu_idx Index of the MMU to use.
 * @param value Value to store.
 * @param memop Value size specification.
 */
void plgapi_qemu_st(CPUArchState *env, uint64_t virt_addr, uint32_t mmu_index, uint64_t value, uint32_t memop);


#endif /* TCG_PLUGIN_API_H_ */
