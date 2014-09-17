/*********************************************************************************************************************
 * The MIT License (MIT)                                                                                             *
 * Copyright (c) 2012-2014 Maksim Salau                                                                               *
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
#include <unistd.h>
#include <stdio.h>

extern int verbose_level;

port_handle_t serial_open(const char *port)
{
    port_handle_t fd;
    char port_full_name[20];
    snprintf(port_full_name, sizeof port_full_name - 2u,
             "\\\\.\\%s", port);
    if (4 <= verbose_level)
    {
        printf("\t\tOpen port: %s\n", port_full_name);
    }
    fd = CreateFile(port_full_name,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    0);
    if (INVALID_HANDLE_VALUE == fd)
    {
        int error_num = GetLastError();
        char error_string[1024];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      error_num,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      error_string,
                      sizeof error_string,
                      NULL);
        fprintf(stderr, "Unable to open port: (%i) %s\n", error_num, error_string);
    }
    else
    {
        DCB dcbSerialParams;
        GetCommState(fd, &dcbSerialParams);
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = TWOSTOPBITS;
        dcbSerialParams.Parity = NOPARITY;
        SetCommState(fd, &dcbSerialParams);

        COMMTIMEOUTS timeouts;
        timeouts.ReadIntervalTimeout=50;
        timeouts.ReadTotalTimeoutConstant=50;
        timeouts.ReadTotalTimeoutMultiplier=10;
        timeouts.WriteTotalTimeoutConstant=0;
        timeouts.WriteTotalTimeoutMultiplier=0;
        SetCommTimeouts(fd, &timeouts);
        FlushFileBuffers(fd);
    }
    return fd;
}

int serial_set_baud(port_handle_t fd, int baud)
{
    DCB dcbSerialParams;
    GetCommState(fd, &dcbSerialParams);
    dcbSerialParams.BaudRate = baud;
    return SetCommState(fd, &dcbSerialParams) != 0 ? 0 : -1;
}

int serial_set_parity(port_handle_t fd, int enable, int odd_parity)
{
    DCB dcbSerialParams;
    GetCommState(fd, &dcbSerialParams);
    dcbSerialParams.Parity = NOPARITY;
    if (enable)
    {
        if (odd_parity)
        {
            dcbSerialParams.Parity = ODDPARITY;
        }
        else
        {
            dcbSerialParams.Parity = EVENPARITY;
        }
    }
    return SetCommState(fd, &dcbSerialParams) != 0 ? 0 : -1;
}

int serial_set_dtr(port_handle_t fd, int level)
{
    int command;
    if (level)
    {
        command = CLRDTR;
    }
    else
    {
        command = SETDTR;
    }
    return EscapeCommFunction(fd, command) != 0 ? 0 : -1;
}

int serial_set_rts(port_handle_t fd, int level)
{
    int command;
    if (level)
    {
        command = CLRRTS;
    }
    else
    {
        command = SETRTS;
    }
    return EscapeCommFunction(fd, command) != 0 ? 0 : -1;
}

int serial_set_txd(port_handle_t fd, int level)
{
    int command;
    if (level)
    {
        command = CLRBREAK;
    }
    else
    {
        command = SETBREAK;
    }
    return EscapeCommFunction(fd, command) != 0 ? 0 : -1;
}

int serial_flush(port_handle_t fd)
{
    if (4 <= verbose_level)
    {
        printf("\t\tFlush IO buffers\n");
    }
    return PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR) != 0 ? 0 : -1;
}

int serial_write(port_handle_t fd, const void *buf, int len)
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
    DWORD bytes_written;
    unsigned char *pbuf = (unsigned char*)buf;
    do
    {
        if (0 == WriteFile(fd, pbuf, bytes_left, &bytes_written, NULL))
        {
            fprintf(stderr, "Failed to write to port.\n");
            return -1;
        }
        pbuf += bytes_written;
        bytes_left -= bytes_written;
    }
    while (0 < bytes_left);
    return len - bytes_left;
}

int serial_read(port_handle_t fd, void *buf, int len)
{
    int bytes_left = len;
    DWORD bytes_read;
    unsigned char *pbuf = (unsigned char*)buf;
    do
    {
        if (0 == ReadFile(fd, pbuf, bytes_left, &bytes_read, NULL))
        {
            fprintf(stderr, "Failed to read from port.\n");
            return -1;
        }
        if (0 == bytes_read)
        {
            break;
        }
        pbuf += bytes_read;
        bytes_left -= bytes_read;
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

int serial_close(port_handle_t fd)
{
    if (4 <= verbose_level)
    {
        printf("\t\tClose port\n");
    }
    return CloseHandle(fd) != 0 ? 0 : -1;
}
