/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "logdata.h"
#include "event.h"
#include "error_debug.h"
#include "module.h"
#include "date.h"
#include "../core/job.h"
#include "../core/nxlog.h"
#include "../common/serialize.h"
#include "../common/expr.h"
#include "../common/expr-parser.h"
#include "../common/alloc.h"
#include "../common/atomic.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define NX_POLLSET_REQEVENTS_KEY "reqevents"

static nxlog_t *_nxlog = NULL;



void nxlog_set(nxlog_t *nxlog)
{
    _nxlog = nxlog;
}



nxlog_t *nxlog_get()
{
    return ( _nxlog );
}


const nx_string_t *nx_get_hostname()
{
    ASSERT(_nxlog != NULL);

    return ( &(_nxlog->hostname) );
}



const char *nx_module_type_to_string(nx_module_type_t type)
{
    int i;

    static nx_keyval_t types[] =
    {
	{ NX_MODULE_TYPE_INPUT, "INPUT" },
	{ NX_MODULE_TYPE_OUTPUT, "OUTPUT" },
	{ NX_MODULE_TYPE_PROCESSOR, "PROCESSOR" },
	{ NX_MODULE_TYPE_EXTENSION, "EXTENSION" },
	{ 0, NULL },
    };

    for ( i = 0; types[i].value != NULL; i++ )
    {
	if ( types[i].key == (int) type )
	{
	    return ( types[i].value );
	}
    }

    nx_panic("invalid type: %d", type);
    return ( "invalid" );
}



const char *nx_module_status_to_string(nx_module_status_t status)
{
    int i;

    static nx_keyval_t statuses[] =
    {
	{ NX_MODULE_STATUS_UNINITIALIZED, "UNINITIALIZED" },
	{ NX_MODULE_STATUS_STOPPED, "STOPPED" },
	{ NX_MODULE_STATUS_PAUSED, "PAUSED" },
	{ NX_MODULE_STATUS_RUNNING, "RUNNING" },
	{ 0, NULL },
    };

    for ( i = 0; statuses[i].value != NULL; i++ )
    {
	if ( statuses[i].key == (int) status )
	{
	    return ( statuses[i].value );
	}
    }

    nx_panic("invalid status: %d", status);
    return ( "invalid" );
}



nx_ctx_t *nx_ctx_get()
{
    if ( _nxlog == NULL )
    {
	return ( NULL );
    }
    return ( _nxlog->ctx );
}



void nx_module_remove_events(nx_module_t *module)
{
    nx_ctx_t *ctx;
    nx_event_t *event, *tmpevent;
    nx_job_t *job;

    ctx = nx_ctx_get();

    nx_lock();
    event = NX_DLIST_FIRST(ctx->events);
    while ( event != NULL )
    {
	if ( event->module == module )
	{
	    tmpevent = event;
	    event = NX_DLIST_NEXT(event, link);
	    NX_DLIST_REMOVE(ctx->events, tmpevent, link);
	    tmpevent->job = NULL;
	}
	else
	{
	    event = NX_DLIST_NEXT(event, link);
	}
    }

    job = module->job;
    if ( job != NULL )
    {
	event = NX_DLIST_FIRST(&(job->events));
	while ( event != NULL )
	{
	    if ( event->module == module )
	    {
		tmpevent = event;
		event = NX_DLIST_NEXT(event, link);
		NX_DLIST_REMOVE(&(job->events), tmpevent, link);
		nx_atomic_sub32(&(job->event_cnt), 1);
		tmpevent->job = NULL;
	    }
	    else
	    {
		event = NX_DLIST_NEXT(event, link);
	    }
	}
    }
    
    nx_unlock();
}



static void nx_module_remove_scheduled_events(nx_module_t *module)
{
    nx_ctx_t *ctx;
    nx_event_t *event, *tmpevent;

    ctx = nx_ctx_get();

    nx_lock();
    event = NX_DLIST_FIRST(ctx->events);
    while ( event != NULL )
    {
	if ( (event->module == module) && (event->type == NX_EVENT_SCHEDULE) )
	{
	    tmpevent = event;
	    event = NX_DLIST_NEXT(event, link);
	    NX_DLIST_REMOVE(ctx->events, tmpevent, link);
	    tmpevent->job = NULL;
	}
	else
	{
	    event = NX_DLIST_NEXT(event, link);
	}
    }

    nx_unlock();
}



void nx_module_remove_events_by_data(nx_module_t *module, void *data)
{
    nx_ctx_t *ctx;
    nx_event_t *event, *tmpevent;
    nx_job_t *job;

    ctx = nx_ctx_get();

    nx_lock();
    event = NX_DLIST_FIRST(ctx->events);
    while ( event != NULL )
    {
	if ( event->data == data )
	{
	    tmpevent = event;
	    event = NX_DLIST_NEXT(event, link);
	    NX_DLIST_REMOVE(ctx->events, tmpevent, link);
	    nx_event_free(tmpevent);
	}
	else
	{
	    event = NX_DLIST_NEXT(event, link);
	}
    }

    job = module->job;
    if ( job != NULL )
    {
	event = NX_DLIST_FIRST(&(job->events));
	while ( event != NULL )
	{
	    if ( event->data == data )
	    {
		tmpevent = event;
		event = NX_DLIST_NEXT(event, link);
		NX_DLIST_REMOVE(&(job->events), tmpevent, link);
		nx_atomic_sub32(&(job->event_cnt), 1);
		nx_event_free(tmpevent);
	    }
	    else
	    {
		event = NX_DLIST_NEXT(event, link);
	    }
	}
    }

    nx_unlock();
}



void nx_module_remove_events_by_type(nx_module_t *module, nx_event_type_t type)
{
    nx_ctx_t *ctx;
    nx_event_t *event, *tmpevent;
    nx_job_t *job;

    ctx = nx_ctx_get();

    nx_lock();

    event = NX_DLIST_FIRST(ctx->events);
    while ( event != NULL )
    {
	if ( (event->module == module) && (event->type == type) )
	{
	    tmpevent = event;
	    event = NX_DLIST_NEXT(event, link);
	    NX_DLIST_REMOVE(ctx->events, tmpevent, link);
	    nx_event_free(tmpevent);
	}
	else
	{
	    event = NX_DLIST_NEXT(event, link);
	}
    }

    job = module->job;
    if ( job != NULL )
    {
	event = NX_DLIST_FIRST(&(job->events));
	while ( event != NULL )
	{
	    if ( event->type == type )
	    {
		tmpevent = event;
		event = NX_DLIST_NEXT(event, link);
		NX_DLIST_REMOVE(&(job->events), tmpevent, link);
		nx_atomic_sub32(&(job->event_cnt), 1);
		nx_event_free(tmpevent);
	    }
	    else
	    {
		event = NX_DLIST_NEXT(event, link);
	    }
	}
    }

    nx_unlock();
}



apr_status_t nx_module_input_fill_buffer_from_socket(nx_module_input_t *input)
{
    apr_status_t rv;
    apr_size_t len;
    apr_sockaddr_t *from;

    ASSERT(input != NULL);
    ASSERT(input->desc_type == APR_POLL_SOCKET);
    ASSERT(input->desc.s != NULL);
    ASSERT(input->buf != NULL);

    if ( input->bufstart == input->bufsize )
    {
	input->bufstart = 0;
	input->buflen = 0;
    }
    if ( input->buflen == 0 )
    {
	input->bufstart = 0;
    }

    len = (apr_size_t) (input->bufsize - (input->buflen + input->bufstart));
    from = nx_module_input_data_get(input, "recv_from");
    if ( from == NULL )
    {
	from = apr_pcalloc(input->pool, sizeof(apr_sockaddr_t));
	nx_module_input_data_set(input, "recv_from", from);
    }

    rv = apr_socket_recvfrom(from, input->desc.s, 0, input->buf + input->bufstart + input->buflen, &len);
    input->buflen += (int) len;
    log_debug("Module %s read %u bytes", input->module->name, (unsigned int) len);

    return ( rv );
}



void nx_module_data_available(nx_module_t *module)
{
    nx_event_t *event;

    ASSERT(module != NULL);

    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_DATA_AVAILABLE;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
    nx_module_pollset_wakeup(module);
}



