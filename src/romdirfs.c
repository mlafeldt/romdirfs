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

#define FUSE_USE_VERSION  26
#include <fuse.h>

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
	u_int8_t	*data;

	STAILQ_ENTRY(_romfile) node;
} romfile_t;

/* Queue to hold multiple ROMDIR files */
typedef STAILQ_HEAD(_romfile_queue, _romfile) romfile_queue_t;


/*
 * Parse file for ROMDIR entries and add them to the queue.
 */
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
		if (file == NULL)
			return -1;
		memcpy(file, &entry, ROMENT_SIZE); /* roment_t fits into romfile_t */
		file->offset = offset;
		file->data = NULL; /* we'll read the file data on demand */
		STAILQ_INSERT_TAIL(queue, file, node);

		/* offset must be aligned to 16 bytes */
		if (!(entry.size % 0x10))
			offset += entry.size;
		else
			offset += (entry.size + 0x10) & 0xfffffff0;
	} while (read(fd, &entry, ROMENT_SIZE) == ROMENT_SIZE && entry.name[0]);

	return 0;
}

/*
 * Show information about a ROMDIR file.
 */
static void romfile_show(const romfile_t *file)
{
	printf("%-10s %04x %08x %08x-%08x\n", file->name,
		file->extinfo_size, file->size,
		file->offset, file->offset + file->size);
}

/*
 * Read file data from ROMDIR fs.
 */
static int romfile_read(int fd, romfile_t *file)
{
	/* read data only once */
	if (file->data == NULL && file->size > 0) {
		file->data = (u_int8_t*)malloc(file->size);
		if (file->data == NULL)
			return -1;
		if (lseek(fd, file->offset, SEEK_SET) == -1)
			return -1;
		if (read(fd, file->data, file->size) != file->size)
			return -1;
	}

	return 0;
}

/*
 * Extract file from ROMDIR fs.
 */
static int romfile_extract(int fd, romfile_t *file)
{
	int fd_out;

	if (romfile_read(fd, file) < 0)
		return -1;

	fd_out = open(file->name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd_out == -1) {
		perror(file->name);
		return -1;
	}

	if (file->data != NULL)
		write(fd_out, file->data, file->size);

	close(fd_out);
	return 0;
}

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

static int romdir_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));

	if (!strcmp(path, "/")) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (!strcmp(path, hello_path)) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
	} else {
		return -ENOENT;
	}

	return 0;
}

static int romdir_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info *fi)
{
	if (strcmp(path, "/")) {
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, hello_path + 1, NULL, 0);

	return 0;
}

static int romdir_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, hello_path))
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int romdir_read(const char *path, char *buf, size_t size, off_t offset,
	struct fuse_file_info *fi)
{
	size_t len;

	if (strcmp(path, hello_path))
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len){
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else {
		size = 0;
	}

	return size;
}

static struct fuse_operations romdir_ops = {
	.getattr = romdir_getattr,
	.readdir = romdir_readdir,
	.open = romdir_open,
	.read = romdir_read
};

int main(int argc, char *argv[])
{
#if 0
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

	if (__parse_file(fd, &queue) < 0) {
		close(fd);
		return -1;
	}

	STAILQ_FOREACH(file, &queue, node) {
		romfile_show(file);
		romfile_extract(fd, file);
		STAILQ_REMOVE(&queue, file, _romfile, node);
		free(file);
	}

	close(fd);
	return 0;
#else
	return fuse_main(argc, argv, &romdir_ops, NULL);
#endif
}
