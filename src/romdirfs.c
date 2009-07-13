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
#define ROM_HDR_SIZE 16

/* Structure of a ROMDIR entry */
typedef struct _romhdr {
	char		name[10];
	u_int16_t	extinfo_size;
	u_int32_t	size;
} romhdr_t;

typedef struct _roment {
	romhdr_t	header;
	u_int32_t	offset;
	u_int8_t	*data;

	STAILQ_ENTRY(_roment) node;
} roment_t;

typedef STAILQ_HEAD(_roment_queue, _roment) roment_queue_t;

static void __show_entry(const roment_t *entry)
{
	printf("%-10s %04x %08x %08x-%08x\n", entry->header.name,
		entry->header.extinfo_size, entry->header.size,
		entry->offset, entry->offset + entry->header.size);
}

/* Parse file for rom entries. */
static int __parse_romfile(int fd, roment_queue_t *queue)
{
	int reset_found = 0;
	romhdr_t header;
	roment_t *entry = NULL;
	u_int32_t offset = 0;

	STAILQ_INIT(queue);

	/* find RESET module */
	lseek(fd, 0, SEEK_SET);
	while (read(fd, &header, ROM_HDR_SIZE) == ROM_HDR_SIZE) {
		if (!strcmp(header.name, "RESET")) {
			reset_found = 1;
			break;
		}
	}

	if (!reset_found) {
		printf("RESET module not found\n");
		return -1;
	}

	do {
		/* add rom entry to queue */
		entry = (roment_t*)malloc(sizeof(roment_t));
		memcpy(&entry->header, &header, ROM_HDR_SIZE);
		entry->offset = offset;
		STAILQ_INSERT_TAIL(queue, entry, node);

		/* offset must be aligned to 16 bytes */
		if (!(header.size % 0x10))
			offset += header.size;
		else
			offset += (header.size + 0x10) & 0xfffffff0;
	} while (read(fd, &header, ROM_HDR_SIZE) == ROM_HDR_SIZE && header.name[0]);

	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	roment_queue_t queue;
	roment_t *entry;

	if (argc != 2)
		return -1;

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	__parse_romfile(fd, &queue);

	STAILQ_FOREACH(entry, &queue, node) {
		__show_entry(entry);
		STAILQ_REMOVE(&queue, entry, _roment, node);
		free(entry);
	}

	close(fd);

	return 0;
}
