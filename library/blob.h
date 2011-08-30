/*
 * 2008+ Copyright (c) Evgeniy Polyakov <zbr@ioremap.net>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __EBLOB_BLOB_H
#define __EBLOB_BLOB_H

#include "eblob/blob.h"
#include "hash.h"
#include "lock.h"
#include "list.h"

#ifndef __unused
#define __unused	__attribute__ ((unused))
#endif

#define EBLOB_BLOB_INDEX_SUFFIX			".index"
#define EBLOB_BLOB_DEFAULT_HASH_SIZE		(1<<24)
#define EBLOB_BLOB_DEFAULT_BLOB_SIZE		50*1024*1024*1024ULL
#define EBLOB_BLOB_DEFAULT_RECORDS_IN_BLOB	10000000

/*
 * Compare two IDs.
 * Returns  1 when id1 > id2
 *         -1 when id1 < id2
 *          0 when id1 = id2
 */
static inline int eblob_id_cmp(const unsigned char *id1, const unsigned char *id2)
{
	unsigned int i = 0;

	for (i*=sizeof(unsigned long); i<EBLOB_ID_SIZE; ++i) {
		if (id1[i] < id2[i])
			return -1;
		if (id1[i] > id2[i])
			return 1;
	}

	return 0;
}

struct eblob_map_fd {
	int			fd;
	uint64_t		offset, size;
	
	void			*data;

	uint64_t		mapped_size;
	void			*mapped_data;
};

int eblob_data_map(struct eblob_map_fd *map);
void eblob_data_unmap(struct eblob_map_fd *map);

struct eblob_base_type {
	int			type, index;
	struct list_head	bases;
};

struct eblob_base_ctl {
	struct list_head	base_entry;

	int			type, index;

	pthread_mutex_t		lock;
	int			data_fd, index_fd;
	off_t			data_offset, index_offset;

	void			*data;
	unsigned long long	data_size;
	unsigned long long	index_size;

	atomic_t		refcnt;
	int			need_sorting;

	struct eblob_map_fd	sort;

	char			name[0];
};

void eblob_base_ctl_cleanup(struct eblob_base_ctl *ctl);

int eblob_base_setup_data(struct eblob_base_ctl *ctl);

struct eblob_stat {
	FILE			*file;
	pthread_mutex_t		lock;

	int			need_check;

	unsigned long long	disk;
	unsigned long long	removed;
	unsigned long long	hashed;
};

void eblob_stat_cleanup(struct eblob_stat *s);
int eblob_stat_init(struct eblob_stat *s, char *path);
void eblob_stat_update(struct eblob_stat *s, long long disk, long long removed, long long hashed);

struct eblob_backend {
	struct eblob_config	cfg;

	struct eblob_lock	csum_lock;

	pthread_mutex_t		lock;

	int			max_type;
	struct eblob_base_type	*types;

	struct eblob_hash	*hash;

	struct eblob_stat	stat;

	int			need_exit;
	pthread_t		defrag_tid;
	pthread_t		sync_tid;
};

int eblob_add_new_base(struct eblob_backend *b, int type);
int eblob_load_data(struct eblob_backend *b);
void eblob_base_types_cleanup(struct eblob_backend *b);

int eblob_lookup_type(struct eblob_backend *b, struct eblob_key *key, struct eblob_ram_control *res, int *diskp);
int eblob_remove_type(struct eblob_backend *b, struct eblob_key *key, int type);
int eblob_insert_type(struct eblob_backend *b, struct eblob_key *key, struct eblob_ram_control *ctl);

int eblob_disk_index_lookup(struct eblob_backend *b, struct eblob_key *key, int type,
		struct eblob_ram_control **dst, int *dsize);

int eblob_blob_iterate(struct eblob_iterate_control *ctl);

void *eblob_defrag(void *data);
void eblob_base_remove(struct eblob_backend *b, struct eblob_base_ctl *ctl);

int eblob_generate_sorted_index(struct eblob_backend *b, struct eblob_base_ctl *bctl);

#endif /* __EBLOB_BLOB_H */
