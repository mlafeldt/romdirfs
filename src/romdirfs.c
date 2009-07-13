#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>

#define ROMENT_SIZE 16

typedef struct _roment {
	char		name[10];
	u_int16_t	extinfo_size;
	u_int32_t	size;
} roment_t;


static void show_roment(const roment_t *roment, u_int32_t offset)
{
#if 0
	printf("%-10s  %5i  %5i  %5i\n", roment->name,
		roment->extinfo_size, roment->size, offset);
#else
	printf("%-10s %04x %08x %08x-%08x\n", roment->name,
		roment->extinfo_size, roment->size, offset,
		offset + roment->size);
#endif
}

static int read_romfile(const char *romfile)
{
	int fd;
	int reset_found = 0;
	roment_t roment;
	u_int32_t offset = 0;

	fd = open(romfile, O_RDONLY);
	if (fd == -1) {
		perror(__FUNCTION__);
		return -1;
	}

	while (read(fd, &roment, ROMENT_SIZE) == ROMENT_SIZE) {
		if (!strcmp(roment.name, "RESET")) {
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
		show_roment(&roment, offset);

		if (!(roment.size % 0x10))
			offset += roment.size;
		else
			offset += (roment.size + 0x10) & 0xfffffff0;
	} while (read(fd, &roment, ROMENT_SIZE) == ROMENT_SIZE && roment.name[0]);

	close(fd);
	return 0;
}


int main(int argc, char *argv[])
{
	if (argc > 1)
		read_romfile(argv[1]);

	return 0;
}
