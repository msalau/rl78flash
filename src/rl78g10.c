/*********************************************************************************************************************
 * The MIT License (MIT)                                                                                             *
 * Copyright (c) 2014 Maksim Salau                                                                                    *
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
#include "rl78g10.h"
#include "crc16_ccit.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "wait_kbhit.h"

extern int verbose_level;

static int get_size_from_code (unsigned int code)
{
    int size;
    switch (code)
    {
    case 0x01: size = 512; break; 
    case 0x03: size = 1024; break; 
    case 0x07: size = 1024*2; break; 
    case 0x0F: size = 1024*4; break; 
    case 0x1F: size = 1024*8; break; 
    case 0x3F: size = 1024*16; break; 
    case 0x7F: size = 1024*32; break; 
    case 0xFF: size = 1024*64; break; 
    default:   size = -1; break;
    }
    return size;
}

static void rl78g10_set_reset(port_handle_t fd, int mode, int value)
{
    if (MODE_RESET_RTS == mode)
    {
        serial_set_rts(fd, value);
    }
    else
    {
        serial_set_dtr(fd, value);
    }
}

int rl78g10_reset_init(port_handle_t fd, int wait, int mode)
{
    unsigned char buf[2];
    rl78g10_set_reset(fd, mode, 0);                         /* RESET -> 0 */
    serial_set_txd(fd, 0);                                  /* TOOL0 -> 0 */
    if (wait)
    {
        printf("Turn MCU's power on and press any key...");
        wait_kbhit();
        printf("\n");
    }
    serial_flush(fd);
    usleep(1000);
    rl78g10_set_reset(fd, mode, 1);                         /* RESET -> 1 */
    usleep(1000);
    serial_set_txd(fd, 1);                                  /* TOOL0 -> 1 */
    usleep(1000);
    serial_flush(fd);
    if (3 <= verbose_level)
    {
        printf("Send 1-byte data for setting mode\n");
    }
    buf[0] = CMD_MODE_SET;
    serial_write(fd, buf, 1);
    serial_read(fd, buf, 2);
    if (buf[1] != STATUS_ACK)
    {
        fprintf(stderr, "Unexpected response %02X\n", buf[1]);
        return -1;
    }
    return 0;
}

int rl78_reset(port_handle_t fd, int mode)
{
    serial_set_txd(fd, 1);                                  /* TOOL0 -> 1 */
    rl78g10_set_reset(fd, mode, 0);                         /* RESET -> 0 */
    usleep(10000);
    rl78g10_set_reset(fd, mode, 1);                         /* RESET -> 1 */
    return 0;
}

int rl78g10_erase_write(port_handle_t fd, const void *data, int size)
{
    unsigned char buf[5];
    if (3 <= verbose_level)
    {
        printf("Send command byte\n");
    }
    buf[0] = CMD_ERASE_WRITE;
    serial_write(fd, buf, 1);
    serial_read(fd, buf, 3);
    if (buf[1] != STATUS_ACK)
    {
        fprintf(stderr, "Unexpected response %02X\n", buf[1]);
        return -1;
    }
    if (get_size_from_code(buf[2]) != size)
    {
        fprintf(stderr, "Unexpected flash size %i, expected %i\n",
                get_size_from_code(buf[2]), size);
        buf[0] = STATUS_NACK;
        serial_write(fd, buf, 1);
        serial_read(fd, buf, 1);
        return -1;
    }
    if (3 <= verbose_level)
    {
        printf("Acknowledge erasing\n");
    }
    buf[0] = STATUS_ACK;
    serial_write(fd, buf, 1);
    serial_read(fd, buf, 1);
    /* Wait till end of erase cycle */
    int i = 100;
    int n;
    do
    {
        n = serial_read(fd, buf, 1);
        --i;
    }
    while (n == 0 && i != 0);

    if (n < 0)
    {
        perror("Unable to read from port:");
        return -1;
    }
    if (buf[0] != STATUS_ACK)
    {
        fprintf(stderr, "Unexpected response %02X\n", buf[1]);
        return -1;
    }
    if (3 <= verbose_level)
    {
        printf("Write data\n");
    }
    const unsigned char *pdata = (const unsigned char*)data;
    for (i = size; i; pdata += 4, i -= 4)
    {
        memcpy(buf, pdata, 4);
        serial_write(fd, buf, 4);
        serial_read(fd, buf, 5);
        if (buf[4] != STATUS_ACK)
        {
            fprintf(stderr, "Unexpected response %02X\n", buf[4]);
            return -2;
        }
    }
    if (3 <= verbose_level)
    {
        printf("Read verification status\n");
    }
    serial_read(fd, buf, 1);
    if (buf[0] != STATUS_ACK)
    {
        fprintf(stderr, "Unexpected response %02X\n", buf[1]);
        return -1;
    }
    return 0;
}

int rl78g10_crc_check(port_handle_t fd, const void *data, int size)
{
    unsigned char buf[5];
    if (3 <= verbose_level)
    {
        printf("Send command byte\n");
    }
    buf[0] = CMD_CRC_CHECK;
    serial_write(fd, buf, 1);
    serial_read(fd, buf, 3);
    if (buf[1] != STATUS_ACK)
    {
        fprintf(stderr, "Unexpected response %02X\n", buf[1]);
        return -1;
    }
    if (get_size_from_code(buf[2]) != size)
    {
        fprintf(stderr, "Unexpected flash size %i, expected %i\n",
                get_size_from_code(buf[2]), size);
        buf[0] = STATUS_NACK;
        serial_write(fd, buf, 1);
        serial_read(fd, buf, 1);
        return -1;
    }
    if (3 <= verbose_level)
    {
        printf("Acknowledge checking\n");
    }
    buf[0] = STATUS_ACK;
    serial_write(fd, buf, 1);
    serial_read(fd, buf, 1);
    
    /* Wait till end of CRC calculation */
    int i = 100;
    int n;
    int recieved = 0;
    unsigned char *pbuf = buf;
    do
    {
        n = serial_read(fd, pbuf, 3);
        pbuf += n;
        recieved += n;
        --i;
    }
    while (n >= 0 && recieved < 3 && i != 0);

    if (n < 0 || recieved < 3)
    {
        perror("Unable to read from port:");
        return -1;
    }
    
    if (buf[0] != STATUS_ACK)
    {
        fprintf(stderr, "Unexpected response %02X\n", buf[1]);
        return -1;
    }
    unsigned int crc_recv = ((unsigned int)buf[2] << 8) | buf[1];
    unsigned int crc_calc = crc16(data, size);

    if (crc_recv != crc_calc)
    {
        fprintf(stderr, "CRC don't match (remote: %04Xh, local: %04Xh)\n", crc_recv, crc_calc);
        return -2;
    }
    if (1 <= verbose_level)
    {
        printf("CRC match %04Xh\n", crc_calc);
    }
    return 0;
}

