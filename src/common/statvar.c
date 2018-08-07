/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "error_debug.h"
#include "statvar.h"
#include "exception.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


nx_module_var_t *nx_module_var_create(nx_module_t *module,
				      const char *name,
				      size_t namelen,
				      apr_time_t expiry)
{
    nx_module_var_t *var;
    apr_hash_t *vars;
    nx_event_t *event;

    ASSERT(module != NULL);
    ASSERT(name != NULL);
    ASSERT(namelen > 0);

    if ( module->vars == NULL )
    {
	module->vars = apr_hash_make(module->pool);
    }
    vars = module->vars;
    var = apr_hash_get(vars, name, (apr_ssize_t) namelen);
    if ( var != NULL )
    { // already created
	return ( var );
    }

    var = malloc(sizeof(nx_module_var_t));
    var->value.defined = FALSE;
    var->name = malloc(namelen + 1);
    var->namelen = namelen;
    var->event = NULL;
    var->expiry = expiry;
    memcpy(var->name, name, namelen);
    var->name[namelen] = '\0';

    apr_hash_set(vars, var->name, (apr_ssize_t) var->namelen, var);

    if ( expiry > 0 )
    {
	event = nx_event_new();
	event->module = module;
	event->type = NX_EVENT_VAR_EXPIRY;
	event->delayed = TRUE;
	event->time = var->expiry;
	event->priority = module->priority;
	event->data = var;
	var->event = event;
	nx_event_add(event);
    }

    return ( var );
}



void nx_module_var_delete(nx_module_t *module, const char *name, size_t namelen)
{
    nx_module_var_t *var;

    ASSERT(module != NULL);
    ASSERT(name != NULL);
    ASSERT(namelen > 0);

    if ( module->vars == NULL )
    {
	return;
    }
    var = apr_hash_get(module->vars, name, (apr_ssize_t) namelen);
    if ( var == NULL )
    {
	return;
    }

    apr_hash_set(module->vars, name, (apr_ssize_t) namelen, NULL);
    if ( var->event != NULL )
    {
	nx_event_remove(var->event);
	nx_event_free(var->event);
	var->event = NULL;
    }
    nx_value_kill(&(var->value));
    free(var->name);
    free(var);
}



void nx_module_var_expiry(nx_module_t *module, nx_event_t *event)
{
    nx_module_var_t *var;

    ASSERT(event->data != NULL);

    var = (nx_module_var_t *) event->data;
    ASSERT(module->vars != NULL);
    log_debug("module variable '%s' expired, deleting", var->name);
    apr_hash_set(module->vars, var->name, (apr_ssize_t) var->namelen, NULL);
    nx_value_kill(&(var->value));
    free(var->name);
    free(var);
}



nx_module_stat_t *nx_module_stat_create(nx_module_t *module,
					nx_module_stat_type_t type,
					const char *name,
					size_t namelen,
					int64_t interval,
					apr_time_t timeval,
					apr_time_t expiry)
{
    nx_module_stat_t *stat;
    apr_hash_t *stats;
    nx_event_t *event;

    ASSERT(module != NULL);
    ASSERT(name != NULL);
    ASSERT(namelen > 0);

    if ( module->stats == NULL )
    {
	module->stats = apr_hash_make(module->pool);
    }
    stats = module->stats;
    stat = apr_hash_get(stats, name, (apr_ssize_t) namelen);
    if ( stat != NULL )
    { // already created
	return ( stat );
    }

    stat = malloc(sizeof(nx_module_stat_t));
    memset(stat, 0, sizeof(nx_module_stat_t));
    stat->value.defined = FALSE;
    stat->value.type = NX_VALUE_TYPE_INTEGER;
    stat->interval = interval;
    stat->name = malloc(namelen + 1);
    stat->namelen = namelen;
    stat->expiry = expiry;
    stat->type = type;
    if ( timeval == 0 )
    {
	stat->last_update = apr_time_now();
    }
    else
    {
	stat->last_update = timeval;
    }
    memcpy(stat->name, name, namelen);
    stat->name[namelen] = '\0';
    apr_hash_set(stats, stat->name, (apr_ssize_t) stat->namelen, stat);

    if ( expiry > 0 )
    {
	event = nx_event_new();
	event->module = module;
	event->type = NX_EVENT_STAT_EXPIRY;
	event->delayed = TRUE;
	event->time = stat->expiry;
	event->priority = module->priority;
	event->data = stat;
	stat->event = event;
	nx_event_add(event);
    }

    return ( stat );
}



