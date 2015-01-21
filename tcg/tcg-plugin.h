/*
 * Copyright (C) 2011 STMicroelectronics
 * Copyright (C) 2014-2015 EURECOM
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-license-identifier: MIT
 */

/**
 * TCG Plugin support.
 *
 * @author Jonas Zaddach <zaddach@eurecom.fr>
 * @license MIT
 */

#ifndef TCG_PLUGIN_H
#define TCG_PLUGIN_H

#include "exec/exec-all.h"
#include "tcg.h"
#include "tcgplugin-hooks.h"

#ifndef TCG_TARGET_REG_BITS
#include "tcg.h"
#endif

#if TARGET_LONG_BITS == 32
#define MAKE_TCGV MAKE_TCGV_I32
#else
#define MAKE_TCGV MAKE_TCGV_I64
#endif

enum MemoryAccessType
{
    MEMORY_ACCESS_TYPE_READ = 0,
    MEMORY_ACCESS_TYPE_WRITE = 1
};

/***********************************************************************
 * Hooks inserted into QEMU here and there.
 */

#ifdef CONFIG_TCG_PLUGIN
    void tcg_plugin_guest_arch_init(TCGv_ptr cpu_env);
    void tcg_plugin_register_helpers(TCGContext* s);
    void tcg_plugin_register_info(uint64_t pc, CPUArchState *env, TCGContext *s, TranslationBlock *tb);
    void tcg_plugin_before_gen_tb(CPUArchState *env, TCGContext *s, TranslationBlock *tb);
    void tcg_plugin_after_gen_tb(CPUArchState *env, TCGContext *s, TranslationBlock *tb);
    void tcg_plugin_after_gen_opc(TCGOpcode opname, uint16_t *opcode, TCGArg *opargs, uint8_t nb_args);
    void tcgplugin_tb_flush(TCGContext *tcg_ctx, CPUArchState *env);
#else
#   define tcg_plugin_guest_arch_init(cpu_env)
#   define tcg_plugin_register_helpers(tcg_ctx)
#   define tcg_plugin_register_info(pc, env, tb)
#   define tcg_plugin_before_gen_tb(env, tb)
#   define tcg_plugin_after_gen_tb(env, tb)
#   define tcg_plugin_after_gen_opc(opname, tcg_opcode, tcg_opargs_, nb_args)
#   define tcgplugin_tb_flush(tcg_ctx, env)
#endif /* !CONFIG_TCG_PLUGIN */

/***********************************************************************
 * TCG plugin interface.
 */

/* This structure shall be 64 bits, see call_tb_helper_code() for
 * details.  */
typedef struct
{
    uint16_t cpu_index;
    uint16_t size;
    union {
        char type;
        uint32_t icount;
    };
} __attribute__((__packed__, __may_alias__)) TPIHelperInfo;

#define TPI_MAX_OP_ARGS 6
typedef struct
{
    TCGOpcode name;
    uint64_t pc;
    uint8_t nb_args;

    uint16_t *opcode;
    TCGArg *opargs;

    /* Should be used by the plugin only.  */
    void *data;
} TPIOpCode;

struct TCGPluginInterface;
typedef struct TCGPluginInterface TCGPluginInterface;

typedef void (* tpi_cpus_stopped_t)(const TCGPluginInterface *tpi);

typedef void (* tpi_before_gen_tb_t)(const TCGPluginInterface *tpi);
typedef void (* tpi_after_gen_tb_t)(const TCGPluginInterface *tpi);

typedef void (* tpi_after_gen_opc_t)(const TCGPluginInterface *tpi, const TPIOpCode *opcode);

typedef void (* tpi_decode_instr_t)(const TCGPluginInterface *tpi, uint64_t pc);

typedef void (* tpi_pre_tb_helper_code_t)(const TCGPluginInterface *tpi,
                                          TPIHelperInfo info, uint64_t address,
                                          uint64_t data1, uint64_t data2);

typedef void (* tpi_pre_tb_helper_data_t)(const TCGPluginInterface *tpi,
                                          TPIHelperInfo info, uint64_t address,
                                          uint64_t *data1, uint64_t *data2);
