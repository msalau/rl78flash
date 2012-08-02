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
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

extern int verbose_level;

int rl78_reset_init(int fd, int baud)
{
    serial_set_dtr(fd, 0);
    serial_set_rts(fd, 0);
    usleep(1000);
    serial_set_dtr(fd, 1);                                  /* RESET -> 1 */
    usleep(10);
    serial_set_rts(fd, 1);                                  /* TOOL0 -> 1 */
    usleep(1000);
    int rc;
    int r;
    if (3 <= verbose_level)
    {
        printf("Send SYNC byte\n");
    }
    rc = serial_write(fd, "\x3A", 1);
    rc = serial_read(fd, &r, 1);
    serial_sync(fd);
    usleep(1000);
    return rl78_cmd_baud_rate_set(fd, baud, 3300);
}

int rl78_reset(int fd)
{
    serial_set_rts(fd, 1);                                  /* TOOL0 -> 1 */
    serial_set_dtr(fd, 0);                                  /* RESET -> 0 */
    usleep(10000);
    serial_set_dtr(fd, 1);                                  /* RESET -> 1 */
    return 0;
}

static
int checksum(const void *data, int len)
{
    unsigned int sum = 0;
    const unsigned char *p = data;
    for (; len; --len)
    {
        sum -= *p++;
    }
    return sum & 0x00FF;
}

int rl78_send_cmd(int fd, int cmd, const void *data, int len)
{
    if (255 < len)
    {
        return -1;
    }
    unsigned char buf[len + 5];
    buf[0] = SOH;
    buf[1] = (len + 1) & 0xFFU;
    buf[2] = cmd;
    memcpy(&buf[3], data, len);
    buf[len + 3] = checksum(&buf[1], len + 2);
    buf[len + 4] = ETX;
    int ret = serial_write(fd, buf, sizeof buf);
    // Read back echo
    int rc = serial_read(fd, buf, sizeof buf);
    return ret;
}

int rl78_send_data(int fd, const void *data, int len, int last)
{
    if (256 < len)
    {
        return -1;
    }
    unsigned char buf[len + 4];
    buf[0] = STX;
    buf[1] = len & 0xFFU;
    memcpy(&buf[2], data, len);
    buf[len + 2] = checksum(&buf[1], len + 1);
    buf[len + 3] = last ? ETX : ETB;
    int ret = serial_write(fd, buf, sizeof buf);
    // Read back echo
    int rc = serial_read(fd, buf, sizeof buf);
    return ret;
}

int rl78_recv(int fd, void *data, int *len, int explen)
{
    unsigned char in[MAX_RESPONSE_LENGTH];
    int rc;
    int data_len;
    // receive header
    rc = serial_read(fd, in, 2);
    data_len = in[1];
    if (0 == data_len)
    {
        data_len = 256;
    }
    if ((MAX_RESPONSE_LENGTH - 2) <= data_len
        || STX != in[0])
    {
        serial_flush(fd);
        return RESPONSE_FORMAT_ERROR;
    }
    if (explen != data_len)
    {
        serial_flush(fd);
        return RESPONSE_EXPECTED_LENGTH_ERROR;
    }
    // receive data field, checksum and footer byte
    rc = serial_read(fd, in + 2, data_len + 2);
    switch (in[data_len + 3])
    {
    case ETB:
    case ETX:
        break;
    default:
        return RESPONSE_FORMAT_ERROR;
    }
    if (checksum(in + 1, data_len + 1) != in[data_len + 2])
    {
        return RESPONSE_CHECKSUM_ERROR;
    }
    memcpy(data, in + 2, data_len);
    *len = data_len;
    return RESPONSE_OK;
}

int rl78_cmd_reset(int fd)
{
    if (3 <= verbose_level)
    {
        printf("Send \"Reset\" command\n");
    }
    rl78_send_cmd(fd, CMD_RESET, NULL, 0);
    int len = 0;
    unsigned char data[3];
    int rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    else
    {
        if (3 <= verbose_level)
        {
            printf("\tOK\n");
        }
    }
    return 0;
}

