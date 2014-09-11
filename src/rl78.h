/*********************************************************************************************************************
 * The MIT License (MIT)                                                                                             *
 * Copyright (c) 2012-2014 Maxim Salov                                                                               *
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

#define RL78_BAUD_115200     0x00
#define RL78_BAUD_250000     0x01
#define RL78_BAUD_500000     0x02
#define RL78_BAUD_1000000    0x03

#define FLASH_BLOCK_SIZE        1024
#define CODE_OFFSET             (0U)
#define DATA_OFFSET             (0x000F1000U)

#define MAX_RESPONSE_LENGTH 32

#define RESPONSE_OK                     (0)
#define RESPONSE_CHECKSUM_ERROR         (-1)
#define RESPONSE_FORMAT_ERROR           (-2)
#define RESPONSE_EXPECTED_LENGTH_ERROR  (-3)

#define SET_MODE_1WIRE_UART 0x3A
#define SET_MODE_2WIRE_UART 0x00

#define RL78_MIN_VOLTAGE    1.8f
#define RL78_MAX_VOLTAGE    5.5f

#define MODE_UART      1
#define MODE_UART_1    0
#define MODE_UART_2    MODE_UART
#define MODE_RESET     2
#define MODE_RESET_DTR 0
#define MODE_RESET_RTS MODE_RESET
#define MODE_MAX_VALUE (MODE_UART | MODE_RESET)
#define MODE_MIN_VALUE 0

#include "serial.h"

int rl78_reset_init(port_handle_t fd, int wait, int baud, int mode, float voltage);
int rl78_reset(port_handle_t fd, int mode);
int rl78_send_cmd(port_handle_t fd, int cmd, const void *data, int len);
int rl78_send_data(port_handle_t fd, const void *data, int len, int last);
int rl78_recv(port_handle_t fd, void *data, int *len, int explen);
int rl78_cmd_reset(port_handle_t fd);
int rl78_cmd_baud_rate_set(port_handle_t fd, int baud, float voltage);
int rl78_cmd_silicon_signature(port_handle_t fd, char device_name[11], unsigned int *code_size, unsigned int *data_size);
int rl78_cmd_block_erase(port_handle_t fd, unsigned int address);
int rl78_cmd_block_blank_check(port_handle_t fd, unsigned int address_start, unsigned int address_end);
int rl78_cmd_checksum(port_handle_t fd, unsigned int address_start, unsigned int address_end);
int rl78_cmd_programming(port_handle_t fd, unsigned int address_start, unsigned int address_end, const void *rom);
unsigned int rl78_checksum(const void *rom, unsigned int len);
int rl78_cmd_verify(port_handle_t fd, unsigned int address_start, unsigned int address_end, const void *rom);
int rl78_program(port_handle_t fd, void *code, unsigned int code_size, void *data, unsigned int data_size);
int rl78_erase(port_handle_t fd, unsigned int code_size, unsigned int data_size);
int rl78_verify(port_handle_t fd, void *code, unsigned int code_size, void *data, unsigned int data_size);

#endif  // RL78_H__
