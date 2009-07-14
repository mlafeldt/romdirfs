#ifndef _ROMDIR_H_
#define _ROMDIR_H_

#include <sys/types.h>
#include <sys/queue.h>

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
	u_int32_t	hash;

	STAILQ_ENTRY(_romfile) node;
} romfile_t;

/* Queue to hold multiple ROMDIR files */
typedef STAILQ_HEAD(_romfile_queue, _romfile) romfile_queue_t;


u_int32_t strhash(const char *name);
int romdir_read(int fd, romfile_queue_t *queue);
void romdir_show(const romfile_t *file);
int romdir_extract(int fd, romfile_t *file);
romfile_t *romdir_find_file_by_hash(const romfile_queue_t *queue, u_int32_t hash);

#endif /* _ROMDIR_H_ */
