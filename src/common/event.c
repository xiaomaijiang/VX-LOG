/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../core/job.h"
#include "../core/nxlog.h"
#include "error_debug.h"
#include "event.h"
#include "atomic.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


const char *nx_event_type_to_string(nx_event_type_t type)
{
    switch ( type )
    {
	case NX_EVENT_POLL:
	    return "POLL";
	case NX_EVENT_ACCEPT:
	    return "ACCEPT";
	case NX_EVENT_READ:
	    return "READ";
	case NX_EVENT_WRITE:
	    return "WRITE";
	case NX_EVENT_RECONNECT:
	    return "RECONNECT";
	case NX_EVENT_DISCONNECT:
	    return "DISCONNECT";
	case NX_EVENT_DATA_AVAILABLE:
	    return "DATA_AVAILABLE";
	case NX_EVENT_TIMEOUT:
	    return "TIMEOUT";
	case NX_EVENT_SCHEDULE:
	    return "SCHEDULE";
	case NX_EVENT_VAR_EXPIRY:
	    return "VAR_EXPIRY";
	case NX_EVENT_STAT_EXPIRY:
	    return "STAT_EXPIRY";
	case NX_EVENT_MODULE_SPECIFIC:
	    return "MODULE_SPECIFIC";
	case NX_EVENT_MODULE_START:
	    return "MODULE_START";
	case NX_EVENT_MODULE_STOP:
	    return "MODULE_STOP";
	case NX_EVENT_MODULE_PAUSE:
	    return "MODULE_PAUSE";
	case NX_EVENT_MODULE_RESUME:
	    return "MODULE_RESUME";
	case NX_EVENT_MODULE_SHUTDOWN:
	    return "MODULE_SHUTDOWN";
	default:
	    nx_panic("invalid type: %d", type);
    }
    return ( "invalid" );
}



// TODO: make this a macro
void nx_lock()
{
    nxlog_t *nxlog;

    nxlog = nxlog_get();
    if ( nxlog->mutex != NULL )
    {
	CHECKERR(apr_thread_mutex_lock(nxlog->mutex));
    }
}



void nx_unlock()
{
    nxlog_t *nxlog;

    nxlog = nxlog_get();
    if ( nxlog->mutex != NULL )
    {
	CHECKERR(apr_thread_mutex_unlock(nxlog->mutex));
    }
}



nx_event_t *nx_event_new()
{
    nx_event_t *event;

    event = malloc(sizeof(nx_event_t));
    memset(event, 0, sizeof(nx_event_t));

    return ( event );
}



void nx_event_free(nx_event_t *event)
{
    ASSERT(event != NULL);

    ASSERT(NX_DLIST_NEXT(event, link) == NULL);
    ASSERT(NX_DLIST_PREV(event, link) == NULL);

    free(event);
    //memset(event, 0xFFFF, sizeof(nx_event_t));
    //log_debug("event 0x%lx freed", event);
}


/*
static void event_queue_check(nx_job_t *job)
{
    apr_uint32_t event_cnt = 0;
    nx_event_t *ev;

    for ( ev = NX_DLIST_FIRST(&(job->events));
	  ev != NULL;
	  ev = NX_DLIST_NEXT(ev, link) )
    {
	event_cnt++;
    }
    ASSERT(event_cnt == job->event_cnt);
}
*/


