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

#include "om_tcp.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define OM_TCP_DEFAULT_PORT 514
#define OM_TCP_DEFAULT_CONNECT_TIMEOUT (APR_USEC_PER_SEC * 30)
#define OM_TCP_MAX_RECONNECT_INTERVAL 200

static void om_tcp_accept(nx_module_t *module);
static void om_tcp_free_listener(nx_module_input_t *input);

static void om_tcp_add_reconnect_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_om_tcp_conf_t *omconf;

    omconf = (nx_om_tcp_conf_t *) module->config;

    if ( omconf->listen == TRUE )
    {
	return;
    }

    if ( omconf->reconnect == 0 )
    {
	omconf->reconnect = 1;
    }
    else
    {
	omconf->reconnect *= 2;
    }
    if ( omconf->reconnect > OM_TCP_MAX_RECONNECT_INTERVAL )
    { // limit
	omconf->reconnect = OM_TCP_MAX_RECONNECT_INTERVAL;
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



static void om_tcp_stop(nx_module_t *module)
{
    nx_om_tcp_conf_t *omconf;
    apr_pool_t *pool;

    ASSERT(module != NULL);

    log_debug("om_tcp_stop");
    ASSERT(module->config != NULL);
    omconf = (nx_om_tcp_conf_t *) module->config;

    if ( omconf->listen == TRUE )
    {
	nx_module_input_t *conn;
	nx_module_input_t *tmp;

	ASSERT(omconf->connections != NULL);
	for ( conn = NX_DLIST_FIRST(omconf->connections); conn != NULL; )
	{
	    tmp = conn;
	    conn = NX_DLIST_NEXT(conn, link);
	    om_tcp_free_listener(tmp);
	}
    }

    if ( omconf->sock != NULL )
    {
	if ( omconf->connected == TRUE )
	{
	    log_debug("om_tcp closing socket");
	    nx_module_pollset_remove_socket(module, omconf->sock);
	}
	pool = apr_socket_pool_get(omconf->sock);
	apr_pool_destroy(pool);
	omconf->sock = NULL;
    }
    omconf->connected = FALSE;
}



static void om_tcp_write(nx_module_t *module)
{
    nx_om_tcp_conf_t *omconf;
    nx_logdata_t *logdata;
    apr_size_t nbytes;
    boolean done = FALSE;
    apr_status_t rv;

    ASSERT(module != NULL);

    log_debug("om_tcp_write");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not writing any more data", module->name);
	return;
    }

    omconf = (nx_om_tcp_conf_t *) module->config;

    do
    {
	if ( module->output.buflen > 0 )
	{
	    nbytes = module->output.buflen;
	    ASSERT(omconf->sock != NULL);
	    if ( (rv = apr_socket_send(omconf->sock, module->output.buf + module->output.bufstart,
				       &nbytes)) != APR_SUCCESS )
	    {
		if ( (APR_STATUS_IS_EINPROGRESS(rv) == TRUE) ||
		     (APR_STATUS_IS_EAGAIN(rv) == TRUE) )
		{
		    nx_module_pollset_add_socket(module, omconf->sock, 
						 APR_POLLIN | APR_POLLOUT | APR_POLLHUP);
		    nx_module_add_poll_event(module);
		    done = TRUE;
		}
		else
		{
		    throw(rv, "om_tcp send failed");
		}
	    }
	    else
	    { // Sent OK
		log_debug("om_tcp sent %d bytes", (int) nbytes);
		if ( nbytes < module->output.buflen )
		{
		    log_debug("om_tcp sent less (%d) than requested (%d)", (int) nbytes,
			      (int) (module->output.buflen - module->output.bufstart));
		    nx_module_pollset_add_socket(module, omconf->sock, 
						 APR_POLLIN | APR_POLLOUT | APR_POLLHUP);
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
	else
	{
	    nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP);
	}
	//log_info("output buflen: %d, bufstart: %d", (int) module->output.buflen, (int) module->output.bufstart);

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



/** 
 * This multiplexes data to multiple connected clients with `Listen TRUE`
 */
static void om_tcp_write_multi(nx_module_t *module)
{
    nx_om_tcp_conf_t *omconf;
    nx_module_input_t *conn;
    nx_module_input_t *tmp;

    nx_logdata_t *logdata;
    apr_size_t nbytes;
    boolean all_sent = TRUE;
    boolean had_data = FALSE;
    apr_status_t rv;

    ASSERT(module != NULL);

    //log_debug("om_tcp_write");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not writing any more data", module->name);
	return;
    }

    omconf = (nx_om_tcp_conf_t *) module->config;

    ASSERT(omconf->connections != NULL);

    conn = NX_DLIST_FIRST(omconf->connections);

    // do not drop events when no clients are connected
    if ( (conn == NULL) && (omconf->queueinlistenmode == TRUE) )
    {
	log_debug("no connected clients");
	return;
    }

    while ( conn != NULL )
    {
	if ( conn->buflen > 0 )
	{
	    nbytes = (apr_size_t) conn->buflen;
	    had_data = TRUE;
	    ASSERT(conn->desc.s != NULL);
	    if ( (rv = apr_socket_send(conn->desc.s, conn->buf + conn->bufstart,
				       &nbytes)) != APR_SUCCESS )
	    {
		if ( (APR_STATUS_IS_EINPROGRESS(rv) == TRUE) ||
		     (APR_STATUS_IS_EAGAIN(rv) == TRUE) )
		{
		    nx_module_pollset_add_socket(module, conn->desc.s, 
						 APR_POLLIN | APR_POLLOUT | APR_POLLHUP);
		    nx_module_add_poll_event(module);
		}
		else
		{
		    log_aprerror(rv, "om_tcp send failed");
		    tmp = conn;
		    conn = NX_DLIST_NEXT(conn, link);
		    om_tcp_free_listener(tmp);
		    continue;
		}
	    }
	    else
	    { // Sent OK
		log_debug("om_tcp sent %d bytes", (int) nbytes);
		if ( nbytes < (apr_size_t) conn->buflen )
		{
		    log_debug("om_tcp sent less (%d) than requested (%d)", (int) nbytes,
			      (int) (conn->buflen - conn->bufstart));
		    nx_module_pollset_add_socket(module, conn->desc.s, 
						 APR_POLLIN | APR_POLLOUT | APR_POLLHUP);
		    nx_module_add_poll_event(module);
		}
	    }
	    ASSERT(nbytes <= (apr_size_t) conn->buflen);
	    conn->bufstart += (int) nbytes;
	    conn->buflen -= (int) nbytes;
	    if ( conn->buflen == 0 )
	    { // all bytes have been sucessfully written
		conn->bufstart = 0;
	    }
	    else
	    {
		all_sent = FALSE;
	    }
	}
	else
	{
	    nx_module_pollset_add_socket(module, conn->desc.s, APR_POLLIN | APR_POLLHUP);
	    nx_module_add_poll_event(module);
	}
	//log_info("output buflen: %d, bufstart: %d", (int) conn->buflen, (int) conn->bufstart);
	conn = NX_DLIST_NEXT(conn, link);
    }
 
    if ( all_sent == TRUE )
    {
	if ( module->output.logdata != NULL )
	{
	    nx_module_logqueue_pop(module, module->output.logdata);
	    nx_logdata_free(module->output.logdata);
	    module->output.logdata = NULL;
	}
	module->output.buflen = 0;
	module->output.bufstart = 0;
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
	// now copy it to each connection's buffer

	for ( conn = NX_DLIST_FIRST(omconf->connections);
	      conn != NULL;
	      conn = NX_DLIST_NEXT(conn, link) )
	{
	    ASSERT(module->output.buflen < (apr_size_t) conn->bufsize);
	    memcpy(conn->buf, module->output.buf, module->output.buflen);
	    conn->buflen = (int) module->output.buflen;
	    conn->bufstart = 0;
	}
	nx_module_add_poll_event(module);
	if ( had_data == TRUE )
	{
	    nx_module_data_available(module);
	}
    }
}



static void om_tcp_read(nx_module_t *module, nx_event_t *event)
{
    nx_om_tcp_conf_t *omconf;
    apr_status_t rv;
    char buf[100];
    apr_size_t nbytes;
    
    // The server disconnected or sent something
    // We close the socket in either case
    ASSERT(module != NULL);

    omconf = (nx_om_tcp_conf_t *) module->config;

    log_debug("om_tcp read");

    if ( (omconf->connected != TRUE) && (omconf->listen == FALSE) )
    {
	return;
    }
    if ( omconf->listen == TRUE )
    {
	apr_socket_t *sock = (apr_socket_t *) event->data;
	nx_module_input_t *conn;
	nx_module_input_t *tmp;

	if ( sock == omconf->listensock )
	{
	    om_tcp_accept(module);
	    nx_module_add_poll_event(module);
	    nx_module_data_available(module);
	    return;
	}

	for ( conn = NX_DLIST_FIRST(omconf->connections); conn != NULL; )
	{
	    ASSERT(conn->desc.s != NULL);
	    nbytes = sizeof(buf);
    	    while ( (rv = apr_socket_recv(conn->desc.s, buf, &nbytes)) == APR_SUCCESS )
	    {
		log_warn("om_tcp received data from remote end (got %d bytes)", (int) nbytes);
		nbytes = sizeof(buf);
	    }
	    if ( APR_STATUS_IS_EAGAIN(rv) )
	    {
		nx_module_pollset_add_socket(module, conn->desc.s, APR_POLLIN | APR_POLLHUP);
		nx_module_add_poll_event(module);
	    }
	    else if ( rv != APR_SUCCESS )
	    {
		log_aprerror(rv, "om_tcp detected a connection error");
		tmp = conn;
		conn = NX_DLIST_NEXT(conn, link);
		om_tcp_free_listener(tmp);
		continue;
	    }
	    conn = NX_DLIST_NEXT(conn, link);
	}

	nx_module_data_available(module);
	nx_module_add_poll_event(module);
	return;
    }

    nbytes = sizeof(buf);
    ASSERT(omconf->sock != NULL);
    while ( (rv = apr_socket_recv(omconf->sock, buf, &nbytes)) == APR_SUCCESS )
    {
	log_warn("om_tcp received data from remote end (got %d bytes)", (int) nbytes);
	nbytes = sizeof(buf);
    }
    if ( APR_STATUS_IS_EAGAIN(rv) )
    {
	nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP);
	nx_module_add_poll_event(module);
	return;
    }
    if ( rv != APR_SUCCESS )
    {
	throw(rv, "om_tcp detected a connection error");
    }
}



static void om_tcp_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_om_tcp_conf_t *omconf;
    unsigned int port;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    omconf = apr_pcalloc(module->pool, sizeof(nx_om_tcp_conf_t));
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
	else if ( strcasecmp(curr->directive, "listen") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "QueueInListenMode") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "Reconnect") == 0 )
	{
	    log_warn("The 'Reconnect' directive at %s:%d has been deprecated",
		     curr->filename, curr->line_num);
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

    nx_cfg_get_boolean(module->directives, "listen", &(omconf->listen));
    nx_cfg_get_boolean(module->directives, "queueinlistenmode", &(omconf->queueinlistenmode));

    if ( module->output.outputfunc == NULL )
    {
	module->output.outputfunc = nx_module_output_func_lookup("linebased");
    }
    ASSERT(module->output.outputfunc != NULL);

    if ( omconf->host == NULL )
    {
	nx_conf_error(module->directives, "Mandatory 'Host' parameter missing");
    }
    if ( omconf->port == 0 )
    {
	omconf->port = OM_TCP_DEFAULT_PORT;
    }

    omconf->connected = FALSE;
    
    if ( omconf->listen == TRUE )
    {
	omconf->connections = apr_pcalloc(module->pool, sizeof(nx_module_input_list_t));
    }
}



