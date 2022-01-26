#ifndef IPLIB_MAKER_DEFINES_HH
#define IPLIB_MAKER_DEFINES_HH

#include <string>
#include <list>
#include <map>

using namespace std;

#define DEFAULT_BLOCK_SIZE  (1024 * 8) // must be a multiple of 8

#define INDEX_BLOCK_LENGTH  12
#define TOP_BLOCK_LENGTH    12

struct index_block_t
{
    /* XXX: for IPv4 only, 4 bytes is enough for now */
    uint32_t start_ip;
    uint32_t end_ip;

    uint32_t offset;
    uint32_t data_len;

    index_block_t(uint32_t start_ip, uint32_t end_ip,
                  uint32_t offset, uint32_t data_len)
    {
        this->start_ip = start_ip;
        this->end_ip = end_ip;
        this->offset = offset;
        this->data_len = data_len;
    }
    index_block_t()
    {
        this->start_ip = this->end_ip = 0;
        this->offset = this->data_len = 0;
    }
    ~index_block_t()
    {
        this->start_ip = this->end_ip = 0;
        this->offset = this->data_len = 0;
    }
};

struct header_block_t
{
    /* index block start ip address */
    uint32_t idx_start_ip;
    /* index offset in the iplib file */
    uint32_t idx_offset;

    header_block_t(uint32_t start_ip, uint32_t offset)
    {
        this->idx_start_ip = start_ip;
        this->idx_offset = offset;
    }
    header_block_t()
    {
        this->idx_start_ip = this->idx_offset = 0;
    }
    ~header_block_t()
    {
        this->idx_start_ip = this->idx_offset = 0;
    }
};

struct data_block_t
{
	uint32_t offset; /* data offset in the iplib file */
	string data;

	data_block_t(string data, uint32_t offset)
	{
		this->data = data;
		this->offset = offset;
	}
    ~data_block_t()
	{
		this->offset = 0;
	}
};

/* 参考自:
 * https://github.com/lionsoul2014/ip2region/tree/master/maker/java
 *
 * iplib 数据结构:
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
struct iplib_maker
{
    /* block_size must be a multiple of 8 */
    iplib_maker(uint32_t block_size)
    {
        if ( (block_size % 8) != 0) {
            block_size = DEFAULT_BLOCK_SIZE;
        }
        this->index_block_size = block_size;
    }
    ~iplib_maker()
    {
        if (this->src_stream)
        {
            this->src_stream.close();
        }
        for (auto it = this->index_pool.begin();
             it != this->index_pool.end(); )
        {
            delete (*it); // delete index_block_t*
            this->index_pool.erase(it++);
        }
    }
    bool set_iplib_maker(const char *source);

    bool build_iplib(const char *output_file);

private:
    void write_data_blocks(FILE *iplib_w);
    // write single data block
    void write_data_block(uint32_t start_ip, uint32_t end_ip,
                          string data, FILE *iplib_w);

    /* max index data block size. must be a multiple of 8 */
    uint32_t index_block_size;

    ifstream src_stream; /* ip source file */

    list<index_block_t*>  index_pool;

    // used to determine if content is reusable
    map<string, uint32_t> data_offset_map;
};

#endif /* IPLIB_MAKER_DEFINES_HH */
