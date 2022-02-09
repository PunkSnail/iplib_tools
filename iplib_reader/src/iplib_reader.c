
#include "iplib_reader.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define INDEX_BLOCK_LENGTH 12
#define MAGIC_STRING "PUNK"

#ifdef __APPLE__ /* a workaround for missing pthread spinlocks on macOS */

typedef int32_t pthread_spinlock_t;

static int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    __asm__ __volatile__ ("" ::: "memory");
    *lock = 0;
    return 0;
}

static int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    return 0;
}

static int pthread_spin_lock(pthread_spinlock_t *lock)
{
    while (1)
    {
        for (int i = 0; i < 10000; i++)
        {
            if (__sync_bool_compare_and_swap(lock, 0, 1)) {
                return 0;
            }
        }
        sched_yield();
    }
}

static int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    __asm__ __volatile__ ("" ::: "memory");
    *lock = 0;
    return 0;
}
#endif /* __APPLE__ */

/* struct for reading the IP library */
struct iplib_reader {
    char *db_mem;   //db binary string for memory search mode
	uint32_t index_start;	// index area start offset
	uint32_t total_blocks;	// total index blocks number
    bool is_punk_lib; // the punk iplib starts with "PUNK"
    char describe_cache[MAX_DESCRIBE_LENGTH];
    pthread_spinlock_t spin_lock;
};

// refer: https://github.com/lionsoul2014/ip2region

/* PUNK iplib 数据结构:
 * iplib 数据结构:
 *  所有长度偏移等整数信息均为小端序(Little-Endian)
 *
 * 1. 文件头:
 * +------------+------------+------------+------------+
 * | 4 bytes    | 4 bytes    | 4 bytes    | 4 bytes    |
 * +------------+------------+------------+------------+
 *    "PUNK"    | 索引区开始 | 索引区结束 | 保留字段
 *
 * 2. 数据区: 仅当数据长度 >= 255 字节时, 数据的前两字节才储存长度
 * +------------+-----------------------+
 * | 2 bytes    | describe string       | ...
 * +------------+-----------------------+
 *  数据长度    | 描述字符串
 *
 * 3. 数据索引区:
 * +------------+-----------+--------+---------+
 * | 4 bytes    | 4 bytes   | 1 byte | 3 bytes | ...
 * +------------+-----------+--------+---------+
 *  start ip    |  end ip   |数据长度| 数据偏移
 *  */
static bool load_db_to_mem(iplib_reader_t *p_reader, FILE *db_fd)
{
    uint32_t index_end = 0;
    if (p_reader->db_mem != NULL) {
        return true;
    }
    //get the size of the file
    fseek(db_fd, 0, SEEK_END);
    uint32_t filesize = (uint32_t)ftell(db_fd);
    fseek(db_fd, 0, SEEK_SET);

    p_reader->db_mem = (char*)malloc(filesize);
    if (p_reader->db_mem == NULL ) {
        return false;
    }
    if (fread(p_reader->db_mem, filesize, 1, db_fd) != 1) {
        return false;
    }
    char *buffer = p_reader->db_mem;
    if (!strncmp(buffer, MAGIC_STRING, strlen(MAGIC_STRING)))
    {
        p_reader->is_punk_lib = true;
        buffer += strlen(MAGIC_STRING);
    } else {
        p_reader->is_punk_lib = false;
    }
    memcpy(&p_reader->index_start, buffer, sizeof(uint32_t));
    memcpy(&index_end, buffer + 4, sizeof(uint32_t));

    p_reader->total_blocks =
        (index_end - p_reader->index_start)
        / INDEX_BLOCK_LENGTH + 1;

    return true;
}

/* create a new iplib_reader_t instance */
iplib_reader_t *iplib_reader_create(const char *db_file_path)
{
    iplib_reader_t *p_reader = (iplib_reader_t*)
        calloc(1, sizeof(iplib_reader_t));

    pthread_spin_init(&p_reader->spin_lock, 0);
    // open the db file
    FILE *db_fd = fopen(db_file_path, "rb");
    if (db_fd == NULL)
    {
        fprintf(stderr, "failed to open the db file %s\n",
                db_file_path);
        goto create_error_end;
    }
    if (!load_db_to_mem(p_reader, db_fd))
    {
        fprintf(stderr, "failed to load the db file to memory\n");
        goto create_error_end;
    }
    fclose(db_fd);
    return p_reader;

create_error_end:
    if (db_fd) {
        fclose(db_fd);
    }
    iplib_reader_destroy(p_reader);
    return NULL;
}

static inline uint32_t
get_data_offset(iplib_reader_t *p_reader, uint32_t ip)
{
    char *p_data = NULL;
    uint32_t offset = 0, blocks = p_reader->total_blocks;

    for (uint32_t curr, i = 0; i <= blocks; )
    {
        curr = (i + blocks) >> 1;
        p_data = p_reader->db_mem +
            p_reader->index_start + curr * INDEX_BLOCK_LENGTH;

        if (ip < *((uint32_t*)p_data))
        {   // less then start
            blocks = curr - 1;
            continue;
        }
        if (ip > *((uint32_t*)(p_data + 4)))
        {
            i = curr + 1;
            continue;
        }
        offset = *((uint32_t*)(p_data + 8));
        break;
    }
    return offset;
}

/* return: C string describe
 * refer:
 * https://github.com/lionsoul2014/ip2region/tree/master/binding/c */
const char *iplib_reader_search(iplib_reader_t *p_reader, uint32_t ip,
                                char *output, uint32_t output_len)
{
    const char *result = "empty";

    uint32_t offset = get_data_offset(p_reader, ip);
    if (0 == offset) {
        return result;
    }
    char *p_data = p_reader->db_mem + (offset & 0x00FFFFFF);
    uint16_t data_len = (uint8_t)((offset >> 24) & 0xFF);

    if (false == p_reader->is_punk_lib && data_len > 4)
    {
        data_len = (uint16_t)(data_len - 4);
        p_data += 4; /* skip the int32_t id field */
    }
    if (p_reader->is_punk_lib && data_len == 0xFF)
    {
        memcpy(&data_len, p_data, sizeof(uint16_t));
        data_len = (uint16_t)(data_len - 2);
        p_data += 2; /* skip length field */
    }
    if (output && data_len < output_len)
    {
        memcpy(output, p_data, data_len);
        output[data_len] = '\0';
        result = output;
    }
    else if (data_len < MAX_DESCRIBE_LENGTH)
    {   // output is NULL, the internal describe_cache will be used
        strncpy(p_reader->describe_cache, p_data, data_len);
        p_reader->describe_cache[data_len] = '\0';
        result = p_reader->describe_cache;
    }
    return result;
}

const char *iplib_reader_search_s(iplib_reader_t *p_reader, uint32_t ip)
{
    const char *result = NULL;
    pthread_spin_lock(&p_reader->spin_lock);

    result = iplib_reader_search(p_reader, ip, NULL, 0);

    pthread_spin_unlock(&p_reader->spin_lock);

    return result;
}

/* destroy the specifield iplib_reader object */
void iplib_reader_destroy(iplib_reader_t *p_reader)
{
    if (p_reader == NULL) {
        return;
    }
    pthread_spin_destroy(&p_reader->spin_lock);
    // free the db binary string
    if (p_reader->db_mem != NULL)
    {
        free(p_reader->db_mem);
        p_reader->db_mem = NULL;
    }
    free(p_reader);
}

