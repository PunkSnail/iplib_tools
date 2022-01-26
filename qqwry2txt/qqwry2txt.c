#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <getopt.h> /* getopt_long */
#include <iconv.h>
#include <time.h>

#define MAX_LINE_SIZE 1024 // cache of single line

static char g_src_line[MAX_LINE_SIZE] = { 0 };
static char g_dst_line[MAX_LINE_SIZE] = { 0 };
static char g_line_cache[MAX_LINE_SIZE] = { 0 };

void show_help(void)
{
    printf("Options:\n"
           "-f <file>       纯真 IP 库文件位置\n"
           "-o <file>       解析后的输出位置, 默认 \"./output.txt\"\n"
           "-h, --help      Display this information\n");
}

static void gbk_to_utf8(const char* src, char* dst, uint32_t len)
{
    size_t inlen = strlen(src) + 1;
    size_t outlen = len;

    char* inbuf = g_line_cache;
    memcpy(inbuf, src, len);

    iconv_t ret = iconv_open("UTF-8", "GBK");
    if (ret != (iconv_t)-1) {
        iconv(ret, &inbuf, &inlen, &dst, &outlen);
        iconv_close(ret);
    }
}

static void write_c_string(FILE *in_stream, FILE *out_stream)
{
    for (uint32_t i = 0; i < MAX_LINE_SIZE; i++)
    {
        if ((g_src_line[i] = (char)fgetc(in_stream)) == 0)
        {
            gbk_to_utf8(g_src_line, g_dst_line, MAX_LINE_SIZE - 1);
            fprintf(out_stream, "|%s", g_dst_line);
            return; // normal case
        }
    }
    printf("[ERROR] The cache is not big enough\n");
    exit(0);
}

static void recursion_parse(FILE *in_stream, FILE *out_stream, int flag)
{
    uint8_t mod;
    uint32_t offset = 0;
    uint32_t pre_offset = 0;

    mod = (uint8_t)fgetc(in_stream);
    if (mod > 2)
    {
        fseek(in_stream, -1L, SEEK_CUR);
        write_c_string(in_stream, out_stream);
        if (flag == 0)
        {
            recursion_parse(in_stream, out_stream, 1);
        }
        return;
    }
    fread(&offset, 3, 1, in_stream);
    if (mod == 1)
    {
        fseek(in_stream, offset, SEEK_SET);
        recursion_parse(in_stream, out_stream, flag);
        return;
    }
    pre_offset = (uint32_t)ftell(in_stream);
    fseek(in_stream, offset, SEEK_SET);
    recursion_parse(in_stream, out_stream, 1);
    if (flag == 0)
    {
        fseek(in_stream, pre_offset, SEEK_SET);
        recursion_parse(in_stream, out_stream, 1);
    }
}

static void parse_record(FILE *in_stream,
                         FILE *out_stream, uint32_t offset)
{
    uint32_t ip = 0; // end of ip range

    fseek(in_stream, offset, SEEK_SET);
    fread(&ip, 4, 1, in_stream);
    // write end of ip range
    fprintf(out_stream, "%u.%u.%u.%u", (ip >> 24) & 0xFF,
             (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

    recursion_parse(in_stream, out_stream, 0);
    fputs("\n", out_stream);
}

static void parse_qqwry_dat(FILE *in_stream, FILE *out_stream)
{
    uint32_t start_offset, end_offset;
    uint32_t record_offset = 0, ip = 0;

    fseek(in_stream, 0L, SEEK_SET);
    fread(&start_offset, 4, 1, in_stream);
    fread(&end_offset, 4, 1, in_stream);

    for (uint32_t i = start_offset; i <= end_offset; )
    {
        fseek(in_stream, i, SEEK_SET);
        fread(&ip, 4, 1, in_stream);
        fread(&record_offset, 3, 1, in_stream);
        i = (uint32_t)ftell(in_stream);
        // write start of ip range
        fprintf(out_stream, "%u.%u.%u.%u|", (ip >> 24) & 0xFF,
             (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

        parse_record(in_stream, out_stream, record_offset);
    }
}

static int run_trans(const char *dat_path, const char *out_path)
{
    int result = -1;
    FILE *out_stream = fopen(out_path, "wb");
    if (out_stream == NULL)
    {
        printf("open '%s' failed %s\n", out_path, strerror(errno));
        return result;
    }
    FILE *in_stream = fopen(dat_path, "rb");
    if (in_stream == NULL)
    {
        printf("open '%s' failed %s\n", dat_path, strerror(errno));
        goto trans_end;
    }
    parse_qqwry_dat(in_stream, out_stream);

    result = 0;
    fclose(in_stream);
trans_end:
    fclose(out_stream);
    return result;
}

int main(int argc, char *argv[])
{
    int opt;
    const char *dat_path = NULL;
    const char *out_path = "./output.txt";

    static struct option ops[] = {
        { "file", required_argument, NULL, 'f' },
        { "output", required_argument, NULL, 'o' },
        { "help", no_argument, NULL, 'h' }
    };
    while ((opt = getopt_long(argc, argv, "f:o:h", ops, NULL)) != -1)
    {
        switch(opt)
        {
        case 'f':
            dat_path = optarg;
            break;
        case 'o':
            out_path = optarg;
            break;
        case 'h':
            show_help();
            return 0;
        default:
            fprintf(stderr, "Try: %s --help\n", argv[0]);
            return -1;
        }
    }
    if (NULL == dat_path) {
        fprintf(stderr, "未指定纯真 IP 库位置\n");
        show_help();
        return -1;
    }
    clock_t start = clock();

    run_trans(dat_path, out_path);

    printf("\n %lfs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
}
