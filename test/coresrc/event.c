/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/core/nxlog.h"
#include "../../src/core/job.h"
#include "../../src/core/modules.h"
#include "../../src/core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

nxlog_t nxlog;

nx_event_t *testevent = NULL;
int evcnt = 0;


static nx_event_t *add_read_event(nx_module_t *module)
{
    nx_event_t *event;

    event = nx_event_new();
    event->module = module;
    event->delayed = FALSE;
    event->type = NX_EVENT_READ;
    event->priority = module->priority;
    nx_event_add(event);
    //log_debug("read event_add 0x%lx", event);
    return ( event);
}



static void im_test_start(nx_module_t *module)
{
    nx_event_t *event;

    log_debug("start");

    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->time = apr_time_now() + 100;
    event->type = NX_EVENT_MODULE_PAUSE;
    event->priority = module->priority;
    nx_event_add(event);
    //log_debug("pause event_add 0x%lx", event);

    testevent = add_read_event(module);
}



static void im_test_pause(nx_module_t *module)
{
    nx_event_t *event;

    ASSERT(module != NULL);

    log_debug("pause");

    if ( testevent != NULL )
    {
	nx_event_remove(testevent);
	nx_event_free(testevent);
	testevent = NULL;
    }

    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->time = apr_time_now() + 10;
    event->type = NX_EVENT_MODULE_RESUME;
    event->priority = module->priority;
    nx_event_add(event);
    //log_debug("resume event_add 0x%lx", event);
}



static void im_test_resume(nx_module_t *module)
{
    nx_event_t *event;

    log_debug("resume");

    ASSERT(module != NULL);

    if ( testevent != NULL )
    {
	nx_event_remove(testevent);
	nx_event_free(testevent);
	//log_debug("event 0x%lx removed from eventqueue", event);
	testevent = NULL;
    }

    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->time = apr_time_now() + 100;
    event->type = NX_EVENT_MODULE_PAUSE;
    event->priority = module->priority;
    nx_event_add(event);
    //log_debug("pause event_add 0x%lx", event);

    testevent = add_read_event(module);
}



static void im_test_read(nx_module_t *module)
{
    log_debug("read");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    evcnt++;
    if ( evcnt <= 100000 )
    {
	testevent = add_read_event(module);
    }
    else
    {
	nxlog.terminating = TRUE;
    }
}



static void im_test_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_test_read(module);
	    break;
	case NX_EVENT_POLL:
	    break;
	default:
	    nx_abort("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION test_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    NULL,			// config
    im_test_start,		// start
    NULL, 			// stop
    im_test_pause,		// pause
    im_test_resume,		// resume
    NULL,			// init
    NULL,			// shutdown
    im_test_event,		// event
    NULL,			// info
    NULL,			// exports
};



static void test_dedupe(nx_module_t *module)
{
    nx_event_t *e1, *e2, *e3, *e4;
    
    nx_event_dedupe(module->job);

    nx_module_set_status(module, NX_MODULE_STATUS_RUNNING);
    nx_module_pause(module);
    nx_module_pause(module);
    nx_module_resume(module);
    nx_module_resume(module);
    nx_module_pause(module);
    nx_module_resume(module);
    nx_module_add_poll_event(module);
    nx_module_resume(module);
    nx_module_add_poll_event(module);
    nx_module_resume(module);
    nx_module_resume(module);
    nx_module_resume(module);
    nx_module_resume(module);

    nx_event_dedupe(module->job);
/*
    {
	nx_event_t *event;

	for ( event = NX_DLIST_FIRST(&(module->job->events));
	      event != NULL;
	      event = NX_DLIST_NEXT(event, link) )
	{
	    printf("%s\n", nx_event_type_to_string(event->type));
	}
    }
*/
    // now we should only have Pause Resume
    e4 = NX_DLIST_LAST(&(module->job->events));
    ASSERT(e4 != NULL);
    ASSERT(e4->type == NX_EVENT_MODULE_RESUME);
    e3 = NX_DLIST_PREV(e4, link);
    ASSERT(e3 != NULL);
    ASSERT(e3->type == NX_EVENT_POLL);
    e2 = NX_DLIST_PREV(e3, link);
    ASSERT(e2 != NULL);
    ASSERT(e2->type == NX_EVENT_MODULE_RESUME);
    e1 = NX_DLIST_PREV(e2, link);
    ASSERT(e1 != NULL);
    ASSERT(e1->type == NX_EVENT_MODULE_PAUSE);
    ASSERT(NULL == NX_DLIST_PREV(e1, link));
}



int main(int argc, const char * const *argv, const char * const *env)
{
    nx_module_t *module;
    nx_directive_t dir;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    nxlog_init(&nxlog);
   
    module = nx_module_new(NX_MODULE_TYPE_INPUT, "testmodule", 0);
    module->dsoname = "im_testmodule";
    module->decl = &test_module;
    memset(&dir, 0, sizeof(nx_directive_t));
    dir.directive = apr_pstrdup(nxlog.pool, "Module");
    dir.args = apr_pstrdup(nxlog.pool, "im_test");
    module->directives = &dir;
    NX_DLIST_INSERT_TAIL(nxlog.ctx->modules, module, link);

    nx_ctx_init_jobs(nxlog.ctx);

    test_dedupe(module);

    //nxlog.ctx->loglevel = NX_LOGLEVEL_DEBUG;
    nxlog_create_threads(&nxlog);

    nx_module_start(module);

    nxlog_mainloop(&nxlog, FALSE);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