static void om_tcp_connect(nx_module_t *module)
{
    nx_om_tcp_conf_t *omconf;
    apr_sockaddr_t *sa;
    apr_pool_t *pool = NULL;
    int i;
    apr_status_t rv;

    ASSERT(module->config != NULL);

    omconf = (nx_om_tcp_conf_t *) module->config;

    omconf->connected = FALSE;

    pool = nx_pool_create_child(module->pool);

    CHECKERR_MSG(apr_sockaddr_info_get(&sa, omconf->host, APR_INET, omconf->port, 
				       0, pool),
		 "apr_sockaddr_info failed for %s:%d", omconf->host, omconf->port);
    CHECKERR_MSG(apr_socket_create(&(omconf->sock), sa->family, SOCK_STREAM,
				   APR_PROTO_TCP, pool), "couldn't create tcp socket");
    CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_NONBLOCK, 1),
		 "couldn't set SO_NONBLOCK on connecting socket");
    CHECKERR_MSG(apr_socket_timeout_set(omconf->sock, OM_TCP_DEFAULT_CONNECT_TIMEOUT),
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
    CHECKERR_MSG(rv, "couldn't connect to tcp socket on %s:%d", omconf->host, omconf->port);
    omconf->connected = TRUE;
    omconf->reconnect = 0;
    
    CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_NONBLOCK, 1),
		 "couldn't set SO_NONBLOCK on tcp socket");
    CHECKERR_MSG(apr_socket_timeout_set(omconf->sock, 0),
		 "couldn't set socket timeout on tcp socket");
    CHECKERR_MSG(apr_socket_opt_set(omconf->sock, APR_SO_KEEPALIVE, 1),
                 "couldn't set TCP_KEEPALIVE on connecting socket");
}



