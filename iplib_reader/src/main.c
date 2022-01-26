
#include "iplib_reader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h> /* inet_pton */
#include <getopt.h> /* getopt_long */

static void show_help(void)
{
    printf("Options:\n"
           "-f <file>       IP library file path\n"
           "-s <ip>         The IP address to search\n"
           "-h, --help      Display this information\n");
}

static uint32_t ip_to_uint(const char *ip_str)
{
    struct in_addr addr;

    if (NULL == ip_str) {
        return 0;
    }
    if (inet_pton(AF_INET, ip_str, (void*)&addr) != 1)
    {
        printf("[ERROR] invalid ip '%s' %s\n",
               ip_str, errno ? strerror(errno) : "");
        return 0;
    }
    return ntohl(addr.s_addr);
}

static double get_time()
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);

    return (double)(tv.tv_sec * 1000) + ((double)tv.tv_usec)/1000;
}

int main(int argc, char *argv[])
{
    const char *db_file_path = NULL;
    const char *ip_str = NULL;
    int opt;

    static struct option ops[] = {
        { "file", required_argument, NULL, 'f' },
        { "search", required_argument, NULL, 's' },
        { "help", no_argument, NULL, 'h' }
    };
    while ((opt = getopt_long(argc, argv, "f:s:h", ops, NULL)) != -1)
    {
        switch(opt)
        {
        case 'f':
            db_file_path = optarg;
            break;
        case 's':
            ip_str = optarg;
            break;
        case 'h':
            show_help();
            return 0;
        default:
            fprintf(stderr, "Try: %s --help\n", argv[0]);
            return -1;
        }
    }
    if (NULL == ip_str || NULL == db_file_path)
    {
        show_help();
        return -1;
    }
    iplib_reader_t *p_reader = iplib_reader_create(db_file_path);
    if (p_reader == NULL)
    {
        printf("[ERROR] Failed to create the iplib_reader object\n");
        return -1;
    }
    double start = get_time();
    const char *describe =
        iplib_reader_search_s(p_reader, ip_to_uint(ip_str));

    printf("'%s' %.5f ms\n", describe, get_time() - start);

    iplib_reader_destroy(p_reader);

    return 0;
}
