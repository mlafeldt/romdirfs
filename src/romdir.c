#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include "romdir.h"

/*
 * String hashing function as specified by the ELF ABI.
 */
u_int32_t strhash(const char *name)
{
	u_int8_t *p = (u_int8_t*)name;
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

	/* find RESET file */
	lseek(fd, 0, SEEK_SET);
	while (read(fd, &entry, ROMENT_SIZE) == ROMENT_SIZE) {
		if (!strcmp(entry.name, "RESET")) {
			reset_found = 1;
			break;
		}
	}

	if (!reset_found) {
		fprintf(stderr, "Error: RESET file not found\n");
		return -1;
	}

	/* read ROMDIR entries */
	do {
		/* add file to queue */
		file = (romfile_t*)malloc(sizeof(romfile_t));
		if (file == NULL)
			return -1;
		memcpy(file, &entry, ROMENT_SIZE); /* roment_t fits into romfile_t */
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
		if (file->size > 0) {
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
	}

	return 0;
}

/*
 * Show information about a ROMDIR file.
 */
void romdir_show(const romfile_t *file)
{
	printf("%-10s %08x  %04x %08x %08x-%08x\n", file->name, file->hash,
		file->extinfo_size, file->size, file->offset,
		file->offset + file->size);
}

/*
 * Extract file from ROMDIR fs.
 */
int romdir_extract(int fd, romfile_t *file)
{
	int fd_out = open(file->name, O_CREAT | O_WRONLY | O_TRUNC, 0644);

	if (fd_out == -1) {
		perror(file->name);
		return -1;
	}

	if (file->data != NULL)
		write(fd_out, file->data, file->size);

	close(fd_out);
	return 0;
}

/*
 * Search queue for file with a specific hash.
 */
romfile_t *romdir_find_file_by_hash(const romfile_queue_t *queue, u_int32_t hash)
{
	romfile_t *file = NULL;

	STAILQ_FOREACH(file, queue, node) {
		if (hash == file->hash)
			break;
	}

	return file;
}
