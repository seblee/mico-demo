//////////////////////////////////////////////////////////////
// smtpclient.c for send mail by navy 2004.3.22

//#define DEBUG
//#define SMTP_DEBUG5

//#define SMTP_IO_DEBUG /* added by jinfeng */

// modified by jinfeng 2014.9.12


/////////////////////////////////////////////////////////////////////
//
// ������һЩ����SOCKETͨѶ�Ļ����ĺ궨��Ͱ���ͷ��������WIN32��UNIX
//
/////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "mico.h"
#include "smtpclient.h"

typedef unsigned char byte;
#define smtp_log(M, ...) custom_log("SMTP", M, ##__VA_ARGS__)

#define socketerrno

#define CONNECT_DELAY_TIME      (30)
#define SOCKET_READ_TIMEOUT     CONNECT_DELAY_TIME


#define LINE_LEN                (1024)
#define LONGLINE_LEN            (1024)
#define SHORTLINE_LEN           (256)
#define MAXLINE                 (8192)



/////////////////////////////////////////////////////////////////////
//
// ������һЩ����SMTP���ŵĺ궨��
//
/////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define SMTP_DEBUG(a) a
#else
#define SMTP_DEBUG(a) /*a*/
#endif


#define SMTP_DOT                (".")
#define SMTP_CRLF               ("\r\n")
#define SMTP_CR                 ("\r")
#define SMTP_LF                 ("\n")

#define SMTP_CMD_EHLO           ("EHLO MXCHIP")
#define SMTP_CMD_HELO           ("HELO ")
#define SMTP_CMD_AUTH_LOGIN     ("AUTH LOGIN")
#define SMTP_CMD_MAIL_FROM      ("MAIL FROM: ")
#define SMTP_CMD_RCPT_TO        ("RCPT TO: <")
#define SMTP_CMD_DATA           ("DATA")
#define SMTP_CMD_HELP           ("HELP")
#define SMTP_CMD_RSET           ("RSET")
#define SMTP_CMD_NOOP           ("NOOP")
#define SMTP_CMD_QUIT           ("QUIT")
#define SMTP_END_OF_MAIL        ("\r\n.\r\n")
#define SMTP_CMD_TLS            ("STARTTLS")

#define SMTP_RCV_END            SMTP_CRLF
#define SMTP_RCV_ESMTP          ("ESMTP")
#define SMTP_RCV_SMTP           ("SMTP")
#define SMTP_RCV_SERVOK         ("220")     // 220 �������
#define SMTP_RCV_HELO           ("250")     // 250 Ҫ����ʼ��������

#define SMTP_RCV_EHLO           ("250")     // 250 Ҫ����ʼ��������
#define SMTP_RCV_AUTH_LOGIN     ("334")
#define SMTP_RCV_AUTH_USER      ("334")
#define SMTP_RCV_AUTH_PASSWD    ("334")
#define SMTP_RCV_AUTH_OK        ("235")
#define SMTP_RCV_MAIL_FROM      ("250")
#define SMTP_RCV_RCPT_TO        ("250")
#define SMTP_RCV_DATA           ("354")
#define SMTP_RCV_SEND_END       ("250")
#define SMTP_RCV_RSET           ("250")
#define SMTP_RCV_NOOP           ("250")
#define SMTP_RCV_QUIT           ("221")     // 221 ����رմ����ŵ�

#define SMTP_SEND_BLOCK_SIZE    (1024)      // ÿ�η����ż����ݵĿ�Ĵ�С

#define SMTP_APPEND_TO_MAX 8

#define SMTP_MAX_TO_ADDR_NUM 10
////////////////////////////////////////////////////////////////////


int smtp_step = 0;
int smtp_fail_num = 0;

char g_line_mesg[LINE_LEN];

/////////////////////////////////////////////////////////////////////
//
// ������һЩ����TCP/IP��SOCKETͨѶ���ú�����������WIN32��UNIX
//
/////////////////////////////////////////////////////////////////////