boolean nx_module_can_send(nx_module_t *module, double multiplier)
{
    nx_module_t *curr = NULL;
    nx_route_t *route;
    int i, j;

    ASSERT(module != NULL);
    ASSERT(multiplier > 0 );
    
    switch ( module->type )
    {
	case NX_MODULE_TYPE_OUTPUT:
	    if ( nx_logqueue_size(module->queue) >= module->queue->limit * multiplier)
	    {
		return ( FALSE );
	    }
	    break;
	case NX_MODULE_TYPE_PROCESSOR:
	    route = ((nx_route_t **)module->routes->elts)[0];
	    // find next module
	    for ( i = 0; i < route->modules->nelts; i++ )
	    {
		if ( (i > 0) && (curr == module) )
		{
		    curr = ((nx_module_t **) route->modules->elts)[i];
		    break;
		}
		curr = ((nx_module_t **) route->modules->elts)[i];
	    }

	    if ( curr->type == NX_MODULE_TYPE_PROCESSOR )
	    {
		if ( nx_logqueue_size(curr->queue) >= curr->queue->limit * multiplier )
		{
		    return ( FALSE );
		}
	    }
	    else
	    { // next module is an OUTPUT module
		for ( ; i < route->modules->nelts; i++ )
		{
		    curr = ((nx_module_t **) route->modules->elts)[i];
		    ASSERT(curr->type == NX_MODULE_TYPE_OUTPUT);
		    if ( nx_logqueue_size(curr->queue) >= curr->queue->limit * multiplier )
		    {
			return ( FALSE );
		    }
		}
	    }
	    break;
	case NX_MODULE_TYPE_INPUT:
	    for ( i = 0; i < module->routes->nelts; i++ )
	    {
		route = ((nx_route_t **) module->routes->elts)[i];
		for ( j = 0; j < route->modules->nelts; j++ )
		{
		    curr = ((nx_module_t **) route->modules->elts)[j];
		    if ( curr->type == NX_MODULE_TYPE_OUTPUT )
		    {
			if ( nx_logqueue_size(curr->queue) >= curr->queue->limit * multiplier )
			{
			    return ( FALSE );
			}
		    }
		    else if ( curr->type == NX_MODULE_TYPE_PROCESSOR )
		    {
			if ( nx_logqueue_size(curr->queue) >= curr->queue->limit * multiplier )
			{
			    return ( FALSE );
			}
			continue; //only check the first processor module in the route
		    }
		}
	    }
	    break;
	default:
	    nx_panic("invalid type: %d", module->type);
    }

    return ( TRUE );
}



/*
 * This is only called from reroute() and add_to_route() procedures
 * Flow Control is not supported here because it cannot be.
 */
void nx_module_add_logdata_to_route(nx_module_t *module UNUSED,
				    nx_route_t *route,
				    nx_logdata_t *logdata)
{
    int i;
    nx_module_t *curr;
    nx_logdata_t *tmp;

    for ( i = 0; i < route->modules->nelts; i++ )
    {
	curr = ((nx_module_t **) route->modules->elts)[i];
	
	if ( curr->type == NX_MODULE_TYPE_PROCESSOR )
	{
	    if ( nx_logqueue_size(curr->queue) >= curr->queue->limit )
	    { // cannot forward so we drop it since there is no flow-control
	    }
	    else
	    {
		tmp = nx_logdata_clone(logdata);
		nx_logqueue_push(curr->queue, tmp);
		nx_module_data_available(curr);
	    }
	    break; //skip remaining modules
	}
	
	if ( curr->type == NX_MODULE_TYPE_OUTPUT )
	{
	    if ( nx_logqueue_size(curr->queue) >= curr->queue->limit )
	    { // cannot forward so we drop it since there is no flow-control
	    }
	    else
	    {
		tmp = nx_logdata_clone(logdata);
		nx_logqueue_push(curr->queue, tmp);
		nx_module_data_available(curr);
	    }
	}
    }
}



void nx_module_add_logdata_input(nx_module_t *module,
				 nx_module_input_t *input,
				 nx_logdata_t *logdata)
{
    int i, j, cnt = 0;
    nx_logdata_t *tmp = logdata;
    nx_route_t *route;
    nx_module_t *curr;
    boolean sent = FALSE;

    ASSERT(module != NULL);
    ASSERT(logdata != NULL);
    ASSERT(module->type == NX_MODULE_TYPE_INPUT);

    if ( nx_logdata_get_field(logdata, "EventReceivedTime") == NULL )
    {
	nx_logdata_set_datetime(logdata, "EventReceivedTime", apr_time_now());
    }
    if ( nx_logdata_get_field(logdata, "SourceModuleName") == NULL )
    {
	nx_logdata_set_string(logdata, "SourceModuleName", module->name);
    }
    if ( nx_logdata_get_field(logdata, "SourceModuleType") == NULL )
    {
	nx_logdata_set_string(logdata, "SourceModuleType", module->dsoname);
    }

    nx_module_lock(module);
    (module->evt_recvd)++;
    nx_module_unlock(module);

    if ( module->exec != NULL )
    {
	nx_expr_eval_ctx_t eval_ctx;
	nx_exception_t e;

	nx_expr_eval_ctx_init(&eval_ctx, logdata, module, input);
	try
	{
	    nx_expr_statement_list_execute(&eval_ctx, module->exec);
	}
	catch(e)
	{
	    log_exception(e);
	}
	if ( eval_ctx.logdata == NULL )
	{ // dropped ?
	    nx_logdata_free(logdata);
	    nx_expr_eval_ctx_destroy(&eval_ctx);
	    return;
	}
	nx_expr_eval_ctx_destroy(&eval_ctx);
    }

    // count how many we need to add
    for ( i = 0; i < module->routes->nelts; i++ )
    {
	route = ((nx_route_t **) module->routes->elts)[i];
	for ( j = 0; j < route->modules->nelts; j++ )
	{
	    curr = ((nx_module_t **) route->modules->elts)[j];
	
	    if ( curr->type == NX_MODULE_TYPE_PROCESSOR )
	    {
		cnt++;
		break; //skip remaining modules
	    }
	
	    if ( curr->type == NX_MODULE_TYPE_OUTPUT )
	    {
		cnt++;
	    }
	}
    }
    if ( cnt == 0 )
    { // no modules to forward to. Can happen if route is incomplete
	return;
    }

    // and now add
    for ( i = 0; i < module->routes->nelts; i++ )
    {
	route = ((nx_route_t **) module->routes->elts)[i];
	for ( j = 0; j < route->modules->nelts; j++ )
	{
	    curr = ((nx_module_t **) route->modules->elts)[j];
	
	    if ( curr->type == NX_MODULE_TYPE_PROCESSOR )
	    {
		if ( cnt > 1 )
		{
		    tmp = nx_logdata_clone(logdata);
		}
		else
		{
		    tmp = logdata;
		}
		if ( module->flowcontrol == FALSE )
		{
		    if ( nx_logqueue_size(curr->queue) >= curr->queue->limit )
		    { // cannot forward, drop it
			nx_logdata_free(tmp);
		    }
		    else
		    {
			nx_logqueue_push(curr->queue, tmp);
			sent = TRUE;
		    }
		    nx_module_data_available(curr);
		}
		else
		{ // flow-control enabled
		    if ( (nx_logqueue_push(curr->queue, tmp) >= curr->queue->limit) &&
			 (nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING) )
		    {
			nx_module_pause(module);
		    }
		    else
		    {
			nx_module_data_available(curr);
		    }
		    sent = TRUE;
		}
		cnt--;
		break; //skip remaining modules
	    }
	
	    if ( curr->type == NX_MODULE_TYPE_OUTPUT )
	    {
		if ( cnt > 1 )
		{
		    tmp = nx_logdata_clone(logdata);
		}
		else
		{
		    tmp = logdata;
		}
		if ( module->flowcontrol == FALSE )
		{
		    if ( nx_logqueue_size(curr->queue) >= curr->queue->limit )
		    { // cannot forward, drop it
			nx_logdata_free(tmp);
		    }
		    else
		    {
			nx_logqueue_push(curr->queue, tmp);
			sent = TRUE;
		    }
		    nx_module_data_available(curr);
		}
		else
		{ // flow-control enabled
		    if ( (nx_logqueue_push(curr->queue, tmp) >= curr->queue->limit) &&
			 (nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING) )
		    {
			nx_module_pause(module);
		    }
		    if ( nx_module_can_send(curr, 1.0) == TRUE )
		    {
			nx_module_data_available(curr);
		    }
		    sent = TRUE;
		}
		cnt--;
	    }
	}
    }

    if ( sent == TRUE )
    {
	nx_module_lock(module);
	(module->evt_fwd)++;
	nx_module_unlock(module);
    }
}



