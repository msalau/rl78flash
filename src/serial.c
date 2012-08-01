/*********************************************************************************************************************
 * The MIT License (MIT)                                                                                             *
 * Copyright (c) 2012 Maxim Salov                                                                                    *
 *                                                                                                                   *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated      *
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation   *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and  *
 * to permit persons to whom the Software is furnished to do so, subject to the following conditions:                *
 *                                                                                                                   *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions     *
 * of the Software.                                                                                                  *
 *                                                                                                                   *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO  *
 * THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF         *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS *
 * IN THE SOFTWARE.                                                                                                  *
 *********************************************************************************************************************/

#include "serial.h"
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>

extern int verbose_level;

int serial_open(const char *port)
{
    int fd;
    if (4 <= verbose_level)
    {
	printf("\t\tOpen port: %s\n", port);
    }
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (-1 == fd)
    {
        perror("Unable to open port ");
    }
    else
    {
        struct termios options;
        fcntl(fd, F_SETFL, 0);
        tcgetattr(fd, &options);
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        options.c_cflag &= ~(HUPCL | CSIZE | PARENB | CRTSCTS);
        options.c_cflag |= CLOCAL | CREAD | CS8 | CSTOPB;
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY);
        options.c_oflag &= ~OPOST;
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 1;
        tcsetattr(fd, TCSANOW, &options);
	usleep(1000);
	ioctl(fd, TCFLSH, TCIOFLUSH);
    }
    return fd;
}

int serial_flush(int fd)
{
    serial_sync(fd);
    ioctl(fd, TCFLSH, TCIFLUSH);
}

int serial_set_baud(int fd, int baud)
{
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);
    tcsetattr(fd, TCSANOW, &options);
}

int serial_set_dtr(int fd, int level)
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    if (level)
    {
        status &= ~TIOCM_DTR;
    }
    else
    {
        status |= TIOCM_DTR;
    }
    ioctl(fd, TIOCMSET, &status);
    return level;
}

int serial_set_rts(int fd, int level)
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    if (level)
    {
        status &= ~TIOCM_RTS;
    }
    else
    {
        status |= TIOCM_RTS;
    }
    ioctl(fd, TIOCMSET, &status);
    return level;
}

int serial_write(int fd, const void *buf, int len)
{
    if (4 <= verbose_level)
    {
	unsigned char *p = (unsigned char*)buf;
	unsigned int i;
	printf("\t\tsend(%u): ", len);
	for (i = len; 0 < i; --i)
	{
	    printf("%02X ", *p++);
	}
	printf("\n");
    }
    return write(fd, buf, len);
}

int serial_read(int fd, void *buf, int len)
{
    int rc = read(fd, buf, len);
    if (4 <= verbose_level)
    {
	unsigned char *p = buf;
	unsigned int i;
	printf("\t\trecv(%u): ", len);
	for (i = rc; 0 < i; --i)
	{
	    printf("%02X ", *p++);
	}
	printf("\n");
    }
    return rc;
}

int serial_sync(int fd)
{
    return fsync(fd);
}

int serial_close(int fd)
{
    if (4 <= verbose_level)
    {
	printf("\t\tClose port\n");
    }
    return close(fd);
}