static void Connect(SMTP *psmtp)
{
    int err;
    struct hostent* hostent_content = NULL;
    char **pptr = NULL;
    char ipstr[16];
    struct in_addr in_addr;
    struct sockaddr_in addr;
    
    hostent_content = gethostbyname( psmtp->smtpserver);
    require_action_quiet( hostent_content != NULL, exit, err = kNotFoundErr);
    pptr = hostent_content->h_addr_list;
    in_addr.s_addr = *(uint32_t *)(*pptr);
    strcpy( ipstr, inet_ntoa(in_addr));
    smtp_log("SMTP server address: host:%s, ip: %s", psmtp->smtpserver, ipstr);
    
    psmtp->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = in_addr.s_addr;
    addr.sin_port = htons(psmtp->smtpport);

    err = connect( psmtp->sockfd, (struct sockaddr *)&addr, sizeof(addr) );
    require_noerr( err, exit );
    smtp_log( "Connect success!" );

exit:
    if ( err != kNoErr ) 
    {
        smtp_log( "Connect fail!" );
        close(psmtp->sockfd);
        psmtp->sockfd = -1;
    }
}

static ssize_t writen(SMTP *psmtp, const void *vptr, size_t n)
{
    int ret;

    if (psmtp->ssl == NULL) {
        ret = send(psmtp->sockfd, vptr, n, 0);
    } else {
        ret = ssl_send(psmtp->ssl, (uint8_t *)vptr, n);
    }
    
    if (-1 == ret)
    {
        if (psmtp->ssl) {
            ssl_close(psmtp->ssl);
            psmtp->ssl = NULL;
        }
        close(psmtp->sockfd);
        psmtp->sockfd = -1;
    }
#ifdef SMTP_IO_DEBUG
    else
    {
        printf("%s", vptr);
    }
#endif

    return ret;
}

static ssize_t readtimeout(SMTP *psmtp, void *buf, unsigned int len, int timeoutsec)
{
    int nread = 0;
    fd_set readfds;
    struct timeval timeout;

    if (psmtp->ssl) {
        if (ssl_pending(psmtp->ssl) > 0) {
            nread = ssl_recv(psmtp->ssl, buf, len);
            return nread;
        }
    }
    timeout.tv_sec = timeoutsec;
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(psmtp->sockfd, &readfds);
    select(1, &readfds, NULL, NULL, &timeout);

    if (FD_ISSET(psmtp->sockfd, &readfds))
    {
        if (psmtp->ssl != NULL) {
            nread = ssl_recv(psmtp->ssl, buf, len);
            if (nread < 0) {
                ssl_close(psmtp->ssl);
                psmtp->ssl = NULL;
            }
        } else {
            nread = recv(psmtp->sockfd, buf, len, 0);
        }
        if (-1 == nread)
        {
            close(psmtp->sockfd);
            psmtp->sockfd = -1;
        }
#ifdef SMTP_IO_DEBUG
        else
        {
            printf("%s", buf);
        }
#endif
    }

    return nread;
}

/////////////////////////////////////////////////////////////////////////
//      Tools
/////////////////////////////////////////////////////////////////////////

static char* nextline(char *sbuf, const int size, char **pptr, char **ppchr, char *chr)
{
    char *ptr = NULL, *pchr = NULL;

    if (sbuf == NULL || size <= 0 || pptr == NULL)
        return NULL;

    if ((*pptr) == NULL)(*pptr) = sbuf;

    if (ppchr != NULL && chr != NULL && (*ppchr) != NULL)
    {
        (**ppchr) = (*chr);
        (*ppchr)  = NULL;
        (*chr)    = 0;
    }

    if ((*pptr) - sbuf >= size || (*pptr) < sbuf)
        return NULL;

    ptr = pchr = (*pptr);

    for (;;)
    {
        if (pchr - sbuf >= size)
        {
            if (((*pptr) = pchr) == ptr)
                return NULL;

            (*ppchr) = NULL;
            (*chr)   = 0;
            (*pptr)  = sbuf + size;

            return ptr;
        }

        switch ((*pchr))
        {
        case '\0':
            if (((*pptr) = pchr) == ptr)
                return NULL;
            else
                return ptr;

        case '\n':
            pchr ++;

            if (pchr - sbuf < size)
            {
                if (ppchr != NULL && chr != NULL)
                {
                    (*ppchr) = pchr;
                    (*chr)   = (*pchr);
                }

                (*pchr) = '\0';
            }

            (*pptr) = pchr;
            return ptr;
        }

        pchr ++;
    }

    return ptr;
}

