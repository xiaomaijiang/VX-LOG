/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/alloc.h"

#include "om_http.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define OM_HTTP_DEFAULT_CONNECT_TIMEOUT (APR_USEC_PER_SEC * 30)
#define OM_HTTP_TIMEOUT_SEC 10


static void throw_sslerror(const char	*fmt,
			   ...) PRINTF_FORMAT(1,2) NORETURN;

static void throw_sslerror(const char	*fmt,
			   ...)
{
    char buf[NX_LOGBUF_SIZE];
    va_list ap;

    va_start(ap, fmt);
    apr_vsnprintf(buf, NX_LOGBUF_SIZE, fmt, ap);
    va_end(ap);
    throw_msg("%s: %s", buf, ERR_reason_error_string(ERR_get_error()));
}



static void om_http_add_reconnect_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_om_http_conf_t *modconf;

    modconf = (nx_om_http_conf_t *) module->config;

    log_info("reconnecting in %d seconds", modconf->reconnect);
    
    event = nx_event_new();
    event->module = module;
    if ( modconf->reconnect == 0 )
    {
	event->delayed = FALSE;
    }
    else
    {
	event->delayed = TRUE;
	event->time = apr_time_now() + APR_USEC_PER_SEC * modconf->reconnect;
    }
    event->type = NX_EVENT_RECONNECT;
    event->priority = module->priority;
    nx_event_add(event);
}



static void om_http_timeout_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_om_http_conf_t *modconf;

    modconf = (nx_om_http_conf_t *) module->config;
 
    ASSERT(modconf->timeout_event == NULL);

    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->type = NX_EVENT_TIMEOUT;
    event->time = apr_time_now() + APR_USEC_PER_SEC * OM_HTTP_TIMEOUT_SEC;
    event->priority = module->priority;
    nx_event_add(event);
    modconf->timeout_event = event;
}



static void om_http_disconnect(nx_module_t *module, boolean reconnect)
{
    nx_om_http_conf_t *modconf;

    ASSERT(module != NULL);

    modconf = (nx_om_http_conf_t *) module->config;

    log_debug("om_http_disconnect");

    modconf->connected = FALSE;
    if ( modconf->sock != NULL )
    {
	nx_module_pollset_remove_socket(module, modconf->sock);
	apr_socket_close(modconf->sock);
	modconf->sock = NULL;
    }

    if ( modconf->ssl != NULL )
    {
	nx_ssl_destroy(&(modconf->ssl));
	modconf->ssl = NULL;
    }

    apr_pool_clear(modconf->pool);

    if ( reconnect == TRUE )
    {
	if ( (modconf->reqbufsize > 0) || (modconf->resp_wait == TRUE) )
	{ // reconnect if we still have data to process
	    om_http_add_reconnect_event(module);
	}
    }
}



static void om_http_reset(nx_module_t *module)
{
    nx_om_http_conf_t *modconf;

    ASSERT(module != NULL);

    modconf = (nx_om_http_conf_t *) module->config;

    modconf->logdata = NULL;

    if ( modconf->bio_resp_head != NULL )
    {
	BIO_free_all(modconf->bio_resp_head);
	modconf->bio_resp_head = NULL;
    }

    if ( modconf->bio_resp_body != NULL )
    {
	BIO_free_all(modconf->bio_resp_body);
	modconf->bio_resp_body = NULL;
    }

    modconf->resp_state = NX_OM_HTTP_RESP_STATE_START;
    modconf->content_length = 0;
    modconf->got_resp_body = FALSE;
    modconf->got_resp_head = FALSE;
    if ( modconf->timeout_event != NULL )
    {
	nx_event_remove(modconf->timeout_event);
	nx_event_free(modconf->timeout_event);
	modconf->timeout_event = NULL;
    }
    
    // POLLIN is to detect disconnection
    if ( modconf->sock != NULL )
    {
	nx_module_pollset_add_socket(module, modconf->sock, APR_POLLIN | APR_POLLHUP);
    }
}



