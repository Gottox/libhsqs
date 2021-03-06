/******************************************************************************
 *                                                                            *
 * Copyright (c) 2022, Enno Boland <g@s01.de>                                 *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions are     *
 * met:                                                                       *
 *                                                                            *
 * * Redistributions of source code must retain the above copyright notice,   *
 *   this list of conditions and the following disclaimer.                    *
 * * Redistributions in binary form must reproduce the above copyright        *
 *   notice, this list of conditions and the following disclaimer in the      *
 *   documentation and/or other materials provided with the distribution.     *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS    *
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,  *
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR     *
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR          *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,      *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,        *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR         *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF     *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING       *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               *
 *                                                                            *
 ******************************************************************************/

/**
 * @author       Enno Boland (mail@eboland.de)
 * @file         utils.h
 */

#include <stddef.h>
#include <stdint.h>

#ifndef HSQS_UTILS_H

#define HSQS_UTILS_H

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

#define ADD_OVERFLOW(a, b, res) __builtin_add_overflow(a, b, res)
#define SUB_OVERFLOW(a, b, res) __builtin_sub_overflow(a, b, res)
#define MULT_OVERFLOW(a, b, res) __builtin_mul_overflow(a, b, res)

#define HSQS_NO_UNUSED __attribute__((warn_unused_result))
#define HSQS_UNALIGNED __attribute__((packed, aligned(1)))

#define STATIC_ASSERT(cond) _Static_assert(cond, #cond)

#define HSQS_DEVIDE_CEIL(x, y) (((x - 1) / y) + 1)

#define HSQS_PADDING(x, p) HSQS_DEVIDE_CEIL(x, p) * p

typedef size_t hsqs_index_t;

HSQS_NO_UNUSED void *hsqs_memdup(const void *source, size_t size);

#endif /* end of include guard HSQS_UTILS_H */