#ifdef SMTP_DEBUG5
static char* formattime(const time_t t, char *buf, const int buflen)
{
    struct tm *dt;
    memset(buf, 0, buflen);

    if (t > 0)
    {
        dt = (struct tm *) localtime(&t);
        snprintf(buf, buflen, "%d/%02d/%02d_%02d:%02d:%02d",
                 dt->tm_year + 1900,
                 dt->tm_mon + 1,
                 dt->tm_mday,
                 dt->tm_hour,
                 dt->tm_min,
                 dt->tm_sec);
    }

    return buf;
}
#endif


//-----------------------------------------------------------------------
//      SMTP Decoder and Encoder
//-----------------------------------------------------------------------
static const char Base64Code[] =
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

// strlen(paInput)/strlen(paOutput) = 4/3
static int Base64Encode(char *paInput, char *paOutput, int *iCount)
{
    int i = 0;
    char caThree[3] = {0}, *p = NULL, *q = NULL;

    if (paInput == NULL || paOutput == NULL || iCount == NULL)
        return -1;

    for (p = paInput, q = paOutput; *p != '\0';)
    {
        memset(caThree, 0, sizeof(caThree));

        for (i = 0; ((i < 3) && (*p != '\0'));)
            caThree[i++] = *p++;

        *iCount += 3;

        *q++ = Base64Code[((caThree[0] >> 2) & 0x3f)];
        *q++ = Base64Code[(((caThree[0] & 0x03) << 4) | ((caThree[1] >> 4) & 0x0f))];

        if (i < 2)
            *q++ = '=';
        else
            *q++ = Base64Code[(((caThree[1] & 0x0f) << 2) | ((caThree[2] >> 6) & 0x03))];

        if (i < 3)
            *q++ = '=';
        else
            *q++ = Base64Code[(caThree[2] & 0x3f)];
    }

    *q = '\0';
    return 0;
}

// strlen(paInput)/strlen(paOutput) = 4/3
MICO_UNUSED static int Base64Decode(char *paInput, char *paOutput)
{
    int i;
    char caFour[4] = {0}, *p = NULL, *q = NULL;

    if (paInput == NULL || paOutput == NULL)
        return -1;

    for (p = paInput, q = paOutput; *p != '\0';)
    {
        memset(caFour, 0, sizeof(caFour));

        for (i = 0; ((i < 4) && (*p != '\0'));)
        {
            if (*p >= 'A' && *p <= 'Z')
                *p -= 65;
            else if (*p >= 'a' && *p <= 'z')
                *p -= 71;
            else if (*p >= '0' && *p <= '9')
                *p += 4;
            else if (*p == '+')
                *p = 62;
            else if (*p == '/')
                *p = 63;
            else if (*p == '=')
                *p = 0;

            caFour[i++] = *p++;
        }

        *q++ = (((caFour[0] & 0x3f) << 2) | ((caFour[1] & 0x3f) >> 4));
        *q++ = (((caFour[1] & 0x3f) << 4) | ((caFour[2] & 0x3f) >> 2));
        *q++ = (((caFour[2] & 0x3f) << 6) | (caFour[3] & 0x3f));
    }

    *q = '\0';
    return 0;
}