/* This is a hack to deduplicate events from the queue */
static void _event_dedupe(nx_job_t *job)
{
    boolean found = TRUE;
    nx_event_t *curr, *e1, *e2, *e3, *e4;

    curr = NX_DLIST_LAST(&(job->events));

    while ( (curr != NULL) && (found == TRUE) )
    {
	found = FALSE;
	e4 = curr;
	e3 = NX_DLIST_PREV(e4, link);
	if ( e3 == NULL )
	{
	    break;
	}
	ASSERT(e4->module == e3->module);

	curr = e3;
	// event event
	if ( (e4->type == e3->type) &&
	     (e4->data == e3->data) &&
	     (e4->priority == e3->priority) )
	{
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 1);
	    //event_queue_check(job);
	    found = TRUE;
	}

	// e1    e2     e3    e4
	// Pause Resume Pause Resume
	else if ( (e4->type == NX_EVENT_MODULE_RESUME) &&
		  (e3->type == NX_EVENT_MODULE_PAUSE) &&
		  ((e2 = NX_DLIST_PREV(e3, link)) != NULL) &&
		  (e2->type == NX_EVENT_MODULE_RESUME) &&
		  ((e1 = NX_DLIST_PREV(e2, link)) != NULL) &&
		  (e1->type == NX_EVENT_MODULE_PAUSE) )
	{
	    curr = e2;
	    NX_DLIST_REMOVE(&(job->events), e3, link);
	    nx_event_free(e3);
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 2);
	    //event_queue_check(job);
	    found = TRUE;
	}
	// Resume Pause Resume Pause
	else if ( (e4->type == NX_EVENT_MODULE_PAUSE) &&
		  (e3->type == NX_EVENT_MODULE_RESUME) &&
		  ((e2 = NX_DLIST_PREV(e3, link)) != NULL) &&
		  (e2->type == NX_EVENT_MODULE_PAUSE) &&
		  ((e1 = NX_DLIST_PREV(e2, link)) != NULL) &&
		  (e1->type == NX_EVENT_MODULE_RESUME) )
	{
	    curr = e2;
	    NX_DLIST_REMOVE(&(job->events), e3, link);
	    nx_event_free(e3);
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 2);
	    //event_queue_check(job);
	    found = TRUE;
	}
	//  DataAvailable Resume DataAvailable Resume
	else if ( (e4->type == NX_EVENT_MODULE_RESUME) &&
		  (e3->type == NX_EVENT_DATA_AVAILABLE) &&
		  ((e2 = NX_DLIST_PREV(e3, link)) != NULL) &&
		  (e2->type == NX_EVENT_MODULE_RESUME) &&
		  ((e1 = NX_DLIST_PREV(e2, link)) != NULL) &&
		  (e1->type == NX_EVENT_DATA_AVAILABLE) )

	{
	    curr = e2;
	    NX_DLIST_REMOVE(&(job->events), e3, link);
	    nx_event_free(e3);
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 2);
	    //event_queue_check(job);
	    found = TRUE;
	}
	//  Resume Poll Resume Poll
	else if ( (e4->type == NX_EVENT_POLL) &&
		  (e3->type == NX_EVENT_MODULE_RESUME) &&
		  ((e2 = NX_DLIST_PREV(e3, link)) != NULL) &&
		  (e2->type == NX_EVENT_POLL) &&
		  ((e1 = NX_DLIST_PREV(e2, link)) != NULL) &&
		  (e1->type == NX_EVENT_MODULE_RESUME) )

	{
	    curr = e2;
	    NX_DLIST_REMOVE(&(job->events), e3, link);
	    nx_event_free(e3);
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 2);
	    //event_queue_check(job);
	    found = TRUE;
	}
	//  Poll Resume Poll Resume
	else if ( (e4->type == NX_EVENT_MODULE_RESUME) &&
		  (e3->type == NX_EVENT_POLL) &&
		  ((e2 = NX_DLIST_PREV(e3, link)) != NULL) &&
		  (e2->type == NX_EVENT_MODULE_RESUME) &&
		  ((e1 = NX_DLIST_PREV(e2, link)) != NULL) &&
		  (e1->type == NX_EVENT_POLL) )

	{
	    curr = e2;
	    NX_DLIST_REMOVE(&(job->events), e3, link);
	    nx_event_free(e3);
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 2);
	    //event_queue_check(job);
	    found = TRUE;
	}
	//  DataAvailable Poll DataAvailable Poll
	else if ( (e4->type == NX_EVENT_POLL) &&
		  (e3->type == NX_EVENT_DATA_AVAILABLE) &&
		  ((e2 = NX_DLIST_PREV(e3, link)) != NULL) &&
		  (e2->type == NX_EVENT_POLL) &&
		  ((e1 = NX_DLIST_PREV(e2, link)) != NULL) &&
		  (e1->type == NX_EVENT_DATA_AVAILABLE) )

	{
	    curr = e2;
	    NX_DLIST_REMOVE(&(job->events), e3, link);
	    nx_event_free(e3);
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 2);
	    //event_queue_check(job);
	    found = TRUE;
	}
	//  Poll DataAvailable Poll DataAvailable
	else if ( (e4->type == NX_EVENT_DATA_AVAILABLE) &&
		  (e3->type == NX_EVENT_POLL) &&
		  ((e2 = NX_DLIST_PREV(e3, link)) != NULL) &&
		  (e2->type == NX_EVENT_DATA_AVAILABLE) &&
		  ((e1 = NX_DLIST_PREV(e2, link)) != NULL) &&
		  (e1->type == NX_EVENT_POLL) )

	{
	    curr = e2;
	    NX_DLIST_REMOVE(&(job->events), e3, link);
	    nx_event_free(e3);
	    NX_DLIST_REMOVE(&(job->events), e4, link);
	    nx_event_free(e4);
	    nx_atomic_sub32(&(job->event_cnt), 2);
	    //event_queue_check(job);
	    found = TRUE;
	}
    }
}



// exported for the unit test
void nx_event_dedupe(nx_job_t *job)
{
    _event_dedupe(job);
}



