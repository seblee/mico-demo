#include "mico.h"
#include "smtpclient.h"

#define smtp_log(M, ...) custom_log("SMTP", M, ##__VA_ARGS__)

#define SMTP_SERVER_ADDR "smtp.mxhichina.com"
#define SMTP_SERVER_PORT 465
#define SMTP_SENDER_ADDR "yanghb@mxchip.com"
//#define SMTP_SENDER_PASS "bunengshuodemimi"
#define SMTP_SENDER_PASS "YHB!cisco1"

static mico_semaphore_t wait_sem = NULL;

int smtp_test()
{
    SMTP* smtp;
    int i = 0;
    char subject[64];
    
    smtp_log("start smtp test");
    smtp = SmtpInit(SMTP_SERVER_ADDR, SMTP_SERVER_PORT, SMTP_SENDER_ADDR, 
      SMTP_SENDER_ADDR, SMTP_SENDER_PASS);

    if (smtp == NULL) {
        smtp_log("smtp init error");
        return -1;
    }

    SmtpSetSmtpMode(smtp, SMTP_SECURITY_TLS);
    SmtpAddCc(smtp, SMTP_SENDER_ADDR);
    SmtpAddTo(smtp, "haibo344@sina.com");
    while(1) {
        sprintf(subject, "subject %d", i);
        SmtpSendMail(smtp, subject, "body");
        i++;
        if (i > 2)
            break;
    }

    smtp_log("smtp sent");
    SmtpDeinit(smtp);

    return 0;
}


static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            mico_rtos_set_semaphore( &wait_sem );
            break;
        case NOTIFY_STATION_DOWN:
        case NOTIFY_AP_UP:
        case NOTIFY_AP_DOWN:
            break;
    }
}

int main( void )
{
    OSStatus err = kNoErr;

    mico_rtos_init_semaphore( &wait_sem, 1 );

    /*Register user function for MiCO nitification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                       (void *) micoNotify_WifiStatusHandler, NULL );
    require_noerr( err, exit );

    /* Start MiCO system functions according to mico_config.h */
    err = mico_system_init( mico_system_context_init( 0 ) );
    require_noerr( err, exit );

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );
    smtp_log( "wifi connected successful" );

    /* Start SMTP test */
    smtp_test();
    
    exit:
    if ( wait_sem ) mico_rtos_deinit_semaphore( &wait_sem );
    return err;
}



