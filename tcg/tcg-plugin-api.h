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




#endif /* TCG_PLUGIN_API_H_ */