void nx_module_progress_logdata(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_module_t *curr = NULL;
    nx_route_t *route;
    nx_logdata_t *tmp = logdata;
    int i;
    boolean sent = FALSE;

    ASSERT(module != NULL);
    ASSERT(logdata != NULL);
    ASSERT(module->type == NX_MODULE_TYPE_PROCESSOR);

    log_debug("%s nx_module_progress_logdata()", module->name);

    // FIXME: this is popped before it is added to the destination
    // queues, thus there is a chance of data loss.
    // logdata should have a logqueue pointer where it belongs
    // logqueue_push should remove after adding to the new queue if it belongs already to a source queue
    if ( module->queue->needpop == TRUE )
    {
	nx_module_logqueue_pop(module, logdata);
    }

    route = ((nx_route_t **)module->routes->elts)[0];
    // find next module
    for ( i = 0; i < route->modules->nelts; i++ )
    {
	if ( (i > 0) && (curr == module) )
	{
	    curr = ((nx_module_t **) route->modules->elts)[i];
	    break;
	}
	curr = ((nx_module_t **) route->modules->elts)[i];
    }
    if ( curr->type == NX_MODULE_TYPE_PROCESSOR )
    {
	if ( module->flowcontrol == FALSE )
	{
	    if ( nx_logqueue_size(curr->queue) >= curr->queue->limit )
	    { // cannot forward, drop it
		nx_logdata_free(tmp);
	    }
	    else
	    {
		nx_logqueue_push(curr->queue, tmp);
		// increase counters
		sent = TRUE;
	    }
	    nx_module_data_available(curr);
	}
	else
	{ // flow-control enabled
	    if ( nx_logqueue_push(curr->queue, tmp) >= curr->queue->limit )
	    {
		nx_module_pause(module);
	    }
	    else
	    {
		nx_module_data_available(curr);
	    }
	    // increase counters
	    sent = TRUE;
	}
    }
    else
    {
	for ( ; i < route->modules->nelts; i++ )
	{
	    curr = ((nx_module_t **) route->modules->elts)[i];
	    ASSERT(curr->type == NX_MODULE_TYPE_OUTPUT);
	    if ( i < route->modules->nelts - 1 )
	    {
		tmp = nx_logdata_clone(logdata);
	    }
	    else
	    {
		tmp = logdata;
	    }
	    if ( module->flowcontrol == FALSE )
	    {
		if ( nx_logqueue_size(curr->queue) >= curr->queue->limit )
		{ // cannot forward, drop it
		    nx_logdata_free(tmp);
		}
		else
		{
		    nx_logqueue_push(curr->queue, tmp);
		    // increase counters
		    sent = TRUE;
		}
		nx_module_data_available(curr);
	    }
	    else
	    { // flow-control enabled
		if ( nx_logqueue_push(curr->queue, tmp) >= curr->queue->limit )
		{
		    nx_module_pause(module);
		}
		else
		{
		    nx_module_data_available(curr);
		}
		// increase counters
		sent = TRUE;
	    }
	}
    }

    if ( sent == TRUE )
    {
	nx_module_lock(module);
	(module->evt_fwd)++;
	nx_module_unlock(module);
    }
}



boolean nx_module_common_keyword(const char *keyword)
{
    ASSERT(keyword != NULL);

    // TODO: some directives are only for specific module types,
    // e.g. FlowControl is not valid for output modules.

    if ( strcasecmp(keyword, "Module") == 0 )
    {
	return ( TRUE );
    }
    if ( strcasecmp(keyword, "Processors") == 0 )
    { //TODO: deprecated, remove it
	return ( TRUE );
    }
    if ( strcasecmp(keyword, "Exec") == 0 )
    {
	return ( TRUE );
    }
    if ( strcasecmp(keyword, "Schedule") == 0 )
    {
	return ( TRUE );
    }
    if ( strcasecmp(keyword, "FlowControl") == 0 )
    {
	return ( TRUE );
    }

    return ( FALSE );
}


// FIXME: make this a macro
void nx_module_lock(nx_module_t *module)
{
    ASSERT(module != NULL);

    CHECKERR(apr_thread_mutex_lock(module->mutex));
}



void nx_module_unlock(nx_module_t *module)
{
    ASSERT(module != NULL);

    CHECKERR(apr_thread_mutex_unlock(module->mutex));
}



static void resume_senders(nx_module_t *module)
{
    int i, j;
    nx_route_t *route;
    nx_module_t *curr;

    // resume senders if possible
    for ( i = 0; i < module->routes->nelts; i++ )
    {
	// for all routes this module belongs to
	route = ((nx_route_t **) module->routes->elts)[i];
	for ( j = 0; j < route->modules->nelts; j++ )
	{
	    // find ourselves
	    curr = ((nx_module_t **) route->modules->elts)[j];
	    if ( curr == module )
	    {
		ASSERT(j > 0);
		if ( module->type == NX_MODULE_TYPE_PROCESSOR )
		{
		    j--;
		}
		else
		{
		    for ( ; (((nx_module_t **) route->modules->elts)[j])->type == NX_MODULE_TYPE_OUTPUT; j-- )
		    {
			ASSERT(j >= 0);
		    }
		}
		// now we have the preceeding module
		curr = ((nx_module_t **) route->modules->elts)[j];
		if ( curr->type == NX_MODULE_TYPE_PROCESSOR )
		{
		    if ( nx_module_can_send(curr, 0.7) == TRUE )
		    {
			nx_module_resume(curr);
			if ( nx_module_get_status(curr) == NX_MODULE_STATUS_RUNNING )
			{ // notify module to start processing
			    nx_module_data_available(curr);
			}
		    }
		}
		else
		{
		    for ( ; j >= 0; j-- )
		    {
			curr = ((nx_module_t **) route->modules->elts)[j];
			ASSERT(curr->type == NX_MODULE_TYPE_INPUT);
			if ( nx_module_can_send(curr, 0.7) == TRUE )
			{
			    nx_module_resume(curr);
			}
		    }
		}
		break;
	    }
	}
    }
}



nx_logdata_t *nx_module_logqueue_peek(nx_module_t *module)
{
    nx_logdata_t *logdata = NULL;
    int queuesize;

    ASSERT(module != NULL);
    ASSERT(module->type != NX_MODULE_TYPE_INPUT );

    ASSERT(module->queue != NULL);

    queuesize = nx_logqueue_peek(module->queue, &logdata);
    log_debug("%s get_next_logdata: %s (queuesize: %d)", module->name, logdata == NULL ? "got NULL" : "got", queuesize);

    if ( nx_logqueue_size(module->queue) > 0 )
    {
	nx_module_data_available(module);
    }

    resume_senders(module);

    if ( logdata != NULL )
    {
	nx_module_lock(module);
	(module->evt_recvd)++;
	nx_module_unlock(module);

	if ( module->exec != NULL )
	{
	    nx_expr_eval_ctx_t eval_ctx;
	    nx_exception_t e;
	    
	    nx_expr_eval_ctx_init(&eval_ctx, logdata, module, NULL);
	    try
	    {
		nx_expr_statement_list_execute(&eval_ctx, module->exec);
	    }
	    catch(e)
	    {
		log_exception(e);
	    }
	    if ( eval_ctx.logdata == NULL )
	    { // dropped?
		nx_logqueue_pop(module->queue, logdata);
		nx_logdata_free(logdata);
		logdata = NULL;
	    }
	    nx_expr_eval_ctx_destroy(&eval_ctx);
	}
    }

    if ( (logdata != NULL) && (module->type == NX_MODULE_TYPE_OUTPUT) )
    {
	// increase counters
	nx_module_lock(module);
	(module->evt_fwd)++;
	nx_module_unlock(module);
    }

    return ( logdata );
}



void nx_module_logqueue_pop(nx_module_t *module, nx_logdata_t *logdata)
{
    ASSERT(module != NULL);
    ASSERT(module->type != NX_MODULE_TYPE_INPUT );
    ASSERT(module->queue != NULL);

    nx_logqueue_pop(module->queue, logdata);
}



void nx_module_logqueue_drop(nx_module_t *module, nx_logdata_t *logdata)
{
    ASSERT(module != NULL);
    ASSERT(module->type != NX_MODULE_TYPE_INPUT );
    ASSERT(module->queue != NULL);

    // increase counters
    nx_module_lock(module);
    (module->evt_fwd)--;
    nx_module_unlock(module);
    nx_logqueue_pop(module->queue, logdata);
    nx_logdata_free(logdata);
}



const char *nx_module_get_cachedir()
{
    nx_ctx_t *ctx;

    ctx = nx_ctx_get();

    return ( ctx->cachedir );
}



static void nx_module_add_scheduled_event(nx_schedule_entry_t *sched, nx_module_t *module)
{
    nx_event_t *event;

    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_SCHEDULE;
    event->delayed = TRUE;
    event->time = nx_schedule_entry_next_run(sched, apr_time_now());
    event->priority = module->priority;
    event->data = sched;
    nx_event_add(event);
}



static void nx_module_add_scheduled_events(nx_module_t *module)
{
    nx_schedule_entry_t *sched;

    if ( module->schedule == NULL )
    {
	return;
    }
    for ( sched = NX_DLIST_FIRST(module->schedule);
	  sched != NULL;
	  sched = NX_DLIST_NEXT(sched, link) )
    {
	nx_module_add_scheduled_event(sched, module);
    }
}



