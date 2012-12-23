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
#include "rl78.h"
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
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
    return ioctl(fd, TCFLSH, TCIFLUSH);
}

int serial_set_baud(int fd, int baud)
{
    int new_baud;
    switch (baud)
    {
    case BAUD_115200:
        new_baud = B115200;
        return 0;
#ifdef B250000
    case BAUD_250000:
        new_baud = B250000;
        break;
#endif
#ifdef B500000
    case BAUD_500000:
        new_baud = B500000;
        break;
#endif
#ifdef B1000000
    case BAUD_1000000:
        new_baud = B1000000;
        break;
#endif
    default:
        fprintf(stderr, "Selected baudrate is not supported on your platform\n");
        return -1;
    }
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, new_baud);
    cfsetospeed(&options, new_baud);
    return tcsetattr(fd, TCSANOW, &options);
}

int serial_set_dtr(int fd, int level)
{
    int command;
    const int dtr = TIOCM_DTR;
    if (level)
    {
        command = TIOCMBIC;
    }
    else
    {
        command = TIOCMBIS;
    }
    return ioctl(fd, command, &dtr);
}

int serial_set_rts(int fd, int level)
{
    int command;
    const int rts = TIOCM_RTS;
    if (level)
    {
        command = TIOCMBIC;
    }
    else
    {
        command = TIOCMBIS;
    }
    return ioctl(fd, command, &rts);
}

int serial_set_txd(int fd, int level)
{
    int command;
    if (level)
    {
        command = TIOCCBRK;
    }
    else
    {
        command = TIOCSBRK;
    }
    return ioctl(fd, command);
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
    int bytes_left = len;
    int rc = 0;
    unsigned char *pbuf = (unsigned char*)buf;
    do
    {
        rc = write(fd, pbuf, bytes_left);
        if (0 > rc)
        {
            fprintf(stderr, "Failed to write to port.\n");
            return rc;
        }
        pbuf += rc;
        bytes_left -= rc;
    }
    while (0 < bytes_left);
    return len - bytes_left;
}

int serial_read(int fd, void *buf, int len)
{
    int bytes_left = len;
    int rc = 0;
    unsigned char *pbuf = (unsigned char*)buf;
    do
    {
        rc = read(fd, pbuf, bytes_left);
        if (0 > rc)
        {
            fprintf(stderr, "Failed to read from port.\n");
            return rc;
        }
        if (0 == rc)
        {
            break;
        }
        pbuf += rc;
        bytes_left -= rc;
    }
    while (0 < bytes_left);
    const int nbytes = len - bytes_left;
    if (4 <= verbose_level)
    {
        pbuf = buf;
        unsigned int i;
        printf("\t\trecv(%u): ", nbytes);
        for (i = nbytes; 0 < i; --i)
        {
            printf("%02X ", *pbuf++);
        }
        printf("\n");
    }
    return nbytes;
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