MICO_UNUSED static int MimeEncodeHeader(const char *charset, char *header)
{
    static const char defaultCharset[] = "gb2312";
    char strtmp[LINE_LEN];
    int count = 0;

    if (header == NULL)
        return -1;

    if (charset == NULL)
        charset = defaultCharset;

    memset(strtmp, 0, sizeof(strtmp));

    count = (int)(sizeof(strtmp) * 3 / 4);

    if ((int) strlen(header) > count)
        header[count] = 0;

    count = 0;

    Base64Encode(header, strtmp, &count);

    sprintf(header, "=?%s?B?%s?=", charset, strtmp);

    return count;
}


//-----------------------------------------------------------------------
//  SMTP Helper Function
//-----------------------------------------------------------------------

static void SmtpPrintDebugInfo(SMTP *psmtp)
{
#ifdef SMTP_DEBUG5
    char temp[LINE_LEN];

    if (psmtp == NULL)
        return;

    printf("[SMTPCLIENT DEBUG %s] sockfd: %d, smtphost: %p, sender: %s, from: %s, to: %s,"
           " smtpcmd: %s, smtprcv: %s, errcode: %d, errinfo: %s\n",
           formattime(time(NULL), temp, sizeof(temp)),
           psmtp->sockfd, psmtp->smtpserver,
           psmtp->sender, psmtp->sender,
           psmtp->smtpcmd, psmtp->smtprcv,
           psmtp->errcode, psmtp->errinfo);
#endif
}

static void SmtpRaiseError(SMTP *psmtp, const int errcode, const char *fmt, ...)
{
    char mesg[LINE_LEN];
    va_list ap;

    memset(mesg, 0, sizeof(mesg));

    va_start(ap, fmt);
    vsnprintf(mesg, sizeof(mesg), fmt, ap);
    va_end(ap);

    if (psmtp != NULL)
    {
        psmtp->errcode = errcode;
        snprintf(psmtp->errinfo, sizeof(psmtp->errinfo), mesg);
    }

    SMTP_DEBUG(SmtpPrintDebugInfo(psmtp);)
}

static void SmtpParseMessage(SMTP *psmtp, const char *mesg)
{
    int len = 0, i = 0;
    char *pstr = NULL;

    if (psmtp == NULL) return;

    psmtp->errcode = 0;
    memset(psmtp->errinfo, 0, sizeof(psmtp->errinfo));

    if (mesg != NULL && (len = strlen(mesg)) > 0)
    {
        psmtp->errcode = atoi(mesg);

        pstr = (char *) mesg;

        for (i = 0; i < len; i ++)
        {
            if ((char)(*pstr) == ' ')
            {
                pstr ++;
                break;
            }
            else
                pstr ++;
        }

        strncpy(psmtp->errinfo, pstr, sizeof(psmtp->errinfo) - 1);
    }
}

static int SmtpRecv(SMTP *psmtp, const char *smtprcv, char *buf, const int len)
{
    int errcode = 0, recvlen = 0;

    if (psmtp == NULL)
        return -1;

    if (psmtp->sockfd == -1)
        return -1;

    memset(buf, 0, len);
    memset(psmtp->smtprcv, 0, sizeof(psmtp->smtprcv));

    if ((errcode = readtimeout(psmtp, buf, len - 1, SOCKET_READ_TIMEOUT)) < 0)
    {
        SmtpRaiseError(psmtp, errcode, "Receive from smtp server failed.");

        return -2;
    }

    SMTP_DEBUG(printf("[SMTPCLIENT DEBUG] SmtpRecv: %s\n", buf);)

    SmtpParseMessage(psmtp, buf);

    errcode = 0;

    if (smtprcv != NULL && (recvlen = strlen(smtprcv)) > 0)
    {
        psmtp->recvsize  += recvlen;
        psmtp->recvlines ++;

        strncpy(psmtp->smtprcv, smtprcv, sizeof(psmtp->smtprcv) - 1);

        if (strncmp(buf, psmtp->smtprcv, recvlen) == 0)
            psmtp->errcode = 0;
        else
            errcode = -3;
    }
    else
        psmtp->errcode = 0;

    SMTP_DEBUG(SmtpPrintDebugInfo(psmtp);)

    return errcode;
}

