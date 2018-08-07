/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_STATVAR_H
#define __NX_STATVAR_H

#include "types.h"
#include "str.h"
#include "value.h"
#include "event.h"


typedef enum nx_module_stat_type_t
{
    NX_MODULE_STAT_TYPE_COUNT = 1,
    NX_MODULE_STAT_TYPE_COUNTMIN,
    NX_MODULE_STAT_TYPE_COUNTMAX,
    NX_MODULE_STAT_TYPE_AVG,
    NX_MODULE_STAT_TYPE_AVGMIN,
    NX_MODULE_STAT_TYPE_AVGMAX,
    NX_MODULE_STAT_TYPE_RATE,
    NX_MODULE_STAT_TYPE_RATEMIN,
    NX_MODULE_STAT_TYPE_RATEMAX,
    NX_MODULE_STAT_TYPE_GRAD,
    NX_MODULE_STAT_TYPE_GRADMIN,
    NX_MODULE_STAT_TYPE_GRADMAX,
} nx_module_stat_type_t;



typedef struct nx_module_var_t
{
    char		*name;
    size_t		namelen;
    nx_value_t		value;
    apr_time_t		expiry;		///< 0 for infinite lifetime
    nx_event_t		*event;		///< expiry event
} nx_module_var_t;



typedef struct nx_module_stat_t
{
    char		*name;
    size_t		namelen;
    nx_module_stat_type_t type;
    union
    {
	int64_t			count;
	struct avg
	{
	    int64_t		count;
	} avg;
	struct rate
	{
	    int64_t		count;
	} rate;
	struct grad
	{
	    int64_t		lastrate;
	    int64_t		count;
	} grad;
    };
    nx_value_t		value;
    int64_t		interval;	///< period in seconds over which the statistical counter is calculated
    apr_time_t		last_update;	///< 0 if it was never updated
    apr_time_t		expiry;		///< 0 for infinite lifetime
    nx_event_t		*event;		///< expiry event
} nx_module_stat_t;


nx_module_var_t *nx_module_var_create(nx_module_t *module,
				      const char *name,
				      size_t namelen,
				      apr_time_t expiry);
void nx_module_var_delete(nx_module_t *module, const char *name, size_t namelen);
void nx_module_var_expiry(nx_module_t *module, nx_event_t *event);

nx_module_stat_t *nx_module_stat_create(nx_module_t *module,
					nx_module_stat_type_t type,
					const char *name,
					size_t namelen,
					int64_t interval,
					apr_time_t timeval,
					apr_time_t expiry);
void nx_module_stat_expiry(nx_module_t *module, nx_event_t *event);
void nx_module_stat_delete(nx_module_t *module, const char *name, size_t namelen);
nx_module_stat_type_t nx_module_stat_type_from_string(const char *str);
const char *nx_module_stat_type_to_string(nx_module_stat_type_t type);
void nx_module_stat_update(nx_module_stat_t *stat, apr_time_t timeval);
void nx_module_stat_add(nx_module_stat_t *stat, int64_t value, apr_time_t timeval);

#endif	/* __NX_STATVAR_H */
