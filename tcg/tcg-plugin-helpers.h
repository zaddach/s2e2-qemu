/*
 * tcg-plugin-helpers.h
 *
 *  Created on: Nov 6, 2014
 *      Author: zaddach
 */

#ifndef TCG_TCG_PLUGIN_HELPERS_H_
#define TCG_TCG_PLUGIN_HELPERS_H_

#include "tcg.h"

extern TCGv_ptr tcgplugin_cpu_env;

uint32_t tcgplugin_helper_intercept_qemu_ld_i32(CPUArchState *env, uint32_t addr, uint32_t idx, uint32_t memop);
void tcgplugin_helper_post_qemu_ld_i32(uint32_t addr, uint32_t val, uint32_t idx, uint32_t memop);


static inline void tcgplugin_gen_helper_intercept_ld_i32(
		TCGContext *s,
		TCGv_i32 val,
		TCGv_i32 addr,
		TCGv_i32 idx,
		TCGv_i32 memop)
{
	TCGArg args[4] = {
			GET_TCGV_PTR(tcgplugin_cpu_env),
			GET_TCGV_I32(addr),
			GET_TCGV_I32(idx),
			GET_TCGV_I32(memop)};

	tcg_gen_callN(s, (void *) tcgplugin_helper_intercept_qemu_ld_i32, GET_TCGV_I32(val), 4, args);
}

static inline void tcgplugin_gen_helper_post_qemu_ld_i32(
		TCGContext *s,
		TCGv_i32 addr,
		TCGv_i32 idx,
		TCGv_i32 val,
		TCGv_i32 memop)
{
	TCGArg args[4] = {
			GET_TCGV_I32(addr),
			GET_TCGV_I32(idx),
			GET_TCGV_I32(val),
			GET_TCGV_I32(memop)};

	tcg_gen_callN(s, (void *) tcgplugin_helper_post_qemu_ld_i32, TCG_CALL_DUMMY_ARG, 4, args);
}



#endif /* TCG_TCG_PLUGIN_HELPERS_H_ */
