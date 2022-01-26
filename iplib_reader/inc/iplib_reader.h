#ifndef IPLIB_READER_HH
#define IPLIB_READER_HH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_DESCRIBE_LENGTH 65535

/* struct for reading the IP library
 * refer: https://github.com/lionsoul2014/ip2region */
typedef struct {
    char *db_mem;   //db binary string for memory search mode
	uint32_t index_start;	// index area start offset
	uint32_t total_blocks;	// total index blocks number
    bool is_punk_lib; // the punk iplib starts with "PUNK"
    char describe_cache[MAX_DESCRIBE_LENGTH];
} iplib_reader_t;

/* create a new iplib_reader_t instance */
iplib_reader_t *iplib_reader_create(const char *db_file_path);

/* get the describe with the specified ip address. thread-unsafe
 *
 * NOTE: when the parameter 'output' is NULL, internal 'describe_cache'
 * will be used, if 'output' is available, its length shouldn't be less
 * then MAX_DESCRIBE_LENGTH
 *
 * return: C string describe */
const char *iplib_reader_search(iplib_reader_t *p_reader, uint32_t ip,
                                char *output, uint32_t output_len);

/* return: C string describe */
const char *iplib_reader_search_s(iplib_reader_t *p_reader, uint32_t ip);

/* destroy the specifield iplib_reader object */
void iplib_reader_destroy(iplib_reader_t *p_reader);

#endif	/* IPLIB_READER_HH */
