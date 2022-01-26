#ifndef IPLIB_MAKER_HH
#define IPLIB_MAKER_HH

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iplib_maker iplib_maker_t;

iplib_maker_t *create_iplib_maker(void);

bool run_iplib_maker(iplib_maker_t *p_maker,
                     const char *source_file, const char *output_file);

void destroy_iplib_maker(iplib_maker_t *p_maker);

#ifdef __cplusplus
}
#endif


#endif /* IPLIB_MAKER_HH */
