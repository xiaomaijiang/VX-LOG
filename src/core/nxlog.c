/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <unistd.h>
#include <apr_getopt.h>
#include <apr_thread_proc.h>
#include <apr_signal.h>
#include <apr_file_info.h>

#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "../common/error_debug.h"
#include "../common/cfgfile.h"
#include "../common/event.h"
#include "../common/route.h"
#include "../common/alloc.h"
#include "../common/atomic.h"
#include "job.h"
#include "modules.h"
#include "router.h"
#include "nxlog.h"
#include "core.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


void nxlog_shutdown(nxlog_t *nxlog)
{
    log_debug("nxlog_shutdown() enter");

    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_INPUT);
    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_PROCESSOR);
    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_OUTPUT);
    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_EXTENSION);

    nx_lock();
    nxlog->terminating = TRUE;
    if ( nxlog->worker_cond != NULL )
    {
	ASSERT(apr_thread_cond_broadcast(nxlog->worker_cond) == APR_SUCCESS);
    }
    if ( nxlog->event_cond != NULL )
    {
	ASSERT(apr_thread_cond_signal(nxlog->event_cond) == APR_SUCCESS);
    }
    nx_unlock();

    nxlog_wait_threads(nxlog);

    nx_ctx_save_queues(nxlog->ctx);
    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_INPUT);
    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_PROCESSOR);
    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_OUTPUT);
    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_EXTENSION);
    nx_config_cache_write();
    nx_config_cache_free();
    log_debug("nxlog_shutdown() leave");
}



void nxlog_wait_threads(nxlog_t *nxlog)
{
    unsigned int i;
    boolean running = TRUE;
    int j = 0;
    apr_status_t thrv;

    ASSERT(nxlog->terminating == TRUE);
    
    if ( nxlog->worker_threads_running == NULL )
    {
	return;
    }

    while ( running == TRUE )
    {
	running = FALSE;
	for ( i = 0; i < nxlog->num_worker_thread; i++ )
	{
	    if ( nx_atomic_read32(&(nxlog->worker_threads_running[i])) == TRUE )
	    {
		running = TRUE;
		break;
	    }
	}

	if ( nx_atomic_read32(&(nxlog->event_thread_running)) == TRUE )
	{
	    running = TRUE;
	}
	if ( running == TRUE )
	{
	    j++;
	    if ( j > 10 * 55 ) // wait 55 sec at most for threads to exit
	    {
		log_error("timed out waiting for threads to exit");
		return;
	    }
	    nx_lock();
	    ASSERT(apr_thread_cond_broadcast(nxlog->worker_cond) == APR_SUCCESS);
	    ASSERT(apr_thread_cond_signal(nxlog->event_cond) == APR_SUCCESS);
	    nx_unlock();
	    if ( nx_atomic_read32(&(nxlog->event_thread_running)) == TRUE )
	    {
		log_debug("event_thread still running, waiting for threads to exit");
	    }
	    else
	    {
		log_debug("thread %d still running, waiting for threads to exit", i);
	    }
	    apr_sleep(APR_USEC_PER_SEC / 10);
	}
    }
    
    CHECKERR(apr_thread_join(&thrv, nxlog->event_thread));
    nxlog->event_thread = NULL;
}



