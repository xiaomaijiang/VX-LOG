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

#include "om_udp.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define OM_UDP_DEFAULT_PORT 514
#define OM_UDP_DEFAULT_CONNECT_TIMEOUT (APR_USEC_PER_SEC * 30)
#define OM_UDP_MAX_RECONNECT_INTERVAL 200


static void om_udp_add_reconnect_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_om_udp_conf_t *omconf;

    omconf = (nx_om_udp_conf_t *) module->config;

    if ( omconf->reconnect == 0 )
    {
	omconf->reconnect = 1;
    }
    else
    {
	omconf->reconnect *= 2;
    }
    if ( omconf->reconnect > OM_UDP_MAX_RECONNECT_INTERVAL )
    { // limit
	omconf->reconnect = OM_UDP_MAX_RECONNECT_INTERVAL;
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



static void om_udp_write(nx_module_t *module)
{
    nx_om_udp_conf_t *omconf;
    nx_logdata_t *logdata;
    apr_size_t nbytes;
    boolean done = FALSE;
    apr_status_t rv;

    ASSERT(module != NULL);

    log_debug("om_udp_write");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s is not running, not reading any more data", module->name);
	return;
    }

    omconf = (nx_om_udp_conf_t *) module->config;

    do
    {
	if ( module->output.buflen > 0 )
	{
	    nbytes = module->output.buflen;
	    ASSERT(omconf->sock != NULL);
	    if ( (rv = apr_socket_send(omconf->sock, module->output.buf + module->output.bufstart,
				       &nbytes)) != APR_SUCCESS )
	    {
		if ( APR_STATUS_IS_EPIPE(rv) == TRUE )
		{ // possible for udp??
		    log_debug("om_udp got EPIPE");
		    done = TRUE;
		}
		else if ( (APR_STATUS_IS_EINPROGRESS(rv) == TRUE) ||
			  (APR_STATUS_IS_EAGAIN(rv) == TRUE) )
		{
		    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLOUT);
		    nx_module_add_poll_event(module);
		    done = TRUE;
		}
		else
		{
		    throw(rv, "om_udp apr_socket_send failed");
		}
	    }
	    else
	    {
		log_debug("om_udp sent %d bytes", (int) nbytes);
		if ( nbytes < module->output.buflen )
		{
		    log_debug("om_udp sent less than requested");
		    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLOUT);
		    nx_module_add_poll_event(module);
		    done = TRUE;
		}
	    }
	    ASSERT(nbytes <= module->output.buflen);
	    module->output.bufstart += nbytes;
	    module->output.buflen -= nbytes;
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



static void om_udp_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_om_udp_conf_t *omconf;
    unsigned int port;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    omconf = apr_pcalloc(module->pool, sizeof(nx_om_udp_conf_t));
    module->config = omconf;

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
	else if ( strcasecmp(curr->directive, "sockbufsize") == 0 )
	{
	    if ( omconf->sockbufsize != 0 )
	    {
		nx_conf_error(curr, "SockBufSize is already defined");
	    }
	    if ( sscanf(curr->args, "%u", &(omconf->sockbufsize)) != 1 )
	    {
		nx_conf_error(curr, "invalid SockBufSize: %s", curr->args);
	    }
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
	module->output.outputfunc = nx_module_output_func_lookup("dgram");
    }
    ASSERT(module->output.outputfunc != NULL);
    if ( omconf->host == NULL )
    {
	nx_conf_error(module->directives, "Mandatory 'Host' parameter missing");
    }
    if ( omconf->port == 0 )
    {
	omconf->port = OM_UDP_DEFAULT_PORT;
    }
}



static void om_udp_stop(nx_module_t *module)
{
    nx_om_udp_conf_t *omconf;
    apr_pool_t *pool;

    ASSERT(module->config != NULL);

    omconf = (nx_om_udp_conf_t *) module->config;

    if ( omconf->sock != NULL )
    {
	nx_module_pollset_remove_socket(module, omconf->sock);
	pool = apr_socket_pool_get(omconf->sock);
	apr_pool_destroy(pool);
	omconf->sock = NULL;
    }
}



static void om_udp_connect(nx_module_t *module)
{
    nx_om_udp_conf_t *omconf;
    apr_sockaddr_t *sa;
    apr_pool_t *pool = NULL;
    int i;
    apr_status_t rv;

    ASSERT(module->config != NULL);
    omconf = (nx_om_udp_conf_t *) module->config;

    if ( omconf->sock == NULL )
    {
	pool = nx_pool_create_child(module->pool);
	CHECKERR_MSG(apr_sockaddr_info_get(&sa, omconf->host, APR_INET, omconf->port, 
					   0, pool),
		     "apr_sockaddr_info failed for %s:%d", omconf->host, omconf->port);
	CHECKERR_MSG(apr_socket_create(&(omconf->sock), sa->family, SOCK_DGRAM,
				       APR_PROTO_UDP, pool),
		     "couldn't create udp socket");
	CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_NONBLOCK, 1),
		     "couldn't set SO_NONBLOCK on connecting socket");
	CHECKERR_MSG(apr_socket_timeout_set(omconf->sock, OM_UDP_DEFAULT_CONNECT_TIMEOUT),
		     "couldn't set socket timeout on connecting socket");

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
	CHECKERR_MSG(rv, "couldn't connect to udp socket on %s:%d", omconf->host, omconf->port);


	CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_NONBLOCK, 1),
		     "couldn't set SO_NONBLOCK on udp socket");
	CHECKERR_MSG(apr_socket_timeout_set(omconf->sock, 0),
		     "couldn't set socket timeout on udp socket");
	if ( omconf->sockbufsize != 0 )
	{
	    CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_SNDBUF, omconf->sockbufsize),
			 "couldn't set SO_SNDBUF on udp socket");
	}
	omconf->reconnect = 0;
    }
    else
    {
	log_debug("udp socket already initialized");
    }

}



static void om_udp_start(nx_module_t *module)
{
    om_udp_connect(module);
    log_debug("om_udp started");
}



static void om_udp_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void io_err_handler(nx_module_t *module, nx_exception_t *e) NORETURN;
static void io_err_handler(nx_module_t *module, nx_exception_t *e)
{
    ASSERT(e != NULL);
    ASSERT(module != NULL);

    nx_module_stop_self(module);
    om_udp_stop(module);
    om_udp_add_reconnect_event(module);
    rethrow(*e);
}



static void om_udp_event(nx_module_t *module, nx_event_t *event)
{
    nx_exception_t e;

    ASSERT(event != NULL);

    switch ( event->type )
    {
        case NX_EVENT_RECONNECT:
	    nx_module_start_self(module);
	    break;
        case NX_EVENT_WRITE:
	    om_udp_write(module);
	    break;
	case NX_EVENT_DATA_AVAILABLE:
	    try
	    {
		om_udp_write(module);
	    }
	    catch(e)
	    {
		io_err_handler(module, &e);
	    }
	    break;
	case NX_EVENT_POLL:
	    if ( nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING )
	    {
		nx_module_pollset_poll(module, FALSE);
	    }
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_om_udp_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_udp_config,		// config
    om_udp_start,		// start
    om_udp_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_udp_init,		// init
    NULL,			// shutdown
    om_udp_event,		// event
    NULL,			// info
    NULL,			// exports
};
