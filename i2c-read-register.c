/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <getopt.h>

#define I2C_NODE	"/dev/i2c-"
#define BUFSIZE			512

static int open_bus(int bus)
{
	int file;
	char filename[64];
	snprintf(filename, 64, "%s%d", I2C_NODE, bus);
	if ((file = open(filename, O_RDWR)) < 0)
		/* ERROR HANDLING: you can check errno to see what went wrong */
		perror("Failed to open the i2c bus");

	return file;
}

static int talk_to_device(int busfd, int addr)
{
	int ret = ioctl(busfd, I2C_SLAVE, addr);
	if (ret < 0)
		printf("Failed to acquire bus access and/or talk to slave.\n");

	return ret;
}

static int read_bus(int busfd, unsigned char *buf, int bufsize)
{
	return read(busfd, buf, bufsize);
}

static int write_bus(int busfd, unsigned char *buf, int bufsize)
{
	return write(busfd, buf, bufsize);
}

static int read_register(int busfd, int client_addr, __uint16_t reg, unsigned char *buf, int bufsize)
{
	struct i2c_msg msgs[2];
	struct i2c_rdwr_ioctl_data rdwr_data;
	unsigned char reg_buf[2];
	int ret;

	reg_buf[0] = (reg >> 0) & 0xFF;
	reg_buf[1] = (reg >> 8) & 0xFF;

	msgs[0].addr = client_addr;
	msgs[0].flags = I2C_M_TEN;
	msgs[0].len = 2;
	msgs[0].buf = reg_buf;
	msgs[1].addr = client_addr;
	msgs[1].flags = I2C_M_TEN;
	msgs[1].flags |= I2C_M_RD;
	msgs[1].len = bufsize;
	msgs[1].buf = buf;

	rdwr_data.msgs = msgs;
	rdwr_data.nmsgs = 2;

	ret = ioctl(busfd, I2C_RDWR, rdwr_data);
	if (ret)
		printf("failed calling rdwr ioct: %02xl\n", ret);

	return ret;

	ret = write_bus(busfd, reg_buf, 2);
	if (ret < 0) {
		printf("Failed to write [0x%02x 0x%02x] (reg: %04x).\n", reg_buf[0], reg_buf[1], reg);
		return ret;
	}

	printf("wrote %d bytes\n", ret);
	return read_bus(busfd, buf, bufsize);

}

static void print_buffer(unsigned char *buf, int size)
{
	char str_buf[BUFSIZE * 3] = {0};
	char *pstr_buf = str_buf;
	int i, n;

	for (i = 0; i < size; i++) {
		n = sprintf(pstr_buf, "%02x ", buf[i]);
		pstr_buf += n;
	}
	printf("%s\n", str_buf);
}

static int htoi(char* str)
{
	unsigned int ret;
	sscanf(str, "0x%x", &ret);
	return ret;
}

/**
 * Print usage information.
 */
static int usage(void)
{
	printf("USAGE:\n");
	printf("   %s [OPTION] ADDR SIZE\n", program_invocation_short_name);

	printf("\n");
	printf("where OPTION is either:\n");
	printf("   -h or --help: print this message\n");
	printf("   -b N or --bus=N: the bus to talk to (see i2cdetect -l) (*MANDATORY*)\n");
	printf("   -d 0xXX or --device=0xXX: the device on the bus to talk to (*MANDATORY*)\n");

	return EXIT_FAILURE;
}

static const struct option long_options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "bus", required_argument, NULL, 'b' },
	{ "device", no_argument, NULL, 'd' },
	{ 0, },
};

int main(int argc, char **argv)
{
	unsigned char buf[BUFSIZE] = {0};
	int bus = -1;
	int addr = 0x00;
	int reg_addr;
	int size_to_read;
	int ret;
	int busfd;
	int size_read;

	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "hb:d:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'b':
			bus = atoi(optarg);
			break;
		case 'd':
			addr = htoi(optarg);
			break;
		default:
			return usage();
		}
	}

	if (bus < 0 || addr <= 0)
		return usage();

	if (optind < argc) {
		reg_addr = htoi(argv[optind++]);
		size_to_read = atoi(argv[optind++]);
	} else {
		return usage();
	}

	size_to_read = size_to_read > BUFSIZE ? BUFSIZE : size_to_read;

	printf("Will read %d bytes from the register 0x%04x of the device 0x%02x on bus %d.\n",
			size_to_read, reg_addr, addr, bus);

	busfd = open_bus(bus);
	if (busfd < 0)
		goto out_err;

	ret = talk_to_device(busfd, addr);
	if (ret < 0)
		goto out_err;


	size_read = read_register(busfd, addr, reg_addr, buf, size_to_read);
	print_buffer(buf, size_read);

	close(busfd);
	return 0;
out_err:
	if (busfd >= 0)
		close(busfd);
	return 1;
}
