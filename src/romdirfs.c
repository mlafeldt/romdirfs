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

#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#define FUSE_USE_VERSION  26
#include <fuse.h>
#include <fuse_opt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "romdir.h"

#define APP_NAME	"romdirfs"
#define APP_NAME_BIG	"ROMDIRFS"
#define APP_VERSION	"1.1"

static romdir_t g_romdir = STAILQ_HEAD_INITIALIZER(g_romdir);

struct config {
	char *progname;
	char *filename;
	int debug;
};

static struct config g_config = {
	.progname = NULL,
	.filename = NULL,
	.debug = 0
};

#define DEBUG(args...) \
	do { if (g_config.debug) fprintf(stderr, args); } while (0)

#define HELP_TEXT \
	"usage: "APP_NAME" <file> <mountpoint> [options]\n" \
	"<file> must be a PS2 IOPRP image or BIOS dump\n\n" \
	"ROMDIRFS options:\n" \
	"    -V, --version          print version\n" \
	"    -h, --help             print help\n" \
	"    -D, -o romdirfs_debug  print some debugging information\n\n"

#define ROMDIRFS_OPT(t, p, v) { t, offsetof(struct config, p), v }

enum {
	KEY_HELP,
	KEY_VERSION,
	KEY_DEBUG
};

static struct fuse_opt g_opts[] = {
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


/*
 * Filesystem operations supported by ROMDIRFS
 */

static int romdirfs_getattr(const char *path, struct stat *stbuf)
{
	romfile_t *file;
	uint32_t hash;

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
			/* TODO set file time and user/group IDs */
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
	uint32_t hash;

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
	uint32_t hash;
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

static struct fuse_operations romdirfs_ops = {
	.getattr = romdirfs_getattr,
	.readdir = romdirfs_readdir,
	.open = romdirfs_open,
	.read = romdirfs_read
};

static int romdirfs_opt_proc(void *data, const char *arg, int key,
	struct fuse_args *outargs)
{
	switch (key) {
	case KEY_HELP:
		fprintf(stderr, HELP_TEXT);
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
		if (g_config.filename == NULL) {
			g_config.filename = strdup(arg);
			return 0;
		}
		return 1;

	default:
		fprintf(stderr, "%s: unknown option '%s'\n",
			g_config.progname, arg);
		exit(1);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	romfile_t *file = NULL;
	struct stat sb;
	uint8_t *buf = NULL;
	int fd, ret;

	g_config.progname = argv[0];

	if (fuse_opt_parse(&args, &g_config, g_opts, romdirfs_opt_proc) == -1)
		exit(1);

	if (g_config.filename == NULL) {
		fprintf(stderr, "%s: missing file\n", g_config.progname);
		exit(1);
	}

	fd = open(g_config.filename, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Error: could not open file '%s'\n",
			g_config.filename);
		exit(1);
	}
	if (fstat(fd, &sb) == -1) {
		perror("fstat");
		exit(1);
	}
	if (!S_ISREG(sb.st_mode)) {
		fprintf(stderr, "Error: %s is not a file\n", g_config.filename);
		exit(1);
	}
	if (!sb.st_size || sb.st_size > (4*1024*1024)) { /* BIOS is 4MB in size */
		fprintf(stderr, "Error: invalid file size\n");
		exit(1);
	}

#ifdef USE_MMAP
	/* map file into memory */
	buf = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
#else
	/* read file contents into allocated memory */
	buf = malloc(sb.st_size);
	if (buf == NULL) {
		perror("malloc");
		exit(1);
	}
	if (read(fd, buf, sb.st_size) != sb.st_size) {
		perror("read");
		exit(1);
	}
#endif
	if (close(fd) == -1) {
		perror("close");
		exit(1);
	}

	/* get ROMDIR entries */
	STAILQ_INIT(&g_romdir);
	ret = romdir_read(buf, sb.st_size, &g_romdir);
	if (ret < 0) {
		fprintf(stderr, "Error: could not read ROMDIR entries\n");
		exit(1);
	}

 	DEBUG("ROMDIR entries:\n");
	DEBUG("%-10s %-8s %-17s %8s %-8s %8s\n",
		"name", "hash", "offset", "size", "xi_off", "xi_size");
 	STAILQ_FOREACH(file, &g_romdir, node) {
		DEBUG("%-10s %08x %08x-%08x %8i %08x %8i\n",
			file->name, file->hash, (uint32_t)file->offset,
			(uint32_t)(file->offset + file->size),
			(uint32_t)file->size, (uint32_t)file->xi_offset,
			(uint32_t)file->xi_size);
#if 0
		if (file->xi_data != NULL) {
			uint32_t *w = (uint32_t*)file->xi_data;
			DEBUG("%08x %08x\n", w[0], w[1]);
		}
#endif
	}

	/* use single-threaded mode */
	fuse_opt_add_arg(&args, "-s");

	/* do FUSE magic */
	ret = fuse_main(args.argc, args.argv, &romdirfs_ops, NULL);

	DEBUG("fuse_main() returned %i\n", ret);
	DEBUG("clean up...\n");

	STAILQ_FOREACH(file, &g_romdir, node) {
		STAILQ_REMOVE(&g_romdir, file, _romfile, node);
		free(file);
	}

#ifdef USE_MMAP
	if (munmap(buf, sb.st_size) == -1) {
		perror("munmap"),
		exit(1);
	}
#else
	free(buf);
#endif
	free(g_config.filename);
	fuse_opt_free_args(&args);

	return ret;
}
