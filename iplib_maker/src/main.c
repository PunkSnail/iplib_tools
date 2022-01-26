#include <stdio.h>
#include <time.h>
#include <getopt.h> /* getopt_long */

#include "iplib_maker.h"

static void show_help(void)
{
    printf("Options:\n"
           "-f <file>\tSource text file path\n"
           "-o <file>\tWrite output to <file>; default \"./iplib.db\"\n"
           "-h, --help\tDisplay this information\n");
}

int main(int argc, char *argv[])
{
    const char *source_file = NULL;
    const char *output_file = "./iplib.db";
    int opt;

    static struct option ops[] = {
        { "source", required_argument, NULL, 'f' },
        { "output", required_argument, NULL, 'o' },
        { "help", no_argument, NULL, 'h' }
    };
    while ((opt = getopt_long(argc, argv, "f:o:h", ops, NULL)) != -1)
    {
        switch(opt)
        {
        case 'f':
            source_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        case 'h':
            show_help();
            return 0;
        default:
            fprintf(stderr, "Try: %s --help\n", argv[0]);
            return -1;
        }
    }
    if (NULL == source_file)
    {
        fprintf(stderr,
                "Missing target files, try: %s --help\n", argv[0]);
        return -1;
    }
    iplib_maker_t *p_maker = create_iplib_maker();
    if (NULL == p_maker)
    {
        fprintf(stderr, "create iplib maker failed\n");
        return -1;
    }
    clock_t start = clock();
    run_iplib_maker(p_maker, source_file, output_file);

    printf("\n %lfs\n", (double)(clock() - start) / CLOCKS_PER_SEC);

    destroy_iplib_maker(p_maker);
    p_maker = NULL;
    
    return 0;
}

