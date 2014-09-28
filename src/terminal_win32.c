/*********************************************************************************************************************
 * The MIT License (MIT)                                                                                             *
 * Copyright (c) 2012 Maksim Salau                                                                                    *
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

#include "terminal.h"
#include "rl78.h"
#include "serial.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stddef.h>
#include <signal.h>

#define CTRL_C '\x03'

HANDLE    receiver_stop;

static DWORD WINAPI receiver_func(LPVOID pfd)
{
    char c;
    char prev = '\n';
    port_handle_t fd = *(port_handle_t*)pfd;
    for(;;)
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(receiver_stop, 1))
        {
            break;
        }
        if (1 == serial_read(fd, &c, 1))
        {
            if ('\n' == prev && '\n' != c)
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                printf("[%ld.%ld] ", tv.tv_sec, tv.tv_usec);
            }
            putchar(c);
            fflush(stdout);
            prev = c;
        }
    }
    return 0;
}

void terminal_start(port_handle_t fd, int baud, int mode, int reset)
{
    HANDLE    receiver;
    char c = 0;

    /* Make sure stdin is a terminal. */
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Not a terminal.\n");
        return;
    }
    /* Disable processing of signal characters
     * SIGINT character will be processed in software
     * Also disable canonical mode. */
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD new_mode, old_mode;
    GetConsoleMode(hStdin, &old_mode);
    new_mode = old_mode & ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(hStdin, new_mode);

    serial_set_baud(fd, baud);
    receiver_stop = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                TEXT("ReceiverStopEvent"));
    receiver = CreateThread(NULL,
                            0,
                            receiver_func,
                            &fd,
                            0,
                            NULL);

    if (reset)
    {
        rl78_reset(fd, mode);
    }
    for (;;)
    {
        if (1 == read(STDIN_FILENO, &c, 1))
        {
            if (CTRL_C == c)
            {
                break;
            }
            serial_write(fd, &c, 1);
        }
    }
    SetEvent(receiver_stop);
    WaitForSingleObject(receiver, INFINITE);
    CloseHandle(receiver_stop);
    SetConsoleMode(hStdin, old_mode);
}