static void om_http_create_request(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_om_http_conf_t *modconf;
    BIO *httpbio = NULL;
    char tmpstr[10];
    nx_exception_t e;

    ASSERT ( logdata != NULL );

    modconf = (nx_om_http_conf_t *) module->config;

    ASSERT(modconf->bio_req == NULL);
    try
    {

	if ( (httpbio = BIO_new(BIO_s_mem())) == NULL )
	{
	    throw_sslerror("BIO_new() failed");
	}

	BIO_puts(httpbio, "POST ");
	BIO_puts(httpbio, modconf->server.path);
	BIO_puts(httpbio, " HTTP/1.1\r\n");
	BIO_puts(httpbio, "User-Agent: " PACKAGE "\r\n");
	BIO_puts(httpbio, "Host: ");
	BIO_puts(httpbio, modconf->server.host);
	BIO_puts(httpbio, ":");
	apr_snprintf(tmpstr, sizeof(tmpstr), "%d", modconf->server.port);
	BIO_puts(httpbio, tmpstr);
	BIO_puts(httpbio, "\r\n");
	BIO_puts(httpbio, "Content-Type: ");
	BIO_puts(httpbio, modconf->content_type);
	BIO_puts(httpbio, "\r\n");
	BIO_puts(httpbio, "Connection: Keep-Alive\r\n");
	BIO_puts(httpbio, "Keep-Alive: 300\r\n");
	BIO_puts(httpbio, "Content-Length: ");
	ASSERT(logdata->raw_event != NULL);
	apr_snprintf(tmpstr, sizeof(tmpstr), "%d", (int) logdata->raw_event->len);
	BIO_puts(httpbio, tmpstr);
	BIO_puts(httpbio, "\r\n\r\n");
	BIO_write(httpbio, logdata->raw_event->buf, (int) logdata->raw_event->len);
	BIO_free_all(modconf->bio_req);
	modconf->bio_req = httpbio;
	httpbio = NULL;
	modconf->reqbufsize = (apr_size_t) BIO_get_mem_data(modconf->bio_req, &(modconf->reqbuf));
    }
    catch(e)
    {
	BIO_free_all(modconf->bio_req);
	modconf->bio_req = NULL;
	rethrow(e);
    }
}



static void om_http_send_request(nx_module_t *module)
{
    apr_size_t nbytes;
    apr_status_t rv;
    nx_om_http_conf_t *modconf;
    int sslrv;
    int nbytes2;
    nx_exception_t e;

    modconf = (nx_om_http_conf_t *) module->config;
    
    log_debug("om_http_send_request, bytes remaining: %d", (int) modconf->reqbufsize);

    om_http_timeout_event(module); // add timeout

    while ( modconf->reqbufsize > 0 )
    {
	if ( modconf->server.https == TRUE )
	{
	    nbytes2 = (int) modconf->reqbufsize;
	    sslrv = nx_ssl_write(modconf->ssl, modconf->reqbuf, &nbytes2);
	    switch ( sslrv )
	    {
		case SSL_ERROR_NONE:
		    modconf->reqbufsize -= (apr_size_t) nbytes2;
		    modconf->reqbuf += (apr_size_t) nbytes2;
		    log_debug("om_http sent %d bytes", (int) nbytes2);
		    nx_module_pollset_add_socket(module, modconf->sock, APR_POLLIN | APR_POLLHUP);
		    nx_module_add_poll_event(module);
		    break;
		case SSL_ERROR_ZERO_RETURN: // disconnected
		    log_warn("https server disconnected while sending the request");
		    om_http_disconnect(module, TRUE);
		    om_http_reset(module);
		    break;
		case SSL_ERROR_WANT_WRITE:
		    nx_module_pollset_add_socket(module, modconf->sock, APR_POLLIN | APR_POLLOUT | APR_POLLHUP);
		    nx_module_add_poll_event(module);
		    break;
		case SSL_ERROR_WANT_READ:
		    nx_module_pollset_add_socket(module, modconf->sock, APR_POLLIN | APR_POLLHUP);
		    nx_module_add_poll_event(module);
		    break;
		default:
		    try
		    {
			nx_ssl_check_io_error(modconf->ssl, sslrv);
		    }
		    catch(e)
		    {
			om_http_disconnect(module, TRUE);
			om_http_reset(module);
			rethrow(e);
		    }
		    om_http_disconnect(module, TRUE);
		    om_http_reset(module);
		    break;
	    }
	}
	else
	{
	    nbytes = modconf->reqbufsize;

	    rv = apr_socket_send(modconf->sock, modconf->reqbuf, &nbytes);
	    modconf->reqbufsize -= nbytes;
	    modconf->reqbuf += nbytes;
	    if ( rv != APR_SUCCESS )
	    {
		if ( APR_STATUS_IS_EPIPE(rv) == TRUE )
		{
		    log_debug("om_http got EPIPE");
		    nx_module_stop_self(module);
		    break;
		}
		else if ( (APR_STATUS_IS_EINPROGRESS(rv) == TRUE) ||
			  (APR_STATUS_IS_EAGAIN(rv) == TRUE) )
		{
		    nx_module_pollset_add_socket(module, modconf->sock,
						 APR_POLLIN | APR_POLLOUT | APR_POLLHUP);
		    nx_module_add_poll_event(module);
		    break;
		}
		else
		{
		    throw(rv, "apr_socket_send failed");
		}
	    }
	    else
	    { // Sent OK
		log_debug("om_http sent %d bytes", (int) nbytes);
		nx_module_pollset_add_socket(module, modconf->sock,
					     APR_POLLIN | APR_POLLHUP);
		nx_module_add_poll_event(module);
	    }
	}
    }
}



