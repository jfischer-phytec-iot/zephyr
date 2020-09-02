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

#define SEND_REPORT_SIZE 4096
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

void usage()
{
	printf("Usage: ./hid-issue <hidraw path> <type> <Set size>\n"
	       "hidraw path: /dev/hidrawX\n"
	       "type:\n"
	       "\t'get' to burst only get report requests \n"
	       "\t      (will ignore 'Set size' arg, put any value)\n"
	       "\t'set' to burst only set report requests\n"
	       "\t'alt' to alternate set and get report requests\n"
	       "Set size: Set report size (max %d)\n",
	       SEND_REPORT_SIZE);
}

int main(int argc, char **argv)
{
	const uint8_t send_report[SEND_REPORT_SIZE + 1] = {0};
	uint8_t recv_report[RECV_REPORT_SIZE] = {0};
	enum {SET, GET, ALT};
	char type;
	int set_size = -1;

	if (argc != 4) {
		usage();
		return 1;
	}

	if (strncmp(argv[2], "get", 4) == 0) {
		type = GET;
	} else if (strncmp(argv[2], "set", 4) == 0) {
		type = SET;
	} else if (strncmp(argv[2], "alt", 4) == 0) {
		type = ALT;
	} else {
		usage();
		return 1;
	}
	
	if (type == SET || type == ALT) {
		set_size = strtol(argv[3], NULL, 10);
		if (set_size < 1 || set_size > SEND_REPORT_SIZE) {
			printf("Set size is incorrect\n");
			usage();
			return 1;
		}
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("Unable to open device");
		return 1;
	}

	printf("%s: Start set/get report. Set size: %d\n"
	       "use [ctrl-c] to stop the program and print the result.\n",
	       __func__, set_size);
	signal(SIGINT, int_handler);
	while (1) {
		//printf("Count = %u\n", count);
		if (type == SET || type == ALT ) {
			if (ioctl(fd, HIDIOCSFEATURE(set_size + 1),
				  send_report) < 0) {
				perror("Set feature report");
				error_count++;
			}
			count++;
		}
		if (type == GET || type == ALT) {
			recv_report[0] = 0;
			if (ioctl(fd, HIDIOCGFEATURE(sizeof(recv_report)),
				  recv_report) < 0) {
				perror("Get feature report");
				error_count++;
			}
			count++;
		}
 	}

	close(fd);
	return 0;
}
