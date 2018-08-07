/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

//#include "om_ssl.h"
#include "../../../common/module.h"
#include "../../../common/alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


void nx_expr_proc__om_ssl_reconnect(nx_expr_eval_ctx_t *eval_ctx,
				    nx_module_t *module,
				    nx_expr_list_t *args UNUSED)
{
    nx_event_t *event;

    ASSERT(module != NULL);
    ASSERT(eval_ctx != NULL);
    ASSERT(eval_ctx->module != NULL);

    if ( module != eval_ctx->module )
    {
	throw_msg("private procedure %s->reconnect() called from %s",
		  module->name, eval_ctx->module->name);
    }

    log_debug("%s->reconnect() called", module->name);

    event = nx_event_new();
    event->module = module;
    event->delayed = FALSE;
    event->type = NX_EVENT_DISCONNECT;
    event->time = apr_time_now();
    event->priority = module->priority;
    nx_event_add(event);
}
