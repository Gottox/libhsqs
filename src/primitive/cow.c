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
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               *
 *                                                                            *
 ******************************************************************************/

/**
 * @author       Enno Boland (mail@eboland.de)
 * @file         cow.c
 */

#include "cow.h"
#include "../mapper/mapper.h"
#include <stdbool.h>
#include <stdlib.h>

int
hsqs_cow_init(struct HsqsCow *cow, int compression_id, int block_size) {
	cow->compression_id = compression_id;
	cow->block_size = block_size;

	cow->state = HSQS_COW_EMPTY;
	cow->content.mapping.rc = NULL;
	cow->content.mapping.mapping = NULL;

	return 0;
}

static int
cow_init_buffered(struct HsqsCow *cow) {
	int rv = 0;

	struct HsqsRefCount *mapping_rc = cow->content.mapping.rc;
	const uint8_t *source = hsqs_cow_data(cow);
	const size_t source_size = hsqs_cow_size(cow);
	struct HsqsBuffer *buffer = &cow->content.buffer;

	rv = hsqs_buffer_init(buffer, cow->compression_id, cow->block_size);
	if (rv < 0) {
		return rv;
	}

	if (cow->state != HSQS_COW_EMPTY) {
		rv = hsqs_buffer_append(buffer, source, source_size);
	}

	hsqs_ref_count_release(mapping_rc);
	cow->state = HSQS_COW_BUFFERED;

	return rv;
}

static int
cow_init_pass_through(
		struct HsqsCow *cow, struct HsqsRefCount *mapping_rc,
		const size_t mapping_index, const size_t mapping_size) {
	cow->state = HSQS_COW_PASS_THROUGH;
	cow->content.mapping.rc = mapping_rc;
	cow->content.mapping.mapping = hsqs_ref_count_retain(mapping_rc);
	cow->content.mapping.offset = mapping_index;
	cow->content.mapping.size = mapping_size;
	return 0;
}

int
hsqs_cow_append_block(
		struct HsqsCow *cow, struct HsqsRefCount *mapping,
		const size_t mapping_index, const size_t mapping_size,
		bool is_compressed) {
	int rv = 0;

	if (cow->state == HSQS_COW_EMPTY && !is_compressed) {
		return cow_init_pass_through(cow, mapping, mapping_index, mapping_size);
	}

	// TODO: check if mapping is cohesive

	if (cow->state != HSQS_COW_BUFFERED) {
		rv = cow_init_buffered(cow);
		if (rv < 0) {
			return rv;
		}
	}

	struct HsqsMapping *m = hsqs_ref_count_retain(mapping);
	const uint8_t *data = hsqs_mapping_data(m);

	rv = hsqs_buffer_append_block(
			&cow->content.buffer, &data[mapping_index], mapping_size,
			is_compressed);
	hsqs_ref_count_release(mapping);

	return rv;
}

const uint8_t *
hsqs_cow_data(const struct HsqsCow *cow) {
	struct HsqsMapping *mapping;
	const uint8_t *data;
	switch (cow->state) {
	case HSQS_COW_EMPTY:
		return NULL;
	case HSQS_COW_PASS_THROUGH:
		mapping = cow->content.mapping.mapping;
		data = hsqs_mapping_data(mapping);
		return &data[cow->content.mapping.offset];
	case HSQS_COW_BUFFERED:
		return hsqs_buffer_data(&cow->content.buffer);
	}
	abort();
}

size_t
hsqs_cow_size(const struct HsqsCow *cow) {
	switch (cow->state) {
	case HSQS_COW_EMPTY:
		return 0;
	case HSQS_COW_PASS_THROUGH:
		return cow->content.mapping.size;
	case HSQS_COW_BUFFERED:
		return hsqs_buffer_size(&cow->content.buffer);
	}
	abort();
}

int
hsqs_cow_cleanup(struct HsqsCow *cow) {
	if (cow->state == HSQS_COW_BUFFERED) {
		return hsqs_buffer_cleanup(&cow->content.buffer);
	}
	hsqs_ref_count_release(cow->content.mapping.rc);
	hsqs_buffer_cleanup(&cow->content.buffer);

	cow->content.mapping.mapping = NULL;
	cow->content.mapping.rc = NULL;
	cow->content.mapping.size = 0;
	cow->content.mapping.offset = 0;
	cow->state = HSQS_COW_EMPTY;
	return 0;
}