void nx_module_process_schedule_event(nx_module_t *module, nx_event_t *event)
{
    nx_schedule_entry_t *sched;
    nx_expr_eval_ctx_t eval_ctx;
    nx_exception_t e;

    ASSERT(module != NULL);
    ASSERT(event != NULL);
    ASSERT(event->data != NULL);

    sched = (nx_schedule_entry_t *) event->data;
	
    nx_expr_eval_ctx_init(&eval_ctx, NULL, module, NULL);
    try
    {
	nx_expr_statement_list_execute(&eval_ctx, sched->exec);
    }
    catch(e)
    {
	nx_expr_eval_ctx_destroy(&eval_ctx);
	nx_module_add_scheduled_event(sched, module);
	rethrow_msg(e, "Scheduled execution failed");
    }
    nx_expr_eval_ctx_destroy(&eval_ctx);
    nx_module_add_scheduled_event(sched, module);
}



void nx_module_start(nx_module_t *module)
{
    nx_event_t *event;

    ASSERT(module != NULL);

    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_MODULE_START;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
}



void nx_module_start_self(nx_module_t *module)
{
    nx_module_status_t status;

    ASSERT(module != NULL);
    ASSERT(module->decl != NULL);

    log_debug("START: %s", module->name);

    status = nx_module_get_status(module);

    if ( status == NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s already running", module->name);
	return;
    }
    else if ( status == NX_MODULE_STATUS_UNINITIALIZED )
    {
	nx_module_init(module);
	return;
    }
    else if ( status == NX_MODULE_STATUS_PAUSED )
    {
	nx_module_resume(module);
	return;
    }

    ASSERT(nx_module_get_status(module) == NX_MODULE_STATUS_STOPPED);

    if ( module->decl->start != NULL )
    {
	module->decl->start(module);
    }
    nx_module_add_scheduled_events(module);

    nx_module_set_status(module, NX_MODULE_STATUS_RUNNING);

    // process saved queues
    if ( (module->queue != NULL) && (nx_logqueue_size(module->queue) > 0) )
    {
	nx_module_data_available(module);
    }
}



void nx_module_stop(nx_module_t *module)
{
    nx_event_t *event;

    ASSERT(module != NULL);

    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_MODULE_STOP;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
    nx_module_pollset_wakeup(module);
}



void nx_module_stop_self(nx_module_t *module)
{
    nx_module_status_t status;

    ASSERT(module != NULL);

    log_debug("STOP: %s", module->name);

    status = nx_module_get_status(module);

    if ( status == NX_MODULE_STATUS_STOPPED )
    {
	log_debug("module %s already stopped", module->name);
	return;
    }
    else if ( status == NX_MODULE_STATUS_UNINITIALIZED )
    {
	log_debug("not stopping uninitialized module %s", module->name);
	return;
    }

    // Modules should take care of removing their own events
    // so we don't do this here, only internally handled events
    nx_module_remove_scheduled_events(module);

    nx_module_set_status(module, NX_MODULE_STATUS_STOPPED);

    if ( (module->decl != NULL) && (module->decl->stop != NULL) )
    {
	module->decl->stop(module);
    }
}



void nx_module_pause(nx_module_t *module)
{
    nx_event_t *event;

    ASSERT(module != NULL);

    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_MODULE_PAUSE;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
    nx_module_pollset_wakeup(module);
}



void nx_module_pause_self(nx_module_t *module)
{
    nx_module_status_t status;

    ASSERT(module != NULL);
    ASSERT(module->decl != NULL);

    log_debug("PAUSE: %s", module->name);

    status = nx_module_get_status(module);

    if ( status == NX_MODULE_STATUS_STOPPED )
    {
	log_debug("module %s already stopped, cannot pause", module->name);
	return;
    }
    else if ( status == NX_MODULE_STATUS_UNINITIALIZED )
    {
	log_debug("cannot pause uninitialized module %s", module->name);
	return;
    }
    else if ( status == NX_MODULE_STATUS_PAUSED )
    {
	log_debug("module %s already paused", module->name);
	return;
    }

    if ( module->decl->pause != NULL )
    {
	module->decl->pause(module);
    }
    nx_module_set_status(module, NX_MODULE_STATUS_PAUSED);
}



void nx_module_resume(nx_module_t *module)
{
    nx_event_t *event;

    ASSERT(module != NULL);

    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_MODULE_RESUME;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
    nx_module_pollset_wakeup(module);
}



void nx_module_resume_self(nx_module_t *module)
{
    nx_module_status_t status;

    ASSERT(module != NULL);
    ASSERT(module->decl != NULL);

    log_debug("RESUME: %s", module->name);

    status = nx_module_get_status(module);

    if ( status == NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s already running, skipping resume", module->name);
	return;
    }
    else if ( status == NX_MODULE_STATUS_UNINITIALIZED )
    {
	log_debug("cannot resume uninitialized module %s", module->name);
	return;
    }
    else if ( status == NX_MODULE_STATUS_STOPPED )
    {
	log_debug("not resuming stopped module %s", module->name);
	return;
    }

    if ( module->decl->resume != NULL )
    {
	module->decl->resume(module);
    }

    nx_module_set_status(module, NX_MODULE_STATUS_RUNNING);

    // notify module to start processing
    if ( (module->queue != NULL) )
    {
	nx_module_data_available(module);
    }
}



void nx_module_shutdown(nx_module_t *module)
{
    nx_event_t *event;

    ASSERT(module != NULL);

    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_MODULE_SHUTDOWN;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
    nx_module_pollset_wakeup(module);
}

/* Get memory size on linux - for debugging
 * Does not work well with APR_POOL_DEBUG
 */
/*
#include <unistd.h>
static int get_mem_size()
{
    int retval = -1;
    apr_status_t rv;
    apr_file_t *statfile = NULL;
    apr_pool_t *pool;
    char statbuf[1024];
    apr_size_t nbytes;
    int pid;
    char comm[PATH_MAX + 2];
    char state;
    int ppid;
    int pgrp;
    int session;
    int tty_nr;
    int tpgid;
    unsigned int flags;
    unsigned long minflt, cminflt, majflt, cmajflt, utime, s_time;
    long int cutime, cs_time, priority, niceval, num_threads, itrealvalue, vsize = 0, rss = 0, rsslim;
    long long unsigned starttime;

    pool = nx_pool_create_core();

    if ( (rv = apr_file_open(&statfile, "/proc/self/stat", APR_READ, APR_OS_DEFAULT, pool)) == APR_SUCCESS )
    {
	nbytes = sizeof(statbuf);
	memset(statbuf, 0, sizeof(statbuf));
	if ( (rv = apr_file_read(statfile, statbuf, &nbytes)) == APR_SUCCESS )
	{
                 //                                            10                                      20
	    if ( sscanf(statbuf, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %ld %lu",
			&pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags, &minflt, 
			&cminflt, &majflt, &cmajflt, &utime, &s_time, &cutime, &cs_time, &priority, &niceval, &num_threads,
			&itrealvalue, &starttime, &vsize, &rss, &rsslim) > 0 )
	    {
		retval = (int) (rss * sysconf(_SC_PAGESIZE));
	    }
	}
	
	apr_file_close(statfile);
    }
    apr_pool_destroy(pool);

    return ( retval );
}
*/

void nx_module_shutdown_self(nx_module_t *module)
{
    nx_module_status_t status;
    int memsize;
    int memsize2;

    ASSERT(module != NULL);

    log_debug("SHUTDOWN: %s", module->name);
    //log_info("mem size: %d", get_mem_size());

    status = nx_module_get_status(module);

    if ( status == NX_MODULE_STATUS_UNINITIALIZED )
    {
	log_debug("not calling shutdown on uninitialized module %s", module->name);
	return;
    }

    if ( (status == NX_MODULE_STATUS_RUNNING) ||
	 (status == NX_MODULE_STATUS_PAUSED) )
    {
	nx_module_stop_self(module);
    }
    else if ( status == NX_MODULE_STATUS_UNINITIALIZED )
    {
	log_debug("not calling shutdown on uninitialized module %s", module->name);
	return;
    }

    nx_module_remove_events(module);

    if ( module->decl->shutdown != NULL )
    {
	module->decl->shutdown(module);
    }
    
    nx_module_set_status(module, NX_MODULE_STATUS_UNINITIALIZED);

    //FIXME: free module->vars
    //FIXME: free module->stats

    //memsize = get_mem_size();
    //log_info("mem size before pool destroy: %d", memsize);
    
    if ( module->pool != NULL )
    {
	apr_pool_destroy(module->pool);
    }

    //memsize2 = get_mem_size();
    //log_info("mem size after pool destroy: %d (freed: %d)", memsize2, memsize - memsize2);

}



static void nx_module_empty_config_check(nx_module_t *module)
{
    const nx_directive_t *curr;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else
	{
	    nx_conf_error(curr, "invalid module keyword: %s", curr->directive);
	}
	curr = curr->next;
    }
}



/**
 * Find all occurences of Exec under the current block, compile them and return
 * a single statement list
 */