void nxlog_init(nxlog_t *nxlog)
{
    char tmpstr[256];
    size_t i;
    apr_status_t rv;
    apr_sockaddr_t *sa;

    memset(nxlog, 0, sizeof(nxlog_t));
    nxlog_set(nxlog);

    nxlog->pool = nx_pool_create_core();

    nxlog->started = apr_time_now();

    nxlog->ctx = nx_ctx_new();
    nx_ctx_register_builtins(nxlog->ctx);

    CHECKERR(apr_thread_mutex_create(&(nxlog->mutex), APR_THREAD_MUTEX_UNNESTED, nxlog->pool));
    CHECKERR(apr_thread_cond_create(&(nxlog->event_cond), nxlog->pool));
    CHECKERR(apr_thread_cond_create(&(nxlog->worker_cond), nxlog->pool));

    nxlog->daemonized = FALSE;
    nxlog->num_worker_thread = 0;

    nxlog->pid = (int) getpid();

    // determine the short and fqdn hostname
    if ( apr_gethostname(tmpstr, sizeof(tmpstr), NULL) == APR_SUCCESS )
    {
	for ( i = 0; i < strlen(tmpstr); i++ )
	{
	    if ( tmpstr[i] == '.' )
	    { // if we find a dot , the hostname is in FQDN form
		nxlog->hostname.buf = apr_pstrndup(nxlog->pool, tmpstr, i);
		nxlog->hostname.len = (uint32_t) i;

		nxlog->hostname_fqdn.buf = apr_pstrdup(nxlog->pool, tmpstr);
		nxlog->hostname_fqdn.len = (uint32_t) strlen(tmpstr);
		break;
	    }
	}
	if ( nxlog->hostname.buf == NULL )
	{
	    nxlog->hostname.buf = apr_pstrdup(nxlog->pool, tmpstr);
	    nxlog->hostname.len = (uint32_t) strlen(tmpstr);
	}

	if ( nxlog->hostname_fqdn.buf == NULL )
	{
	    if ((rv = apr_sockaddr_info_get(&sa, tmpstr, APR_INET, 0, 0, nxlog->pool)) != APR_SUCCESS )
	    {
		log_aprwarn(rv, "failed to determine FQDN, cannot resolve %s", tmpstr);
		nxlog->hostname_fqdn.buf = apr_pstrdup(nxlog->pool, "localhost.localdomain");
	    }
	    else if ((rv = apr_getnameinfo(&(nxlog->hostname_fqdn.buf), sa, 0)) != APR_SUCCESS )
	    {
		log_aprwarn(rv, "failed to determine FQDN hostname");
		nxlog->hostname_fqdn.buf = apr_pstrdup(nxlog->pool, tmpstr);
	    }

	    nxlog->hostname_fqdn.len = (uint32_t) strlen(nxlog->hostname_fqdn.buf);
	}
    }
    else
    {
	nxlog->hostname.buf = apr_pstrdup(nxlog->pool, "localhost");
	nxlog->hostname.len = (uint32_t) strlen(nxlog->hostname.buf);
	nxlog->hostname_fqdn.buf = apr_pstrdup(nxlog->pool, "localhost.localdomain");
	nxlog->hostname_fqdn.len = (uint32_t) strlen(nxlog->hostname_fqdn.buf);
    }
    nxlog->hostname.flags = NX_STRING_FLAG_CONST;
    nxlog->hostname_fqdn.flags = NX_STRING_FLAG_CONST;
    ASSERT(nxlog->hostname.buf != NULL);
    ASSERT(nxlog->hostname_fqdn.buf != NULL);
}



void nxlog_reload(nxlog_t *nxlog)
{
    apr_status_t rv;
    apr_file_t *file;
    nx_ctx_t *ctx_old;
    log_info("reloading configuration and restarting modules");

    ASSERT(nxlog->cfgfile != NULL);
    // Will fail to open relative config file path in daemon mode, 
    // because we are now chroot-ed to '/' 
    if ( (rv = apr_file_open(&file, nxlog->cfgfile, APR_READ | APR_BUFFERED, APR_OS_DEFAULT,
			     nxlog->pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "Couldn't open config file '%s', not reloading", nxlog->cfgfile);
	return;
    }
    apr_file_close(file);

    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_INPUT);
    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_PROCESSOR);
    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_OUTPUT);
    nx_ctx_stop_modules(nxlog->ctx, NX_MODULE_TYPE_EXTENSION);

    nx_lock();
    nxlog->terminating = TRUE;
    ASSERT(apr_thread_cond_broadcast(nxlog->worker_cond) == APR_SUCCESS);
    ASSERT(apr_thread_cond_signal(nxlog->event_cond) == APR_SUCCESS);
    nx_unlock();

    nxlog_wait_threads(nxlog);

    nx_ctx_save_queues(nxlog->ctx);
    nx_config_cache_write();

    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_INPUT);
    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_PROCESSOR);
    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_OUTPUT);
    nx_ctx_shutdown_modules(nxlog->ctx, NX_MODULE_TYPE_EXTENSION);
    
    ctx_old = nxlog->ctx;
    nxlog->ctx = nx_ctx_new();
    nx_ctx_free(ctx_old);
    nxlog->num_worker_thread = 0;

    // reset flags
    nx_atomic_set32(&(nxlog->reload_request), FALSE);
    nxlog->terminating = FALSE;

    nx_ctx_register_builtins(nxlog->ctx);

    nx_ctx_parse_cfg(nxlog->ctx, nxlog->cfgfile);

    if ( nxlog->ctx->spooldir != NULL )
    { 
	CHECKERR_MSG(apr_filepath_set(nxlog->ctx->spooldir, nxlog->pool),
		     "Couldn't change to SpoolDir '%s'", nxlog->ctx->spooldir);
    }

    nx_ctx_init_logging(nxlog->ctx);

    // read config cache
    nx_config_cache_read();

    // load DSO and read and verify module config
    nx_ctx_config_modules(nxlog->ctx);

    // initialize modules
    nx_ctx_init_modules(nxlog->ctx);

    // initialize log routes
    nx_ctx_init_routes(nxlog->ctx);

    nx_ctx_restore_queues(nxlog->ctx);

    nx_ctx_init_jobs(nxlog->ctx);

    // setup threadpool
    nxlog_create_threads(nxlog);

    nx_ctx_start_modules(nxlog->ctx);

    log_info("configuration reloaded successfully.");
}



