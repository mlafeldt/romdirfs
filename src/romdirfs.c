/*
 * romdirfs.c - ROMDIR filesystem in userspace
 *
 * Copyright (C) 2009 misfire <misfire@xploderfreax.de>
 *
 * This file is part of romdirfs.
 *
 * romdirfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * romdirfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with romdirfs.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#define FUSE_USE_VERSION  26
#include <fuse.h>
#include <fuse_opt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "romdir.h"

#define APP_NAME	"romdirfs"
#define APP_NAME_BIG	"ROMDIRFS"
#define APP_VERSION	"1.0"

static romdir_t g_romdir = STAILQ_HEAD_INITIALIZER(g_romdir);

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
		file = romdir_find_file(&g_romdir, hash);
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

	STAILQ_FOREACH(file, &g_romdir, node)
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
	file = romdir_find_file(&g_romdir, hash);
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
	file = romdir_find_file(&g_romdir, hash);
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

static void *romdirfs_init(struct fuse_conn_info *conn)
{
	return NULL;
}

static void romdirfs_destroy(void *data)
{
	romfile_t *file;

	/* clean up */
	STAILQ_FOREACH(file, &g_romdir, node) {
		free(file->data);
		STAILQ_REMOVE(&g_romdir, file, _romfile, node);
		free(file);
	}
}

static struct fuse_operations romdirfs_ops = {
	.init = romdirfs_init,
	.destroy = romdirfs_destroy,
	.getattr = romdirfs_getattr,
	.readdir = romdirfs_readdir,
	.open = romdirfs_open,
	.read = romdirfs_read
};


static void usage(const char *progname)
{
	fprintf(stderr,
"usage: %s file mountpoint [options]\n\n"
"romdirfs options:\n"
"    -V, --version          print version\n"
"    -h, --help             print help\n"
"    -D, -o romdirfs_debug  print some debugging information\n\n",
	progname);
}

struct config {
	char *file;
	int debug;
};

static struct config romdirfs_conf = {
	.file = NULL,
	.debug = 0
};

enum {
	KEY_HELP,
	KEY_VERSION,
	KEY_DEBUG
};

#define ROMDIRFS_OPT(t, p, v) { t, offsetof(struct config, p), v }

static struct fuse_opt romdirfs_opts[] = {
	FUSE_OPT_KEY("-h",		KEY_HELP),
	FUSE_OPT_KEY("--help",		KEY_HELP),
	FUSE_OPT_KEY("-V",		KEY_VERSION),
	FUSE_OPT_KEY("--version",	KEY_VERSION),
	FUSE_OPT_KEY("-D",		KEY_DEBUG),
	FUSE_OPT_KEY("romdirfs_debug",	KEY_DEBUG),
	ROMDIRFS_OPT("-D",		debug, 1),
	ROMDIRFS_OPT("romdirfs_debug",	debug, 1),
	FUSE_OPT_END
};

#define DEBUG(format, args...) \
	do { if (romdirfs_conf.debug) fprintf(stderr, format, args); } while (0)

static int romdirfs_opt_proc(void *data, const char *arg, int key,
	struct fuse_args *outargs)
{
	switch (key) {
	case KEY_HELP:
		usage(outargs->argv[0]);
		fuse_opt_add_arg(outargs, "-ho");
		fuse_main(outargs->argc, outargs->argv, &romdirfs_ops, NULL);
		exit(1);

	case KEY_VERSION:
		fprintf(stderr, "%s version %s\n", APP_NAME_BIG, APP_VERSION);
		fuse_opt_add_arg(outargs, "--version");
		fuse_main(outargs->argc, outargs->argv, &romdirfs_ops, NULL);
		exit(0);

	case KEY_DEBUG:
		/* add -f for foreground operation */
		return fuse_opt_add_arg(outargs, "-f");

	case FUSE_OPT_KEY_OPT:
		return 1;

	case FUSE_OPT_KEY_NONOPT:
		if (romdirfs_conf.file == NULL) {
			romdirfs_conf.file = strdup(arg);
			return 0;
		}
		return 1;

	default:
		fprintf(stderr, "%s: unknown option '%s'\n",
			outargs->argv[0], arg);
		exit(1);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int fd, ret;
	romfile_t *file;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &romdirfs_conf, romdirfs_opts, romdirfs_opt_proc) == -1)
		return -1;

	if (romdirfs_conf.file == NULL) {
		fprintf(stderr, "usage: rom file missing\n");
		return -1;
	}

	fd = open(romdirfs_conf.file, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Error: could not open rom file\n");
		return -1;
	}

	STAILQ_INIT(&g_romdir);
	ret = romdir_read(fd, &g_romdir);
	close(fd);
	if (ret < 0) {
		fprintf(stderr, "Error: could not read from rom file\n");
		return -1;
	}

	STAILQ_FOREACH(file, &g_romdir, node) {
		printf("%-10s %08x  %04x %08x %08x-%08x\n", file->name, file->hash,
			file->extinfo_size, file->size, file->offset,
			file->offset + file->size);
	}

	ret = fuse_main(args.argc, args.argv, &romdirfs_ops, NULL);

	free(romdirfs_conf.file);
	fuse_opt_free_args(&args);

	return ret;
}
