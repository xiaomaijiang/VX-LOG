/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/alloc.h"

#include "om_ssl.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define OM_SSL_DEFAULT_PORT 514
#define OM_SSL_DEFAULT_CONNECT_TIMEOUT (APR_USEC_PER_SEC * 30)
#define OM_SSL_MAX_RECONNECT_INTERVAL 200

static void om_ssl_add_reconnect_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_om_ssl_conf_t *omconf;

    omconf = (nx_om_ssl_conf_t *) module->config;

    if ( omconf->reconnect == 0 )
    {
	omconf->reconnect = 1;
    }
    else
    {
	omconf->reconnect *= 2;
    }
    if ( omconf->reconnect > OM_SSL_MAX_RECONNECT_INTERVAL )
    {
	omconf->reconnect = OM_SSL_MAX_RECONNECT_INTERVAL;
    }

    log_info("reconnecting in %d seconds", omconf->reconnect);

    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->type = NX_EVENT_RECONNECT;
    event->time = apr_time_now() + APR_USEC_PER_SEC * omconf->reconnect;
    event->priority = module->priority;
    nx_event_add(event);
}



static void om_ssl_stop(nx_module_t *module)
{
    nx_om_ssl_conf_t *omconf;
    apr_pool_t *pool;

    ASSERT(module->config != NULL);

    omconf = (nx_om_ssl_conf_t *) module->config;

    if ( omconf->sock != NULL )
    {
	nx_module_pollset_remove_socket(module, omconf->sock);
	pool = apr_socket_pool_get(omconf->sock);
	apr_pool_destroy(pool);
	omconf->sock = NULL;
	if ( omconf->ssl != NULL )
	{
	    nx_ssl_destroy(&(omconf->ssl));
	    omconf->ssl = NULL;
	}
    }
    omconf->connected = FALSE;
}



static boolean do_handshake(nx_module_t *module)
{
    nx_om_ssl_conf_t *omconf;
    int rv;
    int errcode;

    omconf = (nx_om_ssl_conf_t *) module->config;

    ASSERT(omconf->ssl != NULL);

    if ( !SSL_is_init_finished(omconf->ssl) )
    {
	log_debug("doing handshake");

	if ( (rv = SSL_do_handshake(omconf->ssl)) <= 0 )
	{
	    switch ( (errcode = nx_ssl_check_io_error(omconf->ssl, rv)) )
	    {
		case SSL_ERROR_ZERO_RETURN: // disconnected
		    log_info("remote socket was closed during SSL handshake");
		    nx_module_stop_self(module);
		    om_ssl_add_reconnect_event(module);
		    break;
		case SSL_ERROR_WANT_WRITE:
		    log_debug("om_ssl WANT_WRITE in handshake");
		    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLOUT | APR_POLLHUP);
		    break;
		case SSL_ERROR_WANT_READ:
		    log_debug("om_ssl WANT_READ in handshake");
		    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP);
		    break;
		default:
		    throw_msg("om_ssl couldn't write handshake data (error code: %d)", errcode);
	    }
	    return ( FALSE );
	}

	if ( SSL_is_init_finished(omconf->ssl) )
	{
	    log_debug("handshake successful");
	    // we need to read socket events, because POLLIN is also needed
	    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLOUT | APR_POLLHUP);

	    // if there is data, send it
	    nx_module_data_available(module);
	}
	else
	{
	    return ( FALSE );
	}
    }
    
    return ( TRUE );
}



