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

#ifndef RL78_H__
#define RL78_H__

#define CMD_RESET               0x00
#define CMD_BLOCK_ERASE         0x22
#define CMD_PROGRAMMING         0x40
#define CMD_VERIFY              0x13
#define CMD_BLOCK_BLANK_CHECK   0x32
#define CMD_BAUD_RATE_SET       0x9A
#define CMD_SILICON_SIGNATURE   0xC0
#define CMD_SECURITY_SET        0xA0
#define CMD_SECURITY_GET        0xA1
#define CMD_SECURITY_RELEASE    0xA2
#define CMD_CHECKSUM            0xB0

#define STATUS_COMMAND_NUMBER_ERROR     0x04
#define STATUS_PARAMETER_ERROR          0x05
#define STATUS_ACK                      0x06
#define STATUS_CHECKSUM_ERROR           0x07
#define STATUS_VERIFY_ERROR             0x0F
#define STATUS_PROTECT_ERROR            0x10
#define STATUS_NACK                     0x15
#define STATUS_ERASE_ERROR              0x1A
#define STATUS_IVERIFY_BLANK_ERROR      0x1B
#define STATUS_WRITE_ERROR              0x1C

#define SOH 0x01
#define STX 0x02
#define ETB 0x17
#define ETX 0x03

#define BAUD_115200     0x00
#define BAUD_250000     0x01
#define BAUD_500000     0x02
#define BAUD_1000000    0x03

#define FLASH_BLOCK_SIZE        1024
#define CODE_OFFSET             (0U)
#define DATA_OFFSET             (0x000F1000U)

#define MAX_RESPONSE_LENGTH 32

#define RESPONSE_OK                     (0)
#define RESPONSE_CHECKSUM_ERROR         (-1)
#define RESPONSE_FORMAT_ERROR           (-2)
#define RESPONSE_EXPECTED_LENGTH_ERROR  (-3)

int rl78_reset_init(int fd);
int rl78_reset(int fd);
int rl78_send_cmd(int fd, int cmd, const void *data, int len);
int rl78_send_data(int fd, const void *data, int len, int last);
int rl78_recv(int fd, void *data, int *len, int explen);
int rl78_cmd_reset(int fd);
int rl78_cmd_baud_rate_set(int fd, int baud, int voltage);
int rl78_cmd_silicon_signature(int fd, char device_name[11], unsigned int *code_size, unsigned int *data_size);
int rl78_cmd_block_erase(int fd, unsigned int address);
int rl78_cmd_block_blank_check(int fd, unsigned int address_start, unsigned int address_end);
int rl78_cmd_checksum(int fd, unsigned int address_start, unsigned int address_end);
int rl78_cmd_programming(int fd, unsigned int address_start, unsigned int address_end, const void *rom);
unsigned int rl78_checksum(const void *rom, unsigned int len);
int rl78_cmd_verify(int fd, unsigned int address_start, unsigned int address_end, const void *rom);
int rl78_program(int fd, void *code, unsigned int code_size, void *data, unsigned int data_size);
int rl78_erase(int fd, unsigned int code_size, unsigned int data_size);
int rl78_verify(int fd, void *code, unsigned int code_size, void *data, unsigned int data_size);

#endif  // RL78_H__
