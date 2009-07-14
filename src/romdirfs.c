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
#include <fuse_opt.h>
#include "romdir.h"

static romfile_queue_t g_queue = STAILQ_HEAD_INITIALIZER(g_queue);

static void *romdirfs_init(struct fuse_conn_info *conn)
{
	return NULL;
}

static void romdirfs_destroy(void *data)
{
	romfile_t *file;

	/* clean up */
	STAILQ_FOREACH(file, &g_queue, node) {
		free(file->data);
		STAILQ_REMOVE(&g_queue, file, _romfile, node);
		free(file);
	}
}

static int romdirfs_getattr(const char *path, struct stat *stbuf)
{
	romfile_t *file;
	u_int32_t hash;

	memset(stbuf, 0, sizeof(struct stat));

	if (*path != '/')
		return -ENOENT;
	if (path[1] == '\0') { /* root dir */
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	} else {
		hash = strhash(path + 1);
		file = romdir_find_file_by_hash(&g_queue, hash);
		if (file != NULL) {
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = file->size;
			return 0;
		}
	}

	return -ENOENT;
}

static int romdirfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info *fi)
{
	romfile_t *file;

	if (strcmp(path, "/"))
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	STAILQ_FOREACH(file, &g_queue, node)
		filler(buf, file->name, NULL, 0);

	return 0;
}

static int romdirfs_open(const char *path, struct fuse_file_info *fi)
{
	romfile_t *file;
	u_int32_t hash;

	if (*path != '/')
		return -ENOENT;

	hash = strhash(path + 1);
	file = romdir_find_file_by_hash(&g_queue, hash);
	if (file != NULL) {
		if ((fi->flags & 3) != O_RDONLY)
			return -EACCES;
		else
			return 0;
	}

	return -ENOENT;
}

static int romdirfs_read(const char *path, char *buf, size_t size, off_t offset,
	struct fuse_file_info *fi)
{
	romfile_t *file;
	u_int32_t hash;
	size_t len;

	if (*path != '/')
		return -ENOENT;

	hash = strhash(path + 1);
	file = romdir_find_file_by_hash(&g_queue, hash);
	if (file != NULL) {
		len = file->size;
		if (offset < len){
			if (offset + size > len)
				size = len - offset;
			memcpy(buf, file->data + offset, size);
		} else {
			size = 0;
		}
		return size;
	}

	return -ENOENT;
}

static struct fuse_operations romdirfs_ops = {
	.init = romdirfs_init,
	.destroy = romdirfs_destroy,
	.getattr = romdirfs_getattr,
	.readdir = romdirfs_readdir,
	.open = romdirfs_open,
	.read = romdirfs_read
};

#define ROMDIR_OPT(t, p, v) { t, offsetof(struct romdir, p), v }

static struct fuse_opt romdir_opts[] = {
	FUSE_OPT_END
};

static int romdir_opt_proc(void *data, const char *arg, int key,
	struct fuse_args *outargs)
{
	return 0;
}

int main(int argc, char *argv[])
{
	int fd, ret;
	romfile_t *file;
//	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (argc < 2)
		return -1;

	fd = open("image.bin", O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -1;
	}

//	STAILQ_INIT(&g_queue);
	if (romdir_read(fd, &g_queue) < 0) {
		perror("parse_file");
		close(fd);
		return -1;
	}

	STAILQ_FOREACH(file, &g_queue, node) {
		printf("%-10s %08x  %04x %08x %08x-%08x\n", file->name, file->hash,
			file->extinfo_size, file->size, file->offset,
			file->offset + file->size);
		romdir_extract(fd, file);
	}
	close(fd);
	ret = fuse_main(argc, argv, &romdirfs_ops, NULL);

	return ret;
}