static void om_tcp_listen(nx_module_t *module)
{
    nx_om_tcp_conf_t *omconf;
    nx_exception_t e;

    omconf = (nx_om_tcp_conf_t *) module->config;

    try
    {
	if ( omconf->listensock == NULL )
	{
	    apr_sockaddr_t *sa;

	    CHECKERR_MSG(apr_socket_create(&(omconf->listensock), APR_INET, SOCK_STREAM,
					   APR_PROTO_TCP, module->output.pool),
			 "couldn't create tcp socket");

	    CHECKERR_MSG(apr_sockaddr_info_get(&sa, omconf->host, APR_INET, omconf->port, 
					       0, module->output.pool),
			 "apr_sockaddr_info failed for %s:%d", omconf->host, omconf->port);
	
	    CHECKERR_MSG(apr_socket_opt_set(omconf->listensock, APR_SO_NONBLOCK, 1),
			 "couldn't set SO_NONBLOCK on listen socket");
	    CHECKERR_MSG(apr_socket_timeout_set(omconf->listensock, 0),
			 "couldn't set socket timeout on listen socket");
	    CHECKERR_MSG(apr_socket_opt_set(omconf->listensock, APR_SO_REUSEADDR, 1),
			 "couldn't set SO_REUSEADDR on listen socket");
	    CHECKERR_MSG(apr_socket_opt_set(omconf->listensock, APR_TCP_NODELAY, 1),
			 "couldn't set TCP_NODELAY on listen socket");
	    CHECKERR_MSG(apr_socket_bind(omconf->listensock, sa),
			 "couldn't bind tcp socket to %s:%d", omconf->host, omconf->port);
	}
        else
	{
	    log_debug("tcp socket already initialized");
	}

	CHECKERR_MSG(apr_socket_listen(omconf->listensock, SOMAXCONN),
		     "couldn't listen to tcp socket on %s:%d",
		     omconf->host, omconf->port);
    
	nx_module_pollset_add_socket(module, omconf->listensock, APR_POLLIN | APR_POLLHUP);
    }
    catch(e)
    {
	if ( omconf->listensock != NULL )
	{
	    apr_socket_close(omconf->listensock);
	    omconf->listensock = NULL;
	}
	apr_pool_clear(module->output.pool);
	rethrow(e);
    }
}