static int SmtpSendLine(SMTP *psmtp, const char *line)
{
    int errcode = 0, len = 0;

    if (psmtp == NULL || psmtp->sockfd == -1)
        return -1;

    if (line != NULL && line[0] == '.')
    {
        SMTP_DEBUG(printf("[SMTPCLIENT DEBUG] SmtpSend: %s\n", SMTP_DOT);)

        if ((errcode = writen(psmtp, SMTP_DOT, strlen(SMTP_DOT))) < 0)
        {
            SmtpRaiseError(psmtp, errcode, "Send to smtp server failed.");
            return -2;
        }
    }

    if (line != NULL && (len = strlen(line)) > 0)
    {
        SMTP_DEBUG(printf("[SMTPCLIENT DEBUG] SmtpSend: %s\n", line);)

        if ((errcode = writen(psmtp, line, len)) < 0)
        {
            SmtpRaiseError(psmtp, errcode, "Send to smtp server failed.");
            return -3;
        }
    }

    psmtp->sentsize  += len;
    psmtp->sentlines ++;

    //if( line == NULL || len <= 0 || !( len >= 2 && line[len-1] == '\n' && line[len-2] == '\r' ) ) {
    if (line == NULL || len <= 0 || !(len >= 1 && line[len - 1] == '\n'))
    {
        SMTP_DEBUG(printf("[SMTPCLIENT DEBUG] SmtpSend: \\r\\n\n");)

        if (len >= 1 && line[len - 1] == '\r')
        {
            psmtp->sentsize += 1;

            if ((errcode = writen(psmtp, SMTP_LF, strlen(SMTP_LF))) < 0)
            {
                SmtpRaiseError(psmtp, errcode, "Send LF to smtp server failed.");
                return -4;
            }
        }
        else
        {
            psmtp->sentsize += 2;

            if ((errcode = writen(psmtp, SMTP_CRLF, strlen(SMTP_CRLF))) < 0)
            {
                SmtpRaiseError(psmtp, errcode, "Send CRLF to smtp server failed.");
                return -4;
            }
        }
    }

    return 0;
}

static int SmtpSend(SMTP *psmtp, const char *smtpcmd, const char *fmt, ...)
{
    int len = 0;
    char mesg[LINE_LEN], *pstr = NULL;
    va_list ap;

    if (smtpcmd == NULL && fmt == NULL)
        return 0;

    if (NULL == psmtp || psmtp->sockfd == -1)
        return -1;

    memset(mesg, 0, sizeof(mesg));
    pstr = mesg;

    if (smtpcmd != NULL)
    {
        strncpy(mesg, smtpcmd, sizeof(mesg) - 1);
        len = strlen(mesg);
        pstr += len;
    }

    if (fmt != NULL)
    {
        va_start(ap, fmt);
        vsnprintf(pstr, sizeof(mesg) - len, fmt, ap);
        va_end(ap);
    }

    if (SmtpSendLine(psmtp, mesg) < 0)
        return -2;

    return 0;
}

static int SmtpSendText(SMTP *psmtp, char *text, const int size)
{
    char *ptr = NULL, *pbuf = NULL;
    char *pbak = NULL, chr = 0;

    if (text == NULL || size <= 0)
        return -1;

    pbuf = text;

    while ((ptr = nextline(text, size, &pbuf, &pbak, &chr)) != NULL)
    {
        if (SmtpSendLine(psmtp, ptr) < 0)
            return -2;
    }

    return 0;
}

