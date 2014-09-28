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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rl78g10.h"
#include "serial.h"
#include "srec.h"
#include "terminal.h"

int verbose_level = 0;

const char *usage =
    "rl78g10flash [options] <port> [<file> <size>]\n"
    "\t-a\tAuto mode (Erase/Write-Verify-Reset)\n"
    "\t-w\tWrite memory\n"
    "\t-c\tVerify memory (CRC check)\n"
    "\t-r\tReset MCU (switch to RUN mode)\n"
    "\t-d\tDelay bootloader initialization till keypress\n"
    "\t-m n\tSet communication mode\n"
    "\t\t\tn=1 Single-wire UART, Reset by DTR\n"
    "\t\t\tn=2 Single-wire UART, Reset by RTS\n"
    "\t\t\tdefault: n=1\n"
    "\t-t baud\tStart terminal with specified baudrate\n"
    "\t-v\tVerbose mode\n"
    "\t-h\tDisplay help\n";

int main(int argc, char *argv[])
{
    char verify = 0;
    char write = 0;
    char reset_after = 0;
    char wait = 0;
    char mode = 0;
    char terminal = 0;
    int terminal_baud = 0;

    char *endp;
    int opt;
    while ((opt = getopt(argc, argv, "acvwrdm:t:h?")) != -1)
    {
        switch (opt)
        {
        case 'a':
            write = 1;
            verify = 1;
            reset_after = 1;
            break;
        case 'm':
            mode = strtol(optarg, &endp, 10) - 1;
            if (optarg == endp
                || MODE_MAX_VALUE <= mode)
            {
                printf("%s", usage);
                return 0;
            }
            break;
        case 't':
            terminal = 1;
            terminal_baud = strtol(optarg, &endp, 10);
            if (optarg == endp)
            {
                printf("%s", usage);
                return 0;
            }
            break;
        case 'v':
            ++verbose_level;
            break;
        case 'c':
            verify = 1;
            break;
        case 'w':
            write = 1;
            break;
        case 'r':
            reset_after = 1;
            break;
        case 'd':
            wait = 1;
            break;
        case 'h':
        case '?':
        default:
            printf("%s", usage);
            return 0;
        }
    }
    char *portname = NULL;
    char *filename = NULL;
    char *codesizestr = NULL;
    switch (argc - optind)
    {
    case 3:
        codesizestr = argv[optind + 2];
        /* fallthrough */
    case 2:
        filename = argv[optind + 1];
        /* fallthrough */
    case 1:
        portname = argv[optind + 0];
        break;
    default:
        printf("%s", usage);
        return 0;
    }

    // If file is not specified, but required - show error message
    if ((NULL == filename || NULL == codesizestr)
        && (1 == write || 1 == verify))
    {
        fprintf(stderr, "Specify both file and size\n");
        return -1;
    }

    int codesize = 0;
    if (codesizestr)
    {
        char *endp = NULL;
        codesize = strtol(codesizestr, &endp, 10);
        if (endp && (*endp == 'k' || *endp == 'K'))
            codesize *= 1024;
        /* if codesize is not power of two */
        if (codesize & (codesize - 1))
        {
            fprintf(stderr, "Size (%i) is not valid (not power ot 2)\n", codesize);
            return -2;
        }
        /* if codesize is in valid range: 512...64k */
        if (codesize < 512 || codesize > 64*1024)
        {
            fprintf(stderr, "Size (%i) is not valid (not in range)\n", codesize);
            return -2;
        }
    }

    // If no actions are specified - do nothing :)
    if (0 == write
        && 0 == verify
        && 0 == reset_after
        && 0 == terminal)
    {
        return 0;
    }

    port_handle_t fd = serial_open(portname);
    int rc = 0;
    if (INVALID_HANDLE_VALUE == fd)
    {
        return -1;
    }
    rc = serial_set_parity(fd, ENABLE, ODD);
    if (rc < 0)
    {
        perror("Failed to set port attributes:");
        serial_close(fd);
        return -1;
    }

    do
    {
        if (1 == write || 1 == verify)
        {
            rc = rl78g10_reset_init(fd, wait, mode);
            if (0 > rc)
            {
                fprintf(stderr, "Initialization failed\n");
                break;
            }
            unsigned char code[codesize];

            memset(code, 0xFF, sizeof code);
            if (1 <= verbose_level)
            {
                printf("Read file \"%s\"\n", filename);
            }
            rc = srec_read(filename, code, codesize, NULL, 0);
            if (0 != rc)
            {
                fprintf(stderr, "Read failed\n");
                break;
            }

            if (1 == write)
            {
                if (1 <= verbose_level)
                {
                    printf("Write\n");
                }
                rc = rl78g10_erase_write(fd, code, codesize);
                if (0 != rc)
                {
                    fprintf(stderr, "Write failed\n");
                    break;
                }
            }
            if (1 == verify)
            {
                if (1 <= verbose_level)
                {
                    printf("Verify\n");
                }
                rc = rl78g10_crc_check(fd, code, codesize);
                if (0 != rc)
                {
                    fprintf(stderr, "Verify failed\n");
                    break;
                }
            }
        }
        if (1 == terminal)
        {
            if (1 <= verbose_level)
            {
                printf("Start terminal\n");
            }
            int reset_before_terminal = write || verify || reset_after;
            serial_set_parity(fd, DISABLE, 0);
            terminal_start(fd, terminal_baud, mode, reset_before_terminal);
        }
        else if (1 == reset_after)
        {
            if (1 <= verbose_level)
            {
                printf("Reset MCU\n");
            }
            rl78_reset(fd, mode);
        }
    }
    while (0);
    serial_close(fd);
    printf("\n");
    return 0;
}
