
#include <string.h>
#include <errno.h>
#include <arpa/inet.h> /* inet_pton */
#include <sys/time.h> /* gettimeofday */

#include <iostream>
#include <fstream>

#include "iplib_maker.h"
#include "iplib_maker_defines.h"

using namespace std;

#define HEADER_BLOCK_LENGTH 8 /* 4 bytes start ip + 4 bytes offset */

#define debug_print(...) \
{ \
    printf(__VA_ARGS__); \
}

bool iplib_maker::set_iplib_maker(const char *source)
{
    bool result = false;
    this->src_stream.open(source);
    if (!this->src_stream)
    {
        debug_print("open '%s' failed %s\n", source, strerror(errno));
        return result;
    }
    result = true;
    return result;
}

static FILE *open_iplib(const char *output, const uint32_t header_size)
{
    FILE *iplib_w = fopen(output, "w+");
    if (NULL == iplib_w)
    {
        return NULL;
    }
    fseek(iplib_w, 0, SEEK_SET);
    // leave space for the header
    fseek(iplib_w, header_size, SEEK_SET);
    return iplib_w;
}

static inline bool ip_to_uint(const char *ip_str, uint32_t *p_output)
{
    struct in_addr addr;

    if (ip_str && p_output)
    {
        if (inet_pton(AF_INET, ip_str, (void*)&addr) == 1)
        {
            *p_output = ntohl(addr.s_addr);
            return true;
        }
    }
    return false;
}

static bool iplib_line_parser(string line, uint32_t *p_start,
                              uint32_t *p_end, string *p_data)
{
    for (size_t i = 0, pos = 0; i < 3 &&
         ((pos = line.find("|")) != string::npos); i++)
    {
        switch (i)
        {
        case 0:
            if (!ip_to_uint(line.substr(0, pos).c_str(), p_start))
            {
                return false;
            }
            break;
        case 1:
            if (!ip_to_uint(line.substr(0, pos).c_str(), p_end))
            {
                return false;
            }
            break;
        case 2:
            *p_data = line;
        default:
            break;
        }
        line.erase(0, pos + 1); // skip the "|"
    }
    uint32_t len = (uint32_t)p_data->length();
    if (*p_start > *p_end || 0 == *p_end || 0 == len || len > 0xFFFD)
    {
        return false;
    }
    return true;
}

void iplib_maker::write_data_block(uint32_t start_ip, uint32_t end_ip,
                                   string data, FILE *iplib_w)
{
    uint32_t data_offset = 0;
    uint16_t data_len = (uint16_t)data.length();
    /* if length is greater than or equal to 0xFF, the length will be
     * written to the data header.
     * 如果长度大于等于 0xFF, 则长度将被写入数据头 */
    data_len = (uint16_t)(data_len >= 0xFF ? data_len + 2 : data_len);

    map<string, uint32_t>::iterator it = this->data_offset_map.find(data);

    if (it != this->data_offset_map.end()) // found it
    {
        data_offset = it->second;
    }
    else // add new data_block_t
    {
        data_offset = (uint32_t)ftell(iplib_w);
        if (data_len >= 0xFF)
        {
            fwrite(&data_len, sizeof(char), 2, iplib_w);
        }
        fwrite(data.c_str(), sizeof(char), data.length(), iplib_w);

        this->data_offset_map.insert(pair<string, uint32_t>
                                     (data, data_offset));
    }
    this->index_pool.push_back(new index_block_t(start_ip, end_ip,
                                                 data_offset, data_len));
}

