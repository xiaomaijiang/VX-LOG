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

#include "im_uds.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_UDS_DEFAULT_SUN_PATH "/dev/log"


static void im_uds_close_socket(nx_module_t *module)
{
    if ( module->input.desc.s != NULL )
    {
	nx_module_pollset_remove_socket(module, module->input.desc.s);
	apr_socket_close(module->input.desc.s);
	module->input.desc.s = NULL;
    }
    apr_pool_clear(module->input.pool);
}



static void im_uds_read(nx_module_t *module)
{
    nx_logdata_t *logdata;
    apr_status_t rv;

    ASSERT(module != NULL);

    log_debug("im_uds_read");

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
	    im_uds_close_socket(module);
	    throw(rv, "Module %s couldn't read from socket", module->name);
	}
    }

    while ( (logdata = module->input.inputfunc->func(&(module->input), module->input.inputfunc->data)) != NULL )
    {
	//log_debug("read (%d): [%s]", (int) logdata->datalen, logdata->data);
	nx_module_add_logdata_input(module, &(module->input), logdata);
    }
}



static void im_uds_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_uds_conf_t *imconf;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_uds_conf_t));
    module->config = imconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "uds") == 0 )
	{
	    if ( imconf->sun_path != NULL )
	    {
		nx_conf_error(curr, "uds is already defined");
	    }
	    imconf->sun_path = apr_pstrdup(module->pool, curr->args);
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

    if ( imconf->sun_path == NULL )
    {
	imconf->sun_path = apr_pstrdup(module->pool, IM_UDS_DEFAULT_SUN_PATH);
    }

    module->input.pool = nx_pool_create_child(module->pool);
}



static void im_uds_start(nx_module_t *module)
{
    nx_im_uds_conf_t *imconf;
    struct sockaddr_un uds;
    int sock;
    apr_os_sock_info_t sockinfo;

    ASSERT(module->config != NULL);
    imconf = (nx_im_uds_conf_t *) module->config;

    if ( module->input.desc.s == NULL )
    {
	unlink(imconf->sun_path);

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

	apr_cpystrn(uds.sun_path, imconf->sun_path, sizeof(uds.sun_path) - 1);
	
	if ( bind(sock, (struct sockaddr *) &uds, sizeof(struct sockaddr_un)) < 0 )
	{
	    close(sock);
	    throw_errno("couldn't bind socket %s", imconf->sun_path);
	}

	if ( chmod(imconf->sun_path, 0666) < 0 )
	{
	    close(sock);
	    log_errno("couldn't chmod %s", imconf->sun_path);
	}
	
	memset(&sockinfo, 0, sizeof(apr_os_sock_info_t));
	sockinfo.family = AF_UNIX;
	sockinfo.type = SOCK_DGRAM;
	sockinfo.protocol = 0;
	sockinfo.os_sock = &sock;
    
	CHECKERR_MSG(apr_os_sock_make(&(module->input.desc.s), &sockinfo, module->input.pool),
		     "apr_os_sock_make failed");
	module->input.desc_type = APR_POLL_SOCKET;
	module->input.module = module;
	module->input.inputfunc = imconf->inputfunc;

	CHECKERR_MSG(apr_socket_opt_set(module->input.desc.s, APR_SO_NONBLOCK, 1),
		     "couldn't set SO_NONBLOCK on uds socket");
	CHECKERR_MSG(apr_socket_timeout_set(module->input.desc.s, 0),
		     "couldn't set socket timeout on uds socket");
    }
    else
    {
	log_debug("uds socket already initialized");
    }

    nx_module_pollset_add_socket(module, module->input.desc.s, APR_POLLIN);
    nx_module_add_poll_event(module);

    log_debug("im_uds started");
}



static void im_uds_stop(nx_module_t *module)
{
    im_uds_close_socket(module);
}



static void im_uds_resume(nx_module_t *module)
{
    ASSERT(module != NULL);

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_STOPPED )
    {
	nx_module_add_poll_event(module);
    }
}



static void im_uds_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void im_uds_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_uds_read(module);
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



NX_MODULE_DECLARATION nx_im_uds_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    im_uds_config,		// config
    im_uds_start,		// start
    im_uds_stop, 		// stop
    NULL,			// pause
    im_uds_resume,		// resume
    im_uds_init,		// init
    NULL,			// shutdown
    im_uds_event,		// event
    NULL,			// info
    NULL,			// exports
};

