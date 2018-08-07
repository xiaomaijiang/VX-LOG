/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_portable.h>

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/alloc.h"

#include "im_tcp.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_TCP_DEFAULT_HOST "localhost"
#define IM_TCP_DEFAULT_PORT 514


static void im_tcp_listen(nx_module_t *module)
{
    nx_im_tcp_conf_t *imconf;
    nx_exception_t e;

    imconf = (nx_im_tcp_conf_t *) module->config;

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
			 "couldn't bind tcp socket to %s:%d", imconf->host, imconf->port);
	}
        else
	{
	    log_debug("tcp socket already initialized");
	}

	CHECKERR_MSG(apr_socket_listen(imconf->listensock, SOMAXCONN),
		     "couldn't listen to tcp socket on %s:%d",
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



static void im_tcp_free_input(nx_module_input_t *input)
{
    nx_im_tcp_conf_t *imconf;

    imconf = (nx_im_tcp_conf_t *) input->module->config;

    ASSERT(input->desc.s != NULL);
    nx_module_pollset_remove_socket(input->module, input->desc.s);
    nx_module_remove_events_by_data(input->module, input->desc.s);
    NX_DLIST_REMOVE(imconf->connections, input, link);
    nx_module_input_free(input);
}



static void im_tcp_accept(nx_module_t *module)
{
    nx_im_tcp_conf_t *imconf;
    apr_socket_t *sock;
    apr_sockaddr_t *sa;
    char *ipstr;
    nx_module_input_t *input = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t rv;
    nx_exception_t e;

    log_debug("im_tcp_accept");
    
    imconf = (nx_im_tcp_conf_t *) module->config;

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

	    if ( imconf->nodelay == TRUE )
	    {
		CHECKERR_MSG(apr_socket_opt_set(sock, APR_TCP_NODELAY, 1),
			     "couldn't set TCP_NODELAY on accepted socket");
	    }

	    CHECKERR_MSG(apr_socket_addr_get(&sa, APR_REMOTE, sock),
			 "couldn't get info on accepted socket");
	    CHECKERR_MSG(apr_sockaddr_ip_get(&ipstr, sa),
			 "couldn't get IP of accepted socket");
	    log_info("connection accepted from %s:%u", ipstr, sa->port);

	    nx_module_pollset_add_socket(module, imconf->listensock, APR_POLLIN | APR_POLLHUP);
	    
	    input = nx_module_input_new(module, pool);
	    input->desc_type = APR_POLL_SOCKET;
	    input->desc.s = sock;
	    input->inputfunc = imconf->inputfunc;

	    CHECKERR_MSG(apr_socket_data_set(sock, input, "input", NULL),
			 "couldn't set data on socket");
	    NX_DLIST_INSERT_TAIL(imconf->connections, input, link);
	    nx_module_input_data_set(input, "recv_from_str", ipstr);

	    nx_module_pollset_add_socket(module, input->desc.s, APR_POLLIN | APR_POLLHUP);
	}
    }
    catch(e)
    {
	apr_pool_destroy(pool);
	rethrow(e);
    }
}



static void im_tcp_got_disconnect(nx_module_t *module, nx_event_t *event)
{
    apr_socket_t *sock;
    nx_module_input_t *input;
    apr_sockaddr_t *sa;
    char *ipstr;

    ASSERT(event != NULL);
    ASSERT(module != NULL);

    log_debug("im_tcp_disconnect");

    sock = (apr_socket_t *) event->data;

    CHECKERR_MSG(apr_socket_data_get((void **) &input, "input", sock),
		 "couldn't get input data from socket");
    CHECKERR_MSG(apr_socket_addr_get(&sa, APR_REMOTE, input->desc.s),
		 "couldn't get info on accepted socket");
    CHECKERR_MSG(apr_sockaddr_ip_get(&ipstr, sa),
		 "couldn't get IP of accepted socket");
    log_warn("TCP connection closed from %s:%u", ipstr, sa->port);
    im_tcp_free_input(input);
}