static int SmtpOpen(SMTP *psmtp)
{
    int ret;
    unsigned int tmp;
    int count = 0;
    int err;
    //CyaSSL_Debugging_ON();

    if (psmtp == NULL) return -1;

    memset(g_line_mesg, 0, sizeof(g_line_mesg));

    Connect(psmtp);

    if (-1 == psmtp->sockfd)
    {
        SmtpRaiseError(psmtp, psmtp->sockfd, "Cannot connect to smtp server.");
        return -1;
    }
		
    if ((psmtp->smtpmode == SMTP_SECURITY_SSL)
            || (psmtp->smtpmode == SMTP_SECURITY_TLS))
    {
        psmtp->ssl = ssl_connect(psmtp->sockfd, 0, NULL, &err);
        if (psmtp->ssl == NULL)
        {
            close(psmtp->sockfd);
            psmtp->sockfd = -1;
            SmtpRaiseError(psmtp, psmtp->sockfd, "SSL connect error %d", err);
            return -1;
        }
    }

    if (SmtpRecv(psmtp, SMTP_RCV_SERVOK, g_line_mesg, sizeof(g_line_mesg)) < 0)
        return -2;

    if (SmtpSend(psmtp, SMTP_CMD_EHLO, NULL) < 0)
        return -3;

    if (SmtpRecv(psmtp, SMTP_RCV_EHLO, g_line_mesg, sizeof(g_line_mesg)) < 0)
        return -4;

    // if starttls, do start tls here
    if ((psmtp->smtpmode == SMTP_SECURITY_STARTTLS)
            && (strstr(g_line_mesg, SMTP_CMD_TLS)))
    {
        //smtp_step = __LINE__;

        if (SmtpSend(psmtp, SMTP_CMD_TLS, NULL) < 0)
            return -3;

        //smtp_step = __LINE__;

        if (SmtpRecv(psmtp, SMTP_RCV_SERVOK, g_line_mesg, sizeof(g_line_mesg)) < 0)
            return -4;

        //smtp_step = __LINE__;

        psmtp->ssl = ssl_connect(psmtp->sockfd, 0, NULL, &err);
        if (psmtp->ssl == NULL)
        {
            close(psmtp->sockfd);
            psmtp->sockfd = -1;
            SmtpRaiseError(psmtp, psmtp->sockfd, "SSL connect error %d", err);
            return -1;
        }

        if (SmtpSend(psmtp, SMTP_CMD_EHLO, NULL) < 0)
            return -3;

        //smtp_step = __LINE__;

        if (SmtpRecv(psmtp, SMTP_RCV_EHLO, g_line_mesg, sizeof(g_line_mesg)) < 0)
            return -4;
    }

    if (strlen(psmtp->user) > 0 && strlen(psmtp->passwd) > 0)
    {
        smtp_fail_num = 4;

        if (SmtpSend(psmtp, SMTP_CMD_AUTH_LOGIN, NULL) < 0)
            return -5;

        if (SmtpRecv(psmtp, SMTP_RCV_AUTH_LOGIN, g_line_mesg, sizeof(g_line_mesg)) < 0)
            return -6;

        Base64Encode(psmtp->user, g_line_mesg, &count);

        if (SmtpSend(psmtp, g_line_mesg, NULL) < 0)
            return -7;

        if (SmtpRecv(psmtp, SMTP_RCV_AUTH_USER, g_line_mesg, sizeof(g_line_mesg)) < 0)
            return -8;

        Base64Encode(psmtp->passwd, g_line_mesg, &count);

        if (SmtpSend(psmtp, g_line_mesg, NULL) < 0)
            return -9;

        if (SmtpRecv(psmtp, SMTP_RCV_AUTH_OK, g_line_mesg, sizeof(g_line_mesg)) < 0)
            return -10;
    }

    return 0;
}

static int SmtpClose(SMTP *psmtp)
{
    if (psmtp == NULL) return -1;

    SmtpSend(psmtp, SMTP_CMD_QUIT, NULL);

    if (psmtp->sockfd != -1)
        close(psmtp->sockfd);

    psmtp->sockfd = -1;
    return 0;
}