typedef void (* tpi_register_helpers_t)(const TCGPluginInterface *tpi);
typedef void (* tpi_shutdown_request_t)(const TCGPluginInterface *tpi, int signal, pid_t pid);
typedef void (* tpi_machine_init_done_t)(const TCGPluginInterface *tpi);
typedef void (* tpi_exit_t)(const TCGPluginInterface *tpi);
typedef void (* tpi_parse_cmdline_t)(const TCGPluginInterface *tpi, int argc, char ** argv);
typedef void (* tpi_tb_alloc)(const TCGPluginInterface *tpi, TranslationBlock *tb);
typedef void (* tpi_tb_free)(const TCGPluginInterface *tpi, TranslationBlock *tb);
typedef void (* tpi_tb_flush)(const TCGPluginInterface *tpi, TCGContext *tcg_ctx, CPUArchState *env);
typedef void (* tpi_monitor_memory_access)(const TCGPluginInterface *tpi, uint64_t address, uint32_t mmu_idx, uint64_t val, uint32_t memop_size, enum MemoryAccessType type);
typedef uint64_t (* tpi_intercept_memory_load)(const TCGPluginInterface *tpi, uint64_t address, uint32_t mmu_idx, uint32_t memop_size);
typedef void (* tpi_intercept_memory_store)(const TCGPluginInterface *tpi, uint64_t address, uint32_t mmu_idx, uint64_t val, uint32_t memop_size);
#define TPI_VERSION 3
struct TCGPluginInterface
{
    /* Compatibility information.  */
    int version;
    const char *guest;
    const char *mode;
    size_t sizeof_CPUState;
    size_t sizeof_TranslationBlock;

    /* Common parameters.  */
    int nb_cpus;
    FILE *output;
    uint64_t low_pc;
    uint64_t high_pc;
    bool verbose;

    /* Parameters for non-generic plugins.  */
    bool is_generic;
    CPUArchState *env;
    TranslationBlock *tb;
    TCGContext *tcg_ctx;

    /* Enablers for translation-time specific hooks */
    bool do_monitor_memory_access;
    bool do_intercept_memory_access;

    /* Plugin's callbacks.  */
    tpi_cpus_stopped_t cpus_stopped;
    tpi_before_gen_tb_t before_gen_tb;
    tpi_after_gen_tb_t  after_gen_tb;
    tpi_pre_tb_helper_code_t pre_tb_helper_code;
    tpi_pre_tb_helper_data_t pre_tb_helper_data;
    tpi_after_gen_opc_t after_gen_opc;
    tpi_decode_instr_t decode_instr;
    tpi_register_helpers_t register_helpers;
    tpi_shutdown_request_t shutdown_request;
    tpi_machine_init_done_t machine_init_done;
    tpi_exit_t exit;
    tpi_parse_cmdline_t parse_cmdline;
    tpi_tb_alloc tb_alloc;
    tpi_tb_free tb_free;
    tpi_tb_flush tb_flush;
    tpi_monitor_memory_access monitor_memory_access;
    tpi_intercept_memory_load intercept_memory_load;
    tpi_intercept_memory_store intercept_memory_store;
};

#define TPI_INIT_VERSION(tpi) do {                                     \
        (tpi).version = TPI_VERSION;                                   \
        (tpi).guest   = TARGET_NAME;                                   \
        (tpi).mode    = TARGET_EMULATION_MODE;                         \
        (tpi).sizeof_CPUState = sizeof(CPUArchState);                  \
        (tpi).sizeof_TranslationBlock = sizeof(TranslationBlock);      \
    } while (0);

#define TPI_INIT_VERSION_GENERIC(tpi) do {                             \
        (tpi).version = TPI_VERSION;                                   \
        (tpi).guest   = "any";                                         \
        (tpi).mode    = "any";                                         \
        (tpi).sizeof_CPUState = 0;                                     \
        (tpi).sizeof_TranslationBlock = 0;                             \
    } while (0);

typedef void (* tpi_init_t)(TCGPluginInterface *tpi);
void tpi_init(TCGPluginInterface *tpi);

void tcgplugin_guest_arch_init(TCGv_ptr cpu_env);

extern bool tcgplugin_intercept_qemu_ldst;
extern bool tcgplugin_monitor_qemu_ldst;

#endif /* TCG_PLUGIN_H */
