/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_portable.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/alloc.h"

#include "om_uds.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_UDS_DEFAULT_SUN_PATH "/dev/log"


static void om_uds_write(nx_module_t *module)
{
    nx_om_uds_conf_t *omconf;
    nx_logdata_t *logdata;
    apr_size_t nbytes;
    boolean done = FALSE;
    apr_status_t rv;

    ASSERT(module != NULL);

    log_debug("om_uds_write");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    omconf = (nx_om_uds_conf_t *) module->config;

    do
    {
	if ( module->output.buflen > 0 )
	{
	    nbytes = module->output.buflen;
	    ASSERT(omconf->desc.s != NULL);
	    if ( (rv = apr_socket_send(omconf->desc.s, module->output.buf + module->output.bufstart,
				       &nbytes)) != APR_SUCCESS )
	    {
		if ( APR_STATUS_IS_EPIPE(rv) == TRUE )
		{ // possible for uds??
		    log_debug("om_uds got EPIPE");
		    done = TRUE;
		}
		else if ( (APR_STATUS_IS_EINPROGRESS(rv) == TRUE) ||
			  (APR_STATUS_IS_EAGAIN(rv) == TRUE) )
		{
		    done = TRUE;
		    nx_module_pollset_add_socket(module, omconf->desc.s, APR_POLLOUT);
		    nx_module_add_poll_event(module);
		}
		else
		{
		    throw(rv, "apr_socket_send failed");
		}
	    }
	    else
	    {
		log_debug("om_uds sent %d bytes", (int) nbytes);
		if ( nbytes < module->output.buflen )
		{
		    log_debug("om_uds sent less than requested");
		    nx_module_pollset_add_socket(module, omconf->desc.s, APR_POLLOUT);
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



static void om_uds_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_om_uds_conf_t *omconf;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    omconf = apr_pcalloc(module->pool, sizeof(nx_om_uds_conf_t));
    module->config = omconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "uds") == 0 )
	{
	    if ( omconf->sun_path != NULL )
	    {
		nx_conf_error(curr, "uds is already defined");
	    }
	    omconf->sun_path = apr_pstrdup(module->pool, curr->args);
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

    if ( omconf->sun_path == NULL )
    {
	omconf->sun_path = apr_pstrdup(module->pool, IM_UDS_DEFAULT_SUN_PATH);
    }
}



static void om_uds_stop(nx_module_t *module)
{
    nx_om_uds_conf_t *omconf;
    apr_pool_t *pool;

    ASSERT(module->config != NULL);

    omconf = (nx_om_uds_conf_t *) module->config;

    if ( omconf->desc.s != NULL )
    {
	nx_module_pollset_remove_socket(module, omconf->desc.s);
	pool = apr_socket_pool_get(omconf->desc.s);
	apr_pool_destroy(pool);
	omconf->desc.s = NULL;
    }
}



static void om_uds_start(nx_module_t *module)
{
    nx_om_uds_conf_t *omconf;
    apr_status_t rv;
    struct sockaddr_un uds;
    int sock;
    apr_os_sock_info_t sockinfo;
    apr_pool_t *pool = NULL;

    ASSERT(module->config != NULL);

    omconf = (nx_om_uds_conf_t *) module->config;

    if ( omconf->desc.s == NULL )
    {
	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if ( sock < 0 )
	{
	    throw_errno("couldn't create AF_UNIX socket");
	}

	memset(&uds, 0, sizeof(uds));
	uds.sun_family = AF_UNIX;

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	uds.s_un.sun_len = sizeof(struct sockaddr_un);
#endif

	apr_cpystrn(uds.sun_path, omconf->sun_path, sizeof(uds.sun_path) - 1);

	memset(&sockinfo, 0, sizeof(apr_os_sock_info_t));
	sockinfo.family = AF_UNIX;
	sockinfo.type = SOCK_DGRAM;
	sockinfo.protocol = 0;
	sockinfo.os_sock = &sock;
    
	pool = nx_pool_create_child(module->pool);
	CHECKERR_MSG(apr_os_sock_make(&(omconf->desc.s), &sockinfo, pool),
		     "apr_os_sock_make failed");

	CHECKERR_MSG(apr_socket_opt_set(omconf->desc.s, APR_SO_NONBLOCK, 1),
		     "couldn't set SO_NONBLOCK on uds socket");
	CHECKERR_MSG(apr_socket_timeout_set(omconf->desc.s, 0),
		     "couldn't set socket timeout on uds socket");
	if ( connect(sock, &uds, sizeof(struct sockaddr_un)) != 0 )
	{
	    rv = apr_get_netos_error();
	    log_aprerror(rv, "couldn't connect to uds socket %s", omconf->sun_path);
	    switch ( rv )
	    {
		case APR_ECONNREFUSED:
		case APR_ECONNABORTED:
		case APR_ECONNRESET:
		case APR_ETIMEDOUT:
		case APR_TIMEUP:
		case APR_EHOSTUNREACH:
		case APR_ENETUNREACH:
		    nx_module_stop_self(module);
		    om_uds_stop(module);
		    return;
		default:
		    nx_module_stop_self(module);
		    om_uds_stop(module);
		    return;
	    }
	}
    }
    else
    {
	log_debug("uds socket already initialized");
    }

    log_debug("om_uds started");
}



static void om_uds_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void om_uds_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
        case NX_EVENT_WRITE:
	    om_uds_write(module);
	    break;
	case NX_EVENT_DATA_AVAILABLE:
	    om_uds_write(module);
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



NX_MODULE_DECLARATION nx_om_uds_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_uds_config,		// config
    om_uds_start,		// start
    om_uds_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_uds_init,		// init
    NULL,			// shutdown
    om_uds_event,		// event
    NULL,			// info
    NULL,			// exports
};
