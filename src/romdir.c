/*
 * romdir.c - read files from ROMDIR filesystem
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

#include <sys/queue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "romdir.h"

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
 * Parse file for ROMDIR entries and add them to @dir.
 */
int romdir_read(const uint8_t *buf, size_t length, romdir_t *dir)
{
	roment_t entry;
	romfile_t *file = NULL;
	uint32_t offset = 0, extinfo_offset = 0;
	int reset_found = 0;

#if 0
	/* find ROMDIR entry named "RESET" */
	while (read(fd, &entry, sizeof(roment_t)) == sizeof(roment_t)) {
		if (!strcmp(entry.name, "RESET")) {
			reset_found = 1;
			break;
		}
	}
#endif
	if (!reset_found)
		return -1;

	/* read ROMDIR entries and add them to the queue */
	do {
		/* ignore "-" entries containing only zeros */
		if (entry.name[0] != '-') {
			file = (romfile_t*)calloc(1, sizeof(romfile_t));
			if (file == NULL)
				return -1;

			strcpy(file->name, entry.name);
			file->size = entry.size;
			file->offset = offset;
			file->data = NULL; /* will be read later */
			file->hash = strhash(file->name);

			if (entry.extinfo_size) {
				file->extinfo_size = entry.extinfo_size;
				file->extinfo_offset = extinfo_offset;
				extinfo_offset += file->extinfo_size;
			}

			STAILQ_INSERT_TAIL(dir, file, node);
		}

		/* offset must be aligned to 16 bytes */
		offset += ALIGN(entry.size, 0x10);

	} while (0); //(read(fd, &entry, sizeof(roment_t)) == sizeof(roment_t) && entry.name[0]);
#if 0
	/* read data for each file */
	STAILQ_FOREACH(file, dir, node) {
		if (!file->size)
			continue;
		if (lseek(fd, file->offset, SEEK_SET) == -1)
			return -1;
		if ((file->data = (uint8_t*)malloc(file->size)) == NULL)
			return -1;
		if (read(fd, file->data, file->size) != file->size) {
			free(file->data);
			file->data = NULL;
			return -1;
		}
	}
#endif
	return 0;
}

/*
 * Extract file from ROMDIR fs to path.
 */
int romdir_extract(const romfile_t *file, const char *path)
{
	char fullpath[1024] = { 0 };
	int fd, err;

	if (path != NULL) {
		strcpy(fullpath, path);
		strcat(fullpath, "/");
	}

	strcat(fullpath, file->name);

	fd = open(fullpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1)
		return -1;

	if (file->data != NULL)
		err = write(fd, file->data, file->size);

	close(fd);

	return 0;
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
