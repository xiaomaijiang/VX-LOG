/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_portable.h>

#include <unistd.h>

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/ssl.h"
#include "../../../common/alloc.h"

#include "im_ssl.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_SSL_DEFAULT_HOST "localhost"
#define IM_SSL_DEFAULT_PORT 514



static void im_ssl_listen(nx_module_t *module)
{
    nx_im_ssl_conf_t *imconf;
    nx_exception_t e;

    imconf = (nx_im_ssl_conf_t *) module->config;

    try
    {
	if ( imconf->listensock == NULL )
	{
	    apr_sockaddr_t *sa;

	    CHECKERR_MSG(apr_socket_create(&(imconf->listensock), APR_INET, SOCK_STREAM,
					   APR_PROTO_TCP, module->input.pool),
			 "couldn't create tcp socket");

	    CHECKERR_MSG(apr_sockaddr_info_get(&sa, imconf->host, APR_INET, imconf->port, 
					       0, module->input.pool),
			 "apr_sockaddr_info failed for %s:%d", imconf->host, imconf->port);
	
	    CHECKERR_MSG(apr_socket_opt_set(imconf->listensock, APR_SO_NONBLOCK, 1),
			 "couldn't set SO_NONBLOCK on listen socket");
	    CHECKERR_MSG(apr_socket_timeout_set(imconf->listensock, 0),
			 "couldn't set socket timeout on listen socket");
	    CHECKERR_MSG(apr_socket_opt_set(imconf->listensock, APR_SO_REUSEADDR, 1),
			 "couldn't set SO_REUSEADDR on listen socket");
	    CHECKERR_MSG(apr_socket_opt_set(imconf->listensock, APR_TCP_NODELAY, 1),
			 "couldn't set TCP_NODELAY on listen socket");
	    CHECKERR_MSG(apr_socket_bind(imconf->listensock, sa),
			 "couldn't bind ssl socket to %s:%d", imconf->host, imconf->port);
	}
        else
	{
	    log_debug("ssl socket already initialized");
	}

	CHECKERR_MSG(apr_socket_listen(imconf->listensock, SOMAXCONN),
		     "couldn't listen to ssl socket on %s:%d",
		     imconf->host, imconf->port);
    
	nx_module_pollset_add_socket(module, imconf->listensock, APR_POLLIN | APR_POLLHUP);
    }
    catch(e)
    {
	if ( imconf->listensock != NULL )
	{
	    apr_socket_close(imconf->listensock);
	    imconf->listensock = NULL;
	}
	apr_pool_clear(module->input.pool);
	rethrow(e);
    }
}



static void im_ssl_free_input(nx_module_input_t *input)
{
    nx_im_ssl_conf_t *imconf;
    SSL *ssl;

    imconf = (nx_im_ssl_conf_t *) input->module->config;

    if ( input->desc.s != NULL )
    {
	apr_socket_close(input->desc.s);
	nx_module_pollset_remove_socket(input->module, input->desc.s);
	nx_module_remove_events_by_data(input->module, input->desc.s);
	apr_socket_data_set(input->desc.s, NULL, "input", NULL);
	NX_DLIST_REMOVE(imconf->connections, input, link);

	ssl = nx_module_input_data_get(input, "ssl");
	nx_ssl_destroy(&ssl);
	nx_module_input_free(input);
    }
}