static void om_tcp_accept(nx_module_t *module)
{
    nx_om_tcp_conf_t *omconf;
    apr_socket_t *sock;
    apr_sockaddr_t *sa;
    char *ipstr;
    nx_module_input_t *input = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t rv;
    nx_exception_t e;

    log_debug("om_tcp_accept");
    
    omconf = (nx_om_tcp_conf_t *) module->config;

    pool = nx_pool_create_child(module->pool);
    try
    {
	if ( (rv = apr_socket_accept(&sock, omconf->listensock, pool)) != APR_SUCCESS )
	{
	    if ( APR_STATUS_IS_EAGAIN(rv) )
	    {
		nx_module_add_poll_event(module);
		apr_pool_destroy(pool);
	    }
	    else
	    {
		throw(rv, "couldn't accept connection on %s:%u (statuscode: %d)",
		      omconf->host, omconf->port, rv);
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

	    //if ( omconf->nodelay == TRUE )
	    {
		CHECKERR_MSG(apr_socket_opt_set(sock, APR_TCP_NODELAY, 1),
			     "couldn't set TCP_NODELAY on accepted socket");
	    }

	    CHECKERR_MSG(apr_socket_addr_get(&sa, APR_REMOTE, sock),
			 "couldn't get info on accepted socket");
	    CHECKERR_MSG(apr_sockaddr_ip_get(&ipstr, sa),
			 "couldn't get IP of accepted socket");
	    log_info("connection accepted from %s:%u", ipstr, sa->port);

	    nx_module_pollset_add_socket(module, omconf->listensock, APR_POLLIN | APR_POLLHUP);
	    
	    input = nx_module_input_new(module, pool);
	    input->desc_type = APR_POLL_SOCKET;
	    input->desc.s = sock;
	    input->inputfunc = NULL;

	    CHECKERR_MSG(apr_socket_data_set(sock, input, "input", NULL),
			 "couldn't set data on socket");
	    NX_DLIST_INSERT_TAIL(omconf->connections, input, link);

	    nx_module_pollset_add_socket(module, input->desc.s, APR_POLLIN | APR_POLLHUP);
	}
    }
    catch(e)
    {
	apr_pool_destroy(pool);
	rethrow(e);
    }
}



static void om_tcp_free_listener(nx_module_input_t *input)
{
    nx_om_tcp_conf_t *omconf;

    omconf = (nx_om_tcp_conf_t *) input->module->config;

    ASSERT(input->desc.s != NULL);
    nx_module_pollset_remove_socket(input->module, input->desc.s);
    nx_module_remove_events_by_data(input->module, input->desc.s);
    NX_DLIST_REMOVE(omconf->connections, input, link);
    nx_module_input_free(input);
}



static void om_tcp_listener_disconnect(nx_module_t *module, nx_event_t *event)
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
    om_tcp_free_listener(input);
    nx_module_data_available(module);
    nx_module_add_poll_event(module);
}



