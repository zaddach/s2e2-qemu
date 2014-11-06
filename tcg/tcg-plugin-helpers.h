/*
 * tcg-plugin-helpers.h
 *
 *  Created on: Nov 6, 2014
 *      Author: zaddach
 */

#ifndef TCG_TCG_PLUGIN_HELPERS_H_
#define TCG_TCG_PLUGIN_HELPERS_H_

#include "tcg.h"

void gen_helper_pre_memory_access_i32(TCGv_i32 do_access, TCGv_i32 address, TCGv_i32 value, TCGv_i32 is_store);





#endif /* TCG_TCG_PLUGIN_HELPERS_H_ */
