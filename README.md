Author: Anton Muravev

Homepage: https://tsb99x.ru

Filename: access-log-tabulator.c

Version: 2

Release Date: 2024-09-03T20:11:41+03:00

Last Update: 2025-02-10T21:07:56+03:00

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