static void im_ssl_accept(nx_module_t *module)
{
    nx_im_ssl_conf_t *imconf;
    apr_socket_t *sock;
    apr_sockaddr_t *sa;
    char *ipstr;
    nx_module_input_t *input = NULL;
    SSL *ssl = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t rv;
    nx_exception_t e;

    log_debug("im_ssl_accept");
    
    imconf = (nx_im_ssl_conf_t *) module->config;

    pool = nx_pool_create_child(module->pool);
    try
    {
	if ( (rv = apr_socket_accept(&sock, imconf->listensock, pool)) != APR_SUCCESS )
	{
	    if ( APR_STATUS_IS_EAGAIN(rv) )
	    {
		nx_module_add_poll_event(module);
		apr_pool_destroy(pool);
	    }
	    else
	    {
		throw(rv, "couldn't accept connection on %s:%u (statuscode: %d)",
		      imconf->host, imconf->port, rv);
	    }
	}

	if ( rv == APR_SUCCESS )
	{
	    CHECKERR_MSG(apr_socket_opt_set(sock, APR_SO_NONBLOCK, 1),
			 "couldn't set SO_NONBLOCK on accepted socket");
	    CHECKERR_MSG(apr_socket_timeout_set(sock, 0),
			 "couldn't set socket timeout on accepted socket");
	    CHECKERR_MSG(apr_socket_opt_set(sock, APR_SO_KEEPALIVE, 1),
			 "couldn't set TCP_KEEPALIVE on accepted socket");
	    CHECKERR_MSG(apr_socket_addr_get(&sa, APR_REMOTE, sock),
			 "couldn't get info on accepted socket");
	    CHECKERR_MSG(apr_sockaddr_ip_get(&ipstr, sa),
			 "couldn't get IP of accepted socket");

	    nx_module_pollset_add_socket(module, imconf->listensock, APR_POLLIN | APR_POLLHUP);
	    
	    ssl = nx_ssl_from_socket(&(imconf->ssl_ctx), sock);
	    ASSERT(ssl != NULL);
	    SSL_set_accept_state(ssl);
	    //SSL_accept(ssl);
    
	    CHECKERR_MSG(apr_socket_opt_set(sock, APR_SO_NONBLOCK, 1),
			 "couldn't set SO_NONBLOCK on accepted socket");
	    CHECKERR_MSG(apr_socket_timeout_set(sock, 0),
			 "couldn't set socket timeout on accepted socket");

	    input = nx_module_input_new(module, pool);
	    input->desc_type = APR_POLL_SOCKET;
	    input->desc.s = sock;
	    input->inputfunc = imconf->inputfunc;
	    ASSERT(input->inputfunc != NULL);
    
	    nx_module_input_data_set(input, "ssl", ssl);
		
	    CHECKERR_MSG(apr_socket_data_set(sock, input, "input", NULL),
			 "couldn't set data on socket");
	    
	    nx_module_pollset_add_socket(module, sock, APR_POLLIN | APR_POLLHUP);
	    log_info("SSL connection accepted from %s:%u", ipstr, sa->port);

	    nx_module_input_data_set(input, "recv_from_str", ipstr);
	    NX_DLIST_INSERT_TAIL(imconf->connections, input, link);
	}
    }
    catch(e)
    {
	if (ssl != NULL)
	{
	    nx_ssl_destroy(&ssl);
	}
	apr_pool_destroy(pool);
	rethrow(e);
    }
}



static void im_ssl_disconnect(nx_module_input_t *input)
{
    char *ipstr;
    apr_sockaddr_t *sa;

    log_debug("im_ssl got disconnect");

    CHECKERR_MSG(apr_socket_addr_get(&sa, APR_REMOTE, input->desc.s),
		 "couldn't get info on remote socket");
    CHECKERR_MSG(apr_sockaddr_ip_get(&ipstr, sa), "couldn't get IP of remote socket");
    log_warn("SSL connection closed from %s:%u", ipstr, sa->port);
    im_ssl_free_input(input);
}


