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

#ifndef RL78G10_H__
#define RL78G10_H__


#define CMD_MODE_SET            0x3A
#define CMD_ERASE_WRITE         0x60
#define CMD_CRC_CHECK           0x53

#define STATUS_COMMAND_NUMBER_ERROR     0x04
#define STATUS_ACK                      0x06
#define STATUS_NACK                     0x15
#define STATUS_ERASE_ERROR              0x1A
#define STATUS_IVERIFY_BLANK_ERROR      0x1B
#define STATUS_WRITE_ERROR              0x1C

#define FLASH_BLOCK_SIZE        4
#define CODE_OFFSET             (0U)

#define MAX_RESPONSE_LENGTH 32

#define RESPONSE_OK                     (0)
#define RESPONSE_CHECKSUM_ERROR         (-1)
#define RESPONSE_FORMAT_ERROR           (-2)
#define RESPONSE_EXPECTED_LENGTH_ERROR  (-3)

#define MODE_RESET_DTR 0
#define MODE_RESET_RTS 1
#define MODE_MAX_VALUE MODE_RESET_RTS

#include "serial.h"

int rl78_reset(port_handle_t fd, int mode);
int rl78g10_reset_init(port_handle_t fd, int wait, int mode);
int rl78g10_erase_write(port_handle_t fd, const void *data, int size);
int rl78g10_crc_check(port_handle_t fd, const void *data, int size);

#endif  // RL78G10_H__
