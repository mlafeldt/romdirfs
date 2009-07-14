#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "romdir.h"

/*
 * String hashing function as specified by the ELF ABI.
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

	/* find RESET file */
	lseek(fd, 0, SEEK_SET);
	while (read(fd, &entry, ROMENT_SIZE) == ROMENT_SIZE) {
		if (!strcmp(entry.name, "RESET")) {
			reset_found = 1;
			break;
		}
	}

	if (!reset_found)
		return -1;

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
 * Extract file from ROMDIR fs.
 */
int romdir_extract(int fd, const romfile_t *file, const char *path)
{
	int fd_out;
	char fullpath[1024] = { 0 };

	if (path != NULL) {
		strcpy(fullpath, path);
		strcat(fullpath, "/");
	}

	strcat(fullpath, file->name);

	fd_out = open(fullpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd_out == -1)
		return -1;

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

		if (romdir_extract(fd, file, path) < 0)
			fprintf(stderr, "Error: could not extract file %s\n", file->name);

		free(file->data);
		STAILQ_REMOVE(&queue, file, _romfile, node);
		free(file);
	}

	return 0;
}
#endif /* _ROMDIR_STANDALONE */
