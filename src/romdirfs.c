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

	TAILQ_ENTRY(_roment) node;
} roment_t;

typedef TAILQ_HEAD(_roment_tbl, _roment) roment_tbl_t;

static void show_header(const romhdr_t *hdr, u_int32_t offset)
{
#if 0
	printf("%-10s  %5i  %5i  %5i\n", hdr->name,
		hdr->extinfo_size, hdr->size, offset);
#else
	printf("%-10s %04x %08x %08x-%08x\n", hdr->name,
		hdr->extinfo_size, hdr->size, offset,
		offset + hdr->size);
#endif
}

static int parse_headers(const char *romfile, roment_tbl_t *tbl)
{
	int fd;
	int reset_found = 0;
	romhdr_t header;
	roment_t *entry = NULL;
	u_int32_t offset = 0;

	TAILQ_INIT(tbl);

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
		entry = (roment_t*)malloc(sizeof(roment_t));
		memcpy(&entry->header, &header, ROM_HDR_SIZE);
		entry->offset = offset;
		TAILQ_INSERT_TAIL(tbl, entry, node);

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
	roment_tbl_t tbl;
	roment_t *entry;

	if (argc > 1) {
		parse_headers(argv[1], &tbl);

		TAILQ_FOREACH(entry, &tbl, node) {
			show_header(&entry->header, entry->offset);
			TAILQ_REMOVE(&tbl, entry, node);
			free(entry);
		}
	}

	return 0;
}