// return TRUE if there was an error and we need to clean up the input
static boolean im_ssl_fill_buffer(nx_module_t *module, nx_module_input_t *input)
{
    volatile int retval;
    SSL *ssl;
    int nbytes;
    nx_exception_t e;

    ASSERT(input != NULL);
    ASSERT(input->desc_type == APR_POLL_SOCKET);
    ASSERT(input->desc.s != NULL);
    ASSERT(input->buf != NULL);

    ssl = (SSL *) nx_module_input_data_get(input, "ssl");
    ASSERT(ssl != NULL);

    if ( input->bufstart == input->bufsize )
    {
	input->bufstart = 0;
	input->buflen = 0;
    }
    if ( input->buflen == 0 )
    {
	input->bufstart = 0;
    }

    nbytes = (int) (input->bufsize - (input->buflen + input->bufstart));

    if ( nbytes > 0 )
    {
	try
	{
	    retval = nx_ssl_read(ssl, input->buf + input->bufstart + input->buflen, &nbytes);
	}
	catch(e)
	{
	    log_exception(e);
	    return ( TRUE );
	}
	ASSERT(nbytes <= (int) (input->bufsize - (input->buflen + input->bufstart)));
	input->buflen += nbytes;
	switch ( retval )
	{
	    case SSL_ERROR_NONE:
		log_debug("Module %s read %u bytes", input->module->name, (unsigned int) input->buflen);
		nx_module_pollset_add_socket(module, input->desc.s, APR_POLLIN | APR_POLLHUP);
		break;
	    case SSL_ERROR_ZERO_RETURN: // disconnected
		log_debug("remote ssl connection closed");
		return ( TRUE );
	    case SSL_ERROR_WANT_WRITE:
		log_debug("im_ssl WANT_WRITE");
		nx_module_pollset_add_socket(module, input->desc.s, APR_POLLOUT | APR_POLLHUP);
		break;
	    case SSL_ERROR_WANT_READ:
		log_debug("im_ssl WANT_READ");
		nx_module_pollset_add_socket(module, input->desc.s, APR_POLLIN | APR_POLLHUP);
		break;
	    default:
		log_error("im_ssl couldn't read, disconnecting (error code: %d)", retval);
		return ( TRUE );
	}
    }
    else
    {
	log_debug("im_ssl_fill_buffer called with full buffer");
    }

    return ( FALSE );
}



static void im_ssl_read(nx_module_t *module, nx_event_t *event)
{
    nx_im_ssl_conf_t *imconf;
    nx_logdata_t *logdata;
    apr_socket_t *sock;
    nx_module_input_t *input = NULL;
    char *ipstr;
    nx_exception_t e;
    boolean disconnect;

    ASSERT(module != NULL);
    ASSERT(event != NULL);

    log_debug("im_ssl_read");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    imconf = (nx_im_ssl_conf_t *) module->config;
    sock = (apr_socket_t *) event->data;

    ASSERT(imconf->listensock != NULL);

    if ( sock == imconf->listensock )
    {
	im_ssl_accept(module);
	return;
    }

    //else we have data from a client connection
    CHECKERR_MSG(apr_socket_data_get((void **) &input, "input", sock),
		 "couldn't get input data from socket");
    ASSERT(input != NULL); // if this is null there is a race/double free in event handling

    disconnect = im_ssl_fill_buffer(module, input);
    try
    {
	while ( (logdata = input->inputfunc->func(input, input->inputfunc->data)) != NULL )
	{
	    //log_debug("read: [%s]", logdata->data);
	    ipstr = nx_module_input_data_get(input, "recv_from_str");
	    nx_logdata_set_string(logdata, "MessageSourceAddress", ipstr);
	    nx_module_add_logdata_input(module, input, logdata);
	}
    }
    catch(e)
    {
	im_ssl_free_input(input);
	rethrow_msg(e, "Module %s couldn't read the input", input->module->name);
    }
    
    if ( disconnect == TRUE )
    {
	im_ssl_disconnect(input);
    }
}