void nx_module_stat_delete(nx_module_t *module, const char *name, size_t namelen)
{
    nx_module_stat_t *stat;

    ASSERT(module != NULL);
    ASSERT(name != NULL);
    ASSERT(namelen > 0);

    if ( module->stats == NULL )
    {
	return;
    }
    stat = apr_hash_get(module->stats, name, (apr_ssize_t) namelen);
    if ( stat == NULL )
    {
	return;
    }

    apr_hash_set(module->stats, name, (apr_ssize_t) namelen, NULL);
    if ( stat->event != NULL )
    {
	nx_event_remove(stat->event);
	nx_event_free(stat->event);
	stat->event = NULL;
    }
    free(stat->name);
    free(stat);
}



void nx_module_stat_expiry(nx_module_t *module, nx_event_t *event)
{
    nx_module_stat_t *stat;

    ASSERT(event->data != NULL);

    stat = (nx_module_stat_t *) event->data;
    ASSERT(module->stats != NULL);
    log_debug("module stat '%s' expired, deleting", stat->name);
    apr_hash_set(module->stats, stat->name, (apr_ssize_t) stat->namelen, NULL);
    free(stat->name);
    free(stat);
}



static nx_keyval_t stat_types[] =
{
    { NX_MODULE_STAT_TYPE_COUNT, "count" },
    { NX_MODULE_STAT_TYPE_COUNTMIN, "countmin" },
    { NX_MODULE_STAT_TYPE_COUNTMAX, "countmax" },
    { NX_MODULE_STAT_TYPE_AVG, "avg" },
    { NX_MODULE_STAT_TYPE_AVGMIN, "avgmin" },
    { NX_MODULE_STAT_TYPE_AVGMAX, "avgmax" },
    { NX_MODULE_STAT_TYPE_RATE, "rate" },
    { NX_MODULE_STAT_TYPE_RATEMIN, "ratemin" },
    { NX_MODULE_STAT_TYPE_RATEMAX, "ratemax" },
    { NX_MODULE_STAT_TYPE_GRAD, "grad" },
    { NX_MODULE_STAT_TYPE_GRADMIN, "gradmin" },
    { NX_MODULE_STAT_TYPE_GRADMAX, "gradmax" },
    { 0, NULL },
};



nx_module_stat_type_t nx_module_stat_type_from_string(const char *str)
{
    int i;

    ASSERT(str != NULL);

    for ( i = 0; stat_types[i].value != NULL; i++ )
    {
	if ( strcasecmp(stat_types[i].value, str) == 0 )
	{
	    return ( stat_types[i].key );
	}
    }

    return ( 0 );
}



const char *nx_module_stat_type_to_string(nx_module_stat_type_t type)
{
    int i;

    for ( i = 0; stat_types[i].value != NULL; i++ )
    {
	if ( (int) type == stat_types[i].key )
	{
	    return ( stat_types[i].value );
	}
    }

    nx_panic("invalid stat type: %d", type);

    return ( "invalid" );
}



