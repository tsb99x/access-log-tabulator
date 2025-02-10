/*

Author: Anton Muravev
Homepage: https://tsb99x.ru
Filename: access-log-tabulator.c
Version: 2
Release Date: 2024-09-03T20:11:41+03:00
Last Update: 2025-02-10T21:07:56+03:00
License: Public Domain <https://unlicense.org>, see at the end of file.
Third-Party Dependencies: none

Compiled successfully with:

        GCC 12
        $ gcc -O2 -std=c90 \
                -Wall -Wextra -Wpedantic \
                -o access-log-tabulator{,.c}

        Clang 14
        $ clang -O2 -std=c90 \
                -Weverything \
                -o access-log-tabulator{,.c}

        MSVC 2010
        > cl.exe /O2 /W4 /Za access-log-tabulator.c

This program will convert logs from Apache format to Tab-Separated Values (TSV)
format, dropping all quoting and additionally converting time field into ISO
8601-like format, which can help with an import into DB and logs sorting by
time.

No sorting is included, this is a line-by-line converter.

Total line length is limited to 4095 symbols, including newline char.

Use like this:

        $ zcat -f *.access.log* \
                | ./access_log_tabulator \
                | (sed -u 1q; sort) \
                > sorted.tsv

Or tail some access log in real-time:

        $ tail -f access.log \
                | ./access_log_tabulator \
                > unsorted.tsv

An explanation of Common and Combined Log Formats is available at:
- https://httpd.apache.org/docs/current/logs.html

*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *err_too_many_args = /**/
    "ERR_TOO_MANY_ARGS";
static const char *err_line_is_too_long = /**/
    "ERR_LINE_IS_TOO_LONG";
static const char *err_wrong_line_format = /**/
    "ERR_WRONG_LINE_FORMAT";
static const char *err_input_read_error = /**/
    "ERR_INPUT_READ_ERROR";
static const char *err_wrong_time_format = /**/
    "ERR_WRONG_TIME_FORMAT";
static const char *err_time_buffer_size_exceeded = /**/
    "ERR_TIME_BUFFER_SIZE_EXCEEDED";
static const char *err_failed_to_parse_month = /**/
    "ERR_FAILED_TO_PARSE_MONTH";
static const char *err_failed_to_parse_apache_datetime = /**/
    "ERR_FAILED_TO_PARSE_APACHE_DATETIME";

static void error(const char *m)
{
        fprintf(stderr, "Error: %s\n", m);
        exit(EXIT_FAILURE);
}

static const char *print_non_spaces(const char *s)
{
        for (; !isspace(*s) && *s != '\0'; s++) {
                putchar(*s);
        }
        return s;
}

static const char *print_enclosed(const char *s, const char op, const char end)
{
        if (*s != op) {
                error(err_wrong_line_format);
        }
        s++;
        for (; *s != end && *s != '\0'; s++) {
                putchar(*s);
        }
        if (*s != end) {
                error(err_wrong_line_format);
        }
        s++;
        return s;
}

static const char *skip_spaces(const char *s)
{
        for (; isspace(*s) && *s != '\0'; s++)
                ;
        return s;
}

static int parse_month(const char m[4])
{
        static const char *const months[] = {
            "Jan",
            "Feb",
            "Mar",
            "Apr",
            "May",
            "Jun",
            "Jul",
            "Aug",
            "Sep",
            "Oct",
            "Nov",
            "Dec",
        };

        int i;

        for (i = 0; i < 12; i++) {
                if (!strcmp(months[i], m)) {
                        return i;
                }
        }
        error(err_failed_to_parse_month);
        return -1;
}

static const char *
parse_apache_datetime(const char *s, struct tm *time, int *gmt_offset)
{
        char mon_buf[4] = {0};
        int chars_read = 0;
        int args_read = sscanf(s,
                               "%2d/%3s/%4d:%2d:%2d:%2d %5d%n",
                               &time->tm_mday,
                               mon_buf,
                               &time->tm_year,
                               &time->tm_hour,
                               &time->tm_min,
                               &time->tm_sec,
                               gmt_offset,
                               &chars_read);

        if (args_read != 7) {
                error(err_failed_to_parse_apache_datetime);
        }
        time->tm_year -= 1900;
        time->tm_mon = parse_month(mon_buf);
        return s + chars_read;
}

static const char *print_timestamp_as_iso(const char *s)
{
        static const char *fmt_iso = "%Y-%m-%dT%H:%M:%S";

        char dt_buf[32] = {0};
        struct tm time = {0};
        int gmt_offset = 0;

        if (*s != '[') {
                error(err_wrong_line_format);
        }
        s++;
        s = parse_apache_datetime(s, &time, &gmt_offset);
        if (!s) {
                error(err_wrong_time_format);
        }
        if (*s != ']') {
                error(err_wrong_line_format);
        }
        s++;
        if (!strftime(dt_buf, sizeof(dt_buf), fmt_iso, &time)) {
                error(err_time_buffer_size_exceeded);
        }
        printf("%s", dt_buf);
        if (gmt_offset >= 0) {
                printf("+%04d", gmt_offset);
        } else {
                printf("%05d", gmt_offset);
        }

        return s;
}

int main(int argc, char *argv[])
{
        char in_buf[4096] = {0};
        const char *s = NULL;

        (void)**argv;

        if (argc > 1) {
                error(err_too_many_args);
        }

        printf("host\t"
               "identity\t"
               "user\t"
               "time\t"
               "request\t"
               "status\t"
               "bytes\t"
               "referrer\t"
               "agent\n");

        while (fgets(in_buf, sizeof(in_buf), stdin)) {
                if (!memchr(in_buf, '\n', sizeof(in_buf))) {
                        error(err_line_is_too_long);
                }

                s = in_buf;
                if (*s == '\n') {
                        putchar('\n');
                        continue;
                }

                /* Common Log Format fields from Apache*/

                /* (%h) host */
                s = print_non_spaces(s);
                s = skip_spaces(s);
                putchar('\t');

                /* (%l) identity */
                s = print_non_spaces(s);
                s = skip_spaces(s);
                putchar('\t');

                /* (%u) user */
                s = print_non_spaces(s);
                s = skip_spaces(s);
                putchar('\t');

                /* (%t) time */
                s = print_timestamp_as_iso(s);
                s = skip_spaces(s);
                putchar('\t');

                /* ("%r") request line */
                s = print_enclosed(s, '"', '"');
                s = skip_spaces(s);
                putchar('\t');

                /* (%s) status code */
                s = print_non_spaces(s);
                s = skip_spaces(s);
                putchar('\t');

                /* (%b) bytes sent */
                s = print_non_spaces(s);
                s = skip_spaces(s);
                putchar('\t');

                /* additional fields in Apache Combined Log Format */

                /* ("%{Referrer}i") referrer */
                s = print_enclosed(s, '"', '"');
                s = skip_spaces(s);
                putchar('\t');

                /* ("%{User-agent}i") user-agent */
                s = print_enclosed(s, '"', '"');
                if (*s != '\n') {
                        error(err_wrong_line_format);
                }
                putchar('\n');
        }

        if (!feof(stdin)) {
                error(err_input_read_error);
        }

        return EXIT_SUCCESS;
}

/*

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

*/
