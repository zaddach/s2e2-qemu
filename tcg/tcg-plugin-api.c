/*
 * tcg-plugin-api.c
 *
 *  Created on: Oct 31, 2014
 *      Author: zaddach
 */

#include "tcg-plugin-api.h"

const char *plgapi_get_arch_name(void)
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

const char *plgapi_get_helper_name(TCGContext *s, void *helper)
{
	TCGHelperInfo *info = g_hash_table_lookup(s->helpers, helper);

	if (!info)  {
		return NULL;
	}

	return info->name;
}

uint64_t plgapi_qemu_ld(CPUArchState *env, uint64_t virt_addr, uint32_t mmu_index, uint32_t memop)
{
#ifdef CONFIG_SOFTMMU
    switch (memop)
    {
    case MO_UB:   return helper_ret_ldub_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_SB:   return helper_ret_ldsb_mmu(env, virt_addr, mmu_index, GETRA());

    case MO_LEUW: return helper_le_lduw_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_LEUL: return helper_le_ldul_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_LEQ:  return helper_le_ldq_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_LESW: return helper_le_ldsw_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_LESL: return helper_le_ldul_mmu(env, virt_addr, mmu_index, GETRA());

    case MO_BEUW: return helper_be_lduw_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_BEUL: return helper_be_ldul_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_BEQ:  return helper_be_ldq_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_BESW: return helper_be_ldsw_mmu(env, virt_addr, mmu_index, GETRA());
    case MO_BESL: return helper_be_ldul_mmu(env, virt_addr, mmu_index, GETRA());
    default:
        printf("ERROR: unknown memory operation %d\n", memop);
        tcg_abort();
        return 0; /* Should never be reached */
    }
#else
    //TODO: This code has never been tested, possible bugs
    //TODO: target_ulong does not necessarily have the same size as a host pointer. What to do?
    // How do -user systems work if the target and the host have different pointer sizes?
    printf("ERROR: Code in %s (%s:%d) has never been tested. Remove this warning and test before use.\n",
            __func__, __FILE__, __LINE__);
    tcg_abort();

    switch (memop)
    {
    case MO_UB:   return *((uint8_t *) virt_addr);
    case MO_SB:   return (uint64_t)((int64_t) *((int8_t *) virt_addr));

    case MO_LEUW: return le16toh(*((uint16_t *) virt_addr));
    case MO_LEUL: return le32toh(*((uint32_t *) virt_addr));
    case MO_LEQ:  return le64toh(*((uint64_t *) virt_addr));
    case MO_LESW: return (uint64_t)((int64_t) le16toh(*((int16_t *) virt_addr)));
    case MO_LESL: return (uint64_t)((int64_t) le32toh(*((int32_t *) virt_addr)));

    case MO_BEUW: return be16toh(*((uint16_t *) virt_addr));
    case MO_BEUL: return be32toh(*((uint32_t *) virt_addr));
    case MO_BEQ:  return be64toh(*((uint64_t *) virt_addr));
    case MO_BESW: return (uint64_t) ((int64_t) be16toh(*((int16_t *) virt_addr)));
    case MO_BESL: return (uint64_t) ((int64_t) be32toh(*((int32_t *) virt_addr)));
    default:
        printf("ERROR: unknown memory operation %d\n", memop);
        tcg_abort();
        return 0; /* Should never be reached */
    }
#endif /* CONFIG_SOFTMMU */
}

void plgapi_qemu_st(CPUArchState *env, uint64_t virt_addr, uint32_t mmu_index, uint64_t value, uint32_t memop)
{
#ifdef CONFIG_SOFTMMU
    switch (memop)
    {
    case MO_UB:   helper_ret_stb_mmu(env, virt_addr, value, mmu_index, GETRA()); break;

    case MO_LEUW: helper_le_stw_mmu(env, virt_addr, value, mmu_index, GETRA());  break;
    case MO_LEUL: helper_le_stl_mmu(env, virt_addr, value, mmu_index, GETRA());  break;
    case MO_LEQ:  helper_le_stq_mmu(env, virt_addr, value, mmu_index, GETRA());  break;

    case MO_BEUW: helper_be_stw_mmu(env, virt_addr, value, mmu_index, GETRA());  break;
    case MO_BEUL: helper_be_stl_mmu(env, virt_addr, value, mmu_index, GETRA());  break;
    case MO_BEQ:  helper_be_stq_mmu(env, virt_addr, value, mmu_index, GETRA());  break;
    default:
        printf("ERROR: unknown memory operation %d\n", memop);
        tcg_abort();
        break;
    }
#else

#ifdef HOST_WORDS_BIGENDIAN
#error This code has been written with little endian hosts in mind only. Possible bugs lurk.
#endif

    //TODO: This code has never been tested, possible bugs
    //TODO: target_ulong does not necessarily have the same size as a host pointer. What to do?
    // How do -user systems work if the target and the host have different pointer sizes?
    printf("ERROR: Code in %s (%s:%d) has never been tested. Remove this warning and test before use.\n",
            __func__, __FILE__, __LINE__);
    tcg_abort();

    switch (memop)
    {
    case MO_UB:   *((uint8_t *) virt_addr) = (uint8_t) value;                       break;
    case MO_SB:   *((uint8_t *) virt_addr) = (uint8_t) ((int8_t) value);            break;

    case MO_LEUW: *((uint16_t *) virt_addr) = htole16((uint16_t) value);            break;
    case MO_LEUL: *((uint32_t *) virt_addr) = htole32((uint32_t) value);            break;
    case MO_LEQ:  *((uint64_t *) virt_addr) = htole64(value);                       break;
    case MO_LESW: *((uint16_t *) virt_addr) = htole16((uint16_t) value);            break;
    case MO_LESL: *((uint32_t *) virt_addr) = htole32((uint32_t) value);            break;

    case MO_BEUW: *((uint16_t *) virt_addr) = htobe16((uint16_t) value);            break;
    case MO_BEUL: *((uint32_t *) virt_addr) = htobe32((uint32_t) value);            break;
    case MO_BEQ:  *((uint64_t *) virt_addr) = htobe64(value);                       break;
    case MO_BESW: *((uint16_t *) virt_addr) = htobe16((uint16_t) value);            break;
    case MO_BESL: *((uint32_t *) virt_addr) = htobe32((uint32_t) value);            break;
    default:
        printf("ERROR: unknown memory operation %d\n", memop);
        tcg_abort();
        break;
    }
#endif /* CONFIG_SOFTMMU */
}