static void om_http_stop(nx_module_t *module)
{
    nx_om_http_conf_t *modconf;

    ASSERT(module != NULL);

    log_debug("om_http_stop");
    ASSERT(module->config != NULL);
    modconf = (nx_om_http_conf_t *) module->config;

    if ( modconf->sock != NULL )
    {
	if ( modconf->connected == TRUE )
	{
	    log_debug("om_http closing socket");
	    nx_module_pollset_remove_socket(module, modconf->sock);
	    apr_socket_close(modconf->sock);
	    modconf->sock = NULL;
	}
    }
    if ( modconf->ssl != NULL )
    {
	nx_ssl_destroy(&(modconf->ssl));
	modconf->ssl = NULL;
    }

    modconf->connected = FALSE;
    apr_pool_clear(modconf->pool);
}



static void io_err_handler(nx_module_t *module, nx_exception_t *e) NORETURN;
static void io_err_handler(nx_module_t *module, nx_exception_t *e)
{
    ASSERT(e != NULL);
    ASSERT(module != NULL);

    nx_module_stop_self(module);
    om_http_stop(module);
    om_http_add_reconnect_event(module);
    rethrow(*e);
}



static void om_http_connect(nx_module_t *module)
{
    nx_om_http_conf_t *modconf;
    apr_sockaddr_t *sa;
    int sslrv;
    nx_exception_t e;

    ASSERT(module->config != NULL);

    modconf = (nx_om_http_conf_t *) module->config;

    modconf->connected = FALSE;
    
    log_debug("om_http_connect");

    try
    {
	CHECKERR_MSG(apr_sockaddr_info_get(&sa, modconf->server.host, APR_INET,
					   modconf->server.port, 0, modconf->pool),
		     "apr_sockaddr_info failed for %s:%d",
		     modconf->server.host, modconf->server.port);
	CHECKERR_MSG(apr_socket_create(&(modconf->sock), sa->family, SOCK_STREAM,
				       APR_PROTO_TCP, modconf->pool),
		     "couldn't create tcp socket");
	CHECKERR_MSG(apr_socket_opt_set(modconf->sock, APR_SO_NONBLOCK, 0),
		     "couldn't set SO_NONBLOCK on connecting socket");
	CHECKERR_MSG(apr_socket_timeout_set(modconf->sock, OM_HTTP_TIMEOUT_SEC * APR_USEC_PER_SEC),
		     "couldn't set socket timeout on connecting socket");
	
	log_info("connecting to %s:%d", modconf->server.host, modconf->server.port);
	
	CHECKERR_MSG(apr_socket_connect(modconf->sock, sa),
		     "couldn't connect to tcp socket on %s:%d", modconf->server.host,
		     modconf->server.port);

	if ( modconf->server.https == TRUE )
	{
	    ASSERT(modconf->ssl == NULL);
	    modconf->ssl = nx_ssl_from_socket(&(modconf->ssl_ctx), modconf->sock);
	    CHECKERR_MSG(apr_socket_opt_set(modconf->sock, APR_SO_NONBLOCK, 0),
			 "couldn't set SO_NONBLOCK on connecting socket");
	    CHECKERR_MSG(apr_socket_timeout_set(modconf->sock, OM_HTTP_TIMEOUT_SEC * APR_USEC_PER_SEC),
			 "couldn't set socket timeout on connecting socket");

	    if ( (sslrv = SSL_connect(modconf->ssl)) < 0 )
	    {
		nx_ssl_check_io_error(modconf->ssl, sslrv);
		throw_msg("SSL connect failed");
	    }
	}

	modconf->connected = TRUE;

	CHECKERR_MSG(apr_socket_opt_set(modconf->sock, APR_SO_NONBLOCK, 1),
		     "couldn't set SO_NONBLOCK on tcp socket");
	CHECKERR_MSG(apr_socket_timeout_set(modconf->sock, 0),
		     "couldn't set socket timeout on tcp socket");
	CHECKERR_MSG(apr_socket_opt_set(modconf->sock, APR_SO_KEEPALIVE, 1),
		     "couldn't set TCP_KEEPALIVE on connecting socket");
	modconf->reconnect = 0;

	// POLLIN is to detect disconnection
	nx_module_pollset_add_socket(module, modconf->sock, APR_POLLIN | APR_POLLHUP);
	log_debug("connected to server OK");

	nx_module_data_available(module);
    }
    catch(e)
    {
	if ( modconf->reconnect == 0 )
	{
	    modconf->reconnect = 1;
	}
	else
	{
	    modconf->reconnect *= 2;
	}
	if ( modconf->reconnect > 20 * 60 )
	{ // max 20 minutes
	    modconf->reconnect = 20 * 60;
	}
	apr_pool_clear(modconf->pool);
	io_err_handler(module, &e);
    }
}



