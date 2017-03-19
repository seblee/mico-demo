#include "httpd.h"

#ifdef EMW3165
//#define USE_FILESYS
#endif

#ifndef USE_FILESYS
void datafs_init( void );
#endif

void http_ota_register_handlers();
void http_sys_register_file_handlers(void *app_context_c);

int web_set_update(httpd_request_t *req);

int http_server_fs_start( void *app_context_c );

uint8_t fatfs_sys_creat( void );
uint8_t open_file(const char *name, uint8_t file_op);
unsigned int get_file_size( void );
uint8_t read_file(char *rtext, size_t buf_len, uint32_t *byteread, int *is_offset);
uint8_t write_file(const char *wtext, size_t inLen);
uint8_t close_file( void );
