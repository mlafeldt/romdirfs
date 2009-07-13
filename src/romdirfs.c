#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>

/* Size of one entry in ROMDIR header table */
#define ROM_HDR_SIZE 16

/* One entry in ROMDIR header table */
typedef struct _romhdr {
	char		name[10];
	u_int16_t	extinfo_size;
	u_int32_t	size;
} romhdr_t;

typedef struct _roment {
	romhdr_t	header;
	u_int32_t	offset;

	STAILQ_ENTRY(_roment) node;
} roment_t;

typedef STAILQ_HEAD(_roment_queue, _roment) roment_queue_t;

static void show_entry(const roment_t *entry)
{
	printf("%-10s %04x %08x %08x-%08x\n", entry->header.name,
		entry->header.extinfo_size, entry->header.size,
		entry->offset, entry->offset + entry->header.size);
}

static int parse_romfile(const char *romfile, roment_queue_t *queue)
{
	int fd;
	int reset_found = 0;
	romhdr_t header;
	roment_t *entry = NULL;
	u_int32_t offset = 0;

	STAILQ_INIT(queue);

	fd = open(romfile, O_RDONLY);
	if (fd == -1) {
		perror(__FUNCTION__);
		return -1;
	}

	/* find RESET module */
	while (read(fd, &header, ROM_HDR_SIZE) == ROM_HDR_SIZE) {
		if (!strcmp(header.name, "RESET")) {
			reset_found = 1;
			break;
		}
	}

	if (!reset_found) {
		printf("RESET not found\n");
		close(fd);
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

	close(fd);
	return 0;
}

int main(int argc, char *argv[])
{
	roment_queue_t queue;
	roment_t *entry;

	if (argc > 1) {
		parse_romfile(argv[1], &queue);

		STAILQ_FOREACH(entry, &queue, node) {
			show_entry(entry);
			STAILQ_REMOVE(&queue, entry, _roment, node);
			free(entry);
		}
	}

	return 0;
}