static void om_http_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;
    nx_om_http_conf_t *modconf;

    log_debug("nx_om_http_data_available()");
    
    modconf = (nx_om_http_conf_t *) module->config;

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }

    if ( modconf->logdata == NULL )
    { // not processing something, dequeue a new logdata

	if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
	{
	    return;
	}
	modconf->logdata = logdata;
	om_http_create_request(module, logdata);
	modconf->resp_wait = TRUE;
     }

    if ( modconf->connected == FALSE )
    {
	if ( modconf->reconnect == 0 )
	{
	    om_http_connect(module);
	}
	return;
    }

    if ( modconf->reqbufsize > 0 )
    {
	om_http_send_request(module);
    }

    BIO_free_all(modconf->bio_req);
    modconf->bio_req = NULL;
}



static void om_http_check_resp_head(nx_module_t *module)
{
    nx_om_http_conf_t *modconf;
    char *ptr;
    int i;

    modconf = (nx_om_http_conf_t *) module->config;

    BIO_get_mem_data(modconf->bio_resp_head, &ptr);

    if ( strncmp(ptr, "HTTP/", 5) != 0 )
    {
	throw_msg("invalid HTTP response");
    }
    for ( ; *ptr != ' '; ptr++ );
    for ( ; *ptr == ' '; ptr++ );

    if ( !((strncmp(ptr, "201 ", 4) == 0) 
	   || (strncmp(ptr, "202 ", 4) == 0)
	   || (strncmp(ptr, "200 ", 4) == 0)) )
    {
	for ( i = 0; (ptr[i] != '\0') && (ptr[i] != '\r'); i++ );
	ptr[i] = '\0';
	throw_msg("HTTP response status is not OK: %s", ptr);
    }
    for ( i = 0; (ptr[i] != '\0') && (ptr[i] != '\r'); i++ );
    for ( ; (ptr[i] == '\r') || (ptr[i] == '\n'); i++ );
    ptr += i;

    for ( ; ; )
    {
	if ( strncasecmp(ptr, "Content-Length:", 15) == 0 )
	{
	    ptr += 15;
	    for ( ; *ptr == ' '; ptr++ );
	    modconf->content_length = (apr_size_t) atoi(ptr);
	    for ( ; (*ptr != '\0') && (*ptr != '\r'); ptr++ );
	}
	else
	{
	    for ( ; (*ptr != '\0') && (*ptr != '\r'); ptr++ );
	}
	if ( *ptr == '\r' )
	{
	    ptr++;
	}
	if ( *ptr == '\n' )
	{
	    ptr++;
	}
	
	if ( *ptr == '\0' )
	{
	    break;
	}
    }
/*
    if ( modconf->content_length == 0 )
    {
	throw_msg("Content-Length required in response");
    }
*/
    if ( modconf->content_length > 10000 )
    {
	throw_msg("Content-Length too large");
    }
}