static void* APR_THREAD_FUNC nxlog_event_thread(apr_thread_t *thd, void *data UNUSED)
{
    nx_event_t *event = NULL;
    nx_event_t *tmpevent = NULL;
    apr_time_t next_run = 0;
    apr_time_t now = 0;
    boolean done = FALSE;
    nxlog_t *nxlog;
    nx_ctx_t *ctx;

    nxlog = nxlog_get();
    ctx = nx_ctx_get();

    nx_atomic_set32(&(nxlog->event_thread_running), TRUE);

    log_debug("event thread started");

    while ( done != TRUE )
    {
	next_run = 0;
	now = apr_time_now();

	nx_lock();
	event = nx_event_next(NULL);
	while ( event != NULL )
	{
	    log_debug("new event in event_thread [%s:%s]",
		      event->module == NULL ? "core" : event->module->name,
		      nx_event_type_to_string(event->type));

	    ASSERT(event->delayed == TRUE);

	    if ( event->time <= now )
	    {
		tmpevent = event;
		event = nx_event_next(event);
		NX_DLIST_REMOVE(ctx->events, tmpevent, link);
		//log_debug("event 0x%lx removed in event_thread", tmpevent);
		nx_event_to_jobqueue(tmpevent);
		//NX_DLIST_CHECK(ctx->events, link);
	    }
	    else
	    { // time in the future
		apr_time_t tmp_next_run = 0;
		
		// get next event when we need to prcess events again
		tmp_next_run = event->time;
		
		if ( (tmp_next_run < next_run) || (next_run == 0) )
		{
		    next_run = tmp_next_run;
		}
		event = nx_event_next(event);
	    }
	}

	// wait for new event
	if ( next_run > 0 )
	{
	    apr_interval_time_t next_run_relative = next_run - now;
	    
	    if ( next_run_relative > 0 )
	    {
		log_debug("future event, event thread sleeping %ldms in cond_timedwait",
			  (long int) next_run_relative);
		apr_thread_cond_timedwait(nxlog->event_cond, nxlog->mutex,
					  next_run_relative);
	    }
	    else
	    {
		log_debug("next run rel: %llu", (long long unsigned int) next_run_relative);
	    }
	}
	else
	{ // no events or no future event, wait for signal indefinetely
	    log_debug("no events or no future events, event thread sleeping in condwait");

	    apr_thread_cond_timedwait(nxlog->event_cond, nxlog->mutex, NX_POLL_TIMEOUT);
	}
	nx_unlock();

	if ( nxlog->terminating == TRUE )
	{
	    if ( nxlog_data_available() != TRUE )
	    {
		log_debug("data_available() == FALSE, processing finished");
		done = TRUE;
	    }
	    else
	    {
		log_debug("data_available() == TRUE");
	    }
	}
    }

    log_debug("event thread exiting");
    nx_atomic_set32(&(nxlog->event_thread_running), FALSE);

    apr_thread_exit(thd, APR_SUCCESS);

    return ( NULL );
}