static void im_ssl_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_ssl_conf_t *imconf;
    unsigned int port;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_ssl_conf_t));
    module->config = imconf;

    imconf->ssl_ctx.keypass = nx_module_get_resource(module, "keypass", NX_RESOURCE_TYPE_PASSWORD);

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "host") == 0 )
	{
	    if ( imconf->host != NULL )
	    {
		nx_conf_error(curr, "host is already defined");
	    }
	    imconf->host = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "port") == 0 )
	{
	    if ( imconf->port != 0 )
	    {
		nx_conf_error(curr, "port is already defined");
	    }
	    if ( sscanf(curr->args, "%u", &port) != 1 )
	    {
		nx_conf_error(curr, "invalid port: %s", curr->args);
	    }
	    imconf->port = (apr_port_t) port;
	}
	else if ( strcasecmp(curr->directive, "certfile") == 0 )
	{
	    if ( imconf->ssl_ctx.certfile != NULL )
	    {
		nx_conf_error(curr, "CertFile is already defined");
	    }
	    imconf->ssl_ctx.certfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "certkeyfile") == 0 )
	{
	    if ( imconf->ssl_ctx.certkeyfile != NULL )
	    {
		nx_conf_error(curr, "CertKeyFile is already defined");
	    }
	    imconf->ssl_ctx.certkeyfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "keypass") == 0 )
	{
	    if ( imconf->ssl_ctx.keypass != NULL )
	    {
		nx_conf_error(curr, "KeyPass is already defined");
	    }
	    imconf->ssl_ctx.keypass = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "cafile") == 0 )
	{
	    if ( imconf->ssl_ctx.cafile != NULL )
	    {
		nx_conf_error(curr, "CAFile is already defined");
	    }
	    imconf->ssl_ctx.cafile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "cadir") == 0 )
	{
	    if ( imconf->ssl_ctx.cadir != NULL )
	    {
		nx_conf_error(curr, "CADir is already defined");
	    }
	    imconf->ssl_ctx.cadir = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "crlfile") == 0 )
	{
	    if ( imconf->ssl_ctx.crlfile != NULL )
	    {
		nx_conf_error(curr, "CRLFile is already defined");
	    }
	    imconf->ssl_ctx.crlfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "crldir") == 0 )
	{
	    if ( imconf->ssl_ctx.crldir != NULL )
	    {
		nx_conf_error(curr, "CRLDir is already defined");
	    }
	    imconf->ssl_ctx.crldir = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "RequireCert") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "AllowUntrusted") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "InputType") == 0 )
	{
	    if ( imconf->inputfunc != NULL )
	    {
		nx_conf_error(curr, "InputType is already defined");
	    }

	    if ( curr->args != NULL )
	    {
		imconf->inputfunc = nx_module_input_func_lookup(curr->args);
	    }
	    if ( imconf->inputfunc == NULL )
	    {
		nx_conf_error(curr, "Invalid InputType '%s'", curr->args);
	    }
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( imconf->inputfunc == NULL )
    {
	imconf->inputfunc = nx_module_input_func_lookup("linebased");
    }
    ASSERT(imconf->inputfunc != NULL);

    if ( imconf->ssl_ctx.certfile == NULL )
    {
	nx_conf_error(module->directives, "'CertFile' missing for module im_ssl");
    }
    if ( imconf->ssl_ctx.certkeyfile == NULL )
    {
	nx_conf_error(module->directives, "'CertKeyFile' missing for module im_ssl");
    }
/*
    if ( imconf->ssl_ctx.keypass == NULL )
    {
	nx_conf_error(module->directives, "'KeyPass' missing for module im_ssl");
    }
*/
    imconf->ssl_ctx.require_cert = TRUE;
    imconf->ssl_ctx.allow_untrusted = FALSE;
    nx_cfg_get_boolean(module->directives, "RequireCert",
		       &(imconf->ssl_ctx.require_cert));
    nx_cfg_get_boolean(module->directives, "AllowUntrusted",
		       &(imconf->ssl_ctx.allow_untrusted));

    if ( imconf->host == NULL )
    {
	imconf->host = apr_pstrdup(module->pool, IM_SSL_DEFAULT_HOST);
    }
    if ( imconf->port == 0 )
    {
	imconf->port = IM_SSL_DEFAULT_PORT;
    }

    module->input.pool = nx_pool_create_child(module->pool);
    imconf->connections = apr_pcalloc(module->pool, sizeof(nx_module_input_list_t));
}



static void im_ssl_init(nx_module_t *module)
{
    nx_im_ssl_conf_t *imconf;

    ASSERT(module->config != NULL);

    imconf = (nx_im_ssl_conf_t *) module->config;

    nx_ssl_ctx_init(&(imconf->ssl_ctx), module->pool);
    nx_module_pollset_init(module);
}