nx_expr_statement_list_t *nx_module_parse_exec_block(nx_module_t *module,
						     apr_pool_t *pool,
						     const nx_directive_t * volatile curr)
{
    nx_expr_statement_list_t * volatile statements = NULL;
    nx_expr_statement_list_t * volatile retval = NULL;
    nx_expr_statement_t *stmnt;
    nx_exception_t e;

    ASSERT(curr != NULL);

    while ( curr != NULL )
    {
	if ( strcasecmp(curr->directive, "Exec") == 0 )
	{
	    try
	    {
		statements = nx_expr_parse_statements(module, curr->args, pool, curr->filename,
						      curr->line_num, curr->argsstart);
	    }
	    catch(e)
	    {
		log_debug("Error in Exec block: [%s]", curr->args);
		rethrow_msg(e, "Couldn't parse Exec block at %s:%d",
			    curr->filename, curr->line_num);
	    }
	    if ( retval == NULL )
	    {
		retval = statements;
	    }
	    else
	    {
		ASSERT(statements != NULL);
		while ( (stmnt = NX_DLIST_FIRST(statements)) != NULL )
		{
		    NX_DLIST_REMOVE(statements, stmnt, link);
		    NX_DLIST_INSERT_TAIL(retval, stmnt, link);
		}
	    }
	}
	curr = curr->next;
    }

    return ( retval );
}



static void nx_module_parse_schedule_blocks(nx_module_t *module)
{
    const nx_directive_t * volatile curr, * volatile curr2;
    nx_schedule_entry_t *sched;
    apr_time_t first;
    nx_exception_t e;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    while ( curr != NULL )
    {
	sched = NULL;
	if ( strcasecmp(curr->directive, "Schedule") == 0 )
	{
	    if ( module->schedule == NULL )
	    {
		module->schedule = apr_pcalloc(module->pool, sizeof(nx_schedule_entry_list_t));
		NX_DLIST_INIT(module->schedule, nx_schedule_entry_t, link);
	    }
	    curr2 = curr->first_child;
	    if ( curr2 == NULL )
	    {
		nx_conf_error(curr, "empty Schedule block");
	    }
	    sched = apr_pcalloc(module->pool, sizeof(nx_schedule_entry_t));
	    first = 0;
	    while ( curr2 != NULL )
	    {
		if ( strcasecmp(curr2->directive, "When") == 0 )
		{
		    if ( sched->type == NX_SCHEDULE_ENTRY_TYPE_CRONTAB )
		    {
			nx_conf_error(curr2, "'When' is already defined within the Schedule block");
		    }
		    if ( sched->type == NX_SCHEDULE_ENTRY_TYPE_EVERY )
		    {
			nx_conf_error(curr2, "'Every' is already defined within the Schedule block");
		    }
		    sched->type = NX_SCHEDULE_ENTRY_TYPE_CRONTAB;
		    try
		    {
			nx_schedule_entry_parse_crontab(sched, curr2->args);
		    }
		    catch(e)
		    {
			nx_conf_error(curr2, "couldn't parse value for directive 'When': %s", nx_exception_get_message(&e, 0));
		    }
		}
		else if ( strcasecmp(curr2->directive, "Every") == 0 )
		{
		    if ( sched->type == NX_SCHEDULE_ENTRY_TYPE_CRONTAB )
		    {
			nx_conf_error(curr2, "'When' is already defined within the Schedule block");
		    }
		    if ( sched->type == NX_SCHEDULE_ENTRY_TYPE_EVERY )
		    {
			nx_conf_error(curr2, "'Every' is already defined within the Schedule block");
		    }
		    try
		    {
			nx_schedule_entry_parse_every(sched, curr2->args);
		    }
		    catch(e)
		    {
			nx_conf_error(curr2, "couldn't parse value for directive 'Every': %s", nx_exception_get_message(&e, 0));
		    }
		    sched->type = NX_SCHEDULE_ENTRY_TYPE_EVERY;
		}
		else if ( strcasecmp(curr2->directive, "First") == 0 )
		{
		    if ( first != 0 )
		    {
			nx_conf_error(curr2, "'First' is already defined within the Schedule block");
		    }
		    if ( nx_date_parse_iso(&first, curr2->args, NULL) != APR_SUCCESS )
		    {
			nx_conf_error(curr2, "invalid datetime given for 'First' directive: %s", curr2->args);
		    }
		}
		curr2 = curr2->next;
	    }
	    sched->exec = nx_module_parse_exec_block(module, module->pool, curr->first_child);
	    if ( sched->exec == NULL )
	    {
		nx_conf_error(curr, "'Exec' missing from Schedule block");
	    }
	    if ( (first != 0) && (sched->type == NX_SCHEDULE_ENTRY_TYPE_CRONTAB) )
	    {
		nx_conf_error(curr, "cannot use 'First' with 'When' in Schedule block");
	    }
	    if ( first != 0 )
	    {
		sched->first = first;
	    }
	    if ( sched->type == 0 )
	    {
		nx_conf_error(curr, "'When' or 'Every' must be defined in Schedule block");
	    }

	    NX_DLIST_INSERT_TAIL(module->schedule, sched, link);
	}
	curr = curr->next;
    }
}



void nx_module_config(nx_module_t *module)
{
    ASSERT(module != NULL);
    ASSERT(module->decl != NULL);

    log_debug("CONFIG: %s", module->name);

    ASSERT(module->status == NX_MODULE_STATUS_UNINITIALIZED);

    if ( module->decl->config != NULL )
    {
	module->decl->config(module);
    }
    else
    {
	nx_module_empty_config_check(module);
    }

    module->exec = nx_module_parse_exec_block(module, module->pool, module->directives);
    nx_module_parse_schedule_blocks(module);
}



void nx_module_init(nx_module_t *module)
{
    ASSERT(module != NULL);
    ASSERT(module->decl != NULL);

    log_debug("INIT: %s", module->name);

    if ( module->status != NX_MODULE_STATUS_UNINITIALIZED )
    {
	log_error("module %s already initialized", module->name);
	return;
    }

    if ( module->decl->init != NULL )
    {
	module->decl->init(module);
    }

    module->status = NX_MODULE_STATUS_STOPPED;
}



char *nx_module_info(nx_module_t *module)
{
    ASSERT(module != NULL);
    ASSERT(module->decl != NULL);

    if ( module->decl->info == NULL )
    {
	return ( NULL );
    }

    return ( module->decl->info(module) );
}




const void *nx_module_get_resource(nx_module_t *module,
				   const char *name,
				   nx_resource_type_t type)
{
    char tmpbuf[256];

    ASSERT(module != NULL);

    ASSERT(apr_snprintf(tmpbuf, sizeof(tmpbuf), "%s::%s", module->name, name) < (int) sizeof(tmpbuf));

    return ( nx_resource_get(tmpbuf, type) );
}



nx_module_input_t *nx_module_input_new(nx_module_t *module, apr_pool_t *pool)
{
    nx_module_input_t *input = NULL;
    int bufsize = NX_MODULE_DEFAULT_INPUT_BUFSIZE;

    if ( pool == NULL )
    {
	pool = nx_pool_create_child(module->pool);
    }
    input = apr_pcalloc(pool, sizeof(nx_module_input_t));
    input->pool = pool;
    if ( (module->type == NX_MODULE_TYPE_INPUT) && (module->input.bufsize != 0) )
    {
	bufsize = module->input.bufsize;
    }
    input->buf = apr_pcalloc(pool, (apr_size_t) bufsize);
    input->bufsize = bufsize;
    input->module = module;

    return ( input );
}


/*
nx_module_output_t *nx_module_output_new(nx_module_t *module, apr_pool_t *pool)
{
    nx_module_output_t *output = NULL;

    ASSERT(pool != NULL);

    output = apr_pcalloc(pool, sizeof(nx_module_output_t));
    output->pool = pool;
    output->buf = apr_pcalloc(pool, NX_MODULE_DEFAULT_OUTPUT_BUFSIZE);
    output->bufsize = NX_MODULE_DEFAULT_OUTPUT_BUFSIZE;
    output->module = module;

    return ( output );
}
*/


void nx_module_input_free(nx_module_input_t *input)
{
    ASSERT(input != NULL);

    if ( input->desc_type == APR_POLL_SOCKET )
    {
	if ( input->desc.s != NULL )
	{
	    apr_socket_close(input->desc.s);
	    nx_module_remove_events_by_data(input->module, input->desc.s);
	    input->desc.s = NULL;
	}
    }
    else if ( input->desc_type == APR_POLL_FILE )
    {
	if ( input->desc.f != NULL )
	{
	    apr_file_close(input->desc.f);
	    nx_module_remove_events_by_data(input->module, input->desc.f);
	    input->desc.f = NULL;
	}
    }
    ASSERT(input->pool != NULL);
    apr_pool_destroy(input->pool);
}