static void om_ssl_write(nx_module_t *module)
{
    nx_om_ssl_conf_t *omconf;
    nx_logdata_t *logdata;
    int nbytes;
    boolean done = FALSE;
    int retval;

    ASSERT(module != NULL);

    log_debug("om_ssl_write");

    omconf = (nx_om_ssl_conf_t *) module->config;

    if ( omconf->ssl == NULL )
    { // not connected
	return;
    }

    if ( do_handshake(module) == FALSE )
    {
	return;
    }

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not writing any more data", module->name);
	return;
    }
 
    do
    {
	if ( module->output.buflen > 0 )
	{
	    nbytes = (int) module->output.buflen;
	    retval = nx_ssl_write(omconf->ssl, module->output.buf + module->output.bufstart, &nbytes);

	    switch ( retval )
	    {
		case SSL_ERROR_NONE:
		    if ( nbytes < (int) module->output.buflen )
		    {
			nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP);
			done = TRUE;
		    }
		    log_debug("om_ssl sent %d bytes", nbytes);
		    ASSERT(nbytes <= (int) module->output.buflen);
		    module->output.bufstart += (apr_size_t) nbytes;
		    module->output.buflen -= (apr_size_t) nbytes;
		    break;
		case SSL_ERROR_ZERO_RETURN: // disconnected
		    log_info("remote closed SSL socket during write");
		    nx_module_stop_self(module);
		    om_ssl_add_reconnect_event(module);
		    done = TRUE;
		    break;
		case SSL_ERROR_WANT_WRITE:
		    done = TRUE;
		    log_debug("om_ssl WANT_WRITE");
		    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLOUT | APR_POLLHUP);
		    break;
		case SSL_ERROR_WANT_READ:
		    log_debug("om_ssl WANT_READ");
		    done = TRUE;
		    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP);
		    break;
		default:
		    nx_module_stop_self(module);
		    om_ssl_add_reconnect_event(module);
		    return;
	    }
	    if ( module->output.buflen == 0 )
	    { // all bytes have been sucessfully written
		module->output.bufstart = 0;
		if ( module->output.logdata != NULL )
		{
		    nx_module_logqueue_pop(module, module->output.logdata);
		    nx_logdata_free(module->output.logdata);
		    module->output.logdata = NULL;
		}
	    }
	}
	else
	{
	    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP);
	}

	if ( module->output.buflen == 0 )
	{
	    if ( (logdata = nx_module_logqueue_peek(module)) != NULL )
	    {
		module->output.logdata = logdata;
		module->output.outputfunc->func(&(module->output),
						module->output.outputfunc->data);
		if ( module->output.buflen == 0 )
		{ // nothing to do in case the data is zero length or already dropped
		    module->output.bufstart = 0;
		    if ( module->output.logdata != NULL )
		    {
			nx_module_logqueue_pop(module, module->output.logdata);
			nx_logdata_free(module->output.logdata);
			module->output.logdata = NULL;
		    }
		}
	    }
	    else
	    {
		done = TRUE;
	    }
	}
    } while ( done != TRUE );
}



static void om_ssl_read(nx_module_t *module)
{
    nx_om_ssl_conf_t *omconf;
    int nbytes;
    char buf[100];
    int rv;

    // If we have a read event on the fd,
    // the remote socket disconnected or sent something
    // We close the socket in either case

    ASSERT(module != NULL);
    
    omconf = (nx_om_ssl_conf_t *) module->config;

    log_debug("om_ssl read");
    if ( omconf->connected != TRUE )
    {
	return;
    }

    if ( omconf->ssl == NULL )
    { // not connected
	return;
    }

    if ( do_handshake(module) == FALSE )
    {
	return;
    }

    nbytes = sizeof(buf);
    while ( (rv = nx_ssl_read(omconf->ssl, buf, &nbytes)) == SSL_ERROR_NONE )
    {
	log_debug("om_ssl_read %d bytes", (int) nbytes);
	nbytes = sizeof(buf);
    }
    switch ( rv )
    {
	case SSL_ERROR_ZERO_RETURN: // disconnected
	    log_info("remote closed SSL socket");
	    nx_module_stop_self(module);
	    om_ssl_add_reconnect_event(module);
	    break;
	case SSL_ERROR_WANT_WRITE:
	    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLOUT | APR_POLLHUP);
	    break;
	case SSL_ERROR_WANT_READ:
	    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP );
	    break;
	default:
	    throw_msg("om_ssl couldn't read");
    }
}



