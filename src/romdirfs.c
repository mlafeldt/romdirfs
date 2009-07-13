#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>

/* Size of one ROMDIR entry */
#define ROMENT_SIZE	16

/* ROMDIR entry */
typedef struct _roment {
	char		name[10];
	u_int16_t	extinfo_size;
	u_int32_t	size;
} roment_t;

/* ROMDIR file information */
typedef struct _romfile {
	char		name[10];
	u_int16_t	extinfo_size;
	u_int32_t	size;
	u_int32_t	offset;

	STAILQ_ENTRY(_romfile) node;
} romfile_t;

/* Queue to hold multiple ROMDIR files */
typedef STAILQ_HEAD(_romfile_queue, _romfile) romfile_queue_t;


static void __show_romfile(const romfile_t *file)
{
	printf("%-10s %04x %08x %08x-%08x\n", file->name,
		file->extinfo_size, file->size,
		file->offset, file->offset + file->size);
}

/* Parse file for ROMDIR entries. */
static int __parse_file(int fd, romfile_queue_t *queue)
{
	roment_t entry;
	romfile_t *file = NULL;
	u_int32_t offset = 0;
	int reset_found = 0;

	STAILQ_INIT(queue);

	/* find RESET module */
	lseek(fd, 0, SEEK_SET);
	while (read(fd, &entry, ROMENT_SIZE) == ROMENT_SIZE) {
		if (!strcmp(entry.name, "RESET")) {
			reset_found = 1;
			break;
		}
	}

	if (!reset_found) {
		printf("RESET module not found\n");
		return -1;
	}

	do {
		/* add ROMDIR entry to queue */
		file = (romfile_t*)malloc(sizeof(romfile_t));
		memcpy(file, &entry, ROMENT_SIZE);
		file->offset = offset;
		STAILQ_INSERT_TAIL(queue, file, node);

		/* offset must be aligned to 16 bytes */
		if (!(entry.size % 0x10))
			offset += entry.size;
		else
			offset += (entry.size + 0x10) & 0xfffffff0;
	} while (read(fd, &entry, ROMENT_SIZE) == ROMENT_SIZE && entry.name[0]);

	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	romfile_queue_t queue;
	romfile_t *file;

	if (argc != 2)
		return -1;

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	__parse_file(fd, &queue);

	STAILQ_FOREACH(file, &queue, node) {
		__show_romfile(file);
		STAILQ_REMOVE(&queue, file, _romfile, node);
		free(file);
	}

	close(fd);

	return 0;
}