static inline string & trim_str_space(string &s)
{
    if (s.empty())
    {
        return s;
    }
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

void iplib_maker::write_data_blocks(FILE *iplib_w)
{
    string line, data;
    uint32_t start_ip, end_ip;

    while (getline(this->src_stream, line))
    {
        line = trim_str_space(line);
        if (line.length() == 0 || line.c_str()[0] == '#')
        {
            continue; // invalid line, skip
        }
        start_ip = end_ip = 0;
        data.clear();

        if (!iplib_line_parser(line, &start_ip, &end_ip, &data))
        {
            debug_print("skip invalid line '%s'\n", line.c_str());
            continue;
        }
        this->write_data_block(start_ip, end_ip, data, iplib_w);
    }
}

static void
write_index_blocks(list<index_block_t*> &index_pool,
                   FILE *iplib_w)
{
    uint8_t block_buf[INDEX_BLOCK_LENGTH] = { 0 };

    for (auto it = index_pool.begin(); it != index_pool.end(); ++it)
    {
        uint32_t mix = ((*it)->data_len < 0xFF) ?
            (*it)->offset | (((*it)->data_len << 24) & 0xFF000000) :
            (*it)->offset | 0xFF000000;

        memcpy(block_buf + 0, &(*it)->start_ip, sizeof(uint32_t));
        memcpy(block_buf + 4, &(*it)->end_ip, sizeof(uint32_t));
        memcpy(block_buf + 8, &mix, sizeof(uint32_t));

        fwrite(block_buf, sizeof(char), INDEX_BLOCK_LENGTH, iplib_w);
    }
}

static void
write_header_blocks(uint32_t start, FILE *iplib_w)
{
    uint32_t curr = (uint32_t)ftell(iplib_w);
    fseek(iplib_w, 0, SEEK_SET); // set the file pointer to the header

    char top_block[TOP_BLOCK_LENGTH] ={ 0 };
    uint32_t end = curr - INDEX_BLOCK_LENGTH;

    memcpy(&top_block[0], "PUNK", 4);
    memcpy(&top_block[4], &start, sizeof(uint32_t));
    memcpy(&top_block[8], &end, sizeof(uint32_t));

    fwrite(top_block, sizeof(char), TOP_BLOCK_LENGTH, iplib_w);

    fseek(iplib_w , 0, SEEK_END); // reset now
}

/* 计算固定头加上预留的头部跳表空间长度
 * 考虑到索引区单个元素大小固定, 且有序, 跳表意义不大, 所以目前不使用 */
uint32_t unuse_calc_header_size(ifstream *p_stream, uint32_t limits)
{
    string line, data;
    uint32_t line_count = 0;

    while (getline(*p_stream, line))
    {
        uint32_t start_ip = 0, end_ip = 0;
        line = trim_str_space(line);
        if (line.length() == 0 || line.c_str()[0] == '#')
        {
            continue; // skip invalid line
        }
        data.clear();
        if (iplib_line_parser(line, &start_ip, &end_ip, &data))
        {
            line_count++; // valid line
        }
    }
    /* reset the stream */
    p_stream->clear();
    p_stream->seekg(0, ios::beg);

    uint32_t blocks = ((line_count/limits) + 2); // + 2 for start and end
    /* top block + header block size */
    return TOP_BLOCK_LENGTH + (blocks * HEADER_BLOCK_LENGTH);
}

bool iplib_maker::build_iplib(const char *output)
{
    bool result = false;

    FILE *iplib_w =
        open_iplib(output, TOP_BLOCK_LENGTH);

    if (NULL == iplib_w)
    {
        debug_print("open '%s' failed %s\n", output, strerror(errno));
        return result;
    }
    this->write_data_blocks(iplib_w);
    /* the following operation will modify the position of the pointer,
     * so record it here */
    uint32_t record_offset = (uint32_t)ftell(iplib_w);

    write_index_blocks(this->index_pool, iplib_w);
    // write the header blocks
    write_header_blocks(record_offset, iplib_w);

    fclose(iplib_w);
    debug_print("done.\n");

    result = true;
    return result;
}

iplib_maker_t *create_iplib_maker(void)
{
    return new iplib_maker(DEFAULT_BLOCK_SIZE);
}

bool run_iplib_maker(iplib_maker *p_maker,
                     const char *source_file, const char *output_file)
{
    bool result = false;

    if (false == p_maker->set_iplib_maker(source_file))
    {
        debug_print("set database maker failed\n");
        return result;
    }
    result = p_maker->build_iplib(output_file);

    if (false == result)
    {
        debug_print("build ip database library failed\n");
    }
    return result;
}

void destroy_iplib_maker(iplib_maker_t *p_maker)
{
    delete p_maker;
}