void nx_module_output_free(nx_module_output_t *output)
{
    ASSERT(output != NULL);

    ASSERT(output->pool != NULL);
    apr_pool_destroy(output->pool);
}



void *nx_module_data_get(nx_module_t *module, const char *key)
{
    nx_module_data_t *cur;
    void *retval = NULL;

    ASSERT(module != NULL);

    cur = module->data;

    while ( cur != NULL )
    {
        if ( strcmp(cur->key, key) == 0 )
	{
            retval = cur->data;
            break;
        }
        cur = cur->next;
    }

    return ( retval );
}



void *nx_module_input_data_get(nx_module_input_t *input, const char *key)
{
    nx_module_data_t *cur;
    void *retval = NULL;

    ASSERT(input != NULL);

    cur = input->data;

    while ( cur != NULL )
    {
        if ( strcmp(cur->key, key) == 0 )
	{
            retval = cur->data;
            break;
        }
        cur = cur->next;
    }

    return ( retval );
}



void *nx_module_output_data_get(nx_module_output_t *output, const char *key)
{
    nx_module_data_t *cur;
    void *retval = NULL;

    ASSERT(output != NULL);

    cur = output->data;

    while ( cur != NULL )
    {
        if ( strcmp(cur->key, key) == 0 )
	{
            retval = cur->data;
            break;
        }
        cur = cur->next;
    }

    return ( retval );
}


#define NX_MODULE_INPUT_NAME_LENGTH 250

void nx_module_input_name_set(nx_module_input_t *input, const char *name)
{
    ASSERT(input != NULL);
    ASSERT(name != NULL);

    if ( input->name == NULL )
    {
	input->name = apr_pcalloc(input->pool, NX_MODULE_INPUT_NAME_LENGTH);
	apr_cpystrn(input->name, name, NX_MODULE_INPUT_NAME_LENGTH);
    }
    else
    {
	if ( strcmp(input->name, name) == 0 )
	{ // already have the same, don't update
	    return;
	}
	else
	{
	    apr_cpystrn(input->name, name, NX_MODULE_INPUT_NAME_LENGTH);
	}
    }
}



const char *nx_module_input_name_get(nx_module_input_t *input)
{
    apr_sockaddr_t *sa;

    ASSERT(input != NULL);

    if ( input->name == NULL )
    {
	if ( input->desc_type == APR_POLL_FILE )
	{
	    const char *filename;

	    filename = (const char *) nx_module_input_data_get(input, "filename");
	    if ( filename!= NULL )
	    {
		input->name = apr_pstrdup(input->pool, filename);
		return ( filename );
	    }
	}
        else if ( (input->desc_type == APR_POLL_SOCKET) && (input->desc.s != NULL) )
	{
	    char *ipstr;

	    if ( (apr_socket_addr_get(&sa, APR_REMOTE, input->desc.s) == APR_SUCCESS) &&
		 (apr_sockaddr_ip_get(&ipstr, sa) == APR_SUCCESS) )
	    {
		input->name = apr_pstrdup(input->pool, ipstr);
		return ( input->name );
	    }
	}
	return ( "unknown" );
    }
    return ( input->name );
}



/**
 * Set a data on a module_t structure
 * If a data already exists with the same key, it will be overwritten
 */
void nx_module_data_set(nx_module_t *module, const char *key, void *data)
{
    nx_module_data_t *new;
    nx_module_data_t *cur;

    ASSERT(module != NULL);

    cur = module->data;

    while ( cur != NULL )
    {
        if ( strcmp(cur->key, key) == 0 )
	{
	    cur->data = data;
	    return;
        }
        cur = cur->next;
    }

    new = apr_palloc(module->pool, sizeof(nx_module_data_t));
    new->key = apr_pstrdup(module->pool, key);
    new->data = data;
    new->next = module->data;
    module->data = new;
}



/**
 * Set a data on a module_input structure
 * If a data already exists with the same key, it will be overwritten
 */
void nx_module_input_data_set(nx_module_input_t *input, const char *key, void *data)
{
    nx_module_data_t *new;
    nx_module_data_t *cur;

    ASSERT(input != NULL);

    cur = input->data;

    while ( cur != NULL )
    {
        if ( strcmp(cur->key, key) == 0 )
	{
	    cur->data = data;
	    return;
        }
        cur = cur->next;
    }

    new = apr_palloc(input->pool, sizeof(nx_module_data_t));
    new->key = apr_pstrdup(input->pool, key);
    new->data = data;
    new->next = input->data;
    input->data = new;
}



/**
 * Set a data on a module_output structure
 * If a data already exists with the same key, it will be overwritten
 */

void nx_module_output_data_set(nx_module_output_t *output, const char *key, void *data)
{
    nx_module_data_t *new;
    nx_module_data_t *cur;

    ASSERT(output != NULL);

    cur = output->data;

    while ( cur != NULL )
    {
        if ( strcmp(cur->key, key) == 0 )
	{
	    cur->data = data;
	    return;
        }
        cur = cur->next;
    }

    new = apr_palloc(output->pool, sizeof(nx_module_data_t));
    new->key = apr_pstrdup(output->pool, key);
    new->data = data;
    new->next = output->data;
    output->data = new;
}



nx_module_input_func_decl_t *nx_module_input_func_lookup(const char *fname)
{
    nx_ctx_t *ctx;
    nx_module_input_func_decl_t *inputfunc;

    ctx = nx_ctx_get();

    for ( inputfunc = NX_DLIST_FIRST(ctx->input_funcs);
	  inputfunc != NULL;
	  inputfunc = NX_DLIST_NEXT(inputfunc, link) )
    {
	if ( strcasecmp(fname, inputfunc->name) == 0 )
	{
	    return ( inputfunc );
	}
    }

    return ( NULL );
}



nx_module_output_func_decl_t *nx_module_output_func_lookup(const char *fname)
{
    nx_ctx_t *ctx;
    nx_module_output_func_decl_t *outputfunc;

    ctx = nx_ctx_get();

    for ( outputfunc = NX_DLIST_FIRST(ctx->output_funcs);
	  outputfunc != NULL;
	  outputfunc = NX_DLIST_NEXT(outputfunc, link) )
    {
	if ( strcasecmp(fname, outputfunc->name) == 0 )
	{
	    return ( outputfunc );
	}
    }

    return ( NULL );
}



void nx_module_input_func_register(const nx_module_t *module,
				   const char *fname,
				   nx_module_input_func_t *func,
				   nx_module_input_func_t *flush,
				   void *data)
{
    nx_ctx_t *ctx;
    nx_module_input_func_decl_t *inputfunc;

    ctx = nx_ctx_get();

    if ( nx_module_input_func_lookup(fname) != NULL )
    {
	throw_msg("inputreader function '%s' is already registered", fname);
    }

    inputfunc = apr_pcalloc(ctx->pool, sizeof(nx_module_input_func_decl_t));
    ASSERT(inputfunc != NULL);

    inputfunc->func = func;
    inputfunc->flush = flush;
    inputfunc->module = module;
    inputfunc->name = apr_pstrdup(ctx->pool, fname);
    inputfunc->data = data;
    NX_DLIST_INSERT_TAIL(ctx->input_funcs, inputfunc, link);

    log_debug("inputreader '%s' registered", fname);
}



void nx_module_output_func_register(const nx_module_t *module,
				    const char *fname,
				    nx_module_output_func_t *func,
				    void *data)
{
    nx_ctx_t *ctx;
    nx_module_output_func_decl_t *outputfunc;

    ctx = nx_ctx_get();

    if ( nx_module_output_func_lookup(fname) != NULL )
    {
	throw_msg("outputwriter function '%s' is already registered", fname);
    }

    outputfunc = apr_pcalloc(ctx->pool, sizeof(nx_module_output_func_decl_t));
    ASSERT(outputfunc != NULL);

    outputfunc->func = func;
    outputfunc->module = module;
    outputfunc->name = apr_pstrdup(ctx->pool, fname);
    outputfunc->data = data;
    NX_DLIST_INSERT_TAIL(ctx->output_funcs, outputfunc, link);

    log_debug("outputwriter '%s' registered", fname);
}



void nx_module_set_status(nx_module_t *module, nx_module_status_t status)
{
    ASSERT(module != NULL);
    
    nx_module_lock(module);
    module->status = status;
    nx_module_unlock(module);
}



/**
 * This will query the current status of the module
 * Be careful when using this function as the event queue of the module
 * can contain status change event also. Thus relying on the info returned
 * by this function can be racy.
 */
nx_module_status_t nx_module_get_status(nx_module_t *module)
{
    nx_module_status_t status;

    ASSERT(module != NULL);
    
    nx_module_lock(module);
    status = module->status;
    nx_module_unlock(module);
    
    return ( status );
}