//-----------------------------------------------------------------------
//                                              User Interface
//-----------------------------------------------------------------------

static int sendmail(SMTP *psmtp, char *body, int bodysize)
{
    char mesg[LONGLINE_LEN];
    char *ptr = NULL, *pbuf = NULL;
    char *pbak = NULL, chr = 0;
    struct addr_list *list;

    memset(mesg, 0, sizeof(mesg));

    psmtp->errcode = 0;
    memset(psmtp->errinfo, 0, sizeof(psmtp->errinfo));

    if (SmtpOpen(psmtp) < 0)
        goto safe_return;

    smtp_fail_num = CMD_EMAIL_MSG_FAIL;

    if (SmtpSend(psmtp, SMTP_CMD_MAIL_FROM, "<%s>", psmtp->sender) < 0)
        goto safe_return;


    if (SmtpRecv(psmtp, SMTP_RCV_MAIL_FROM, mesg, sizeof(mesg)) < 0)
        goto safe_return;


    list = psmtp->to_list;
    while(list != NULL) {
        if (SmtpSend(psmtp, SMTP_CMD_RCPT_TO, "%s>\r\n", list->addr) < 0)
            goto safe_return;

        if (SmtpRecv(psmtp, SMTP_RCV_RCPT_TO, mesg, sizeof(mesg)) < 0)
            goto safe_return;
        list = list->next;
    }
    
    list = psmtp->cc_list;
    while(list != NULL) {
        if (SmtpSend(psmtp, SMTP_CMD_RCPT_TO, "%s>\r\n", list->addr) < 0)
            goto safe_return;

        if (SmtpRecv(psmtp, SMTP_RCV_RCPT_TO, mesg, sizeof(mesg)) < 0)
            goto safe_return;
        list = list->next;
    }

    if (SmtpSend(psmtp, SMTP_CMD_DATA, NULL) < 0)
        goto safe_return;

    if (SmtpRecv(psmtp, SMTP_RCV_DATA, mesg, sizeof(mesg)) < 0)
        goto safe_return;

    if (body != NULL && bodysize > 0)
    {
        if (SmtpSendText(psmtp, body, bodysize) < 0)
            goto safe_return;
    }

    if (SmtpSend(psmtp, NULL, SMTP_END_OF_MAIL) < 0)
        goto safe_return;

    if (SmtpRecv(psmtp, SMTP_RCV_SEND_END, mesg, sizeof(mesg)) < 0)
        goto safe_return;

safe_return:
    SmtpClose(psmtp);

    return psmtp->errcode;
}

SMTP* SmtpInit(char *server, uint16_t port, const char *psender, 
      const char *puser, const char *ppasswd)
{
    SMTP* smtp;

    smtp = (SMTP*)malloc(sizeof(SMTP));
    if (smtp == NULL)
        return NULL;
    memset(smtp, 0, sizeof(SMTP));
    strncpy(smtp->smtpserver, server, 64);
    smtp->smtpport = port;
    strncpy(smtp->sender, psender, EMAIL_ADDR_MAX_LEN);
    strncpy(smtp->user, puser, EMAIL_ADDR_MAX_LEN);
    strncpy(smtp->passwd, ppasswd, 64);

    return smtp;
}

int SmtpDeinit(SMTP* psmtp)
{
    if (psmtp) {
        free(psmtp);
    }

    return 0;
}

int SmtpAddTo(SMTP *psmtp, const char *pemail)
{
    struct addr_list *list;
    
    if (pemail == NULL)
        return -1;
    if (strlen(pemail) == 0)
        return -1;
    if (strlen(pemail) > EMAIL_ADDR_MAX_LEN)
        return -1;

    list = (struct addr_list*)malloc(sizeof(struct addr_list));
    if (list == NULL)
        return -1;

    list->next = psmtp->to_list;
    strcpy(list->addr, pemail);
    psmtp->to_list = list;

    return 0;
}

