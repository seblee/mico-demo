/*
 * libwebsockets-test-client - libwebsockets test implementation
 *
 * Copyright (C) 2011-2016 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The person who associated a work with this deed has dedicated
 * the work to the public domain by waiving all of his or her rights
 * to the work worldwide under copyright law, including all related
 * and neighboring rights, to the extent allowed by law. You can copy,
 * modify, distribute and perform the work, even for commercial purposes,
 * all without asking permission.
 *
 * The test apps are intended to be adapted for use in your code, which
 * may be proprietary.  So unlike the library itself, they are licensed
 * Public Domain.
 */

#include "lws_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


#include "libwebsockets.h"

#define LWS_DEMO_DEBUG MICO_DEBUG_ON
#define lws_demo_log(M, ...) MICO_LOG(LWS_DEMO_DEBUG, "CHAT", M, ##__VA_ARGS__)

static int deny_deflate, test_post;
static struct lws *wsi_demo;
static volatile int force_exit;
static unsigned int opts;

/*
 * This demo shows how to connect multiple websockets simultaneously to a
 * websocket server (there is no restriction on their having to be the same
 * server just it simplifies the demo).
 */

enum demo_protocols {

	PROTOCOL_DEMO,

	/* always last */
	DEMO_PROTOCOL_COUNT
};


/*
 * dumb_increment protocol
 *
 * since this also happens to be protocols[0], some callbacks that are not
 * bound to a specific protocol also turn up here.
 */

static int
callback_demo(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	const char *which = "http";
	char buf[50 + LWS_PRE];
	int n;

	switch (reason) {

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		lws_demo_log("demo: LWS_CALLBACK_CLIENT_ESTABLISHED");
		break;

	case LWS_CALLBACK_CLOSED:
		lws_demo_log("demo: LWS_CALLBACK_CLOSED");
		wsi_demo = NULL;
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		lws_demo_log("rx %d '%s'", (int)len, (char *)in);
		break;

	/* because we are protocols[0] ... */

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		if (wsi == wsi_demo) {
			which = "wsi_demo";
			wsi_demo = NULL;
		}
		lws_demo_log("CLIENT_CONNECTION_ERROR: %s: %s", which, (char *)in);
		break;

	case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
		if ((strcmp(in, "deflate-stream") == 0) && deny_deflate) {
			lws_demo_log("denied deflate-stream extension");
			return 1;
		}
		if ((strcmp(in, "x-webkit-deflate-frame") == 0))
			return 1;
		if ((strcmp(in, "deflate-frame") == 0))
			return 1;
		break;

	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		lws_demo_log("lws_http_client_http_response %d",
				lws_http_client_http_response(wsi));
		break;

	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		{
			char buffer[1024 + LWS_PRE];
			char *px = buffer + LWS_PRE;
			int lenx = sizeof(buffer) - LWS_PRE;

			lws_demo_log("LWS_CALLBACK_RECEIVE_CLIENT_HTTP");

			/*
			 * Often you need to flow control this by something
			 * else being writable.  In that case call the api
			 * to get a callback when writable here, and do the
			 * pending client read in the writeable callback of
			 * the output.
			 */
			if (lws_http_client_read(wsi, &px, &lenx) < 0)
				return -1;
			while (lenx--)
				putchar(*px++);
		}
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
	    lws_demo_log("Client wsi %p writable", wsi);
	    sprintf((char *)&buf[LWS_PRE],"hello");
        n = lws_write(wsi, (unsigned char *)&buf[LWS_PRE], strlen(&buf[LWS_PRE]), opts | LWS_WRITE_TEXT);
        if (n < 0)
            return -1;
		break;

	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		if (test_post) {
			unsigned char **p = (unsigned char **)in, *end = (*p) + len;

			if (lws_add_http_header_by_token(wsi,
					WSI_TOKEN_HTTP_CONTENT_LENGTH,
					(unsigned char *)"29", 2, p, end))
				return -1;
			if (lws_add_http_header_by_token(wsi,
					WSI_TOKEN_HTTP_CONTENT_TYPE,
					(unsigned char *)"application/x-www-form-urlencoded", 33, p, end))
				return -1;

			/* inform lws we have http body to send */
			lws_client_http_body_pending(wsi, 1);
			lws_callback_on_writable(wsi);
		}
		break;

	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
		strcpy(buf + LWS_PRE, "text=hello&send=Send+the+form");
		n = lws_write(wsi, (unsigned char *)&buf[LWS_PRE], strlen(&buf[LWS_PRE]), LWS_WRITE_HTTP);
		if (n < 0)
			return -1;
		/* we only had one thing to send, so inform lws we are done
		 * if we had more to send, call lws_callback_on_writable(wsi);
		 * and just return 0 from callback.  On having sent the last
		 * part, call the below api instead.*/
		lws_client_http_body_pending(wsi, 0);
		break;

	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
	    wsi_demo = NULL;
		force_exit = 1;
		break;

	default:
		break;
	}

	return 0;
}