static const char *field_list_parse(char **src)
{
    size_t i = 0;
    char *retval = NULL;

    if ( **src == '\0' )
    {
	return ( NULL );
    }

    // skip space
    for ( ; (**src == ' ') || (**src == '\t'); (*src)++ );
    if ( **src == '$' )
    {
	// ignore leading dollar
	(*src)++;
    }
    retval = *src;

    for ( i = 0; **src != '\0'; (*src)++ )
    {
        if ( (**src == ' ') ||
	     (**src == '\t') ||
	     (**src == ',') ||
	     (**src == ';') )
	{
	    (*src)++;
	    break;
	}
	retval[i] = **src;
	i++;
    }
    retval[i] = '\0';

    // skip space
    while ( (**src == ' ') || (**src == '\t') )
    {
	**src = '\0';
	(*src)++;
    }
    // skip delimiter
    while ( (**src == ',') || (**src == ';') )
    {
	**src = '\0';
	(*src)++;
    }

    return ( retval );
}



int nx_module_parse_fields(const char **fields, char *string)
{
    char *ptr = string;
    const char *field;
    int num_field = 0;

    ASSERT(string != NULL);
    ASSERT(fields != NULL);

    while ( (field = field_list_parse(&ptr)) != NULL )
    {
	if ( num_field >= NX_MODULE_MAX_FIELDS - 1 )
	{
	    throw_msg("maximum number of fields reached, limit is %d",
		      NX_MODULE_MAX_FIELDS);
	}
	fields[num_field] = field;
	num_field++;
    }
    fields[num_field] = NULL;

    return ( num_field );
}



int nx_module_parse_types(nx_value_type_t *types, char *string)
{
    char *ptr = string;
    const char *type;
    int num_type = 0;

    ASSERT(types != NULL);
    ASSERT(string != NULL);

    while ( (type = field_list_parse(&ptr)) != NULL )
    {
	if ( num_type >= NX_MODULE_MAX_FIELDS - 1 )
	{
	    throw_msg("maximum number of types reached, limit is %d",
		      NX_MODULE_MAX_FIELDS);
	}
	types[num_type] = nx_value_type_from_string(type);
	num_type++;
    }
    types[num_type] = 0;

    return ( num_type );
}

// TODO: this size is pretty big and uses 500k per pollset
#define NX_POLLSET_NUM	10000 // This also limits the number of connections per socket

void nx_module_pollset_init(nx_module_t *module)
{
    apr_pollset_t *pollset = NULL;
    apr_status_t rv;
    int trycnt = 0;
    const char *methodname;
    static apr_uint32_t _pollset_num = NX_POLLSET_NUM; // TODO: possible race condition with multiple modules

#ifdef HAVE_APR_POLLSET_WAKEUP
    apr_uint32_t flags = APR_POLLSET_WAKEABLE;
#else
    apr_uint32_t flags = 0;
#endif

    // There is a bug in apr, pipe creation used for pollset wakeup can fail on
    // windows 2003 with WSAEWOUDLBLOCK (10035) because of a non-blocking accept
    // http://osdir.com/ml/dev-apr-apache/2010-08/msg00039.html
    // So the following is a workaround
    for ( ; ; )
    {
	rv = apr_pollset_create(&pollset, _pollset_num,
				module->pool, flags);
	if ( APR_STATUS_IS_EINVAL(rv) )
	{
	    // Detect the number of pollset entries this system can support
	    // windows2003 gives an invalid argument above 500
	    _pollset_num -= 10; // TODO: possible race condition with multiple modules
	}
	else if ( APR_STATUS_IS_EAGAIN(rv) )
	{ 
	    apr_sleep(APR_USEC_PER_SEC / 10);
	    trycnt++;
	    if ( trycnt > 50 )
	    {
		break;
	    }
	}
	else
	{
	    break;
	}
    }

    if ( (rv != APR_SUCCESS) || (pollset == NULL) )
    {
	throw(rv, "failed to create pollset");
    }
#ifdef HAVE_APR_POLLSET_WAKEUP
    methodname = apr_pollset_method_name(pollset);
    log_debug("Pollset initialized for module %s (method: %s)",
	      module->name, methodname);
#endif

    module->pollset = pollset;
}



void nx_module_pollset_wakeup(nx_module_t *module)
{
    ASSERT(module != NULL);

#ifdef HAVE_APR_POLLSET_WAKEUP
    if ( module->pollset != NULL )
    {
	if ( nx_atomic_read32(&(module->in_poll)) != 0 )
	{
	    apr_pollset_wakeup(module->pollset);
	}
    }
#endif
}



void nx_module_pollset_add_file(nx_module_t *module,
				apr_file_t *file,
				apr_int16_t reqevents)
{
    apr_pollfd_t pfd;
    apr_status_t rv;
    apr_int16_t *setevents = NULL;
    apr_pool_t *pool;

    ASSERT(module != NULL);
    ASSERT(file != NULL);

    apr_file_data_get((void **) &setevents, NX_POLLSET_REQEVENTS_KEY, file);
    if ( (setevents != NULL) && (*setevents != 0) )
    {
	if ( *setevents == reqevents )
	{
	    log_debug("file already added to pollset with reqevents [%x]", reqevents);
	    return;
	}
	nx_module_pollset_remove_file(module, file);
    }

    pfd.desc.f = file;
    pfd.desc_type = APR_POLL_FILE;
    pfd.reqevents = reqevents;
    pfd.client_data = module;
    pfd.p = module->pool;

    if ( (rv = apr_pollset_add(module->pollset, &pfd)) != APR_SUCCESS )
    {
	char errmsg[200];
	
	if ( APR_STATUS_IS_EEXIST(rv) )
	{
//		nx_panic("socket is already added");
//		log_aprerror(rv, "file is already added");
	}
	else
	{
	    apr_strerror(rv, errmsg, sizeof(errmsg));
	    nx_panic("failed to add descriptor to pollset: %s", errmsg);
	}
    }

    if ( setevents == NULL )
    {
	pool = apr_file_pool_get(file);
	setevents = apr_palloc(pool, sizeof(apr_int16_t));
	*setevents = reqevents;
	apr_file_data_set(file, (void *) setevents, NX_POLLSET_REQEVENTS_KEY, NULL);
    }
    else
    {
	*setevents = reqevents;
    }
}



void nx_module_pollset_remove_file(nx_module_t *module,
				   apr_file_t *file)
{
    apr_status_t rv;
    apr_pollfd_t pfd;
    apr_int16_t *reqevents = NULL;

    ASSERT(module != NULL);
    ASSERT(file != NULL);

    apr_file_data_get((void **) &reqevents, NX_POLLSET_REQEVENTS_KEY, file);
    if ( reqevents == NULL )
    {
	return;
    }
    pfd.desc.f = file;
    pfd.desc_type = APR_POLL_FILE;
    pfd.reqevents = *reqevents;
    pfd.client_data = module;
    pfd.p = module->pool;

   if ( (rv = apr_pollset_remove(module->pollset, &pfd)) != APR_SUCCESS )
    {
	char errmsg[200];

	if ( APR_STATUS_IS_NOTFOUND(rv) )
	{
//		log_aprerror(rv, "Could not remove descriptor from pollset");
	}
	else
	{
	    apr_strerror(rv, errmsg, sizeof(errmsg));
	    nx_panic("failed to remove descriptor to pollset: %s", errmsg);
	}
    }
   *reqevents = 0;
}


/*
 * NOTE!!! We are using the apr_pollset without APR_POLLSET_NOCPOY.
 * On Linux the epoll implementation allocates a copy of apr_pollfd_t out of the pool
 * of the pollset. Here is how that works:
 *  apr_pollset_remove() adds the copy of apr_pollfd_t to a Dead List.
 *  apr_pollset_add() checks the Free List and uses an entry from there.
 *  apr_pollset_poll adds all entries from the Dead List to the Free List
 * Calling _add() + _remove() several times without a call to _poll() results in excess
 * allocations from the pool of the pollset - i.e. leak.
 *
 * TODO: The code including all modules needs to be reviewed and checked whether
 * it is possible for a module to emit multiple _add()s with different event requests.
 */

