#include <httpd.h>
#include <http_parse.h>
#include <http-strings.h>

#include "MICO.h"
#include "MICOAppDefine.h"
#include "httpd_priv.h"
#include "http_server/http_server.h"


#define http_server_log(M, ...) custom_log("http server", M, ##__VA_ARGS__)

static bool is_http_init;
static bool is_handlers_registered;
static app_context_t *app_context;

static int _httpd_start()
{
  OSStatus err = kNoErr;
  http_server_log("initializing web-services");
  
  /*Initialize HTTPD*/
  if (is_http_init == false) {
    err = httpd_init();
    require_noerr_action( err, exit, http_server_log("failed to initialize httpd") );
    is_http_init = true;
  }
  
  /*Start http thread*/
#ifndef USE_FILESYS
  datafs_init( );
#endif

  err = httpd_start();
  if (err != kNoErr) {
    http_server_log("failed to start httpd thread");
    httpd_shutdown();
  }
exit:
  return err;
}

static void _httpd_auth_init( void )
{
  httpd_auth_init(app_context->appConfig->at_config.web_login.login_username, app_context->appConfig->at_config.web_login.login_password);
}

int http_server_fs_start( void *app_context_c )
{
  OSStatus err = kNoErr;
  app_context = app_context_c;
  
  err = _httpd_start();
  require_noerr( err, exit );
  
  _httpd_auth_init();
  
  if (is_handlers_registered == false) {   
#ifdef USE_FILESYS    
    http_ota_register_handlers();
#endif
    http_sys_register_file_handlers( app_context );
    is_handlers_registered = true;
  }
  
exit:
  return err;
}

int http_server_stop()
{
  OSStatus err = kNoErr;
  
  /* HTTPD and services */
  http_server_log("stopping down httpd");
  err = httpd_stop();
  require_noerr_action( err, exit, http_server_log("failed to halt httpd") );
  
exit:
  return err;
}

