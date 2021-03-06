/*
 * romdir.c - read files from ROMDIR filesystem
 *
 * Copyright (C) 2009 Mathias Lafeldt <mathias.lafeldt@gmail.com>
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

#include <sys/queue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "romdir.h"

/**
 * roment_t - ROMDIR entry
 * @name: entry name
 * @xi_size: size of information in "EXTINFO" for this entry
 * @size: entry size
 *
 * This data structure represents one entry in the ROMDIR entry table.  The
 * first entry is always "RESET", followed by "ROMDIR" and "EXTINFO".
 */
typedef struct _roment {
	char		name[10];
	uint16_t	xi_size;
	uint32_t	size;
} roment_t;

/**
 * strhash - String hashing function as specified by the ELF ABI.
 * @name: string to calculate hash from
 * @return: 32-bit hash value
 */
uint32_t strhash(const char *name)
{
	const uint8_t *p = (uint8_t*)name;
	uint32_t h = 0, g;

	while (*p) {
		h = (h << 4) + *p++;
		if ((g = (h & 0xf0000000)) != 0)
			h ^= (g >> 24);
		h &= ~g;
	}

	return h;
}

#define ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))

/*
 * Parse buffer @buf for ROMDIR entries and add them to @dir.
 */
int romdir_read(const uint8_t *buf, size_t length, romdir_t *dir)
{
	roment_t *entry = NULL;
	romfile_t *file = NULL, *xinfo = NULL;
	off_t i, off = 0, xoff = 0;

	/* find ROMDIR entry named "RESET" */
	for (i = 0; i < length; i += sizeof(roment_t)) {
		if (!strcmp((char*)&buf[i], "RESET")) {
			entry = (roment_t*)&buf[i];
			break;
		}
	}

	if (entry == NULL)
		return -1; /* RESET not found */

	/* add ROMDIR entries to queue */
	do {
		/* ignore "-" entries containing only zeros */
		if (entry->name[0] != '-') {
			file = (romfile_t*)calloc(1, sizeof(romfile_t));
			if (file == NULL)
				return -1;

			strcpy(file->name, entry->name);
			file->hash = strhash(file->name);
			file->offset = off;
			file->size = entry->size;
			if (file->size > 0)
				file->data = &buf[off];

			if (entry->xi_size) {
				file->xi_offset = xoff;
				file->xi_size = entry->xi_size;
				xoff += file->xi_size;
			}

			STAILQ_INSERT_TAIL(dir, file, node);
		}

		/* offset must be aligned to 16 bytes */
		off += ALIGN(entry->size, 0x10);
	} while ((++entry)->name[0]);

	/* get extinfo for each file */
	xinfo = romdir_find_file(dir, HASH_EXTINFO);
	if (xinfo != NULL) {
		STAILQ_FOREACH(file, dir, node) {
			if (file->hash != HASH_EXTINFO && file->xi_size > 0)
				file->xi_data = xinfo->data + file->xi_offset;
		}
	}

	return 0;
}

/*
 * Extract @file from ROMDIR fs to @path.
 */
int romdir_extract(const romfile_t *file, const char *path)
{
	char fullpath[1024] = { 0 };
	int fd, ret = 0;

	if (path != NULL) {
		strcpy(fullpath, path);
		strcat(fullpath, "/");
	}

	strcat(fullpath, file->name);

	fd = open(fullpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1)
		return -1;

	if (file->data != NULL)
		ret = write(fd, file->data, file->size);

	close(fd);

	return ret;
}

/*
 * Search @dir for file by @hash.
 */
romfile_t *romdir_find_file(const romdir_t *dir, uint32_t hash)
{
	romfile_t *file;

	STAILQ_FOREACH(file, dir, node) {
		if (hash == file->hash)
			return file;
	}

	return NULL;
}
