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
#include <unistd.h>
#include <string.h>

int verbose_level = 0;

const char *usage =
    "rl78flash [options] <port> [<file>]\n"
    "\t-v\tVerbose mode (several times increase verbose level)\n"
    "\t-i\tDispaly info about mcu\n"
    "\t-a\tAuto mode (Erase-Write-Verify-Reset)\n"
    "\t-e\tErase memory\n"
    "\t-w\tWrite memory\n"
    "\t-c\tVerify memory\n"
    "\t-r\tReset MCU (switch to RUN mode)\n"
    "\t-h\tDisplay help\n";

int main(int argc, char *argv[])
{
    char erase = 0;
    char verify = 0;
    char write = 0;
    char reset_after = 0;
    char display_info = 0;

    int opt;
    while ((opt = getopt(argc, argv, "avwcreih?")) != -1)
    {
        switch (opt)
        {
        case 'a':
            erase = 1;
            write = 1;
            verify = 1;
            reset_after = 1;
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

    // Disable buffering for stdout
    setbuf(stdout, NULL);

    // If no actions are specified - do nothing :)
    if (0 == write
        && 0 == erase
        && 0 == reset_after
        && 0 == display_info)
    {
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

    char device_name[11];
    unsigned int code_size, data_size;
    if (1 == write
        || 1 == erase
        || 1 == verify
        || 1 == display_info)
    {
        rl78_reset_init(fd);
        rl78_cmd_reset(fd);
        rl78_cmd_silicon_signature(fd, device_name, &code_size, &data_size);
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
            printf("Erase\n", filename);
            if (0 != rc)
            {
                fprintf(stderr, "Erase failed\n");
            }
        }
        rl78_erase(fd, code_size, data_size);
    }
    if (1 == write
        || 1 == verify)
    {
        unsigned char code[code_size];
        unsigned char data[data_size];
        memset(code, 0xFF, sizeof code);
        memset(data, 0xFF, sizeof data);
        if (1 <= verbose_level)
        {
            printf("Read file \"%s\"\n", filename);
        }
        rc = srec_read(filename, code, code_size, data, data_size);
        if (0 == rc)
        {
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
                }
            }
        }
        else
        {
            fprintf(stderr, "Read failed\n");
        }
    }
    if (1 == reset_after)
    {
        if (1 <= verbose_level)
        {
            printf("Reset MCU\n");
        }
        rl78_reset(fd);
    }
    serial_close(fd);
    return 0;
}
