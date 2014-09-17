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

#ifndef SERIAL_H__
#define SERIAL_H__

#if WIN32 != 1

typedef int port_handle_t;
#define INVALID_HANDLE_VALUE (-1)

#else

#include <windows.h>
typedef HANDLE port_handle_t;

#endif

#define DISABLE 0
#define ENABLE  1
#define EVEN    0
#define ODD     1

port_handle_t serial_open(const char *port);
int serial_set_baud(port_handle_t fd, int baud);
int serial_set_parity(port_handle_t fd, int enable, int odd_parity);
int serial_set_dtr(port_handle_t fd, int level);
int serial_set_rts(port_handle_t fd, int level);
int serial_set_txd(port_handle_t fd, int level);
int serial_flush(port_handle_t fd);
int serial_write(port_handle_t fd, const void *buf, int len);
int serial_read(port_handle_t fd, void *buf, int len);
int serial_close(port_handle_t fd);

#endif  // SERIAL_H__