static void om_http_read_response(nx_module_t *module)
{
    nx_om_http_conf_t *modconf;
    apr_status_t rv;
    const char *ptr;
    boolean got_eof = FALSE;
    apr_size_t len, i;
    int nbytes;
    int sslrv;
    nx_exception_t e;

    log_debug("nx_om_http_read_response()");
    
    modconf = (nx_om_http_conf_t *) module->config;

    if ( modconf->connected == FALSE )
    {
	return;
    }

    len = NX_OM_HTTP_RESPBUFSIZE;
    memset(modconf->respbuf, 0, len);

    if ( modconf->server.https == TRUE )
    {
	nbytes = NX_OM_HTTP_RESPBUFSIZE;
	sslrv = nx_ssl_read(modconf->ssl, modconf->respbuf, &nbytes);
	switch ( sslrv )
	{
	    case SSL_ERROR_NONE:
		break;
	    case SSL_ERROR_ZERO_RETURN: // disconnected
		log_warn("https server disconnected while reading the response");
		om_http_disconnect(module, TRUE);
		if ( nbytes == 0 )
		{
		    om_http_reset(module);
		    return;
		}
		break;
	    case SSL_ERROR_WANT_WRITE:
	    case SSL_ERROR_WANT_READ:
		log_debug("got EAGAIN for nonblocking read in module %s", module->name);
		break;
	    default:
		try
		{
		    nx_ssl_check_io_error(modconf->ssl, sslrv);
		}
		catch(e)
		{
		    om_http_disconnect(module, TRUE);
		    om_http_reset(module);
		    rethrow(e);
		}
		om_http_disconnect(module, TRUE);
		om_http_reset(module);
		return;
	}
	len = (apr_size_t) nbytes;
    }
    else
    {
	rv = apr_socket_recv(modconf->sock, modconf->respbuf, &len);
	log_debug("Module %s read %u bytes", module->name, (unsigned int) len);
	if ( rv != APR_SUCCESS )
	{
	    if ( APR_STATUS_IS_EOF(rv) )
	    {
		got_eof = TRUE;
	    }
	    else if ( APR_STATUS_IS_EAGAIN(rv) )
	    {
		log_debug("got EAGAIN for nonblocking read in module %s", module->name);
	    }
	    else
	    {
		throw(rv, "Module %s couldn't read from socket", module->name);
	    }
	}

	if ( got_eof == TRUE )
	{
	    log_warn("http server disconnected while reading the response");
	    om_http_disconnect(module, TRUE);
	    if ( len == 0 )
	    {
		om_http_reset(module);
		return;
	    }
	}
    }

    if ( (modconf->logdata == NULL) && (len > 0) )
    {
	om_http_reset(module);
	om_http_disconnect(module, TRUE);
	throw_msg("unexpected data from server (%d bytes)", (int) len);
    }

    if ( modconf->bio_resp_head == NULL )
    {
	if ( (modconf->bio_resp_head = BIO_new(BIO_s_mem())) == NULL )
	{
	    throw_sslerror("BIO_new() failed");
	}
    }

    if ( modconf->bio_resp_body == NULL )
    {
	if ( (modconf->bio_resp_body = BIO_new(BIO_s_mem())) == NULL )
	{
	    throw_sslerror("BIO_new() failed");
	}
    }

    if ( modconf->got_resp_head == FALSE )
    {
	ptr = modconf->respbuf;
	for ( i = 0; i < len; i++ )
	{ // seek to start of body
	    switch ( ptr[i] )
	    {
		case '\r':
		    switch ( modconf->resp_state )
		    {
			case NX_OM_HTTP_RESP_STATE_START:
			    modconf->resp_state = NX_OM_HTTP_RESP_STATE_CR;
			    break;
			case NX_OM_HTTP_RESP_STATE_CR_LF:
			    modconf->resp_state = NX_OM_HTTP_RESP_STATE_CR_LF_CR;
			    break;
			default:
			    modconf->resp_state = NX_OM_HTTP_RESP_STATE_START;
			    break;
		    }
		    break;
		case '\n':
		    switch ( modconf->resp_state )
		    {
			case NX_OM_HTTP_RESP_STATE_CR:
			    modconf->resp_state = NX_OM_HTTP_RESP_STATE_CR_LF;
			    break;
			case NX_OM_HTTP_RESP_STATE_CR_LF_CR:
			    modconf->resp_state = NX_OM_HTTP_RESP_STATE_CR_LF_CR_LF;
			    break;
			default:
			    modconf->resp_state = NX_OM_HTTP_RESP_STATE_START;
			    break;
		    }
		    break;
		case '\0':
		    throw_msg("invalid HTTP response, zero byte found");
		default:
		    modconf->resp_state = NX_OM_HTTP_RESP_STATE_START;
		    break;
	    }
	
	    if ( modconf->resp_state == NX_OM_HTTP_RESP_STATE_CR_LF_CR_LF )
	    {
		break;
	    }
	}
	
	if ( modconf->resp_state == NX_OM_HTTP_RESP_STATE_CR_LF_CR_LF )
	{
	    BIO_write(modconf->bio_resp_head, ptr, (int) i);
	    BIO_write(modconf->bio_resp_head, "\0", 1);
	    modconf->got_resp_head = TRUE;
	    om_http_check_resp_head(module);
	    BIO_write(modconf->bio_resp_body, ptr + i, (int) (len - i));
	    modconf->resp_state = NX_OM_HTTP_RESP_STATE_START;
	    if ( modconf->content_length <= (apr_size_t) BIO_get_mem_data(modconf->bio_resp_body, &ptr) )
	    {
		modconf->got_resp_body = TRUE;
	    }
	}
	else
	{
	    BIO_write(modconf->bio_resp_head, ptr, (int) i);
	}
    }
    else
    {
	BIO_write(modconf->bio_resp_body, modconf->respbuf, (int) len);
	if ( modconf->content_length <= (apr_size_t) BIO_get_mem_data(modconf->bio_resp_body, &ptr) )
	{
	    modconf->got_resp_body = TRUE;
	}
    }

    if ( (modconf->got_resp_body == TRUE) && (modconf->got_resp_head == TRUE) )
    {
	log_debug("om_http received and parsed response");

	modconf->resp_wait = FALSE;

	if (modconf->logdata != NULL)
	{
	    nx_module_logqueue_pop(module, modconf->logdata);
	    nx_logdata_free(modconf->logdata);
	}
	om_http_reset(module);
    }
    nx_module_data_available(module);
    nx_module_add_poll_event(module);
}



