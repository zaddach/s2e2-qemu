/*
 * TCG plugin for QEMU: simulate a IO memory mapped device (mainly
 *                      interesting to prototype things in user-mode).
 *
 * Copyright (C) 2011 STMicroelectronics
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
 */

/* You can test this plugin in user-mode with the following code:
 *
 * int main(void)
 * {
 * 	char *device = mmap(0xCAFE0000, 1024, PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
 * 	return printf("printf(%p): %s\n", device, device);
 * }
 */

#include <stdint.h>
#include <inttypes.h>

#include "qemu-common.h"
#include "tcg.h"
#include "tcg-op.h"
#include "tcg-plugin.h"

const char *quote = "Real programmers can write assembly code in any language.  :-)\n\t-- Larry Wall";
#define BASE 0xCAFE0000

static uint64_t after_exec_opc(uint64_t address, uint64_t value, uint32_t max_size)
{
    size_t max_index = strlen(quote);
    size_t index     = address - BASE;
    size_t size      = MIN(max_size, max_index - index + 1);

    if (address < BASE || address > BASE + max_index)
        return value;

    memcpy(&value, &quote[index], size);
    return value;
}

static void after_gen_opc(const TCGPluginInterface *tpi, const TPIOpCode *tpi_opcode)
{
    uint32_t size;

    switch (tpi_opcode->name) {
    case INDEX_op_ld8s_i32:
    case INDEX_op_ld8u_i32:
    case INDEX_op_ld8s_i64:
    case INDEX_op_ld8u_i64:    
        size = 1;
        break;

    case INDEX_op_ld16s_i32:
    case INDEX_op_ld16u_i32:
    case INDEX_op_ld16s_i64:
    case INDEX_op_ld16u_i64:
        size = 2;
        break;

    case INDEX_op_ld_i32:
    case INDEX_op_ld32s_i64:
    case INDEX_op_ld32u_i64:
        size = 4;
        break;

    case INDEX_op_ld_i64:
        size = 8;
        break;

    default:
        return;
    }

    TCGArg args[3];

    TCGv_i64 tcgv_ret  = tcg_temp_new_i64();
    TCGv_i32 tcgv_size = tcg_const_i32(size);

    args[0] = tpi_opcode->opargs[1];
    args[1] = tpi_opcode->opargs[0];
    args[2] = GET_TCGV_I32(tcgv_size);

    tcg_gen_callN(tpi->tcg_ctx, after_exec_opc, GET_TCGV_I64(tcgv_ret), 3, args);

#if TCG_TARGET_REG_BITS == 64
    tcg_gen_mov_i64(MAKE_TCGV_I64(tpi_opcode->opargs[0]), tcgv_ret);
#else
    if (size == 8) {
        tcg_gen_mov_i64(MAKE_TCGV_I64(tpi_opcode->opargs[0]), tcgv_ret);
    }
    else {
        tcg_gen_trunc_i64_i32(MAKE_TCGV_I32(tpi_opcode->opargs[0]), tcgv_ret);
    }
#endif

    tcg_temp_free_i32(tcgv_size);
    tcg_temp_free_i64(tcgv_ret);
}


void tpi_init(TCGPluginInterface *tpi)
{
    TPI_INIT_VERSION(*tpi);
    tpi->after_gen_opc = after_gen_opc;
}