/* list of supported protocols and callbacks */

static struct lws_protocols protocols[] = {
	{
		"",
		callback_demo,
		0,
		128,
	},
	{ NULL, NULL, 0, 0 } /* end */
};

static const struct lws_extension exts[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate; client_max_window_bits"
	},
	{
		"deflate-frame",
		lws_extension_callback_pm_deflate,
		"deflate_frame"
	},
	{ NULL, NULL, NULL /* terminator */ }
};



void sighandler(int sig)
{
	force_exit = 1;
}


static int ratelimit_connects(unsigned int *last, unsigned int secs)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (tv.tv_sec - (*last) < secs)
		return 0;

	*last = tv.tv_sec;

	return 1;
}

int test_ws_client(char *url)
{
	int ret = 0, use_ssl = 0, ietf_version = -1;
	unsigned int rl_dumb = 0, do_ws = 1, pp_secs = 0;
	struct lws_context_creation_info info;
	struct lws_client_connect_info i;
	struct lws_context *context;
	const char *prot, *p;
	char path[300];
	char str[100];
	
	strcpy(str, url);
	memset(&info, 0, sizeof info);

	lws_demo_log("libwebsockets test client - license LGPL2.1+SLE");
	lws_demo_log("(C) Copyright 2010-2016 Andy Green <andy@warmcat.com>");

	memset(&i, 0, sizeof(i));

	if (lws_parse_uri(str, &prot, &i.address, &i.port, &p))
		goto usage;

	/* add back the leading / on path */
	path[0] = '/';
	strncpy(path + 1, p, sizeof(path) - 2);
	path[sizeof(path) - 1] = '\0';
	i.path = path;

	lws_demo_log("%s://%s:%d%s", prot, i.address, i.port, i.path);

    if (!strcmp(prot, "http") || !strcmp(prot, "ws"))
        use_ssl = 0;
    if (!strcmp(prot, "https") || !strcmp(prot, "wss"))
        if (!use_ssl)
            use_ssl = LCCSCF_USE_SSL;

	/*
	 * create the websockets context.  This tracks open connections and
	 * knows how to route any traffic and which protocol version to use,
	 * and if each connection is client or server side.
	 *
	 * For this client-only demo, we tell it to not listen on any port.
	 */

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.ws_ping_pong_interval = pp_secs;
	info.pt_serv_buf_size = 1024;
	if( !use_ssl ){
	info.max_http_header_pool = 1;
	info.max_http_header_data = 1024;
	}

	if (use_ssl) {
		info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

	}

	if (use_ssl & LCCSCF_USE_SSL)
		lws_demo_log(" Using SSL");
	else
		lws_demo_log(" SSL disabled");
	if (use_ssl & LCCSCF_ALLOW_SELFSIGNED)
		lws_demo_log(" Selfsigned certs allowed");
	else
		lws_demo_log(" Cert must validate correctly (use -s to allow selfsigned)");
	if (use_ssl & LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK)
		lws_demo_log(" Skipping peer cert hostname check");
	else
		lws_demo_log(" Requiring peer cert hostname matches");

	context = lws_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed");
		return 1;
	}

	i.context = context;
	i.ssl_connection = use_ssl;
	i.host = i.address;
	i.origin = i.address;
	i.ietf_version_or_minus_one = ietf_version;
	i.client_exts = exts;

	if (!strcmp(prot, "http") || !strcmp(prot, "https")) {
		lws_demo_log("using %s mode (non-ws)", prot);
		if (test_post) {
			i.method = "POST";
			lws_demo_log("POST mode");
		}
		else
			i.method = "GET";
		do_ws = 0;
	} else
		lws_demo_log("using %s mode (ws)", prot);

	/*
	 * sit there servicing the websocket context to handle incoming
	 * packets, and drawing random circles on the mirror protocol websocket
	 *
	 * nothing happens until the client websocket connection is
	 * asynchronously established... calling lws_client_connect() only
	 * instantiates the connection logically, lws_service() progresses it
	 * asynchronously.
	 */

	while (!force_exit) {
			if (do_ws) {
				if (!wsi_demo && ratelimit_connects(&rl_dumb, 2u)) {
					lws_demo_log("demo: connecting");
					i.protocol = protocols[PROTOCOL_DEMO].name;
					i.pwsi = &wsi_demo;
					lws_client_connect_via_info(&i);
				}
			} else {
				if (!wsi_demo && ratelimit_connects(&rl_dumb, 2u)) {
					lws_demo_log("http: connecting");
					i.pwsi = &wsi_demo;
					lws_client_connect_via_info(&i);
				}
			}
		lws_service(context, 500);
	}

	lws_demo_log("Exiting");
	lws_context_destroy(context);

	return ret;

usage:
	fprintf(stderr, "Usage: libwebsockets-test-client "
				"<server address> [--port=<p>] "
				"[--ssl] [-k] [-v <ver>] "
				"[-d <log bitfield>] [-l]\r\n");
	return 1;
}