static void om_http_parse_url(apr_pool_t *pool,
			      const char *url,
			      nx_om_http_server_t *server)
{
    const char *ptr;
    apr_port_t port = 0;
    char portstr[10];
    unsigned int i;

    ASSERT(url != NULL);
    ASSERT(server != NULL);

    server->url = url;

    if ( strncasecmp(url, "http://", 7) == 0 )
    {
	port = 80;
	i = 7;
    }
    else if ( strncasecmp(url, "https://", 8) == 0 )
    {
	port = 443;
	server->https = TRUE;
	i = 8;
    }
    else
    {
	throw_msg("invalid url: %s", url);
    }
    
    for ( ptr = url + i;
	  (*ptr != '\0') && (*ptr != '/') && (*ptr != ':');
	  ptr++ );
    server->host = apr_pstrndup(pool, url + i, (size_t) (ptr - (url + i)));

    if ( *ptr == ':' )
    {
	ptr++;
	for ( i = 0; apr_isdigit(*ptr); i++, ptr++ )
	{
	    if ( i >= sizeof(portstr) )
	    {
		throw_msg("invalid port [%s]", portstr);
	    }
	    portstr[i] = *ptr;
	}
	portstr[i] = '\0';
	if ( atoi(portstr) == 0 )
	{
	    throw_msg("invalid port [%s]", portstr);
	}
	port = (apr_port_t) atoi(portstr);
    }

    if ( *ptr == '/' )
    {
	// TODO: URLencode
	apr_cpystrn(server->path, ptr, sizeof(server->path));
    }
    else
    {
	server->path[0] = '/';
	server->path[1] = '\0';
    }
    server->port = port;

    log_debug("host: [%s], port: %d, path: [%s]", server->host, server->port, server->path);
}



