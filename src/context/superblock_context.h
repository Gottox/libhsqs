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
 * @file         superblock_context.h
 */

#include "../mapper/mapper.h"
#include "../utils.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef SUPERBLOCK_CONTEXT_H

#define SUPERBLOCK_CONTEXT_H

struct HsqsSuperblock;

struct HsqsSuperblockContext {
	const struct HsqsSuperblock *superblock;
	struct HsqsMapping mapping;
};

HSQS_NO_UNUSED int hsqs_superblock_init(
		struct HsqsSuperblockContext *context, struct HsqsMapper *mapper);

const void *hsqs_superblock_data_from_offset(
		const struct HsqsSuperblockContext *context, uint64_t offset);

enum HsqsSuperblockCompressionId
hsqs_superblock_compression_id(const struct HsqsSuperblockContext *context);

uint64_t hsqs_superblock_directory_table_start(
		const struct HsqsSuperblockContext *context);

uint64_t hsqs_superblock_fragment_table_start(
		const struct HsqsSuperblockContext *context);

uint32_t
hsqs_superblock_inode_count(const struct HsqsSuperblockContext *context);

uint64_t
hsqs_superblock_inode_table_start(const struct HsqsSuperblockContext *context);

uint64_t
hsqs_superblock_id_table_start(const struct HsqsSuperblockContext *context);

uint16_t hsqs_superblock_id_count(const struct HsqsSuperblockContext *context);

uint64_t
hsqs_superblock_export_table_start(const struct HsqsSuperblockContext *context);

uint64_t hsqs_superblock_xattr_id_table_start(
		const struct HsqsSuperblockContext *context);

uint64_t
hsqs_superblock_inode_root_ref(const struct HsqsSuperblockContext *context);

bool hsqs_superblock_has_fragments(const struct HsqsSuperblockContext *context);

bool
hsqs_superblock_has_export_table(const struct HsqsSuperblockContext *context);

bool hsqs_superblock_has_compression_options(
		const struct HsqsSuperblockContext *context);

uint32_t
hsqs_superblock_block_size(const struct HsqsSuperblockContext *context);

uint32_t
hsqs_superblock_modification_time(const struct HsqsSuperblockContext *context);

uint32_t hsqs_superblock_fragment_entry_count(
		const struct HsqsSuperblockContext *context);

uint64_t
hsqs_superblock_bytes_used(const struct HsqsSuperblockContext *context);

int hsqs_superblock_cleanup(struct HsqsSuperblockContext *superblock);

#endif /* end of include guard SUPERBLOCK_CONTEXT_H */
