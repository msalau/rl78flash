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

#include "srec.h"
#include "rl78.h"
#include <stdio.h>

extern unsigned int verbose_level;

static
int ascii2hex(const char *str, unsigned int len)
{
    int res = 0;
    unsigned int i = len;
    const char *pstr = str;
    for (; i; --i)
    {
        res <<= 4;
        if ('0' <= *pstr && '9' >= *pstr)
        {
            res += *pstr - '0';
        }
        else if ('A' <= *pstr && 'F' >= *pstr)
        {
            res += *pstr + 10 - 'A';
        }
        else if ('a' <= *pstr && 'f' >= *pstr)
        {
            res += *pstr + 10 - 'a';
        }
        else
        {
            return -1;
        }
        ++pstr;
    }
    return res;
}

int srec_read(const char *filename,
              void *code, unsigned int code_len,
              void *data, unsigned int data_len)
{
    FILE *pfile;
    char line[512];
    int rc = SREC_NO_ERROR;
    pfile = fopen(filename, "r");
    if (NULL == pfile)
    {
        fprintf(stderr, "Unable to open file \"%s\"\n", filename);
        return SREC_IO_ERROR;
    }
    rewind(pfile);
    while (!feof(pfile))
    {
        if (1 != fscanf(pfile, "%s\n", line))
        {
            fprintf(stderr, "Unable to read file \"%s\"\n", filename);
            rc = SREC_IO_ERROR;
            break;
        }
        if (4 <= verbose_level)
        {
            printf("srec: %s\n", line);
        }
        if ('S' != line[0])
        {
            fprintf(stderr, "File format error (\"%s\")\n", line);
            rc = SREC_FORMAT_ERROR;
            break;
        }
        // Ignore non-data frames
        const unsigned int record_type = ascii2hex(&line[1], 1);
        if (1 != record_type
            && 2 != record_type
            && 3 != record_type)
        {
            if (4 <= verbose_level)
            {
                printf("Record with no data (S%u)\n", record_type);
            }
            continue;
        }
        const int address_length = (record_type + 1) * 2; // in symbols
        unsigned int address = ascii2hex(&line[4], address_length);
        const int data_length = ascii2hex(&line[2], 2) - address_length / 2 - 1; // in bytes
        const char *data_p = line + 4 + address_length;
        unsigned char *memory;

        if ((CODE_OFFSET + code_len) > (address + data_length))
        {
            if (NULL == code)
            {
                continue;
            }
            memory = (unsigned char*)code;
            address -= CODE_OFFSET;
            if (4 <= verbose_level)
            {
                printf("srec_code (%06X) : ", address);
            }
        }
        else if (DATA_OFFSET <= address
            && (DATA_OFFSET + data_len) > (address + data_length))
        {
            if (NULL == data)
            {
                continue;
            }
            memory = (unsigned char*)data;
            address -= DATA_OFFSET;
            if (4 <= verbose_level)
            {
                printf("srec_data (%06X) : ", address);
            }
        }
        else
        {
            rc = SREC_MEMORY_ERROR;
            break;
        }
        unsigned int i = data_length;
        for(; 0 < i; --i)
        {
            memory[address] = ascii2hex(data_p, 2);
            if (4 <= verbose_level)
            {
                printf("%02X ", memory[address]);
            }
            ++address;
            data_p += 2;
        }
        if (4 <= verbose_level)
        {
            printf("\n");
        }
    }
    fclose(pfile);
    return rc;
}
