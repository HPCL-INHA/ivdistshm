#ifndef IVDISTSHM_H
#define IVDISTSHM_H

#include <stdint.h>

#include <sys/types.h>

#define PHYS_ADDR 0x800000000
// This used to be sizeof(mem_flags), but later changed to 256 for memory alignment
#define FLAG_PADDING 256
#define SIZE (1048576 + FLAG_PADDING)

#define NO_ERR 0
#define ERR_INVALID 1

typedef uint64_t mem_flags;

typedef struct ivdistshm {
	enum mem_loc {
		INVALID = -1,
		LOCAL,
		REMOTE
	} loc;			// if > 0, memory is allocated remotely
	// else if == 0, memory is allocated locally
	// else if < 0, invalid
	size_t size;		// size of shared memory
	size_t len;		// ?
	void *virt_addr;	// virtual address mapped to physical address of shared memory
	int fd;			// file descriptor for later unlink()
	mem_flags flags;	// copy of flags area
} IVDISTSHM;

// link
int ivdistshm_link_local(IVDISTSHM * ivdistshm, const char *file_name,
			 const int server);
int ivdistshm_link_remote(IVDISTSHM * ivdistshm, const uint64_t phys_addr,
			  const size_t size, const int server);

// read
void *ivdistshm_read(void *dest, IVDISTSHM * src, const size_t num);

// write
void *ivdistshm_write(IVDISTSHM * dest, const void *src, const size_t num);

// unlink
int ivdistshm_unlink(IVDISTSHM * ivdistshm);

#endif