// must be called with the nx lock held
void nx_event_to_jobqueue(nx_event_t *event)
{
    nx_job_t *job;
    nxlog_t *nxlog;

    ASSERT(event->priority != 0);
    ASSERT(NX_DLIST_NEXT(event, link) == NULL);
    ASSERT(NX_DLIST_PREV(event, link) == NULL);

    log_debug("nx_event_to_jobqueue: %s (%s)", nx_event_type_to_string(event->type),
	      event->module == NULL ? "-" : event->module->name);
    
    ASSERT(event->module != NULL);
    job = event->module->job;
    if ( job == NULL )
    {
	nxlog = nxlog_get();
	ASSERT(nxlog->terminating == TRUE);
	return;
    }

    event->job = job;
    //event_queue_check(job);
    NX_DLIST_INSERT_TAIL(&(job->events), event, link);
    //NX_DLIST_CHECK(&(job->events), link);
    nx_atomic_add32(&(job->event_cnt), 1);
    //event_queue_check(job);

    _event_dedupe(job);
    
    if ( NX_DLIST_LAST(&(job->events)) == event )
    { // signal if the event was added
	nxlog = nxlog_get();
	CHECKERR(apr_thread_cond_signal(nxlog->worker_cond));
	log_debug("event added to jobqueue");
    }
}



void nx_event_add(nx_event_t *event)
{
    nx_ctx_t *ctx;
    nxlog_t *nxlog;

    ASSERT(event != NULL);
    ASSERT(event->type != 0);
    ASSERT(event->module != NULL);
    ASSERT(NX_DLIST_NEXT(event, link) == NULL);
    ASSERT(NX_DLIST_PREV(event, link) == NULL);

    nx_lock();

    if ( event->delayed == FALSE )
    {
	nx_event_to_jobqueue(event);
    }
    else
    {
	nxlog = nxlog_get();
	ctx = nxlog->ctx;

	//NX_DLIST_CHECK(ctx->events, link);
	NX_DLIST_INSERT_TAIL(ctx->events, event, link);
	//NX_DLIST_CHECK(ctx->events, link);
	if ( nxlog->event_cond != NULL )
	{ // only signal when the threads are running
	    CHECKERR(apr_thread_cond_signal(nxlog->event_cond));
	}
    }
    
    nx_unlock();
}



void nx_event_remove(nx_event_t *event)
{
    nx_ctx_t *ctx;

    ASSERT(event != NULL);

    nx_lock();
    if ( event->job != NULL )
    {
	NX_DLIST_REMOVE(&(event->job->events), event, link);
	nx_atomic_sub32(&(event->job->event_cnt), 1);
	//event_queue_check(event->job);
	//NX_DLIST_CHECK(&(event->job->events), link);
	//log_debug("event 0x%lx removed from jobqueue", event);
    }
    else
    {
	ctx = nx_ctx_get();
	NX_DLIST_REMOVE(ctx->events, event, link);
	//NX_DLIST_CHECK(ctx->events, link);
	//log_debug("event 0x%lx removed from eventqueue", event);
    }
    nx_unlock();
}



void nx_event_process(nx_event_t *event)
{
    nx_module_t *module;

    ASSERT(event->module != NULL);

    ASSERT(NX_DLIST_NEXT(event, link) == NULL);
    ASSERT(NX_DLIST_PREV(event, link) == NULL);

    module = event->module;
    log_debug("PROCESS_EVENT: %s (%s)", nx_event_type_to_string(event->type), module->name);
    switch ( event->type )
    {
	case NX_EVENT_MODULE_START:
	    nx_module_start_self(module);
	    break;
	case NX_EVENT_MODULE_STOP:
	    nx_module_stop_self(module);
	    break;
	case NX_EVENT_MODULE_PAUSE:
	    nx_module_pause_self(module);
	    break;
	case NX_EVENT_MODULE_RESUME:
	    nx_module_resume_self(module);
	    break;
	case NX_EVENT_MODULE_SHUTDOWN:
	    nx_module_shutdown_self(module);
	    break;
	case NX_EVENT_SCHEDULE:
	    nx_module_process_schedule_event(module, event);
	    break;
	case NX_EVENT_VAR_EXPIRY:
	    nx_module_var_expiry(module, event);
	    break;
	case NX_EVENT_STAT_EXPIRY:
	    nx_module_stat_expiry(module, event);
	    break;
	default:
	    ASSERT(module->decl->event != NULL);
	    module->decl->event(module, event);
	    break;
    }
}



/** 
 * Return the next event, if event is NULL, the first is returned
 * return: NULL if event is the last
 */

nx_event_t *nx_event_next(nx_event_t *event)
{
    nx_event_t *retval;
    nx_ctx_t *ctx;

    if ( event == NULL )
    {
	ctx = nx_ctx_get();
	retval = NX_DLIST_FIRST(ctx->events);
    }
    else
    {
	retval = NX_DLIST_NEXT(event, link);
    }

    return ( retval );
}
