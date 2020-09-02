// SPDX-License-Identifier: GPL-2.0

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <signal.h>

#define SEND_REPORT_SIZE 128
#define RECV_REPORT_SIZE 4096 /* Need to be large enough */

static volatile int fd = -1;
static volatile uint32_t count = 0;
static volatile uint32_t error_count = 0;

void int_handler(int val)
{
	if (fd > 0) {
		close(fd);
	}
	printf("End with count = %u, Error = %u\n", count, error_count);
	exit(0);
}

int main(int argc, char **argv)
{
	const uint8_t send_report[SEND_REPORT_SIZE + 1] = {0};
	uint8_t recv_report[RECV_REPORT_SIZE] = {0};
	
	if (argc != 2) {
		printf("Please specify device hidraw path (/dev/hidrawX).\n");
		return 1;
	}
	
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("Unable to open device");
		return 1;
	}

	printf("%s: Start set/get report. Set size: %d\n"
	       "use [ctrl-c] to stop the program and print the result.\n",
	       __func__, SEND_REPORT_SIZE);
	signal(SIGINT, int_handler);
	while (1) {
		//printf("Count = %u\n", count);
		if (ioctl(fd, HIDIOCSFEATURE(sizeof(send_report)), send_report) < 0) {
			perror("Send feature report");
			error_count++;
		}
		if (ioctl(fd, HIDIOCGFEATURE(sizeof(recv_report)), recv_report) < 0) {
			perror("Receive feature report");
			error_count++;
		}
		recv_report[0] = 0;
		count += 2;
 	}

	close(fd);
	return 0;
}