static void om_ssl_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_om_ssl_conf_t *omconf;
    unsigned int port;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    omconf = apr_pcalloc(module->pool, sizeof(nx_om_ssl_conf_t));
    module->config = omconf;

    omconf->ssl_ctx.keypass = nx_module_get_resource(module, "keypass", NX_RESOURCE_TYPE_PASSWORD);

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "host") == 0 )
	{
	    if ( omconf->host != NULL )
	    {
		nx_conf_error(curr, "host is already defined");
	    }
	    omconf->host = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "port") == 0 )
	{
	    if ( omconf->port != 0 )
	    {
		nx_conf_error(curr, "port is already defined");
	    }
	    if ( sscanf(curr->args, "%u", &port) != 1 )
	    {
		nx_conf_error(curr, "invalid port: %s", curr->args);
	    }
	    omconf->port = (apr_port_t) port;
	}
	else if ( strcasecmp(curr->directive, "Reconnect") == 0 )
	{
	    log_warn("The 'Reconnect' directive at %s:%d has been deprecated",
		     curr->filename, curr->line_num);
	}
	else if ( strcasecmp(curr->directive, "certfile") == 0 )
	{
	    if ( omconf->ssl_ctx.certfile != NULL )
	    {
		nx_conf_error(curr, "CertFile is already defined");
	    }
	    omconf->ssl_ctx.certfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "certkeyfile") == 0 )
	{
	    if ( omconf->ssl_ctx.certkeyfile != NULL )
	    {
		nx_conf_error(curr, "CertKeyFile is already defined");
	    }
	    omconf->ssl_ctx.certkeyfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "keypass") == 0 )
	{
	    if ( omconf->ssl_ctx.keypass != NULL )
	    {
		nx_conf_error(curr, "KeyPass is already defined");
	    }
	    omconf->ssl_ctx.keypass = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "cafile") == 0 )
	{
	    if ( omconf->ssl_ctx.cafile != NULL )
	    {
		nx_conf_error(curr, "CAFile is already defined");
	    }
	    omconf->ssl_ctx.cafile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "cadir") == 0 )
	{
	    if ( omconf->ssl_ctx.cadir != NULL )
	    {
		nx_conf_error(curr, "CADir is already defined");
	    }
	    omconf->ssl_ctx.cadir = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "crlfile") == 0 )
	{
	    if ( omconf->ssl_ctx.crlfile != NULL )
	    {
		nx_conf_error(curr, "CRLFile is already defined");
	    }
	    omconf->ssl_ctx.crlfile = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "crldir") == 0 )
	{
	    if ( omconf->ssl_ctx.crldir != NULL )
	    {
		nx_conf_error(curr, "CRLDir is already defined");
	    }
	    omconf->ssl_ctx.crldir = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "RequireCert") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "AllowUntrusted") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "OutputType") == 0 )
	{
	    if ( module->output.outputfunc != NULL )
	    {
		nx_conf_error(curr, "OutputType is already defined");
	    }

	    if ( curr->args != NULL )
	    {
		module->output.outputfunc = nx_module_output_func_lookup(curr->args);
	    }
	    if ( module->output.outputfunc == NULL )
	    {
		nx_conf_error(curr, "Invalid OutputType '%s'", curr->args);
	    }
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( module->output.outputfunc == NULL )
    {
	module->output.outputfunc = nx_module_output_func_lookup("linebased");
    }
    ASSERT(module->output.outputfunc != NULL);

    omconf->ssl_ctx.require_cert = TRUE;
    omconf->ssl_ctx.allow_untrusted = FALSE;
    nx_cfg_get_boolean(module->directives, "RequireCert", &(omconf->ssl_ctx.require_cert));
    nx_cfg_get_boolean(module->directives, "AllowUntrusted", &(omconf->ssl_ctx.allow_untrusted));

    if ( omconf->host == NULL )
    {
	nx_conf_error(module->directives, "Mandatory 'Host' parameter missing");
    }
    if ( omconf->port == 0 )
    {
	omconf->port = OM_SSL_DEFAULT_PORT;
    }
}



static void om_ssl_init(nx_module_t *module)
{
    nx_om_ssl_conf_t *omconf;

    ASSERT(module->config != NULL);

    omconf = (nx_om_ssl_conf_t *) module->config;

    nx_module_pollset_init(module);
    nx_ssl_ctx_init(&(omconf->ssl_ctx), module->pool);
    omconf->connected = FALSE;
}



