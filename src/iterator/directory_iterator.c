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
 * @file         directory.c
 */

#include "directory_iterator.h"
#include "../context/inode_context.h"
#include "../data/directory.h"
#include "../data/inode.h"
#include "../data/metablock.h"
#include "../error.h"
#include "../hsqs.h"
#include "directory_index_iterator.h"

#include <string.h>

static int
directory_iterator_index_lookup(
		struct HsqsDirectoryIterator *iterator, const char *name,
		const size_t name_len) {
	int rv = 0;
	struct HsqsInodeDirectoryIndexIterator index_iterator = {0};
	struct HsqsInodeContext *inode = iterator->inode;

	rv = hsqs_inode_directory_index_iterator_init(&index_iterator, inode);
	if (rv < 0) {
		return 0;
	}
	while ((rv = hsqs_inode_directory_index_iterator_next(&index_iterator)) >
		   0) {
		const char *index_name =
				hsqs_inode_directory_index_iterator_name(&index_iterator);
		uint32_t index_name_size =
				hsqs_inode_directory_index_iterator_name_size(&index_iterator);

		// BUG: the branch could be taken too early when the name is a prefix
		if (strncmp(name, (char *)index_name,
					MIN(index_name_size, name_len + 1)) < 0) {
			break;
		}
		iterator->next_offset =
				hsqs_inode_directory_index_iterator_index(&index_iterator);
	}
	iterator->remaining_entries = 0;
	return rv;
}

static const struct HsqsDirectoryFragment *
directory_iterator_current_fragment(
		const struct HsqsDirectoryIterator *iterator) {
	const uint8_t *tmp = (const uint8_t *)iterator->fragments;
	return (const struct HsqsDirectoryFragment
					*)&tmp[iterator->current_fragment_offset];
}

static int
directory_data_more(struct HsqsDirectoryIterator *iterator, size_t size) {
	int rv = hsqs_metablock_stream_more(&iterator->metablock, size);
	if (rv < 0) {
		return rv;
	}

	iterator->fragments =
			(struct HsqsDirectoryFragment *)hsqs_metablock_stream_data(
					&iterator->metablock);
	return 0;
}

static struct HsqsDirectoryEntry *
current_entry(const struct HsqsDirectoryIterator *iterator) {
	const uint8_t *tmp = (const uint8_t *)iterator->fragments;
	return (struct HsqsDirectoryEntry *)&tmp[iterator->current_offset];
}

int
hsqs_directory_iterator_lookup(
		struct HsqsDirectoryIterator *iterator, const char *name,
		const size_t name_len) {
	int rv = 0;

	rv = directory_iterator_index_lookup(iterator, name, name_len);
	if (rv < 0)
		return rv;

	while (hsqs_directory_iterator_next(iterator) > 0) {
		size_t entry_name_size = hsqs_directory_iterator_name_size(iterator);
		const char *entry_name = hsqs_directory_iterator_name(iterator);
		if (name_len != entry_name_size) {
			continue;
		}
		if (strncmp(name, (char *)entry_name, entry_name_size) == 0) {
			return 0;
		}
	}

	return -HSQS_ERROR_NO_SUCH_FILE;
}

int
hsqs_directory_iterator_init(
		struct HsqsDirectoryIterator *iterator,
		struct HsqsInodeContext *inode) {
	int rv = 0;
	struct Hsqs *hsqs = inode->hsqs;
	struct HsqsSuperblockContext *superblock = hsqs_superblock(hsqs);

	if (hsqs_inode_type(inode) != HSQS_INODE_TYPE_DIRECTORY) {
		return -HSQS_ERROR_NOT_A_DIRECTORY;
	}
	iterator->block_start = hsqs_inode_directory_block_start(inode);
	iterator->block_offset = hsqs_inode_directory_block_offset(inode);
	iterator->size = hsqs_inode_file_size(inode) - 3;
	iterator->inode = inode;

	rv = hsqs_metablock_stream_init(
			&iterator->metablock, hsqs,
			hsqs_superblock_directory_table_start(superblock), ~0);
	if (rv < 0) {
		return rv;
	}
	rv = hsqs_metablock_stream_seek(
			&iterator->metablock, iterator->block_start,
			iterator->block_offset);
	if (rv < 0) {
		return rv;
	}

	iterator->current_fragment_offset = 0;
	iterator->remaining_entries = 0;
	iterator->next_offset = 0;
	iterator->current_offset = 0;

	return rv;
}

int
hsqs_directory_iterator_name_size(
		const struct HsqsDirectoryIterator *iterator) {
	const struct HsqsDirectoryEntry *entry = current_entry(iterator);
	return hsqs_data_directory_entry_name_size(entry) + 1;
}

