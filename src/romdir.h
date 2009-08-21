/*
 * romdir.h - read files from ROMDIR filesystem
 *
 * Copyright (C) 2009 misfire <misfire@xploderfreax.de>
 *
 * This file is part of romdirfs.
 *
 * romdirfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * romdirfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with romdirfs.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ROMDIR_H_
#define _ROMDIR_H_

#include <sys/queue.h>
#include <stdint.h>

/**
 * roment_t - ROMDIR entry
 * @name: entry name
 * @extinfo_size: size of information in "EXTINFO" for this entry
 * @size: entry size
 *
 * This data structure represents one entry in the ROMDIR entry table.  The
 * first entry is always "RESET", followed by "ROMDIR" and "EXTINFO".
 */
typedef struct _roment {
	char		name[10];
	uint16_t	extinfo_size;
	uint32_t	size;
} roment_t;

/* Some well-known file name hashes */
#define HASH_RESET	0x0056a7a4
#define HASH_ROMDIR	0x057418e2
#define HASH_EXTINFO	0x0ad8e2ef
#define HASH_ROMVER	0x05742aa2
#define HASH_OSDSYS	0x054798e3

/**
 * romfile_t - ROMDIR file information
 * @name: file name
 * @hash: file name hash for fast searching
 * @size: file size
 * @data: buffer holding file data, or NULL if size is 0
 * @extinfo_size: size of information in "EXTINFO" for this entry
 * @extinfo_offset: offset of information in "EXTINFO" for this entry
 *
 * This structure is used to hold the data and metadata of a ROMDIR file.
 */
typedef struct _romfile {
	char		name[10];
	uint32_t	hash;
	uint32_t	size;
	const uint8_t	*data;
	uint16_t	extinfo_size;
	uint32_t	extinfo_offset;

	STAILQ_ENTRY(_romfile) node;
} romfile_t;

/* Queue to hold multiple ROMDIR files */
typedef STAILQ_HEAD(_romdir, _romfile) romdir_t;

uint32_t strhash(const char *name);
int romdir_read(const uint8_t *buf, size_t length, romdir_t *dir);
int romdir_extract(const romfile_t *file, const char *path);
romfile_t *romdir_find_file(const romdir_t *dir, uint32_t hash);

#endif /* _ROMDIR_H_ */