int SmtpRemoveTo(SMTP *psmtp, const char *pemail)
{
    struct addr_list *p, *q;
    
    if (pemail == NULL)
        return -1;

    if (psmtp == NULL)
        return 0;

    if (psmtp->to_list == NULL)
        return 0;

    p = psmtp->to_list;
    if (strcmp(psmtp->to_list->addr, pemail) == 0) {
        psmtp->to_list = p->next;
        free(p);
        return 0;
    }

    q = p->next;
    while(q != NULL) {
        if (strcmp(q->addr, pemail) == 0) {
            p->next = q->next;
            free(q);
            return 0;
        }
    }

    return 0;
}

int SmtpAddCc(SMTP *psmtp, const char *pemail)
{
    struct addr_list *list;
    
    if (pemail == NULL)
        return -1;
    if (strlen(pemail) == 0)
        return -1;
    if (strlen(pemail) > EMAIL_ADDR_MAX_LEN)
        return -1;

    list = (struct addr_list*)malloc(sizeof(struct addr_list));
    if (list == NULL)
        return -1;

    list->next = psmtp->cc_list;
    strcpy(list->addr, pemail);
    psmtp->cc_list = list;

    return 0;
}

int SmtpRemoveCc(SMTP *psmtp, const char *pemail)
{
    struct addr_list *p, *q;
    
    if (pemail == NULL)
        return -1;

    if (psmtp == NULL)
        return 0;

    if (psmtp->cc_list == NULL)
        return 0;

    p = psmtp->cc_list;
    if (strcmp(psmtp->cc_list->addr, pemail) == 0) {
        psmtp->cc_list = p->next;
        free(p);
        return 0;
    }

    q = p->next;
    while(q != NULL) {
        if (strcmp(q->addr, pemail) == 0) {
            p->next = q->next;
            free(q);
            return 0;
        }
    }

    return 0;
}

void SmtpSetSmtpMode(SMTP *psmtp, uint8_t mode)
{
    psmtp->smtpmode = mode;
}

int Smtp_GetError(void)
{
    return smtp_fail_num;
}

/* Return 0: success, other: fail*/
int SmtpSendMail(SMTP *psmtp, char *subject, char *body)
{
    char *smtpbody;
    int err;
    int bodylen = 256 + strlen(subject) + strlen(body) * 4 / 3 + SMTP_MAX_TO_ADDR_NUM * EMAIL_ADDR_MAX_LEN;
    int i;
    int first = 1;
    struct addr_list *list;
    
    smtp_fail_num = CMD_EMAIL_CONNT_FAIL;

    smtpbody = (char*)malloc(bodylen);

    if (smtpbody == NULL)
        return -1;

    memset(smtpbody, 0, bodylen);
    sprintf(smtpbody, "MIME-Version: 1.0\r\nFrom: %s\r\nTo: ", psmtp->sender);
    bodylen = strlen(smtpbody);
    list = psmtp->to_list;
    while(list != NULL) {
        if (first == 1)
        {
            first = 0;
            sprintf(&smtpbody[bodylen], "<%s>", list->addr);
        }
        else
        {
            sprintf(&smtpbody[bodylen], ",<%s>", list->addr);
        }

        bodylen = strlen(smtpbody);
        list = list->next;
    }
    
    sprintf(&smtpbody[bodylen],
            "\r\nSubject: %s\r\nContent-Type: text/plain; charset=\"us-ascii\"\r\nContent-Transfer-Encoding: base64\r\n\r\n",
            subject);
    bodylen = 0;
    Base64Encode(body, &smtpbody[strlen(smtpbody)], &bodylen);
    bodylen = strlen(smtpbody);
    smtpbody[bodylen++] = 0x0d;
    smtpbody[bodylen++] = 0x0a;
    err = sendmail(psmtp, smtpbody, bodylen);

    free(smtpbody);

    return err;
}

