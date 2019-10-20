#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "ivdistshm.h"

#define FLAG_MUTEX 0x01
#define FLAG_FILLED 0x02

#ifndef TIME_POLL_USEC
#define TIME_POLL_USEC 50
#endif

#define memcpy_fast_generic memcpy

// update
static inline int ivdistshm_update(IVDISTSHM * ivdistshm)
{
	ivdistshm->flags = *(mem_flags *) ivdistshm->virt_addr;
	return NO_ERR;
}

// flush
static inline int ivdistshm_flush(IVDISTSHM * ivdistshm)
{
	*(mem_flags *) ivdistshm->virt_addr = ivdistshm->flags;
	return NO_ERR;
}

// atomic lock
static inline int ivdistshm_lock(IVDISTSHM * ivdistshm)
{
	mem_flags remote_flags;

	ivdistshm_update(ivdistshm);
	if (ivdistshm->flags & FLAG_MUTEX) {
		if ((remote_flags =
		     __sync_val_compare_and_swap((mem_flags *) ivdistshm->
						 virt_addr, ivdistshm->flags,
						 ivdistshm->
						 flags & ~FLAG_MUTEX)) ==
		    ivdistshm->flags) {
			ivdistshm->flags &= ~FLAG_MUTEX;
		}
	} else {
		return ERR_INVALID;
	}

	return NO_ERR;
}

// atomic unlock
static inline int ivdistshm_unlock(IVDISTSHM * ivdistshm)
{
	mem_flags my_flags = ivdistshm->flags;
	ivdistshm_update(ivdistshm);
	my_flags |= FLAG_MUTEX;
	ivdistshm->flags = my_flags;
	ivdistshm_flush(ivdistshm);

	return NO_ERR;
}

// link
int ivdistshm_link_local(IVDISTSHM * ivdistshm, const char *file_name,
			 const int server)
{
	// check size of the file mapped to shared memory
	struct stat st;
	if (stat(file_name, &st) < 0) {
		return ERR_INVALID;
	}
	ivdistshm->size = st.st_size;
	ivdistshm->len = ivdistshm->size;

	// get the file descriptor
	ivdistshm->fd = open(file_name, O_RDWR, 0600);
	if (ivdistshm->fd < 0) {
		perror("open()");
		exit(1);
	}

	// map the file to its virtual address space
	ivdistshm->virt_addr =
	    mmap(NULL, ivdistshm->len, PROT_READ | PROT_WRITE, MAP_SHARED,
		 ivdistshm->fd, 0);
	if (!ivdistshm->virt_addr) {
		perror("mmap()");
		exit(1);
	}

	// set memory type as local
	ivdistshm->loc = LOCAL;

	// set flags
	if (server) {
		ivdistshm->flags = FLAG_MUTEX;
		ivdistshm_flush(ivdistshm);
	}

	return NO_ERR;
}

int ivdistshm_link_remote(IVDISTSHM * ivdistshm, const uint64_t phys_addr,
			  const size_t size, const int server)
{
	uint64_t pagesize = getpagesize();
	uint64_t addr = phys_addr & (~(pagesize - 1));
	ivdistshm->size = size;
	ivdistshm->len = (phys_addr & (pagesize - 1)) + size;

	// get the file descriptor
	ivdistshm->fd = open("/dev/mem", O_RDWR, 0600);
	if (ivdistshm->fd < 0) {
		perror("open()");
		exit(1);
	}

	// map the file to its virtual address space
	ivdistshm->virt_addr =
	    mmap(NULL, ivdistshm->len, PROT_READ | PROT_WRITE, MAP_SHARED,
		 ivdistshm->fd, addr);
	if (!ivdistshm->virt_addr) {
		perror("mmap()");
		exit(1);
	}

	// set memory type as remote
	ivdistshm->loc = REMOTE;

	// set flags
	if (server) {
		ivdistshm->flags = FLAG_MUTEX;
		ivdistshm_flush(ivdistshm);
	}

	return NO_ERR;
}

// read
void *ivdistshm_read(void *dest, IVDISTSHM * src, const size_t num)
{
	for (;;) {
		while (ivdistshm_lock(src)) {
			usleep(TIME_POLL_USEC);
		}
		if (src->flags & FLAG_FILLED) {
			memcpy_fast_generic(dest, src->virt_addr + FLAG_PADDING, num);
			src->flags &= ~FLAG_FILLED;
			break;
		} else {
			ivdistshm_unlock(src);
			usleep(TIME_POLL_USEC);
		}
	}
	ivdistshm_unlock(src);

	return dest;
}

// write
void *ivdistshm_write(IVDISTSHM * dest, const void *src, const size_t num)
{
	for (;;) {
		while (ivdistshm_lock(dest)) {
			usleep(TIME_POLL_USEC);
		}
		if (!(dest->flags & FLAG_FILLED)) {
			memcpy_fast_generic(dest->virt_addr + FLAG_PADDING, src, num);
			dest->flags |= FLAG_FILLED;
			break;
		} else {
			ivdistshm_unlock(dest);
			usleep(TIME_POLL_USEC);
		}
	}
	ivdistshm_unlock(dest);

	return dest;
}

// unlink
int ivdistshm_unlink(IVDISTSHM * ivdistshm)
{
	if (ivdistshm->loc < 0) {
		return ERR_INVALID;
	}

	munmap(ivdistshm->virt_addr, ivdistshm->len);
	//close(ivdistshm->fd); // Conflicts with stub for now
	ivdistshm->loc = INVALID;

	return NO_ERR;
}