static void* APR_THREAD_FUNC nxlog_worker_thread(apr_thread_t *thd, void *data UNUSED)
{
    nx_event_t *event = NULL;
    nxlog_t *nxlog;
    boolean done = FALSE;
    unsigned int worker_id;
    boolean found = FALSE;
    nx_job_t *job;
    nx_ctx_t *ctx;
    boolean terminating = FALSE;
    nx_exception_t e;

    nxlog = nxlog_get();
    ctx = nx_ctx_get();

    for ( worker_id = 0; worker_id < nxlog->num_worker_thread; worker_id++ )
    {
	if ( nxlog->worker_threads[worker_id] == thd )
	{
	    found = TRUE;
	    break;
	}
    }
    ASSERT(found == TRUE);

    nx_atomic_set32(&(nxlog->worker_threads_running[worker_id]), TRUE);

    log_debug("worker thread %d started", worker_id);

    while ( done != TRUE )
    {
	nx_lock();

	job = NULL;
	event = NULL;
	if ( nx_ctx_next_job(ctx, &job, &event) != TRUE )
	{ // no jobs in workerqueue, wait for signal
	    log_debug("worker %u waiting for new event", worker_id);
	    CHECKERR(apr_thread_cond_wait(nxlog->worker_cond, nxlog->mutex));
	    log_debug("worker %u got signal for new job", worker_id);
	    nx_ctx_next_job(ctx, &job, &event);
	}
	if ( job != NULL )
	{
	    ASSERT(nx_atomic_read32(&(job->busy)) == FALSE);
	    nx_atomic_set32(&(job->busy), TRUE);
	}
	terminating = nxlog->terminating;
	nx_unlock();

	if ( event != NULL )
	{
	    log_debug("worker %u processing event 0x%lx", worker_id, (long unsigned) event);
	    try
	    {
		nx_event_process(event);
	    }
	    catch(e)
	    {
		log_exception(e);
	    }
	    nx_event_free(event);
	    nx_atomic_set32(&(job->busy), FALSE);
	}
	else
	{
	    log_debug("worker %u got no event to process", worker_id);
	}

	if ( (terminating == TRUE) && (nxlog_data_available() != TRUE) )
	{
	    done = TRUE;
	}
    }

    log_debug("worker thread %d exiting", worker_id);
    nx_atomic_set32(&(nxlog->worker_threads_running[worker_id]), FALSE);

    apr_thread_exit(thd, APR_SUCCESS);

    return ( NULL );
}



