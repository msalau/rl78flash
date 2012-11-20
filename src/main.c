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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include "rl78.h"
#include "serial.h"
#include "srec.h"
#include "terminal.h"

int verbose_level = 0;

const char *usage =
    "rl78flash [options] <port> [<file>]\n"
    "\t-v\tVerbose mode (several times increase verbose level)\n"
    "\t-i\tDispaly info about MCU\n"
    "\t-a\tAuto mode (Erase-Write-Verify-Reset)\n"
    "\t-e\tErase memory\n"
    "\t-w\tWrite memory\n"
    "\t-c\tVerify memory\n"
    "\t-r\tReset MCU (switch to RUN mode)\n"
    "\t-b baud\tSet baudrate (supported baudrates: 115200, 250000, 500000, 1000000)\n"
    "\t\t\tdefault: 115200\n"
    "\t-m n\tSet communication mode\n"
    "\t\t\tn=1 Single-wire UART, Reset by DTR\n"
    "\t\t\tn=2 Two-wire UART, Reset by DTR\n"
    "\t\t\tn=3 Single-wire UART, Reset by RTS\n"
    "\t\t\tn=4 Two-wire UART, Reset by RTS\n"
    "\t\t\tdefault: n=1\n"
    "\t-p v\tSpecify power supply voltage\n"
    "\t\t\tdefault: 3.3\n"
    "\t-t baud\tStart terminal with specified baudrate\n"
    "\t-h\tDisplay help\n";

struct termios stdin_saved_attributes;
struct termios stdout_saved_attributes;

void restore_io_mode(void)
{
    tcsetattr (STDIN_FILENO, TCSANOW, &stdin_saved_attributes);
    tcsetattr (STDOUT_FILENO, TCSANOW, &stdout_saved_attributes);
}

int main(int argc, char *argv[])
{
    char erase = 0;
    char verify = 0;
    char write = 0;
    char reset_after = 0;
    char display_info = 0;
    char mode = 0;
    int baud = 115200;
    float voltage = 3.3f;
    char terminal = 0;
    int terminal_baud = 0;

    char *endp;
    int opt;
    while ((opt = getopt(argc, argv, "ab:cvwreim:p:t:h?")) != -1)
    {
        switch (opt)
        {
        case 'a':
            erase = 1;
            write = 1;
            verify = 1;
            reset_after = 1;
            break;
        case 'b':
            baud = strtoul(optarg, &endp, 10);
            if (optarg == endp)
            {
                printf("%s", usage);
                return 0;
            }
            break;
        case 'm':
            mode = strtol(optarg, &endp, 10) - 1;
            if (optarg == endp
                || MODE_MAX_VALUE < mode
                || MODE_MIN_VALUE > mode)
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
        case 'p':
            if (1 != sscanf(optarg, "%f", &voltage))
            {
                printf("%s", usage);
                return 0;
            }
            if (RL78_MIN_VOLTAGE > voltage
                || RL78_MAX_VOLTAGE < voltage )
            {
                fprintf(stderr, "Operating voltage is out of range. Operating voltage must be in range %1.1fV..%1.1fV.\n",
                        RL78_MIN_VOLTAGE, RL78_MAX_VOLTAGE);
                return 0;
            }
            break;
        case 'v':
            ++verbose_level;
            break;
        case 'c':
            verify = 1;
            break;
        case 'e':
            erase = 1;
            break;
        case 'w':
            write = 1;
            break;
        case 'r':
            reset_after = 1;
            break;
        case 'i':
            display_info = 1;
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
    switch (argc - optind)
    {
    case 2:
        filename = argv[optind + 1];
    case 1:
        portname = argv[optind + 0];
        break;
    default:
        printf("%s", usage);
        return 0;
    }

    // If file is not specified, but required - show error message
    if (NULL == filename
        && (1 == write || 1 == verify))
    {
        fprintf(stderr, "File not specified\n");
        return -1;
    }

    int fd = serial_open(portname);
    int rc = 0;
    if (-1 == fd)
    {
        return -1;
    }

    /* Make sure stdin is a terminal. */
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Not a terminal.\n");
        return -1;
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr(STDIN_FILENO, &stdin_saved_attributes);
    tcgetattr(STDOUT_FILENO, &stdout_saved_attributes);
    atexit(restore_io_mode);

    struct termios tattr;

    /* Disable canonical mode for stdin */
    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~ICANON;
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);

    /* Disable canonical mode for stdout */
    tcgetattr(STDOUT_FILENO, &tattr);
    tattr.c_lflag &= ~ICANON;
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &tattr);

    /* Disable buffering for stdout */
    setvbuf(stdout, NULL, _IONBF, 0);

    // If no actions are specified - do nothing :)
    if (0 == write
        && 0 == verify
        && 0 == erase
        && 0 == reset_after
        && 0 == display_info
        && 0 == terminal)
    {
        return 0;
    }

    do
    {
        if (1 == write
            || 1 == erase
            || 1 == verify
            || 1 == display_info)
        {
            rc = rl78_reset_init(fd, baud, mode, voltage);
            if (0 > rc)
            {
                fprintf(stderr, "Initialization failed\n");
                break;
            }
            rc = rl78_cmd_reset(fd);
            if (0 > rc)
            {
                fprintf(stderr, "Synchronization failed\n");
                break;
            }
            char device_name[11];
            unsigned int code_size, data_size;
            rc = rl78_cmd_silicon_signature(fd, device_name, &code_size, &data_size);
            if (0 > rc)
            {
                fprintf(stderr, "Silicon signature read failed\n");
                break;
            }
            if (1 == display_info)
            {
                printf("Device: %s\n"
                       "Code size: %u kB\n"
                       "Data size: %u kB\n",
                       device_name, code_size / 1024, data_size / 1024
                    );
            }
            if (1 == erase)
            {
                if (1 <= verbose_level)
                {
                    printf("Erase\n");
                }
                rl78_erase(fd, code_size, data_size);
                if (0 != rc)
                {
                    fprintf(stderr, "Erase failed\n");
                    break;
                }
            }
            unsigned char code[code_size];
            unsigned char data[data_size];
            if (1 == write
                || 1 == verify)
            {
                memset(code, 0xFF, sizeof code);
                memset(data, 0xFF, sizeof data);
                if (1 <= verbose_level)
                {
                    printf("Read file \"%s\"\n", filename);
                }
                rc = srec_read(filename, code, code_size, data, data_size);
                if (0 != rc)
                {
                    fprintf(stderr, "Read failed\n");
                    break;
                }
            }
            if (1 == write)
            {
                if (1 <= verbose_level)
                {
                    printf("Write\n");
                }
                rc = rl78_program(fd, code, code_size, data, data_size);
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
                rc = rl78_verify(fd, code, code_size, data, data_size);
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
            terminal_start(fd, terminal_baud, mode);
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