int rl78_cmd_baud_rate_set(int fd, int baud, int voltage)
{
    unsigned char buf[2];
    int new_baud;
    switch (baud)
    {
    case 115200:
    default:
        buf[0] = BAUD_115200;
        new_baud = B115200;
        baud = 115200;
        break;
    /* case 250000:
     *  buf[0] = BAUD_250000;
     *  new_baud = B250000;
     *  break; */
    case 500000:
        buf[0] = BAUD_500000;
        new_baud = B500000;
        break;
    case 1000000:
        buf[0] = BAUD_1000000;
        new_baud = B1000000;
        break;
    }
    buf[1] = voltage / 100;
    if (3 <= verbose_level)
    {
        printf("Send \"Set Baud Rate\" command (baud=%ubps, voltage=%1.1fV)\n", baud, (float)voltage/1000);
    }
    rl78_send_cmd(fd, CMD_BAUD_RATE_SET, buf, 2);
    int len = 0;
    unsigned char data[3];
    int rc = rl78_recv(fd, &data, &len, 3);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    if (3 <= verbose_level)
    {
        printf("\tOK\n");
        printf("\tFrequency: %u MHz\n", data[1]);
        printf("\tMode: %s\n", 0 == data[2] ? "full-speed mode" : "wide-voltage mode");
    }
    serial_set_baud(fd, new_baud);
    return 0;
}

int rl78_cmd_silicon_signature(int fd, char device_name[11], unsigned int *code_size, unsigned int *data_size)
{
    if (3 <= verbose_level)
    {
        printf("Send \"Get Silicon Signature\" command\n");
    }
    rl78_send_cmd(fd, CMD_SILICON_SIGNATURE, NULL, 0);
    int len = 0;
    unsigned char data[22];
    int rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    rc = rl78_recv(fd, &data, &len, 22);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (NULL != device_name)
    {
        memcpy(device_name, data + 3, 10);
        device_name[10] = '\0';
    }
    unsigned int rom_code_address = 0;
    unsigned int rom_data_address = 0;
    memcpy(&rom_code_address, data + 13, 3);
    memcpy(&rom_data_address, data + 16, 3);
    const unsigned int rom_code_size = (rom_code_address + 1);
    const unsigned int rom_data_size = (rom_data_address - 0x000F1000U + 1);

    if (NULL != code_size)
    {
        *code_size = rom_code_size;
    }
    if (NULL != data_size)
    {
        *data_size = rom_data_size;
    }
    if (3 <= verbose_level)
    {
        printf("\tOK\n");
        printf("\tDevice code: %02X%02X%02X\n", data[0], data[1], data[2]);
        printf("\tDevice name: %s\n", device_name);
        printf("\tCode flash size: %ukB\n", rom_code_size / 1024);
        printf("\tData flash size: %ukB\n", rom_data_size / 1024);
        printf("\tFirmware version: %X.%X%X\n", data[19], data[20], data[21]);
    }
    return 0;
}

int rl78_cmd_block_erase(int fd, unsigned int address)
{
    if (3 <= verbose_level)
    {
        printf("Send \"Block Erase\" command (addres=%06X)\n", address);
    }
    rl78_send_cmd(fd, CMD_BLOCK_ERASE, &address, 3);
    int len = 0;
    unsigned char data[1];
    int rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    if (3 <= verbose_level)
    {
        printf("\tOK\n");
    }
    return 0;
}

int rl78_cmd_block_blank_check(int fd, unsigned int address_start, unsigned int address_end)
{
    if (3 <= verbose_level)
    {
        printf("Send \"Block Blank Check\" command (range=%06X..%06X)\n", address_start, address_end);
    }
    unsigned char buf[7];
    memcpy(buf + 0, &address_start, 3);
    memcpy(buf + 3, &address_end, 3);
    buf[6] = 0;
    rl78_send_cmd(fd, CMD_BLOCK_BLANK_CHECK, buf, sizeof buf);
    int len = 0;
    unsigned char data[1];
    int rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0]
        && STATUS_IVERIFY_BLANK_ERROR != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    if (STATUS_ACK == data[0])
    {
        rc = 0;
    }
    else
    {
        rc = 1;
    }

    if (3 <= verbose_level)
    {
        printf("\tOK\n");
        if (0 == rc)
        {
            printf("\tBlock is empty\n");
        }
        else
        {
            printf("\tBlock is not empty\n");
        }
    }
    return rc;
}

int rl78_cmd_checksum(int fd, unsigned int address_start, unsigned int address_end)
{
    if (3 <= verbose_level)
    {
        printf("Send \"Checksum\" command (range=%06X..%06X)\n", address_start, address_end);
    }
    unsigned char buf[6];
    memcpy(buf + 0, &address_start, 3);
    memcpy(buf + 3, &address_end, 3);
    rl78_send_cmd(fd, CMD_CHECKSUM, buf, sizeof buf);
    int len = 0;
    unsigned char data[2];
    int rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    rc = rl78_recv(fd, &data, &len, 2);
    if (3 <= verbose_level)
    {
        printf("\tOK\n");
        printf("\tValue: %02X%02X\n", data[1], data[0]);
    }
    return rc;
}

