#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "ivdistshm.h"

enum ivshmem_registers {
	IntrMask = 0,
	IntrStatus = 4,
	IVPosition = 8,
	Doorbell = 12,
	IVLiveList = 16
};

#define doorbell_path "/dev/uio0"
static int doorbell_fd;
static void *doorbell_mmap;
static void *plain_mmap;

void intr_send()
{
	const unsigned short dest = 1;
	const unsigned short cmd = 0;
	int msg = ((dest & 0xffff) << 16) + (cmd & 0xffff);

	*((int*)(doorbell_mmap + Doorbell)) = msg;
}

void intr_wait()
{
	ssize_t len;
	int buf;

	len = read(doorbell_fd, &buf, sizeof(buf));
	if (unlikely(len != sizeof(buf))) {
		perror("Interrupt received size mismatch");
		exit(1);
	}
}

void intr_init()
{
	uint64_t pagesize, addr;
	int fd;

	// Setup doorbell fd for receiving interrupts
	doorbell_fd = open(doorbell_path, O_RDWR);
	if (doorbell_fd == -1) {
		perror("Failed to open doorbell device path");
		exit(1);
	}

	// Setup doorbell mmap for sending interrupts
	doorbell_mmap = mmap(NULL, 256, PROT_READ | PROT_WRITE, MAP_SHARED, doorbell_fd, 0);
	if (doorbell_mmap == MAP_FAILED) {
		perror("Failed to mmap doorbell device path");
		exit(1);
	}
}