static void om_http_config(nx_module_t *module)
{
    const nx_directive_t * volatile curr;
    nx_om_http_conf_t * volatile modconf;
    const char *url;
    nx_exception_t e;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_om_http_conf_t));
    module->config = modconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "url") == 0 )
	{
	    if ( modconf->server.url != NULL )
	    {
		nx_conf_error(curr, "URL already defined");
	    }

	    url = apr_pstrdup(module->pool, curr->args);
	    try
	    {
		om_http_parse_url(module->pool, url, &(modconf->server));
	    }
	    catch(e)
	    {
		log_exception(e);
		nx_conf_error(curr, "Failed to parse url %s", url);
	    }
	}
	else if ( strcasecmp(curr->directive, "httpscertfile") == 0 )
	{
	    if ( modconf->ssl_ctx.certfile != NULL )
	    {
		nx_conf_error(curr, "HTTPSCertFile is already defined");
	    }
	    modconf->ssl_ctx.certfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "httpscertkeyfile") == 0 )
	{
	    if ( modconf->ssl_ctx.certkeyfile != NULL )
	    {
		nx_conf_error(curr, "HTTPSCertKeyFile is already defined");
	    }
	    modconf->ssl_ctx.certkeyfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "httpskeypass") == 0 )
	{
	    if ( modconf->ssl_ctx.keypass != NULL )
	    {
		nx_conf_error(curr, "HTTPSKeyPass is already defined");
	    }
	    modconf->ssl_ctx.keypass = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "httpscafile") == 0 )
	{
	    if ( modconf->ssl_ctx.cafile != NULL )
	    {
		nx_conf_error(curr, "HTTPSCAFile is already defined");
	    }
	    modconf->ssl_ctx.cafile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "httpscadir") == 0 )
	{
	    if ( modconf->ssl_ctx.cadir != NULL )
	    {
		nx_conf_error(curr, "HTTPSCADir is already defined");
	    }
	    modconf->ssl_ctx.cadir = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "httpscrlfile") == 0 )
	{
	    if ( modconf->ssl_ctx.crlfile != NULL )
	    {
		nx_conf_error(curr, "HTTPSCRLFile is already defined");
	    }
	    modconf->ssl_ctx.crlfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "httpscrldir") == 0 )
	{
	    if ( modconf->ssl_ctx.crldir != NULL )
	    {
		nx_conf_error(curr, "HTTPSCRLDir is already defined");
	    }
	    modconf->ssl_ctx.crldir = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "HTTPSRequireCert") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "HTTPSAllowUntrusted") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "ContentType") == 0 )
	{
	    if ( modconf->content_type != NULL )
	    {
		nx_conf_error(curr, "ContentType is already defined");
	    }
	    modconf->content_type = apr_pstrdup(module->pool, curr->args);
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( modconf->server.url == NULL )
    {
	nx_conf_error(curr, "URL missing");
    }

    if ( modconf->server.https == FALSE )
    { // not HTTPS
	curr = module->directives;
	if ( modconf->ssl_ctx.certfile != NULL )
	{
	    nx_conf_error(curr, "'HTTPSCertFile' is only valid for HTTPS");
	}
	if ( modconf->ssl_ctx.certkeyfile != NULL )
	{
	    nx_conf_error(curr, "'HTTPSCertKeyFile' is only valid for HTTPS");
	}
	if ( modconf->ssl_ctx.keypass != NULL )
	{
	    nx_conf_error(curr, "'HTTPSKeyPass' is only valid for HTTPS");
	}
	if ( modconf->ssl_ctx.cafile != NULL )
	{
	    nx_conf_error(curr, "'HTTPSCAFile' is only valid for HTTPS");
	}
	if ( modconf->ssl_ctx.cadir != NULL )
	{
	    nx_conf_error(curr, "'HTTPSCADir' is only valid for HTTPS");
	}
	if ( modconf->ssl_ctx.crlfile != NULL )
	{
	    nx_conf_error(curr, "'HTTPSCRLFile' is only valid for HTTPS");
	}
	if ( modconf->ssl_ctx.crldir != NULL )
	{
	    nx_conf_error(curr, "'HTTPSCRLDir' is only valid for HTTPS");
	}
    }
    else
    { // HTTPS
	modconf->ssl_ctx.require_cert = TRUE;
	modconf->ssl_ctx.allow_untrusted = FALSE;
	nx_cfg_get_boolean(module->directives, "HTTPSRequireCert",
			   &(modconf->ssl_ctx.require_cert));
	nx_cfg_get_boolean(module->directives, "HTTPSAllowUntrusted",
			   &(modconf->ssl_ctx.allow_untrusted));
    }

    if ( modconf->content_type == NULL )
    {
	modconf->content_type = "text/plain";
    }

    modconf->connected = FALSE;
}