static void om_ssl_connect(nx_module_t *module)
{
    nx_om_ssl_conf_t *omconf;
    apr_sockaddr_t *sa;
    apr_pool_t *pool = NULL;
    int i;
    apr_status_t rv;

    ASSERT(module->config != NULL);

    omconf = (nx_om_ssl_conf_t *) module->config;

    omconf->connected = FALSE;

    pool = nx_pool_create_child(module->pool);

    CHECKERR_MSG(apr_sockaddr_info_get(&sa, omconf->host, APR_INET, omconf->port, 0, pool),
		 "apr_sockaddr_info failed for %s:%d", omconf->host, omconf->port);
    CHECKERR_MSG(apr_socket_create(&(omconf->sock), sa->family, SOCK_STREAM, APR_PROTO_TCP,
				   pool), "couldn't create ssl socket");
    CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_NONBLOCK, 1),
		 "couldn't set SO_NONBLOCK on connecting socket");
    CHECKERR_MSG(apr_socket_timeout_set(omconf->sock, OM_SSL_DEFAULT_CONNECT_TIMEOUT),
		 "couldn't set socket timeout on connecting socket");
    log_info("connecting to %s:%d", omconf->host, omconf->port);
    
    for ( i = 0; i < 100; i++ )
    {
	rv = apr_socket_connect(omconf->sock, sa);
	if ( APR_STATUS_IS_EAGAIN(rv) )
	{
	    apr_sleep(100);
	}
	else
	{
	    break;
	}
    }
    CHECKERR_MSG(rv, "couldn't connect to ssl socket on %s:%d", omconf->host, omconf->port);

    log_debug("apr_socket_connect() succeeded");

    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLOUT | APR_POLLHUP);
    omconf->ssl = nx_ssl_from_socket(&(omconf->ssl_ctx), omconf->sock);
    
    CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_NONBLOCK, 1),
		 "couldn't set SO_NONBLOCK on connecting socket");
    CHECKERR_MSG(apr_socket_timeout_set(omconf->sock, 0),
		 "couldn't set socket timeout on connecting socket");
    CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_KEEPALIVE, 1),
                 "couldn't set TCP_KEEPALIVE on connecting socket");

    SSL_set_connect_state(omconf->ssl);

    log_info("successfully connected to %s:%d", omconf->host, omconf->port);
    omconf->connected = TRUE;
    omconf->reconnect = 0;
}



static void io_err_handler(nx_module_t *module, nx_exception_t *e) NORETURN;
static void io_err_handler(nx_module_t *module, nx_exception_t *e)
{
    ASSERT(e != NULL);
    ASSERT(module != NULL);

    nx_module_stop_self(module);
    om_ssl_stop(module);
    om_ssl_add_reconnect_event(module);
    rethrow(*e);
    
    /*
    //default:
    nx_module_stop_self(module);
    om_ssl_stop(module);
    rethrow_msg(*e, "[%s] fatal connection error, reconnection will not be attempted (statuscode: %d)",
		module->name, e->code);
    */
}



static void om_ssl_start(nx_module_t *module)
{
    nx_exception_t e;

    ASSERT(module->config != NULL);

    try
    {
	om_ssl_connect(module);
    }
    catch(e)
    {
	io_err_handler(module, &e);
    }
    nx_module_add_poll_event(module);
}



static void om_ssl_event(nx_module_t *module, nx_event_t *event)
{
    nx_exception_t e;

    ASSERT(event != NULL);

    switch ( event->type )
    {
        case NX_EVENT_RECONNECT:
	    nx_module_start_self(module);
	    break;
        case NX_EVENT_DISCONNECT:
	    nx_module_stop_self(module);
	    om_ssl_add_reconnect_event(module);
	    break;
        case NX_EVENT_READ:
	    try
	    {
		om_ssl_read(module);
	    }
	    catch(e)
	    {
		io_err_handler(module, &e);
	    }
	    break;
        case NX_EVENT_WRITE:
	    try
	    {
		om_ssl_write(module);
	    }
	    catch(e)
	    {
		io_err_handler(module, &e);
	    }
	    break;
	case NX_EVENT_DATA_AVAILABLE:
	    try
	    {
		om_ssl_write(module);
	    }
	    catch(e)
	    {
		io_err_handler(module, &e);
	    }
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


extern nx_module_exports_t nx_module_exports_om_ssl;

NX_MODULE_DECLARATION nx_om_ssl_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_ssl_config,		// config
    om_ssl_start,		// start
    om_ssl_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_ssl_init,		// init
    NULL,			// shutdown
    om_ssl_event,		// event
    NULL,			// info
    &nx_module_exports_om_ssl,  // exports
};