void nxlog_create_threads(nxlog_t *nxlog)
{
    unsigned i;
    nx_module_t *module;
    unsigned int pollset_cnt = 0;
    unsigned int module_cnt = 0;
    unsigned int thread_cnt = 0;

    ASSERT(nxlog->ctx != NULL);

    // calculate the number of worker threads needed for optimal operation
    for ( module = NX_DLIST_FIRST(nxlog->ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	module_cnt++;
	if ( module->type == NX_MODULE_TYPE_EXTENSION ) 
	{
	    if ( module->pollset != NULL )
	    {
		pollset_cnt++;
	    }
	}
	else if ( module->refcount > 0 )
	{
	    if ( module->pollset != NULL )
	    {
		pollset_cnt++;
	    }
	}
    }

    if ( module_cnt > pollset_cnt )
    {
	thread_cnt = pollset_cnt + 1;
    }
    else
    {
	thread_cnt = pollset_cnt;
    }

    if ( module_cnt > thread_cnt )
    {
	if ( (module_cnt - thread_cnt) / 2 == 0 )
	{
	    thread_cnt++;
	}
	else if ( module_cnt - thread_cnt > 4)
	{
	    thread_cnt += 4;
	}
	else
	{
	    thread_cnt += (module_cnt - thread_cnt) / 2;
	}
    }
    if ( thread_cnt == 0 )
    {
	thread_cnt++;
    }
    if ( nxlog->num_worker_thread > 0 )
    {
	if (thread_cnt > nxlog->num_worker_thread )
	{
	    log_warn("Worker thread count should be increased to %d. Using the value of %d explicitely defined in Threads may result in decreased performance!", thread_cnt, nxlog->num_worker_thread);
	}
    }
    else
    { // if Threads is not defined, we use the calculated number
	nxlog->num_worker_thread = thread_cnt;
    }

    nx_lock();

    nxlog->worker_threads = apr_palloc(nxlog->pool, sizeof(apr_thread_t *) * nxlog->num_worker_thread);
    nxlog->worker_threads_running = apr_pcalloc(nxlog->pool, sizeof(uint32_t) * nxlog->num_worker_thread);

    log_debug("spawning %d worker threads", nxlog->num_worker_thread);
    for ( i = 0; i < nxlog->num_worker_thread; i++)
    {
	nx_thread_create(&(nxlog->worker_threads[i]), NULL, nxlog_worker_thread, NULL, nxlog->pool);
    }

    nx_thread_create(&(nxlog->event_thread), NULL, nxlog_event_thread, NULL, nxlog->pool);

    nx_unlock();
}



boolean nxlog_data_available()
{
    nx_jobgroup_t *jobgroup;
    nx_job_t *job;
    nx_event_t *event = NULL;
    nx_ctx_t *ctx;
    int i;
    nx_route_t *route;
    nx_module_t *module;
    nx_event_type_t type;

    ctx = nx_ctx_get();

    nx_lock();

    for ( jobgroup = NX_DLIST_FIRST(ctx->jobgroups);
	  jobgroup != NULL;
	  jobgroup = NX_DLIST_NEXT(jobgroup, link) )
    {
	for ( job = NX_DLIST_FIRST(&(jobgroup->jobs));
	      job != NULL;
	      job = NX_DLIST_NEXT(job, link) )
	{
	    if ( job->busy == TRUE )
	    {
		for ( module = NX_DLIST_FIRST(ctx->modules);
		      module != NULL;
		      module = NX_DLIST_NEXT(module, link) )
		{
		    if ( module->job == job )
		    {
			log_debug("found a busy job on module %s (status: %s) while checking for unprocessed data", 
				  module->name, nx_module_status_to_string(nx_module_get_status(module)));
			break;
		    }
		}
		if ( (module != NULL) && 
		     (nx_module_get_status(module) != NX_MODULE_STATUS_STOPPED) )
		{
		    nx_unlock();
		    return ( TRUE );
		}
	    }
	    for ( event = NX_DLIST_FIRST(&(job->events));
		  event != NULL;
		  event = NX_DLIST_NEXT(event, link) )
	    {
		switch ( event->type )
		{
		    case NX_EVENT_READ:
		    case NX_EVENT_WRITE:
		    case NX_EVENT_DATA_AVAILABLE:
			type = event->type;
			log_debug("found %s event while checking for unprocessed data",
				  nx_event_type_to_string(type));
			if ( (event->module != NULL) &&
			     (nx_module_get_status(event->module) != NX_MODULE_STATUS_STOPPED) )
			{
			    nx_unlock();
			    return ( TRUE );
			}
		    default:
			break;
		}
	    }
	}
    }

    while ( (event = nx_event_next(event)) != NULL )
    {
	if ( event->delayed == FALSE ) 
	{
	    switch ( event->type )
	    {
		case NX_EVENT_READ:
		case NX_EVENT_WRITE:
		case NX_EVENT_DATA_AVAILABLE:
		    nx_unlock();
		    return ( TRUE );
		default:
		    break;
	    }
	}
    }

    nx_unlock();

    for ( route = NX_DLIST_FIRST(ctx->routes);
          route != NULL;
	  route = NX_DLIST_NEXT(route, link) )
    {
	for ( i = 0; i < route->modules->nelts; i++ )
	{
	    module = ((nx_module_t **) route->modules->elts)[i];
	    if ( (nx_module_get_status(module) != NX_MODULE_STATUS_STOPPED) &&
		 (module->queue != NULL) &&
		 (nx_logqueue_size(module->queue) > 0) )
	    {
		log_debug("module %s has a non-empty queue (%d) with status %s while checking for unprocessed data",
			  module->name, nx_logqueue_size(module->queue),
			  nx_module_status_to_string(nx_module_get_status(module)));
		if ( nx_module_get_status(module) != NX_MODULE_STATUS_UNINITIALIZED )
		{
		    nx_module_data_available(module);
		    return ( TRUE );
		}
	    }
	}  
    }

    return ( FALSE );
}



void nxlog_mainloop(nxlog_t *nxlog, boolean offline)
{

    // TODO put event loop here and get rid of event_thread

    while ( nxlog->terminate_request != TRUE )
    {
	apr_sleep(NX_POLL_TIMEOUT);

	if ( offline == TRUE )
	{
	    if ( nxlog_data_available() != TRUE )
	    {
		break;
	    }
	}
	if ( nx_atomic_read32(&(nxlog->reload_request)) == TRUE )
	{
	    nxlog_reload(nxlog);
	}
    }
    if ( offline != TRUE )
    {
	log_warn(PACKAGE" received a termination request signal, exiting...");
    }
}



void nxlog_dump_info()
{
    nx_ctx_t *ctx;
    nx_route_t *route;
    int queuesize = 0;
    nx_jobgroup_t *jobgroup;
    nx_job_t *job;
    nx_event_t *event;
    int events = 0;
    int i;
    nx_module_t *module;
    nx_string_t *infostr;
    nx_event_type_t eventtypes[NX_EVENT_TYPE_LAST + 1];

    ctx = nx_ctx_get();

    infostr = nx_string_new();
    nx_lock();

    for ( event = NX_DLIST_FIRST(ctx->events);
	  event != NULL;
	  event = NX_DLIST_NEXT(event, link) )
    {
	events++;
    }
    nx_string_sprintf_append(infostr, "event queue has %d events"NX_LINEFEED, events);

    for ( jobgroup = NX_DLIST_FIRST(ctx->jobgroups);
	  jobgroup != NULL;
	  jobgroup = NX_DLIST_NEXT(jobgroup, link) )
    {
	nx_string_sprintf_append(infostr, "jobgroup with priority %d"NX_LINEFEED, jobgroup->priority);
	for ( job = NX_DLIST_FIRST(&(jobgroup->jobs));
	      job != NULL;
	      job = NX_DLIST_NEXT(job, link) )
	{
	    module = nx_ctx_module_for_job(ctx, job);
	    if ( module != NULL )
	    {
		nx_string_sprintf_append(infostr, "job of module %s/%s",
					 module->name, module->dsoname);
	    }
	    else
	    {
		nx_string_append(infostr, "non-module job", -1);
	    }
	    events = 0;
	    memset(eventtypes, 0, sizeof(eventtypes));
	    for ( event = NX_DLIST_FIRST(&(job->events));
		  event != NULL;
		  event = NX_DLIST_NEXT(event, link) )
	    {
		//nx_string_sprintf_append(infostr, " %s"NX_LINEFEED, nx_event_type_to_string(event->type));
		//printf(" %s\n", nx_event_type_to_string(event->type));
		eventtypes[event->type]++;
		events++;
	    }
	    nx_string_sprintf_append(infostr, ", events: %d"NX_LINEFEED, events);
	    for ( i = 1; i <= NX_EVENT_TYPE_LAST; i++ )
	    {
		if ( eventtypes[i] > 0 )
		{
		    nx_string_sprintf_append(infostr, " %s: %d"NX_LINEFEED, nx_event_type_to_string(i),
					     eventtypes[i]);
		}
	    }
	}
    }
    nx_unlock();

    for ( route = NX_DLIST_FIRST(ctx->routes);
	  route != NULL;
	  route = NX_DLIST_NEXT(route, link) )
    {
	nx_string_sprintf_append(infostr, "[route %s]"NX_LINEFEED, route->name);

	for ( i = 0; i < route->modules->nelts; i++ )
	{
	    module = ((nx_module_t **)route->modules->elts)[i];
	    switch ( module->type )
	    {
		case NX_MODULE_TYPE_INPUT:
		    queuesize = 0;
		    break;
		case NX_MODULE_TYPE_OUTPUT:
		    queuesize = module->queue->size;
		    break;
		case NX_MODULE_TYPE_PROCESSOR:
		    queuesize = module->queue->size;
		    break;
		default:
		    nx_panic("invalid module type");
	    }
	    nx_string_sprintf_append(infostr, " - %s: type %s, status: %s queuesize: %d"NX_LINEFEED,
				     module->name, nx_module_type_to_string(module->type),
				     nx_module_status_to_string(nx_module_get_status(module)),
				     queuesize);
	}
    }

    log_info("%s", infostr->buf);
    nx_string_free(infostr);
}