int rl78_cmd_programming(int fd, unsigned int address_start, unsigned int address_end, const void *rom)
{
    if (3 <= verbose_level)
    {
        printf("Send \"Programming\" command (range=%06X..%06X)\n", address_start, address_end);
    }
    unsigned char buf[6];
    memcpy(buf + 0, &address_start, 3);
    memcpy(buf + 3, &address_end, 3);
    rl78_send_cmd(fd, CMD_PROGRAMMING, buf, sizeof buf);
    int len = 0;
    unsigned char data[2];
    int rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    unsigned int rom_length = address_end - address_start + 1;
    unsigned char *rom_p = (unsigned char*)rom;
    unsigned int address_current = address_start;
    unsigned int final_delay = (rom_length / 1024 + 1) * 1500;
    // Send data
    while (rom_length)
    {
        if (3 <= verbose_level)
        {
            printf("\tSend data to address %06X\n", address_current);
        }
        if (256 < rom_length)
        {
            // Not last data frame
            rl78_send_data(fd, rom_p, 256, 0);
            address_current += 256;
            rom_p += 256;
            rom_length -= 256;
        }
        else
        {
            // Last data frame
            rl78_send_data(fd, rom_p, rom_length, 1);
            address_current += rom_length;
            rom_p += rom_length;
            rom_length -= rom_length;
        }
        rc = rl78_recv(fd, &data, &len, 2);
        if (RESPONSE_OK != rc)
        {
            fprintf(stderr, "FAILED\n");
            return rc;
        }
        if (STATUS_ACK != data[0])
        {
            fprintf(stderr, "ACK not received\n");
            return data[0];
        }
        if (STATUS_ACK != data[1])
        {
            fprintf(stderr, "Data not written\n");
            return data[1];
        }
    }
    usleep(final_delay);
    // Receive status of completion
    rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
        }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    if (3 <= verbose_level)
    {
        printf("\tOK\n");
    }
    return rc;
}

unsigned int rl78_checksum(const void *rom, unsigned int len)
{
    unsigned int sum = 0;
    unsigned char *p = (unsigned char*)rom;
    unsigned int i = len;
    for (; i; --i)
    {
        sum -= *p++;
    }
    return sum & 0x0000FFFFU;
}

int rl78_cmd_verify(int fd, unsigned int address_start, unsigned int address_end, const void *rom)
{
    if (3 <= verbose_level)
    {
        printf("Send \"Verify\" command (range=%06X..%06X)\n", address_start, address_end);
    }
    unsigned char buf[6];
    memcpy(buf + 0, &address_start, 3);
    memcpy(buf + 3, &address_end, 3);
    rl78_send_cmd(fd, CMD_VERIFY, buf, sizeof buf);
    int len = 0;
    unsigned char data[2];
    int rc = rl78_recv(fd, &data, &len, 1);
    if (RESPONSE_OK != rc)
    {
        fprintf(stderr, "FAILED\n");
        return rc;
    }
    if (STATUS_ACK != data[0])
    {
        fprintf(stderr, "ACK not received\n");
        return data[0];
    }
    unsigned int rom_length = address_end - address_start + 1;
    unsigned char *rom_p = (unsigned char*)rom;
    unsigned int address_current = address_start;
    unsigned int final_delay = (rom_length / 1024 + 1) * 1500;
    // Send data
    while (rom_length)
    {
        if (3 <= verbose_level)
        {
            printf("\tSend data to address %06X\n", address_current);
        }
        if (256 < rom_length)
        {
            // Not last data frame
            rl78_send_data(fd, rom_p, 256, 0);
            address_current += 256;
            rom_p += 256;
            rom_length -= 256;
        }
        else
        {
            // Last data frame
            rl78_send_data(fd, rom_p, rom_length, 1);
            address_current += rom_length;
            rom_p += rom_length;
            rom_length -= rom_length;
        }
        usleep(10000);
        rc = rl78_recv(fd, &data, &len, 2);
        if (RESPONSE_OK != rc)
        {
            fprintf(stderr, "FAILED\n");
            return rc;
        }
        if (STATUS_ACK != data[0])
        {
            fprintf(stderr, "ACK not received\n");
            return data[0];
        }
        if (STATUS_ACK != data[1])
        {
            fprintf(stderr, "Verify failed\n");
            return data[1];
        }
    }
    if (3 <= verbose_level)
    {
        printf("\tOK\n");
    }
    return rc;
}

static
int allFFs(const void *mem, unsigned int size)
{
    unsigned char *p = (unsigned char*)mem;
    unsigned int i = size;
    for (; i; --i)
    {
        if (0xFF != *p++)
        {
            return 0;
        }
    }
    return 1;
}

