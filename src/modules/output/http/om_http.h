/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_OM_HTTP_H
#define __NX_OM_HTTP_H

#include "../../../common/ssl.h"
#include "../../../common/types.h"
#include "../../../common/logdata.h"

#define NX_OM_HTTP_RESPBUFSIZE 4096


typedef struct nx_om_http_server_t
{
    boolean https;
    const char *url;
    const char *host;
    apr_port_t port;
    char path[2048];
} nx_om_http_server_t;


typedef enum nx_om_http_resp_state_t
{
    NX_OM_HTTP_RESP_STATE_START = 0,
    NX_OM_HTTP_RESP_STATE_CR,
    NX_OM_HTTP_RESP_STATE_CR_LF,
    NX_OM_HTTP_RESP_STATE_CR_LF_CR,
    NX_OM_HTTP_RESP_STATE_CR_LF_CR_LF,
} nx_om_http_resp_state_t;

typedef struct nx_om_http_conf_t
{
    apr_pool_t		*pool;
    nx_om_http_server_t server;
    BIO 		*bio_req;
    const char		*reqbuf;
    apr_size_t		reqbufsize;
    nx_logdata_t	*logdata;
    boolean		connected;
    apr_socket_t	*sock;
    nx_ssl_ctx_t	ssl_ctx;
    SSL			*ssl;
    char		respbuf[NX_OM_HTTP_RESPBUFSIZE];
    BIO			*bio_resp_head;
    BIO			*bio_resp_body;
    apr_size_t		content_length;
    nx_om_http_resp_state_t resp_state;
    boolean		got_resp_head;
    boolean		got_resp_body;
    boolean		resp_wait;
    nx_event_t		*timeout_event;
    int			reconnect; // number of seconds after trying to reconnect
    const char		*content_type;
} nx_om_http_conf_t;


#endif	/* __NX_OM_HTTP_H */