static void io_err_handler(nx_module_t *module, nx_exception_t *e) NORETURN;
static void io_err_handler(nx_module_t *module, nx_exception_t *e)
{
    ASSERT(e != NULL);
    ASSERT(module != NULL);

    nx_module_stop_self(module);
    om_tcp_stop(module);
    om_tcp_add_reconnect_event(module);
    rethrow(*e);

/*
    //default:
    nx_module_stop_self(module);
    om_tcp_stop(module);
    rethrow_msg(*e, "fatal connection error, reconnection will not be attempted (statuscode: %d)",
		e->code);
*/
}



static void om_tcp_start(nx_module_t *module)
{
    nx_om_tcp_conf_t *omconf;
    nx_exception_t e;

    ASSERT(module->config != NULL);

    omconf = (nx_om_tcp_conf_t *) module->config;

    try
    {
	if ( omconf->listen != TRUE )
	{
	    om_tcp_connect(module);
	}
	else
	{
	    om_tcp_listen(module);
	}
    }
    catch(e)
    {
	io_err_handler(module, &e);
    }

    if ( (omconf->connected == TRUE) && (omconf->listen == FALSE) )
    {
	// POLLIN is to detect disconnection
	nx_module_pollset_add_socket(module, omconf->sock, APR_POLLIN | APR_POLLHUP);
	// write data after connection was established
	nx_module_data_available(module);
	nx_module_add_poll_event(module);
    }
    else if ( omconf->listen == TRUE )
    {
	nx_module_add_poll_event(module);
    }
}



static void om_tcp_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void om_tcp_event(nx_module_t *module, nx_event_t *event)
{
    nx_om_tcp_conf_t *omconf;
    nx_exception_t e;

    ASSERT(event != NULL);
    ASSERT(module->config != NULL);

    omconf = (nx_om_tcp_conf_t *) module->config;

    switch ( event->type )
    {
        case NX_EVENT_RECONNECT:
	    nx_module_start_self(module);
	    break;
        case NX_EVENT_DISCONNECT:
	    if ( omconf->listen == FALSE )
	    {
		nx_module_stop_self(module);
		om_tcp_add_reconnect_event(module);
	    }
	    else
	    {
		om_tcp_listener_disconnect(module, event);
	    }
	    break;;
        case NX_EVENT_READ:
	    try
	    {
		om_tcp_read(module, event);
	    }
	    catch(e)
	    {
		io_err_handler(module, &e);
	    }
	    break;
        case NX_EVENT_WRITE:
	    //fallthrough
	case NX_EVENT_DATA_AVAILABLE:
	    try
	    {
		if ( omconf->listen == FALSE )
		{
		    om_tcp_write(module);
		}
		else
		{ // multiplex data to clients in listen mode
		    om_tcp_write_multi(module);
		}
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


extern nx_module_exports_t nx_module_exports_om_tcp;

NX_MODULE_DECLARATION nx_om_tcp_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_tcp_config,		// config
    om_tcp_start,		// start
    om_tcp_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_tcp_init,		// init
    NULL,			// shutdown
    om_tcp_event,		// event
    NULL,			// info
    &nx_module_exports_om_tcp,  // exports
};