void nx_module_pollset_add_socket(nx_module_t *module,
				  apr_socket_t *sock,
				  apr_int16_t reqevents)
{
    apr_pollfd_t pfd;
    apr_status_t rv;
    apr_int16_t *setevents = NULL;
    apr_pool_t *pool;

    ASSERT(module != NULL);
    ASSERT(sock != NULL);
    ASSERT(module->pollset != NULL);

    apr_socket_data_get((void **) &setevents, NX_POLLSET_REQEVENTS_KEY, sock);
    log_debug("add socket [%x]", reqevents);
    if ( (setevents != NULL) && (*setevents != 0) )
    {
	if ( *setevents == reqevents )
	{
	    log_debug("socket already added to pollset with reqevents [%x != %x]", *setevents, reqevents);
	    return;
	}
	// FIXME: this can result in a leak due to the way apr_pollset works with
	// APR_POLLSET_NOCPOY as described above
	log_debug("removing already added socket with different reqevents");
	nx_module_pollset_remove_socket(module, sock);
    }
    log_debug("socket added [%x]", reqevents);

    pfd.desc.s = sock;
    pfd.desc_type = APR_POLL_SOCKET;
    pfd.reqevents = reqevents;
    pfd.client_data = module;
    pfd.p = module->pool;
    
    if ( (rv = apr_pollset_add(module->pollset, &pfd)) != APR_SUCCESS )
    {
	char errmsg[200];
	
	if ( APR_STATUS_IS_EEXIST(rv) )
	{
//		nx_panic("socket is already added");
//		log_aprerror(rv, "socket is already added");
	}
	else
	{
	    apr_strerror(rv, errmsg, sizeof(errmsg));
	    nx_panic("failed to add descriptor to pollset: %s", errmsg);
	}
    }

    if ( setevents == NULL )
    {
	pool = apr_socket_pool_get(sock);
	setevents = apr_palloc(pool, sizeof(apr_int16_t));
	*setevents = reqevents;
	apr_socket_data_set(sock, (void *) setevents, NX_POLLSET_REQEVENTS_KEY, NULL);
    }
    else
    {
	*setevents = reqevents;
    }
}



void nx_module_pollset_remove_socket(nx_module_t *module,
				     apr_socket_t *sock)
{
    apr_pollfd_t pfd;
    apr_status_t rv;
    apr_int16_t *reqevents = NULL;

    ASSERT(module != NULL);
    ASSERT(sock != NULL);

    apr_socket_data_get((void **) &reqevents, NX_POLLSET_REQEVENTS_KEY, sock);
    if ( reqevents == NULL )
    {
	//log_debug("nothing to remove in nx_module_pollset_remove_socket()");
	return;
    }
    log_debug("remove socket [%x]", *reqevents);
    pfd.desc.s = sock;
    pfd.desc_type = APR_POLL_SOCKET;
    pfd.reqevents = *reqevents;
    pfd.client_data = module;
    pfd.p = module->pool;

    if ( (rv = apr_pollset_remove(module->pollset, &pfd)) != APR_SUCCESS )
    {
	char errmsg[200];

	if ( APR_STATUS_IS_NOTFOUND(rv) )
	{
//		log_aprerror(rv, "Could not remove descriptor from pollset");
	}
	else
	{
	    apr_strerror(rv, errmsg, sizeof(errmsg));
	    nx_panic("failed to remove descriptor to pollset: %s", errmsg);
	}
    }
    *reqevents = 0;
}



void nx_module_add_poll_event(nx_module_t *module)
{
    nx_event_t *event;

    event = nx_event_new();
    event->module = module;
    event->delayed = FALSE;
    event->type = NX_EVENT_POLL;
    event->priority = module->priority;
    nx_lock();
    nx_event_to_jobqueue(event);
    nx_unlock();
}


/*
 * Poll for events on a descriptor
 */
void nx_module_pollset_poll(nx_module_t *module, boolean readd)
{
    apr_status_t rv;
    apr_int32_t num;
    const apr_pollfd_t *ret_pfd;
    int i, j;
    nx_event_t *event[4] = { NULL, NULL, NULL, NULL };
    int eventcnt = 0;
    int errcount = 0;
    apr_uint32_t jobevcnt;
    apr_time_t lasterr = 0;

    ASSERT(module != NULL);

    log_debug("nx_module_pollset_poll: %s", module->name);

    jobevcnt = nx_atomic_read32(&(module->job->event_cnt));
    if ( jobevcnt > 0 )
    {
	log_debug("found other events (%u), adding poll event to end of list", jobevcnt);
/*
	{
	    nx_event_t *ev;
	    nx_string_t *tmpstr;

	    tmpstr = nx_string_new();
	    nx_lock();
	    for ( ev = NX_DLIST_FIRST(&(module->job->events));
		  ev != NULL;
		  ev = NX_DLIST_NEXT(ev, link) )
	    {
		//printf(" %s\n", nx_event_type_to_string(ev->type));
		nx_string_sprintf_append(tmpstr, " %s\n", nx_event_type_to_string(ev->type));
	    }
	    nx_unlock();
	    log_debug("%s", tmpstr->buf);
	    nx_string_free(tmpstr);
	}
*/
	nx_module_add_poll_event(module);
	return;
    }

    nx_atomic_set32(&(module->in_poll), 1);
    rv = apr_pollset_poll(module->pollset, NX_POLL_TIMEOUT, &num, &ret_pfd);
    nx_atomic_set32(&(module->in_poll), 0);

    if ( rv == APR_SUCCESS )
    {
	log_debug("module %s got %d poll events", module->name, num);
	    
	for ( i = 0; i < num; i++ )
	{	
	    eventcnt = 0;

	    if ( (ret_pfd[i].rtnevents & (APR_POLLIN | APR_POLLPRI)) != 0 )
	    {
		log_debug("Module %s can read", module->name);
		event[eventcnt] = nx_event_new();
		event[eventcnt]->module = module;
		event[eventcnt]->delayed = FALSE;
		event[eventcnt]->type = NX_EVENT_READ;
		event[eventcnt]->priority = module->priority;
		if ( ret_pfd[i].desc_type == APR_POLL_SOCKET )
		{
		    event[eventcnt]->data = (void *) ret_pfd[i].desc.s;
		}
		eventcnt++;
	    }

	    if ( (ret_pfd[i].rtnevents & APR_POLLOUT) != 0 )
	    {
		log_debug("Module %s can write", module->name);
		event[eventcnt] = nx_event_new();
		event[eventcnt]->module = module;
		event[eventcnt]->delayed = FALSE;
		event[eventcnt]->type = NX_EVENT_WRITE;
		event[eventcnt]->priority = module->priority;
		if ( ret_pfd[i].desc_type == APR_POLL_SOCKET )
		{
		    event[eventcnt]->data = (void *) ret_pfd[i].desc.s;
		}
		eventcnt++;
	    }

	    if ( (ret_pfd[i].rtnevents & APR_POLLHUP) != 0 )
	    {
		log_debug("Module %s got disconnect", module->name);
		event[eventcnt] = nx_event_new();
		event[eventcnt]->module = module;
		event[eventcnt]->delayed = FALSE;
		event[eventcnt]->type = NX_EVENT_DISCONNECT;
		event[eventcnt]->priority = module->priority;
		if ( ret_pfd[i].desc_type == APR_POLL_SOCKET )
		{
		    event[eventcnt]->data = (void *) ret_pfd[i].desc.s;
		}
		eventcnt++;
	    }

	    if ( (ret_pfd[i].rtnevents & APR_POLLNVAL) != 0 )
	    { // Descriptor invalid 
		log_error("invalid descriptor in poll, removing");
		apr_pollset_remove(module->pollset, &ret_pfd[i]);
		apr_sleep(APR_USEC_PER_SEC * 1); //sleep 1 sec in order to avoid flooding logs if it breaks totally
	    }

	    if ( ret_pfd[i].rtnevents == APR_POLLERR )
	    {
		log_error("Module %s got POLLERR", module->name);
	    }
	    else
	    {
		if ( eventcnt == 0 )
		{
		    log_error("unhandled event! (reqevents: %d, rtnevents: %d)", 
			      ret_pfd[i].reqevents, ret_pfd[i].rtnevents);
		    apr_sleep(APR_USEC_PER_SEC * 1); //sleep 1 sec in order to avoid flooding logs if it breaks totally
		}
	    }

	    if ( eventcnt > 0 )
	    {
		nx_lock();
		for ( j = 0; j < eventcnt; j++ )
		{
		    nx_event_to_jobqueue(event[j]);
		}
		nx_unlock();
	    }
	}
    }
    else if ( APR_STATUS_IS_TIMEUP(rv) )
    {
	log_debug("[%s] no poll events, pollset_poll timed out", module->name);
    }
    else if ( APR_STATUS_IS_EINTR(rv) )
    {
	log_debug("[%s] apr_pollset_poll was interrupted", module->name);
    }
    else
    {
	log_aprerror(rv, "[%s] apr_pollset_poll failed (errorcode: %d)", module->name, rv);
	apr_sleep(APR_USEC_PER_SEC * 1); //sleep 1 sec in order to avoid flooding logs if it breaks totally
	if ( errcount >= 10 )
	{
	    nx_panic("[%s] reached critical error count in poll loop", module->name);
	}
	if ( lasterr + APR_USEC_PER_SEC * 5 >= apr_time_now() )
	{ // if the last error was within 5 seconds
	    errcount++;
	}
	lasterr = apr_time_now();
    }

    if ( (eventcnt == 0) || (readd == TRUE) )
    {
	nx_module_add_poll_event(module);
    }

    return;
}