uint64_t
hsqs_directory_iterator_inode_ref(
		const struct HsqsDirectoryIterator *iterator) {
	const struct HsqsDirectoryFragment *fragment =
			directory_iterator_current_fragment(iterator);
	uint32_t block_index = hsqs_data_directory_fragment_start(fragment);
	uint16_t block_offset =
			hsqs_data_directory_entry_offset(current_entry(iterator));

	return hsqs_inode_ref_from_block(block_index, block_offset);
}

enum HsqsInodeContextType
hsqs_directory_iterator_inode_type(
		const struct HsqsDirectoryIterator *iterator) {
	switch (hsqs_data_directory_entry_type(current_entry(iterator))) {
	case HSQS_INODE_TYPE_BASIC_DIRECTORY:
		return HSQS_INODE_TYPE_DIRECTORY;
	case HSQS_INODE_TYPE_BASIC_FILE:
		return HSQS_INODE_TYPE_FILE;
	case HSQS_INODE_TYPE_BASIC_SYMLINK:
		return HSQS_INODE_TYPE_SYMLINK;
	case HSQS_INODE_TYPE_BASIC_BLOCK:
		return HSQS_INODE_TYPE_BLOCK;
	case HSQS_INODE_TYPE_BASIC_CHAR:
		return HSQS_INODE_TYPE_CHAR;
	case HSQS_INODE_TYPE_BASIC_FIFO:
		return HSQS_INODE_TYPE_FIFO;
	case HSQS_INODE_TYPE_BASIC_SOCKET:
		return HSQS_INODE_TYPE_SOCKET;
	}
	return HSQS_INODE_TYPE_UNKNOWN;
}

int
hsqs_directory_iterator_inode_load(
		const struct HsqsDirectoryIterator *iterator,
		struct HsqsInodeContext *inode) {
	uint64_t inode_ref = hsqs_directory_iterator_inode_ref(iterator);
	struct Hsqs *hsqs = iterator->inode->hsqs;

	return hsqs_inode_load_by_ref(inode, hsqs, inode_ref);
}

int
hsqs_directory_iterator_next(struct HsqsDirectoryIterator *iterator) {
	int rv = 0;
	iterator->current_offset = iterator->next_offset;

	if (iterator->next_offset >= iterator->size) {
		// TODO: Check if 0 is really okay here.
		return 0;
	} else if (iterator->remaining_entries == 0) {
		// New fragment begins
		iterator->next_offset += HSQS_SIZEOF_DIRECTORY_FRAGMENT;

		rv = directory_data_more(iterator, iterator->next_offset);
		if (rv < 0) {
			return rv;
		}
		iterator->current_fragment_offset = iterator->current_offset;

		const struct HsqsDirectoryFragment *current_fragment =
				directory_iterator_current_fragment(iterator);
		iterator->remaining_entries =
				hsqs_data_directory_fragment_count(current_fragment) + 1;

		iterator->current_offset = iterator->next_offset;
	}
	iterator->remaining_entries--;

	// Make sure next entry is loaded:
	iterator->next_offset += HSQS_SIZEOF_DIRECTORY_ENTRY;
	rv = directory_data_more(iterator, iterator->next_offset);
	if (rv < 0) {
		return rv;
	}

	// Make sure next entry has its name populated
	iterator->next_offset += hsqs_directory_iterator_name_size(iterator);
	// May invalidate pointers into directory entries. that's why the
	// current_entry() call is repeated below.
	rv = directory_data_more(iterator, iterator->next_offset);
	if (rv < 0) {
		return rv;
	}

	return 1;
}

int
hsqs_directory_iterator_cleanup(struct HsqsDirectoryIterator *iterator) {
	int rv = 0;
	rv = hsqs_metablock_stream_cleanup(&iterator->metablock);
	return rv;
}

const char *
hsqs_directory_iterator_name(const struct HsqsDirectoryIterator *iterator) {
	const struct HsqsDirectoryEntry *entry = current_entry(iterator);
	return (char *)hsqs_data_directory_entry_name(entry);
}

int
hsqs_directory_iterator_name_dup(
		const struct HsqsDirectoryIterator *iterator, char **name_buffer) {
	int size = hsqs_directory_iterator_name_size(iterator);
	const char *entry_name = hsqs_directory_iterator_name(iterator);

	*name_buffer = hsqs_memdup(entry_name, size);
	if (*name_buffer) {
		return size;
	} else {
		return -HSQS_ERROR_MALLOC_FAILED;
	}
}