int rl78_program(int fd, void *code, unsigned int code_size, void *data, unsigned int data_size)
{
    // Make sure size is aligned to flash block boundary
    unsigned int i = code_size & ~(FLASH_BLOCK_SIZE - 1);
    unsigned char *mem = (unsigned char*)code;
    unsigned int address = CODE_OFFSET;
    int rc = 0;;
    for (; i; i -= FLASH_BLOCK_SIZE)
    {
        if (!allFFs(mem, FLASH_BLOCK_SIZE))
        {
            if (3 <= verbose_level)
            {
                printf("Program block %06X\n", address);
            }
            // Check if block is ready to program new content
            rc = rl78_cmd_block_blank_check(fd, address, address + FLASH_BLOCK_SIZE - 1);
            if (0 > rc)
            {
                fprintf(stderr, "Block Blank Check failed (%06X)\n", address);
                break;
            }
            if (0 < rc)
            {
                // If block is not empty - erase it
                rc = rl78_cmd_block_erase(fd, address);
                if (0 > rc)
                {
                    fprintf(stderr, "Block Erase failed (%06X)\n", address);
                    break;
                }
            }
            // Write new content
            rc = rl78_cmd_programming(fd, address, address + FLASH_BLOCK_SIZE - 1, mem);
            if (0 > rc)
            {
                fprintf(stderr, "Programming failed (%06X)\n", address);
                break;
            }
            if (2 == verbose_level)
            {
                printf("*");
            }
        }
        else
        {
            if (3 <= verbose_level)
            {
                printf("No data at block %06X\n", address);
            }
        }
        mem += FLASH_BLOCK_SIZE;
        address += FLASH_BLOCK_SIZE;
    }
    if (2 == verbose_level)
    {
        printf("\n");
    }
    if (0 > rc)
    {
        return rc;
    }
    return rc;
}

int rl78_erase(int fd, unsigned int code_size, unsigned int data_size)
{
    // Make sure size is aligned to flash block boundary
    unsigned int i = code_size & ~(FLASH_BLOCK_SIZE - 1);
    unsigned int address = CODE_OFFSET;
    int rc = 0;
    for (; i; i -= FLASH_BLOCK_SIZE)
    {
        rc = rl78_cmd_block_blank_check(fd, address, address + FLASH_BLOCK_SIZE - 1);
        if (0 > rc)
        {
            fprintf(stderr, "Block Blank Check failed (%06X)\n", address);
            break;
        }
        if (0 < rc)
        {
            // If block is not empty
            rc = rl78_cmd_block_erase(fd, address);
            if (0 > rc)
            {
                fprintf(stderr, "Block Erase failed (%06X)\n", address);
                break;
            }
            if (2 == verbose_level)
            {
                printf("*");
            }
        }
        else
        {
            // if block is already empty
            if (2 == verbose_level)
            {
                printf(".");
            }
        }
        address += FLASH_BLOCK_SIZE;
    }
    if (2 == verbose_level)
    {
        printf("\n");
    }
    if (0 > rc)
    {
        return rc;
    }
    return rc;
}

int rl78_verify(int fd, void *code, unsigned int code_size, void *data, unsigned int data_size)
{
    // Make sure size is aligned to flash block boundary
    unsigned int i = code_size & ~(FLASH_BLOCK_SIZE - 1);
    unsigned char *mem = (unsigned char*)code;
    unsigned int address = CODE_OFFSET;
    int rc = 0;
    for (; i; i -= FLASH_BLOCK_SIZE)
    {
        if (3 <= verbose_level)
        {
            printf("Verify block %06X\n", address);
        }
        if (allFFs(mem, FLASH_BLOCK_SIZE))
        {
            // Check if block is blank
            rc = rl78_cmd_block_blank_check(fd, address, address + FLASH_BLOCK_SIZE - 1);
            if (0 > rc)
            {
                fprintf(stderr, "Block Blank Check failed (%06X)\n", address);
                break;
            }
            if (0 < rc)
            {
                fprintf(stderr, "Block content does not match (%06X)\n", address);
                break;
            }
            if (2 == verbose_level)
            {
                printf(".");
            }
        }
        else
        {
            // If block is not blank
            rc = rl78_cmd_verify(fd, address, address + FLASH_BLOCK_SIZE - 1, mem);
            if (0 != rc)
            {
                fprintf(stderr, "Block content does not match (%06X)\n", address);
                break;
            }
            if (2 == verbose_level)
            {
                printf("*");
            }
        }
        mem += FLASH_BLOCK_SIZE;
        address += FLASH_BLOCK_SIZE;
    }
    if (2 == verbose_level)
    {
        printf("\n");
    }
    if (0 > rc)
    {
        return rc;
    }
    return rc;
}
