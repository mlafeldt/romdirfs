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
#include <sys/types.h>

/* Size of one ROMDIR entry */
#define ROMENT_SIZE	16

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
	u_int16_t	extinfo_size;
	u_int32_t	size;
} roment_t;

/**
 * romfile_t - ROMDIR file information
 * @name: file name
 * @size: file size
 * @extinfo_size: size of information in "EXTINFO" for this entry
 * @offset: file offset of data
 * @data: buffer holding file data, or NULL if size is 0
 * @hash: file name hash for fast searching
 *
 * This structure is used to hold the data and metadata of a ROMDIR file.
 */
typedef struct _romfile {
	char		name[10];
	u_int32_t	size;
	u_int16_t	extinfo_size;
	u_int32_t	offset;
	u_int8_t	*data;
	u_int32_t	hash;

	STAILQ_ENTRY(_romfile) node;
} romfile_t;

/* Queue to hold multiple ROMDIR files */
typedef STAILQ_HEAD(_romdir, _romfile) romdir_t;

u_int32_t strhash(const char *name);
int romdir_read(int fd, romdir_t *dir);
int romdir_extract(const romfile_t *file, const char *path);
romfile_t *romdir_find_file(const romdir_t *dir, u_int32_t hash);

#endif /* _ROMDIR_H_ */