void nx_module_stat_update(nx_module_stat_t *stat, apr_time_t timeval)
{
    int64_t length;
    int64_t tmp;
    int64_t rate;

    ASSERT(stat != NULL);
    ASSERT(stat->last_update > 0);

    length = timeval - stat->last_update;

    //log_info("last_update: %ld, now: %ld, length: %ld", stat->last_update / APR_USEC_PER_SEC,
    //timeval / APR_USEC_PER_SEC, length / APR_USEC_PER_SEC);

    switch ( stat->type )
    {
	case NX_MODULE_STAT_TYPE_COUNT:
	    stat->value.integer = stat->count;
	    stat->value.defined = TRUE;
	    break;
	case NX_MODULE_STAT_TYPE_COUNTMIN:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		if ( stat->value.defined == TRUE )
		{
		    if ( stat->count < stat->value.integer )
		    {
			stat->value.integer = stat->count;
		    }
		}
		else
		{
		    stat->value.integer = stat->count;
		    stat->value.defined = TRUE;
		}
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_COUNTMAX:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		if ( stat->value.defined == TRUE )
		{
		    if ( stat->count > stat->value.integer )
		    {
			stat->value.integer = stat->count;
		    }
		}
		else
		{
		    stat->value.integer = stat->count;
		    stat->value.defined = TRUE;
		}
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_AVG:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
//FIXME: last value is also needed here
		stat->value.integer = ((stat->avg.count * stat->interval * APR_USEC_PER_SEC) / length) / 2;
		stat->value.defined = TRUE;
/*
		log_info("count(%ld) * interval(%ld) / length(%ld) / 2 = %ld", stat->avg.count,
			 stat->interval, length / APR_USEC_PER_SEC, stat->value.integer);
		log_info("avg2: %ld", stat->avg.count /2);
*/
		stat->avg.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_AVGMIN:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		tmp = ((stat->avg.count * stat->interval * APR_USEC_PER_SEC) / length) / 2;
		if ( stat->value.defined == TRUE )
		{
		    if ( tmp < stat->value.integer )
		    {
			stat->value.integer = tmp;
		    }
		}
		else
		{
		    stat->value.integer = tmp;
		    stat->value.defined = TRUE;
		}
		stat->avg.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_AVGMAX:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		tmp = ((stat->avg.count * stat->interval * APR_USEC_PER_SEC) / length) / 2;
		if ( stat->value.defined == TRUE )
		{
		    if ( tmp > stat->value.integer )
		    {
			stat->value.integer = tmp;
		    }
		}
		else
		{
		    stat->value.integer = tmp;
		    stat->value.defined = TRUE;
		}
		stat->avg.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_RATE:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		stat->value.integer = (stat->rate.count * stat->interval * APR_USEC_PER_SEC) / length;
		stat->value.defined = TRUE;
		stat->rate.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_RATEMIN:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		rate = (stat->rate.count * stat->interval * APR_USEC_PER_SEC) / length;
		if ( stat->value.defined == TRUE )
		{
		    if ( rate < stat->value.integer )
		    {
			stat->value.integer = rate;
		    }
		}
		else
		{
		    stat->value.integer = rate;
		    stat->value.defined = TRUE;
		}
		stat->rate.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_RATEMAX:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		rate = (stat->rate.count * stat->interval * APR_USEC_PER_SEC) / length;
		if ( stat->value.defined == TRUE )
		{
		    if ( rate > stat->value.integer )
		    {
			stat->value.integer = rate;
		    }
		}
		else
		{
		    stat->value.integer = rate;
		    stat->value.defined = TRUE;
		}
		stat->rate.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_GRAD:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		rate = (stat->grad.count * stat->interval * APR_USEC_PER_SEC) / length;
		stat->value.integer = rate - stat->grad.lastrate;
		stat->value.defined = TRUE;
		stat->grad.count = 0;
		stat->grad.lastrate = rate;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_GRADMIN:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		rate = (stat->grad.count * stat->interval * APR_USEC_PER_SEC) / length;
		tmp = rate - stat->grad.lastrate;
		if ( stat->value.defined == TRUE )
		{
		    if ( tmp < stat->value.integer )
		    {
			stat->value.integer = tmp;
		    }
		}
		else
		{
		    stat->value.integer = tmp;
		    stat->value.defined = TRUE;
		}
		stat->grad.lastrate = rate;
		stat->grad.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	case NX_MODULE_STAT_TYPE_GRADMAX:
	    ASSERT(stat->interval > 0);
	    if ( length >= stat->interval * APR_USEC_PER_SEC )
	    { // update if we are over the interval
		rate = (stat->grad.count * stat->interval * APR_USEC_PER_SEC) / length;
		tmp = rate - stat->grad.lastrate;
		if ( stat->value.defined == TRUE )
		{
		    if ( tmp > stat->value.integer )
		    {
			stat->value.integer = tmp;
		    }
		}
		else
		{
		    stat->value.integer = tmp;
		    stat->value.defined = TRUE;
		}
		stat->grad.lastrate = rate;
		stat->grad.count = 0;
		stat->last_update = timeval;
	    }
	    break;
	default:
	    nx_panic("invalid stat type: %d", stat->type);
    }
}



void nx_module_stat_add(nx_module_stat_t *stat, int64_t value, apr_time_t timeval)
{
    nx_module_stat_update(stat, timeval);

    switch ( stat->type )
    {
	case NX_MODULE_STAT_TYPE_COUNT:
	case NX_MODULE_STAT_TYPE_COUNTMIN:
	case NX_MODULE_STAT_TYPE_COUNTMAX:
	    stat->count += value;
	    break;
	case NX_MODULE_STAT_TYPE_AVG:
	case NX_MODULE_STAT_TYPE_AVGMIN:
	case NX_MODULE_STAT_TYPE_AVGMAX:
	    stat->avg.count += value;
	    break;
	case NX_MODULE_STAT_TYPE_RATE:
	case NX_MODULE_STAT_TYPE_RATEMIN:
	case NX_MODULE_STAT_TYPE_RATEMAX:
	    stat->rate.count += value;
	    break;
	case NX_MODULE_STAT_TYPE_GRAD:
	case NX_MODULE_STAT_TYPE_GRADMIN:
	case NX_MODULE_STAT_TYPE_GRADMAX:
	    stat->grad.count += value;
	    break;
	default:
	    nx_panic("invalid stat type: %d", stat->type);
    }
//    log_info("added, count: %ld", stat->avg.count);
}
