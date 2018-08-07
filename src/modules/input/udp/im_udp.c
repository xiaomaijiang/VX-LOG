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

#include "im_udp.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_UDP_DEFAULT_HOST "localhost"
#define IM_UDP_DEFAULT_PORT 514


static void im_udp_close_socket(nx_module_t *module)
{
    if ( module->input.desc.s != NULL )
    {
	nx_module_pollset_remove_socket(module, module->input.desc.s);
	apr_socket_close(module->input.desc.s);
	module->input.desc.s = NULL;
    }
    apr_pool_clear(module->input.pool);
}



static void im_udp_read(nx_module_t *module)
{
    nx_logdata_t *logdata;
    apr_sockaddr_t *sa = NULL;
    char ipstr[64];
    apr_status_t rv;

    ASSERT(module != NULL);

    log_debug("im_udp_read");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    if ( (rv = nx_module_input_fill_buffer_from_socket(&(module->input))) != APR_SUCCESS )
    {
	if ( APR_STATUS_IS_EAGAIN(rv) )
	{
	    nx_module_add_poll_event(module);
	}
	else
	{
	    im_udp_close_socket(module);
	    throw(rv, "Module %s couldn't read from socket", module->name);
	}
    }

    if ( module->input.buflen == 0 ) 
    { // couldn't read anything
	return; 
    }

    sa = nx_module_input_data_get(&(module->input), "recv_from");
    ASSERT(sa != NULL);
#ifdef HAVE_APR_SOCKADDR_IP_GETBUF
    if ( (rv = apr_sockaddr_ip_getbuf(ipstr, sizeof(ipstr), sa)) != APR_SUCCESS )
    {
	log_aprerror(rv, "couldn't get remote IP address");
	apr_cpystrn(ipstr, "unknown", sizeof(ipstr));
    }
    else
    {
	log_debug("UDP log message received from %s", ipstr);
    }
#else
    apr_cpystrn(ipstr, "unknown", sizeof(ipstr));
#endif
    nx_module_input_name_set(&(module->input), ipstr);

    while ( (logdata = module->input.inputfunc->func(&(module->input), module->input.inputfunc->data)) != NULL )
    {
	//log_debug("read: [%s]", logdata->data);
	// FIXME use IP4ADDR/IP6DDR type
	nx_logdata_set_string(logdata, "MessageSourceAddress", ipstr);
	nx_module_add_logdata_input(module, &(module->input), logdata);
    }
}



static void im_udp_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_udp_conf_t *imconf;
    unsigned int port;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_udp_conf_t));
    module->config = imconf;

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
	else if ( strcasecmp(curr->directive, "sockbufsize") == 0 )
	{
	    if ( imconf->sockbufsize != 0 )
	    {
		nx_conf_error(curr, "SockBufSize is already defined");
	    }
	    if ( sscanf(curr->args, "%u", &(imconf->sockbufsize)) != 1 )
	    {
		nx_conf_error(curr, "invalid SockBufSize: %s", curr->args);
	    }
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
	imconf->inputfunc = nx_module_input_func_lookup("dgram");
    }
    ASSERT(imconf->inputfunc != NULL);

    if ( imconf->host == NULL )
    {
	imconf->host = apr_pstrdup(module->pool, IM_UDP_DEFAULT_HOST);
    }

    if ( imconf->port == 0 )
    {
	imconf->port = IM_UDP_DEFAULT_PORT;
    }

    module->input.pool = nx_pool_create_child(module->pool);
}



static void im_udp_start(nx_module_t *module)
{
    nx_im_udp_conf_t *imconf;
    apr_sockaddr_t *sa;
    nx_exception_t e;

    ASSERT(module->config != NULL);

    imconf = (nx_im_udp_conf_t *) module->config;

    try
    {
	if ( module->input.desc.s == NULL )
	{
	    CHECKERR_MSG(apr_socket_create(&(module->input.desc.s), APR_INET, SOCK_DGRAM,
					   APR_PROTO_UDP, module->input.pool),
			 "couldn't create udp socket");
	    module->input.desc_type = APR_POLL_SOCKET;
	    module->input.module = module;
	    module->input.inputfunc = imconf->inputfunc;

	    CHECKERR_MSG(apr_sockaddr_info_get(&sa, imconf->host, APR_INET, imconf->port, 
					       0, module->input.pool),
			 "apr_sockaddr_info failed for %s:%d", imconf->host, imconf->port);

	    CHECKERR_MSG(apr_socket_bind(module->input.desc.s, sa),
			 "couldn't bind udp socket to %s:%d", imconf->host, imconf->port);

	    CHECKERR_MSG(apr_socket_opt_set(module->input.desc.s, APR_SO_NONBLOCK, 1),
			 "couldn't set SO_NONBLOCK on udp socket");
	    CHECKERR_MSG(apr_socket_timeout_set(module->input.desc.s, 0),
			 "couldn't set socket timeout on udp socket");
	    if ( imconf->sockbufsize != 0 )
	    {
		CHECKERR_MSG(apr_socket_opt_set(module->input.desc.s, APR_SO_RCVBUF,
						imconf->sockbufsize),
			     "couldn't set SO_RCVBUF on udp socket");
	    }
	}
        else
	{
	    log_debug("udp socket already initialized");
	}

	nx_module_pollset_add_socket(module, module->input.desc.s, APR_POLLIN);
    }
    catch(e)
    {
	im_udp_close_socket(module);
	rethrow_msg(e, "failed to start im_udp");
    }

    nx_module_add_poll_event(module);

    log_debug("im_udp started");
}



static void im_udp_stop(nx_module_t *module)
{
    im_udp_close_socket(module);
}



static void im_udp_resume(nx_module_t *module)
{
    ASSERT(module != NULL);

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_STOPPED )
    {
	nx_module_add_poll_event(module);
    }
}



static void im_udp_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void im_udp_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_udp_read(module);
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



NX_MODULE_DECLARATION nx_im_udp_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    "CAP_NET_BIND_SERVICE",	// capabilities
    im_udp_config,		// config
    im_udp_start,		// start
    im_udp_stop, 		// stop
    NULL,			// pause
    im_udp_resume,		// resume
    im_udp_init,		// init
    NULL,			// shutdown
    im_udp_event,		// event
    NULL,			// info
    NULL,			// exports
};