static void om_http_init(nx_module_t *module)
{
    nx_om_http_conf_t *modconf;

    modconf = (nx_om_http_conf_t *) module->config;

    modconf->pool = nx_pool_create_child(module->pool);

    if ( modconf->server.https == TRUE )
    {
	nx_ssl_ctx_init(&(modconf->ssl_ctx), module->pool);
    }
    nx_module_pollset_init(module);
}



static void om_http_event(nx_module_t *module, nx_event_t *event)
{
    nx_om_http_conf_t *modconf;

    ASSERT(event != NULL);
    ASSERT(module != NULL);

    modconf = (nx_om_http_conf_t *) module->config;

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    om_http_data_available(module);
	    break;
	case NX_EVENT_READ:
	    om_http_read_response(module);
	    break;
	case NX_EVENT_WRITE:
	    om_http_send_request(module);
	    break;
	case NX_EVENT_RECONNECT:
	    nx_module_start_self(module);
	    modconf->reconnect = 0;
	    om_http_data_available(module);
	    break;
	case NX_EVENT_DISCONNECT:
	    if ( modconf->connected == TRUE )
	    { // the disconnection may have been already noticed
		om_http_disconnect(module, TRUE);
		om_http_reset(module);
		log_error("http server disconnected");
	    }
	    break;
	case NX_EVENT_TIMEOUT:
	    modconf->timeout_event = NULL;
	    om_http_reset(module);
	    om_http_disconnect(module, TRUE);
	    log_error("http response timeout from server");
	case NX_EVENT_POLL:
	    if ( (nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING) &&
		 (modconf->sock != NULL) )
	    {
		nx_module_pollset_poll(module, FALSE);
		nx_module_add_poll_event(module);
	    }
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}


extern nx_module_exports_t nx_module_exports_om_http;

NX_MODULE_DECLARATION nx_om_http_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_http_config,		// config
    NULL,			// start
    om_http_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_http_init,		// init
    NULL,			// shutdown
    om_http_event,		// event
    NULL,			// info
    &nx_module_exports_om_http, // exports
};
