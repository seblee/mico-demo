#include <httpd.h>
#include <http_parse.h>
#include <http-strings.h>

#include "MICO.h"
#include "httpd_priv.h"
#include "StringUtils.h"
#include "CheckSumUtils.h"

#include "libmicroplatform.h"
#include "microhttpd.h"
#include "internal.h"
#include "http_server/http_server.h"

#define http_ota_log(M, ...) custom_log("http ota", M, ##__VA_ARGS__)

#define HTTPD_SERVER_HDR_DEFORT (HTTPD_HDR_ADD_SERVER|HTTPD_HDR_ADD_CONN_KEEP_ALIVE)
#define rec_ota_buf_size 1024*2 

MHD_PanicCallback mhd_panic;
void *mhd_panic_cls;

static CRC16_Context crc16_context;
static uint16_t crc;
static uint32_t offset = 0;
static bool is_ota_success = false;
static bool is_web_success = false;
static bool is_format = false;
static mico_logic_partition_t* ota_partition;
  
static char is_file[13];

static mico_Context_t *mico_context;
static int g_ota_handlers_no;
static struct httpd_wsgi_call g_ota_handlers[];


static const unsigned char ota_html[ ] = {
"<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<head>\r\n\
    <meta charset=\"UTF-8\">\r\n\
    <title>ota</title>\r\n\
    <script type=\"text/javascript\">\r\n\
        function check(v)\r\n\
        {\r\n\
            if(v == 1){\r\n\
                var f=document.form_fw;\r\n\
                if(f.ota_fw.value==\"\")\r\n\
                {\r\n\
                    alert(\"Please select a file\");\r\n\
                    return;\r\n\
                }\r\n\
            }else if(v == 2){\r\n\
                var f=document.form_web;\r\n\
                if(f.ota_web.value==\"\")\r\n\
                {\r\n\
                    alert(\"Please select a file\");\r\n\
                    return;\r\n\
                }\r\n\
            }\r\n\
            document.getElementById(\"up_before\").style.visibility=\"hidden\";\r\n\
            document.getElementById(\"up_before\").style.display=\"\";\r\n\
            document.getElementById(\"up_after\").style.visibility=\"visible\";\r\n\
            document.getElementById(\"up_after\").style.display=\"\";\r\n\
            f.submit();\r\n\
        }\r\n\
    </script>\r\n\
    <style>\r\n\
        body{\r\n\
            font-family: Arial;\r\n\
            font-size: large;\r\n\
        }\r\n\
        .center{\r\n\
            margin: auto;\r\n\
            margin-top: 100px;\r\n\
            width: 350px;\r\n\
            height: 100px;\r\n\
        }\r\n\
    </style>\r\n\
</head>\r\n\
<body>\r\n\
    <div class=\"center\" style=\"display: none\" id=\"up_after\">\r\n\
        <p>Update</p>\r\n\
        <p>File uploading,please waiting...</p>\r\n\
    </div>\r\n\
    <div class=\"center\" id=\"up_before\">\r\n\
        <p>Firmare Update</p>\r\n\
        <form name=\"form_fw\" method=\"post\" action=\"otares.html\" enctype=\"multipart/form-data\">\r\n\
            <input name=\"ota_fw\" type=\"file\" value=\"\">\r\n\
            <input onclick=\"check(1)\" type=\"button\" value=\"start\">\r\n\
        </form>\r\n\
        <br><br>\r\n\
        <p>Web Update</p>\r\n\
        <form name=\"form_web\" method=\"post\" action=\"otares.html\" enctype=\"multipart/form-data\">\r\n\
            <input name=\"ota_web\" type=\"file\" value=\"\" multiple>\r\n\
            <input onclick=\"check(2)\" type=\"button\" value=\"start\">\r\n\
        </form>\r\n\
    </div>\r\n\
</body>\r\n\
</html>"
};

static const unsigned char ota_success_html[] = {
"<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<head>\r\n\
    <meta charset=\"UTF-8\">\r\n\
    <title>ota</title>\r\n\
    <style>\r\n\
        body{\r\n\
            font-family: Arial;\r\n\
            font-size: large;\r\n\
            margin: auto;\r\n\
            margin-top: 100px;\r\n\
            width: 150px;\r\n\
            height: 10px;\r\n\
        }\r\n\
    </style>\r\n\
</head>\r\n\
<body>\r\n\
<p>Update Succcess!</p>\r\n\
</body>\r\n\
</html>"
};

static const unsigned char ota_fail_html[] = {
"<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<head>\r\n\
    <meta charset=\"UTF-8\">\r\n\
    <title>ota</title>\r\n\
    <style>\r\n\
        body{\r\n\
            font-family: Arial;\r\n\
            font-size: large;\r\n\
            margin: auto;\r\n\
            margin-top: 100px;\r\n\
            width: 150px;\r\n\
            height: 10px;\r\n\
        }\r\n\
    </style>\r\n\
</head>\r\n\
<body>\r\n\
<p>Update Fail!</p>\r\n\
</body>\r\n\
</html>"
};

