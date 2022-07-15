/*********************************************************************************************************************
 * The MIT License (MIT)                                                                                             *
 * Copyright (c) 2012-2016 Maksim Salau                                                                              *
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
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include "rl78.h"
#include "serial.h"
#include "srec.h"
#include "terminal.h"

int verbose_level = 0;

const char *usage =
    "rl78flash [options] <port> [<file>]\n"
    "\t-v\tVerbose mode (several times increase verbose level)\n"
    "\t-i\tDisplay info about MCU\n"
    "\t-a\tAuto mode (Erase-Write-Verify-Reset)\n"
    "\t-e\tErase memory\n"
    "\t-w\tWrite memory\n"
    "\t-c\tVerify memory\n"
    "\t-x\tDisable Data Flash\n"
    "\t-y\tDisable Code Flash\n"
    "\t-r\tReset MCU (switch to RUN mode)\n"
    "\t-d\tDelay bootloader initialization till keypress\n"
    "\t-b baud\tSet baudrate (supported baudrates: 115200, 250000, 500000, 1000000)\n"
    "\t\t\tdefault: 115200\n"
    "\t-m n\tSet communication mode\n"
    "\t\t\tn=1 Single-wire UART, Reset by DTR\n"
    "\t\t\tn=2 Two-wire UART, Reset by DTR\n"
    "\t\t\tn=3 Single-wire UART, Reset by RTS\n"
    "\t\t\tn=4 Two-wire UART, Reset by RTS\n"
    "\t\t\tdefault: n=1\n"
    "\t-P n\tSet protocol version\n"
    "\t\t\tn=-1 Try to autodetect the protocol version from the unit's Silicon Signature\n"
    "\t\t\tn=0 Protocol A, for most RL78 units\n"
    "\t\t\tn=2 Protocol C, for RL78/G23 units\n"
    "\t\t\tn=3 Protocol D, for RL78/F24 units\n"
    "\t\t\tdefault: n=-1\n"
    "\t-n\tInvert reset\n"
    "\t-p v\tSpecify power supply voltage\n"
    "\t\t\tdefault: 3.3\n"
    "\t-t baud\tStart terminal with specified baudrate\n"
    "\t-h\tDisplay help\n";

int main(int argc, char *argv[])
{
    char erase = 0;
    char verify = 0;
    char write = 0;
    char reset_after = 0;
    char wait = 0;
    char display_info = 0;
    int8_t mode = 0;
    char invert_reset = 0;
    int baud = 115200;
    float voltage = 3.3f;
    char terminal = 0;
    int terminal_baud = 0;
    char nodata = 0;
    char nocode = 0;
    int proto_ver = -1;

    char *endp;
    int opt;
    while ((opt = getopt(argc, argv, "xyab:cvwrdeim:np:P:t:h?")) != -1)
    {
        switch (opt)
        {
        case 'x':
            nodata = 1;
            break;
        case 'y':
            nocode = 1;
            break;
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
                fprintf(stderr, "Baudrate is not defined\n");
                printf("%s", usage);
                return EINVAL;
            }
            break;
        case 'm':
            mode = strtol(optarg, &endp, 10) - 1;
            if (optarg == endp
                || MODE_MAX_VALUE < mode
                || MODE_MIN_VALUE > mode)
            {
                fprintf(stderr, "Invalid mode\n");
                printf("%s", usage);
                return EINVAL;
            }
            break;
        case 't':
            terminal = 1;
            terminal_baud = strtol(optarg, &endp, 10);
            if (optarg == endp)
            {
                fprintf(stderr, "Terminal baudrate is not defined\n");
                printf("%s", usage);
                return EINVAL;
            }
            break;
        case 'p':
            if (1 != sscanf(optarg, "%f", &voltage))
            {
                fprintf(stderr, "Invalid voltage value: %s\n", optarg);
                printf("%s", usage);
                return EINVAL;
            }
            if (RL78_MIN_VOLTAGE > voltage
                || RL78_MAX_VOLTAGE < voltage )
            {
                fprintf(stderr, "Operating voltage is out of range. Operating voltage must be in range %1.1fV..%1.1fV.\n",
                        RL78_MIN_VOLTAGE, RL78_MAX_VOLTAGE);
                return EINVAL;
            }
            break;
        case 'P':
            if (1 != sscanf(optarg, "%d", &proto_ver))
            {
                fprintf(stderr, "Invalid protocol version ID value: %s\n", optarg);
                printf("%s", usage);
                return EINVAL;
            }
            if (1 == proto_ver || PROTOCOL_VERSION_A > proto_ver || PROTOCOL_VERSION_D < proto_ver)
            {
                fprintf(stderr, "Protocol version ID is out of range. See the output of `-h' for the list of allowed values.\n");
                return EINVAL;
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
        case 'd':
            wait = 1;
            break;
        case 'i':
            display_info = 1;
            break;
        case 'n':
            invert_reset = 1;
            break;
        case 'h':
        case '?':
            printf("%s", usage);
            return ECANCELED;
        default:
            fprintf(stderr, "Unknown argument: %c\n", opt);
            printf("%s", usage);
            return EINVAL;
        }
    }
    if (invert_reset)
    {
        mode |= MODE_INVERT_RESET;
    }
    char *portname = NULL;
    char *filename = NULL;
    switch (argc - optind)
    {
    case 2:
        filename = argv[optind + 1];
        /* fall-through */
    case 1:
        portname = argv[optind + 0];
        break;
    default:
        printf("%s", usage);
        return EINVAL;
    }

    // If file is not specified, but required - show error message
    if (NULL == filename
        && (1 == write || 1 == verify))
    {
        fprintf(stderr, "File not specified\n");
        return ENOENT;
    }

    port_handle_t fd = serial_open(portname);
    int rc = 0;
    if (INVALID_HANDLE_VALUE == fd)
    {
        return EBADF;
    }

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

    int retcode = 0;
    unsigned char *code = NULL;
    unsigned char *data = NULL;

    do
    {
        if (1 == write
            || 1 == erase
            || 1 == verify
            || 1 == display_info)
        {
            rc = rl78_reset_init(fd, wait, baud, mode, voltage);
            if (0 > rc)
            {
                fprintf(stderr, "Initialization failed\n");
                retcode = EIO;
                break;
            }
            rc = rl78_cmd_reset(fd);
            if (0 > rc)
            {
                fprintf(stderr, "Synchronization failed\n");
                retcode = EIO;
                break;
            }
            char device_name[11];
            unsigned int code_size, data_size;
            rc = rl78_cmd_silicon_signature(fd, device_name, &code_size, &data_size);
            if (0 > rc)
            {
                fprintf(stderr, "Silicon signature read failed\n");
                retcode = EIO;
                break;
            }
            if (-1 == proto_ver)
            {
                if (!memcmp(device_name, "R7F", 3))
                {
                    /* NOTE: this isn't too precise, that's why a separate
                     * optional flag for overriding is provided */
                    if (device_name[4] == '2') /* RL78/F24: eg. R7F124 */
                    {
                        proto_ver = PROTOCOL_VERSION_D;
                    }
                    else /* RL78/G23: eg. R7F100 */
                    {
                        proto_ver = PROTOCOL_VERSION_C;
                    }
                }
                else if (!memcmp(device_name, "R5F", 3))
                {
                    proto_ver = PROTOCOL_VERSION_A;
                }
                else
                {
                    fprintf(stderr, "Error: unknown device name '%s', please set "
                            "the protocol version manually.\n", device_name);
                    retcode = EINVAL;
                    break;
                }
            }
            if (0 == block_size_table[proto_ver])
            {
                fprintf(stderr, "Error: invalid protocol version ID %d, I don't "
                        "know what flash block size corresponds to this version!\n",
                        proto_ver);
                retcode = EINVAL;
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
            if (!code_size)
            {
                fprintf(stderr, "Invalid code size: %u\n", code_size);
                retcode = EINVAL;
                break;
            }
            if (!nocode && (1 == erase))
            {
                if (1 <= verbose_level)
                {
                    printf("Erase code flash\n");
                }
                rc = rl78_erase(fd, CODE_OFFSET, code_size, proto_ver);
                if (0 != rc)
                {
                    fprintf(stderr, "Code flash erase failed\n");
                    retcode = EIO;
                    break;
                }
            }
            if (!nodata && (1 == erase && data_size))
            {
                if (1 <= verbose_level)
                {
                    printf("Erase data flash\n");
                }
                rc = rl78_erase(fd, DATA_OFFSET, data_size, proto_ver);
                if (0 != rc)
                {
                    fprintf(stderr, "Data flash erase failed\n");
                    retcode = EIO;
                    break;
                }
            }

            code = malloc(code_size);
            if (data_size)
                data = malloc(data_size);

            if (!code || (!data && data_size))
            {
                fprintf(stderr, "Memory allocation failed\n");
                retcode = ENOMEM;
                break;
            }
            if (1 == write
                || 1 == verify)
            {
                memset(code, 0xFF, code_size);
                if (data)
                    memset(data, 0xFF, data_size);
                if (1 <= verbose_level)
                {
                    printf("Read file \"%s\"\n", filename);
                }
                rc = srec_read(filename, code, code_size, data, data_size);
                if (0 != rc)
                {
                    fprintf(stderr, "Read failed\n");
                    retcode = EIO;
                    break;
                }
            }
            if (!nocode && (1 == write))
            {
                if (1 <= verbose_level)
                {
                    printf("Write code flash\n");
                }
                rc = rl78_program(fd, CODE_OFFSET, code, code_size, proto_ver);
                if (0 != rc)
                {
                    fprintf(stderr, "Code flash write failed\n");
                    retcode = EIO;
                    break;
                }
            }
            if (!nodata && (1 == write && data_size))
            {
                if (1 <= verbose_level)
                {
                    printf("Write data flash\n");
                }
                rc = rl78_program(fd, DATA_OFFSET, data, data_size, proto_ver);
                if (0 != rc)
                {
                    fprintf(stderr, "Data flash write failed\n");
                    retcode = EIO;
                    break;
                }
            }
            if (!nocode && (1 == verify))
            {
                if (1 <= verbose_level)
                {
                    printf("Verify Code flash\n");
                }
                rc = rl78_verify(fd, CODE_OFFSET, code, code_size, proto_ver);
                if (0 != rc)
                {
                    fprintf(stderr, "Code flash verification failed\n");
                    retcode = EIO;
                    break;
                }
            }
            if (!nodata && (1 == verify && data_size))
            {
                if (1 <= verbose_level)
                {
                    printf("Verify Data flash\n");
                }
                rc = rl78_verify(fd, DATA_OFFSET, data, data_size, proto_ver);
                if (0 != rc)
                {
                    fprintf(stderr, "Data flash verification failed\n");
                    retcode = EIO;
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
            int reset_before_terminal = write || verify || erase
                || reset_after || display_info;
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

    if (code)
        free(code);
    if (data)
        free(data);

    serial_close(fd);
    printf("\n");
    return retcode;
}