static void im_tcp_read(nx_module_t *module, nx_event_t *event)
{
    nx_im_tcp_conf_t *imconf;
    nx_logdata_t *logdata;
    boolean volatile got_eof = FALSE;
    apr_socket_t *sock;
    nx_module_input_t *input;
    apr_sockaddr_t *sa;
    char *ipstr;
    apr_status_t rv;
    nx_exception_t e;

    ASSERT(event != NULL);
    ASSERT(module != NULL);

    log_debug("im_tcp_read");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    imconf = (nx_im_tcp_conf_t *) module->config;
    sock = (apr_socket_t *) event->data;

    ASSERT(imconf->listensock != NULL);

    if ( sock == imconf->listensock )
    {
	im_tcp_accept(module);
	return;
    }
    
    //else we have data from a client connection
    CHECKERR_MSG(apr_socket_data_get((void **) &input, "input", sock),
		 "couldn't get input data from socket");
    ASSERT(input != NULL); // if this is null there is a race/double free in event handling

    if ( (rv = nx_module_input_fill_buffer_from_socket(input)) != APR_SUCCESS )
    {
	if ( APR_STATUS_IS_ECONNREFUSED(rv) ||
	     APR_STATUS_IS_ECONNABORTED(rv) ||
	     APR_STATUS_IS_ECONNRESET(rv) ||
	     APR_STATUS_IS_ETIMEDOUT(rv) ||
	     APR_STATUS_IS_TIMEUP(rv) ||
	     APR_STATUS_IS_EHOSTUNREACH(rv) ||
	     APR_STATUS_IS_ENETUNREACH(rv) ||
	     APR_STATUS_IS_EOF(rv) )
	{
	    got_eof = TRUE;
	}
	else if ( APR_STATUS_IS_EAGAIN(rv) )
	{
	    nx_module_add_poll_event(module);
	}
	else
	{
	    im_tcp_free_input(input);
	    throw(rv, "Module %s couldn't read from socket", input->module->name);
	}
    }

    try
    {
	while ( (logdata = input->inputfunc->func(input, input->inputfunc->data)) != NULL )
	{
	    //log_debug("read %d bytes:  [%s]", (int) logdata->datalen, logdata->data);
	    ipstr = nx_module_input_data_get(input, "recv_from_str");
	    nx_logdata_set_string(logdata, "MessageSourceAddress", ipstr);
	    nx_module_add_logdata_input(module, input, logdata);
	}
    }
    catch(e)
    {
	im_tcp_free_input(input);
	rethrow_msg(e, "Module %s couldn't read the input", input->module->name);
    }

    if ( got_eof == TRUE )
    {
	CHECKERR_MSG(apr_socket_addr_get(&sa, APR_REMOTE, input->desc.s),
		     "couldn't get info on accepted socket");
	CHECKERR_MSG(apr_sockaddr_ip_get(&ipstr, sa),
		     "couldn't get IP of accepted socket");
	log_aprwarn(rv, "TCP connection closed from %s:%u", ipstr, sa->port);
	im_tcp_free_input(input);
    }
}



static void im_tcp_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_tcp_conf_t *imconf;
    unsigned int port;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_tcp_conf_t));
    module->config = imconf;

    imconf->nodelay = TRUE;

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
	else if ( strcasecmp(curr->directive, "nodelay") == 0 )
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

    nx_cfg_get_boolean(module->directives, "nodelay", &(imconf->nodelay));
    if ( imconf->inputfunc == NULL )
    {
	imconf->inputfunc = nx_module_input_func_lookup("linebased");
    }
    ASSERT(imconf->inputfunc != NULL);

    if ( imconf->host == NULL )
    {
	imconf->host = apr_pstrdup(module->pool, IM_TCP_DEFAULT_HOST);
    }

    if ( imconf->port == 0 )
    {
	imconf->port = IM_TCP_DEFAULT_PORT;
    }

    module->input.pool = nx_pool_create_child(module->pool);
    imconf->connections = apr_pcalloc(module->pool, sizeof(nx_module_input_list_t));
}



static void im_tcp_start(nx_module_t *module)
{
    im_tcp_listen(module);
    nx_module_add_poll_event(module);
    log_debug("im_tcp started");
}



static void im_tcp_stop(nx_module_t *module)
{
    nx_im_tcp_conf_t *imconf;
    nx_module_input_t *conn;
    nx_module_input_t *tmp;

    ASSERT(module->config != NULL);
    imconf = (nx_im_tcp_conf_t *) module->config;

    for ( conn = NX_DLIST_FIRST(imconf->connections); conn != NULL; )
    {
	tmp = conn;
	conn = NX_DLIST_NEXT(conn, link);
	im_tcp_free_input(tmp);
    }
    if ( imconf->listensock != NULL )
    {
	nx_module_pollset_remove_socket(module, imconf->listensock);
	apr_socket_close(imconf->listensock);
	imconf->listensock = NULL;
    }
    apr_pool_clear(module->input.pool);
}



static void im_tcp_resume(nx_module_t *module)
{
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_STOPPED )
    {
	nx_module_pollset_poll(module, TRUE);
    }
}



static void im_tcp_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void im_tcp_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_tcp_read(module, event);
	    break;
	case NX_EVENT_DISCONNECT:
	    im_tcp_got_disconnect(module, event);
	    nx_module_add_poll_event(module);
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



NX_MODULE_DECLARATION nx_im_tcp_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    "CAP_NET_BIND_SERVICE",	// capabilities
    im_tcp_config,		// config
    im_tcp_start,		// start
    im_tcp_stop, 		// stop
    NULL,			// pause
    im_tcp_resume,		// resume
    im_tcp_init,		// init
    NULL,			// shutdown
    im_tcp_event,		// event
    NULL,			// info
    NULL,			// exports
};