static int ota_value_checker (void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size)
{
  int *want_off = cls;
  int idx = *want_off;

  //http_ota_log("idx=%d; key=%s; filename=%s; size=%d; off=%d", idx, key, filename, (int)size, (int)off);
  
  if( (off != 0) && (size == 0) )
    return MHD_YES;
  
  if( !strcmp("ota_fw", key) ){
    if( !strstr(filename, ".bin") ){
      is_ota_success = false;
      return MHD_YES;
    }
    is_ota_success = true;
    if( off == 0){
      if( size == 0 )
        return MHD_YES;
      CRC16_Init( &crc16_context );
      MicoFlashErase( MICO_PARTITION_OTA_TEMP, 0x0, ota_partition->partition_length);
      MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &offset, (uint8_t *)data, size);
      CRC16_Update( &crc16_context, data, size);
    }else{
      MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &offset, (uint8_t *)data, size);
      CRC16_Update( &crc16_context, data, size);      
    }
  }else if( !strcmp("ota_web", key) ){
    if( !strncmp("format", filename, strlen("format")) ){
      if( off == 0){
        is_format = true;
        return MHD_YES;
      }else{
        return MHD_YES;
      }
    }
    is_web_success = true;
    if( off == 0 ){
      if( idx == 0 ){
        strcpy(is_file, filename);
      }
      if( idx != 0 ){
        if( strcmp(is_file, filename) ){
          close_file( );
          http_ota_log("close file:%s", is_file);
          strcpy(is_file, filename);
        }
      }
      if(size == 0)
         return MHD_YES;
      open_file(filename, 0);
      write_file(data, size);
      http_ota_log("open file:%s", filename);
    }else{
      write_file(data, size);
    }
  }else{
    http_ota_log("unknow name");
  }
 
  *want_off = idx + 1;
  return MHD_YES;
}

static int web_send_ota_page(httpd_request_t *req)
{
  OSStatus err = kNoErr;
  
  err = httpd_send_all_header(req, HTTP_RES_200, sizeof(ota_html), HTTP_CONTENT_HTML_STR);
  require_noerr_action( err, exit, http_ota_log("ERROR: Unable to send http wifisetting headers.") );
  
  err = httpd_send_body(req->sock, ota_html, sizeof(ota_html));
  require_noerr_action( err, exit, http_ota_log("ERROR: Unable to send http wifisetting body.") );
  
exit:
  return err; 
}

extern void mico_write_ota_tbl(int len, uint16_t crc);

int web_set_update(httpd_request_t *req)
{
  OSStatus err = kNoErr;
  char buf[rec_ota_buf_size];
  int file_length, rec_length;
  int ota_length = 0;
  
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = 0;
  char multipart[128];
  
  ota_partition = MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP );
  
  memset(is_file, '0', sizeof(is_file));
  offset = 0x0;
  is_ota_success = false;
  is_web_success = false;
  is_format = false;
 
  err = httpd_parse_hdr_tags(req, req->sock, buf, rec_ota_buf_size);
  require_noerr(err, exit);
  
  if(strncmp("multipart/form-data;", req->content_type, strlen("multipart/form-data;"))){
    err = -1;
    goto exit;
  }
  
  file_length = req->body_nbytes;
  ota_length = file_length;
  sprintf(multipart, "%s, %s", MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA,  &req->content_type[strlen("multipart/form-data; ")]);
  
  http_ota_log("content-type=%s; length=%d", multipart, req->body_nbytes);
  
  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value = multipart;
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection, 1024, &ota_value_checker, &want_off);
  
  while(file_length > 0) {
    rec_length = recv(req->sock, buf, (file_length > rec_ota_buf_size) ? rec_ota_buf_size : file_length, 0);
    if(rec_length < 0){
      err = -1;
      goto exit;
    }
    MHD_post_process (pp, buf, rec_length);
    file_length -= rec_length;
  }
  MHD_destroy_post_processor (pp);
  
  if( is_format == true){
#ifdef USE_FILESYS
    fatfs_sys_creat();
#endif    
  }
exit:
  
  if( is_web_success == true )
    close_file( );
  
  if( (is_web_success == true) || (is_ota_success == true) || (is_format == true) ){
    err = httpd_send_all_header(req, HTTP_RES_200, sizeof(ota_success_html), HTTP_CONTENT_HTML_STR);
    err = httpd_send_body(req->sock, ota_success_html, sizeof(ota_success_html));
  }else{
    err = httpd_send_all_header(req, HTTP_RES_200, sizeof(ota_fail_html), HTTP_CONTENT_HTML_STR);
    err = httpd_send_body(req->sock, ota_fail_html, sizeof(ota_fail_html));
  }
  
  if( is_ota_success == true ){
    CRC16_Final( &crc16_context, &crc);
    mico_write_ota_tbl(ota_length, crc);
    mico_system_power_perform( mico_context, eState_Software_Reset );
    mico_thread_sleep( MICO_WAIT_FOREVER );
  }
  
  return err;
}

void http_ota_register_handlers()
{
  int rc;
  mico_context = mico_system_context_get();
  
  rc = httpd_register_wsgi_handlers(g_ota_handlers, g_ota_handlers_no);
  if (rc) {
    http_ota_log("failed to register ota web handler");
  }
}

static struct httpd_wsgi_call g_ota_handlers[] = {
  {"/ota", HTTPD_SERVER_HDR_DEFORT, 0, web_send_ota_page, NULL, NULL, NULL},
  {"/otares.html", HTTPD_SERVER_HDR_DEFORT, 0, NULL, web_set_update, NULL, NULL},
};

static int g_ota_handlers_no = sizeof(g_ota_handlers)/sizeof(struct httpd_wsgi_call);

