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
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "romdir.h"

/**
 * strhash - String hashing function as specified by the ELF ABI.
 * @name: string to calculate hash from
 * @return: 32-bit hash value
 */
u_int32_t strhash(const char *name)
{
	const u_int8_t *p = (u_int8_t*)name;
	u_int32_t h = 0, g;

	while (*p) {
		h = (h << 4) + *p++;
		if ((g = (h & 0xf0000000)) != 0)
			h ^= (g >> 24);
		h &= ~g;
	}

	return h;
}

/*
 * Parse file for ROMDIR entries and add them to the queue.
 */
int romdir_read(int fd, romfile_queue_t *queue)
{
	roment_t entry;
	romfile_t *file = NULL;
	u_int32_t offset = 0;
	int reset_found = 0;

	/* find ROMDIR entry named "RESET" */
	if (lseek(fd, 0, SEEK_SET) == -1)
		return -1;
	while (read(fd, &entry, ROMENT_SIZE) == ROMENT_SIZE) {
		if (!strcmp(entry.name, "RESET")) {
			reset_found = 1;
			break;
		}
	}

	if (!reset_found)
		return -1;

	/* read ROMDIR entries and add them to the queue */
	do {
		/* those entries contain nothing but zeros - ignore them */
		if (entry.name[0] == '-')
			continue;

		file = (romfile_t*)malloc(sizeof(romfile_t));
		if (file == NULL)
			return -1;

		strcpy(file->name, entry.name);
		file->size = entry.size;
		file->extinfo_size = entry.extinfo_size;
		file->offset = offset;
		file->data = NULL; /* will be read later */
		file->hash = strhash(file->name);
		STAILQ_INSERT_TAIL(queue, file, node);

		/* offset must be aligned to 16 bytes */
		if (!(entry.size % 0x10))
			offset += entry.size;
		else
			offset += (entry.size + 0x10) & 0xfffffff0;
	} while (read(fd, &entry, ROMENT_SIZE) == ROMENT_SIZE && entry.name[0]);

	/* read data for each file */
	STAILQ_FOREACH(file, queue, node) {
		if (!file->size)
			continue;
		if (lseek(fd, file->offset, SEEK_SET) == -1)
			return -1;
		if ((file->data = (u_int8_t*)malloc(file->size)) == NULL)
			return -1;
		if (read(fd, file->data, file->size) != file->size) {
			free(file->data);
			file->data = NULL;
			return -1;
		}
	}

	return 0;
}

/*
 * Extract file from ROMDIR fs to path.
 */
int romdir_extract(const romfile_t *file, const char *path)
{
	char fullpath[1024] = { 0 };
	int fd;

	if (path != NULL) {
		strcpy(fullpath, path);
		strcat(fullpath, "/");
	}

	strcat(fullpath, file->name);

	fd = open(fullpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1)
		return -1;

	if (file->data != NULL)
		write(fd, file->data, file->size);

	close(fd);

	return 0;
}

/*
 * Search queue for file by hash.
 */
romfile_t *romdir_find_file(const romfile_queue_t *queue, u_int32_t hash)
{
	romfile_t *file = NULL;

	STAILQ_FOREACH(file, queue, node) {
		if (hash == file->hash)
			break;
	}

	return file;
}

#ifdef _ROMDIR_STANDALONE
#include <stdio.h>

int main(int argc, char *argv[])
{
	int fd, ret;
	const char *path = NULL;
	romfile_t *file = NULL;
	romfile_queue_t queue = STAILQ_HEAD_INITIALIZER(queue);

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <rom file> [output path]\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Error: could not open rom file\n");
		return -1;
	}

	ret = romdir_read(fd, &queue);
	close(fd);
	if (ret < 0) {
		fprintf(stderr, "Error: could not read from rom file\n");
		return -1;
	}

	if (argc > 2)
		path = argv[2];

	printf("%-10s %-17s %8s %4s\n", "filename", "offset", "size", "ext");

	STAILQ_FOREACH(file, &queue, node) {
		printf("%-10s %08x-%08x %8i %4i\n", file->name, file->offset,
			file->offset + file->size, file->size, file->extinfo_size);

		if (romdir_extract(file, path) < 0)
			fprintf(stderr, "Error: could not extract file %s\n",
				file->name);

		free(file->data);
		STAILQ_REMOVE(&queue, file, _romfile, node);
		free(file);
	}

	return 0;
}
#endif /* _ROMDIR_STANDALONE */
