#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>

typedef struct _roment {
	char		name[10];
	u_int16_t	extinfo_size;
	u_int32_t	size;
} roment_t;

#define ROMENT_SIZE 16

void show_roment(const roment_t *roment)
{
	printf("%-10s  0x%04x  0x%04x\n", roment->name,
		roment->extinfo_size, roment->size);
}

int read_romfile(const char *romfile)
{
	int fd;
	int reset_found = 0;
	roment_t roment;

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

	show_roment(&roment);

	while (read(fd, &roment, ROMENT_SIZE) == ROMENT_SIZE && roment.name[0]) {
		show_roment(&roment);
	}

	close(fd);
	return 0;
}


int main(int argc, char *argv[])
{
	if (argc > 1)
		read_romfile(argv[1]);

	return 0;
}
