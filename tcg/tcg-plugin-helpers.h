/*
 * tcg-plugin-helpers.h
 *
 *  Created on: Nov 6, 2014
 *      Author: zaddach
 */

#ifndef TCG_TCG_PLUGIN_HELPERS_H_
#define TCG_TCG_PLUGIN_HELPERS_H_

#include "tcg.h"

#if TARGET_LONG_BITS == 32
#define GET_TCGV(x) GET_TCGV_I32(x)
#else
#define GET_TCGV(x) GET_TCGV_I64(x)
#endif

extern TCGv_ptr tcgplugin_cpu_env;

uint32_t tcgplugin_helper_intercept_qemu_ld_i32(CPUArchState *env, target_ulong addr, uint32_t idx, uint32_t memop);
void tcgplugin_helper_post_qemu_ld_i32(CPUArchState *env, target_ulong addr, uint32_t idx, uint32_t val, uint32_t memop);
void tcgplugin_helper_intercept_qemu_st_i32(CPUArchState *env, target_ulong addr, uint32_t idx, uint32_t val, uint32_t memop);
void tcgplugin_helper_post_qemu_st_i32(CPUArchState *env, target_ulong addr, uint32_t idx, uint32_t val, uint32_t memop);

uint64_t tcgplugin_helper_intercept_qemu_ld_i64(CPUArchState *env, target_ulong addr, uint32_t idx, uint32_t memop);
void tcgplugin_helper_post_qemu_ld_i64(CPUArchState *env, target_ulong addr, uint32_t idx, uint64_t val, uint32_t memop);
void tcgplugin_helper_intercept_qemu_st_i64(CPUArchState *env, target_ulong addr, uint32_t idx, uint64_t val, uint32_t memop);
void tcgplugin_helper_post_qemu_st_i64(CPUArchState *env, target_ulong addr, uint32_t idx, uint64_t val, uint32_t memop);


#define TCGPLUGIN_GEN_HELPER_INTERCEPT_LD(type)                                     \
		static inline void glue(tcgplugin_gen_helper_intercept_ld_, type)(          \
				TCGContext *s,                                                      \
				glue(TCGv_, type) val,                                              \
				TCGv addr,                                                          \
				TCGv_i32 idx,                                                       \
				TCGv_i32 memop)                                                     \
{                                                                                   \
	TCGArg args[4] = {                                                              \
			GET_TCGV_PTR(tcgplugin_cpu_env),                                        \
			GET_TCGV(addr),                                                         \
			GET_TCGV_I32(idx),                                                      \
			GET_TCGV_I32(memop)};                                                   \
																		            \
			tcg_gen_callN(s,                                                        \
					      (void *) glue(tcgplugin_helper_intercept_qemu_ld_, type), \
						  glue(GET_TCGV_, type)(val), 4, args);                     \
}


#define TCGPLUGIN_GEN_HELPER_POST_LD(type)                                          \
		static inline void glue(tcgplugin_gen_helper_post_qemu_ld_, type)(          \
		TCGContext *s,                                                              \
		TCGv addr,                                                                  \
		TCGv_i32 idx,                                                               \
		glue(TCGv_, type) val,                                                      \
		TCGv_i32 memop)                                                             \
{                                                                                   \
	TCGArg args[5] = {                                                              \
			GET_TCGV_PTR(tcgplugin_cpu_env),                                        \
			GET_TCGV(addr),                                                                   \
			GET_TCGV_I32(idx),                                                      \
			glue(GET_TCGV_, type)(val),                                             \
			GET_TCGV_I32(memop)};                                                   \
                                                                                    \
	tcg_gen_callN(s,                                                                \
			(void *) glue(tcgplugin_helper_post_qemu_ld_, type),                    \
			TCG_CALL_DUMMY_ARG, 5, args);                                           \
}

#define TCGPLUGIN_GEN_HELER_INTERCEPT_ST(type)                                      \
static inline void glue(tcgplugin_gen_helper_intercept_st_, type)(                  \
		TCGContext *s,                                                              \
		TCGv addr,                                                                  \
		TCGv_i32 idx,                                                               \
		glue(TCGv_, type) val,                                                      \
		TCGv_i32 memop)                                                             \
{                                                                                   \
	TCGArg args[5] = {                                                              \
			GET_TCGV_PTR(tcgplugin_cpu_env),                                        \
			GET_TCGV(addr),                                                         \
			GET_TCGV_I32(idx),                                                      \
			glue(GET_TCGV_, type)(val),                                             \
			GET_TCGV_I32(memop)};                                                   \
                                                                                    \
	tcg_gen_callN(s,                                                                \
			(void *) glue(tcgplugin_helper_intercept_qemu_st_, type),               \
			TCG_CALL_DUMMY_ARG, 5, args);                                           \
}

#define TCGPLUGIN_GEN_HELPER_POST_ST(type)                                          \
static inline void glue(tcgplugin_gen_helper_post_qemu_st_, type)(                  \
		TCGContext *s,                                                              \
		TCGv addr,                                                                  \
		TCGv_i32 idx,                                                               \
		glue(TCGv_, type) val,                                                      \
		TCGv_i32 memop)                                                             \
{                                                                                   \
	TCGArg args[5] = {                                                              \
			GET_TCGV_PTR(tcgplugin_cpu_env),                                        \
			GET_TCGV(addr),                                                         \
			GET_TCGV_I32(idx),                                                      \
			glue(GET_TCGV_, type)(val),                                             \
			GET_TCGV_I32(memop)};                                                   \
                                                                                    \
	tcg_gen_callN(s,                                                                \
		(void *) glue(tcgplugin_helper_post_qemu_st_, type),                        \
		TCG_CALL_DUMMY_ARG, 5, args);                                               \
}


//static inline void tcgplugin_gen_helper_intercept_ld_i32(
//				TCGContext *s,
//				TCGv val,
//				TCGv_i32 addr,
//				TCGv_i32 idx,
//				TCGv_i32 memop)
//{
//	TCGArg args[4] = {
//			GET_TCGV_PTR(tcgplugin_cpu_env),
//			GET_TCGV(addr),
//			GET_TCGV_I32(idx),
//			GET_TCGV_I32(memop)};
//
//			tcg_gen_callN(s,
//					      (void *) tcgplugin_helper_intercept_qemu_ld_i2, type),
//						  glue(GET_TCGV_, type)(val), 4, args);                     \
//}

TCGPLUGIN_GEN_HELPER_INTERCEPT_LD(i32)
TCGPLUGIN_GEN_HELPER_INTERCEPT_LD(i64)

TCGPLUGIN_GEN_HELPER_POST_LD(i32)
TCGPLUGIN_GEN_HELPER_POST_LD(i64)

TCGPLUGIN_GEN_HELER_INTERCEPT_ST(i32)
TCGPLUGIN_GEN_HELER_INTERCEPT_ST(i64)

TCGPLUGIN_GEN_HELPER_POST_ST(i32)
TCGPLUGIN_GEN_HELPER_POST_ST(i64)

#undef TCGPLUGIN_GEN_HELPER_INTERCEPT_LD
#undef TCGPLUGIN_GEN_HELPER_POST_LD
#undef TCGPLUGIN_GEN_HELER_INTERCEPT_ST
#undef TCGPLUGIN_GEN_HELPER_POST_ST
#undef GET_TCGV


#endif /* TCG_TCG_PLUGIN_HELPERS_H_ */
