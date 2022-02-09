#ifndef IPLIB_READER_HH
#define IPLIB_READER_HH

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_DESCRIBE_LENGTH 65535

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iplib_reader iplib_reader_t;

/* create a new iplib_reader_t instance */
iplib_reader_t *iplib_reader_create(const char *db_file_path);

/* get the describe with the specified ip address. thread-unsafe
 * the return value will be overwritten on the next call.
 *
 * NOTE: when the parameter 'output' is NULL, internal 'describe_cache'
 * will be used, if 'output' is available, its length shouldn't be less
 * then MAX_DESCRIBE_LENGTH
 *
 * return: C string describe */
const char *iplib_reader_search(iplib_reader_t *p_reader, uint32_t ip,
                                char *output, uint32_t output_len);

/* get the describe with the specified ip address. thread-safe
 * the return value will be overwritten on the next call.
 *
 * return: C string describe */
const char *iplib_reader_search_s(iplib_reader_t *p_reader, uint32_t ip);

/* destroy the specifield iplib_reader object */
void iplib_reader_destroy(iplib_reader_t *p_reader);

#ifdef __cplusplus
}
#endif

#endif	/* IPLIB_READER_HH */
