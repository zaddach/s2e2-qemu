/*
 * tcg-plugin-api.h
 *
 *  Created on: Oct 31, 2014
 *      Author: zaddach
 */

#ifndef TCG_PLUGIN_API_H_
#define TCG_PLUGIN_API_H_

#include "tcg.h"

const char *plgapi_get_arch(void);

void plgapi_register_helper(TCGContext *s, void *fnc, const char *name, unsigned flags, unsigned sizemask);




#endif /* TCG_PLUGIN_API_H_ */
