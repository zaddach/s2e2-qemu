/*
 * tcg-plugin-api.c
 *
 *  Created on: Oct 31, 2014
 *      Author: zaddach
 */

#include "tcg-plugin-api.h"

const char *plgapi_get_arch(void)
{
	return TARGET_NAME;
}

typedef struct TCGHelperInfo {
    void *func;
    const char *name;
    unsigned flags;
    unsigned sizemask;
} TCGHelperInfo;

void plgapi_register_helper(TCGContext *s, void *func, const char *name, unsigned flags, unsigned sizemask)
{
	TCGHelperInfo *helper_info = (TCGHelperInfo *) g_malloc0(sizeof(TCGHelperInfo));

	helper_info->func = func;
	helper_info->name = name;
	helper_info->flags = flags;
	helper_info->sizemask = sizemask;

	g_hash_table_insert(s->helpers, (gpointer) func,
	                            (gpointer) helper_info);
}