static void im_ssl_start(nx_module_t *module)
{
    im_ssl_listen(module);
    nx_module_add_poll_event(module);
    log_debug("im_ssl started");
}



static void im_ssl_stop(nx_module_t *module)
{
    nx_im_ssl_conf_t *imconf;
    nx_module_input_t *conn;
    nx_module_input_t *tmp;

    ASSERT(module->config != NULL);

    imconf = (nx_im_ssl_conf_t *) module->config;

    for ( conn = NX_DLIST_FIRST(imconf->connections); conn != NULL; )
    {
	tmp = conn;
	conn = NX_DLIST_NEXT(conn, link);
	im_ssl_free_input(tmp);
    }
    nx_module_pollset_remove_socket(module, imconf->listensock);
    apr_socket_close(imconf->listensock);
    apr_pool_clear(module->input.pool);
    imconf->listensock = NULL;
}



static void im_ssl_resume(nx_module_t *module)
{
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_STOPPED )
    {
	nx_module_add_poll_event(module);
	//we could directly go to poll here
    }
}



static void im_ssl_write(nx_module_t *module, nx_event_t *event)
{
    int rv;
    SSL *ssl;
    apr_socket_t *sock;
    nx_module_input_t *input = NULL;
    int errcode;
    nx_exception_t e;

    sock = (apr_socket_t *) event->data;

    ASSERT(module != NULL);
    
    CHECKERR_MSG(apr_socket_data_get((void **) &input, "input", sock),
		 "couldn't get input data from socket");
    ASSERT(input != NULL);
    ssl = (SSL *) nx_module_input_data_get(input, "ssl");

    ASSERT(ssl != NULL);

    if ( !SSL_is_init_finished(ssl) )
    {
	log_debug("doing handshake");
	try
	{
	    if ( (rv = SSL_do_handshake(ssl)) <= 0 )
	    {
		switch ( (errcode = nx_ssl_check_io_error(ssl, rv)) )
		{
		    case SSL_ERROR_ZERO_RETURN: // disconnected
			throw_msg("im_ssl got disconnected during handshake");
			break;
		    case SSL_ERROR_WANT_WRITE:
			log_debug("im_ssl WANT_WRITE");
			nx_module_pollset_add_socket(module, input->desc.s, APR_POLLOUT | APR_POLLHUP);
			break;
		    case SSL_ERROR_WANT_READ:
			log_debug("im_ssl WANT_READ");
			nx_module_pollset_add_socket(module, input->desc.s, APR_POLLIN | APR_POLLHUP);
			break;
		    default:
			throw_msg("im_ssl couldn't write handshake data (error code: %d)", errcode);
		}
	    }
	}
	catch(e)
	{
	    log_exception(e);
	    im_ssl_disconnect(input);
	    return;
	}
    }
    else
    {
	log_warn("SSL socket should not be sending anything after the handshake");
	nx_module_pollset_add_socket(module, input->desc.s, APR_POLLIN | APR_POLLHUP);
    }
}



static void im_ssl_event(nx_module_t *module, nx_event_t *event)
{
    nx_module_input_t *input = NULL;

    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_ssl_read(module, event);
	    break;
	case NX_EVENT_WRITE:
	    im_ssl_write(module, event);
	    break;
	case NX_EVENT_DISCONNECT:
	    CHECKERR_MSG(apr_socket_data_get((void **) &input, "input",
					     (apr_socket_t *) event->data),
			 "couldn't get input data from socket");
	    im_ssl_disconnect(input);
	    break;
	case NX_EVENT_POLL:
	    if ( nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING )
	    {
		nx_module_pollset_poll(module, TRUE);
	    }
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_im_ssl_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    "CAP_NET_BIND_SERVICE",	// capabilities
    im_ssl_config,		// config
    im_ssl_start,		// start
    im_ssl_stop, 		// stop
    NULL,			// pause
    im_ssl_resume,		// resume
    im_ssl_init,		// init
    NULL,			// shutdown
    im_ssl_event,		// event
    NULL,			// info //FIXME info about connections
    NULL,			// exports
};
