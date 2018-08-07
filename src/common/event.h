/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_EVENT_H
#define __NX_EVENT_H

#include "types.h"
#include "module.h"
#include "dlist.h"

typedef enum nx_event_type_t
{
    NX_EVENT_NONE = 0,
    NX_EVENT_POLL,
    NX_EVENT_ACCEPT,
    NX_EVENT_READ,
    NX_EVENT_WRITE,
    NX_EVENT_RECONNECT,
    NX_EVENT_DISCONNECT,
    NX_EVENT_DATA_AVAILABLE,
    NX_EVENT_TIMEOUT,
    NX_EVENT_SCHEDULE,
    NX_EVENT_VAR_EXPIRY,
    NX_EVENT_STAT_EXPIRY,
    NX_EVENT_MODULE_SPECIFIC,
    NX_EVENT_MODULE_START,
    NX_EVENT_MODULE_STOP,
    NX_EVENT_MODULE_PAUSE,
    NX_EVENT_MODULE_RESUME,
    NX_EVENT_MODULE_SHUTDOWN,
} nx_event_type_t;

#define NX_EVENT_TYPE_LAST NX_EVENT_MODULE_SHUTDOWN

struct nx_event_t
{
    NX_DLIST_ENTRY(nx_event_t)	link;
    nx_module_t			*module;
    nx_event_type_t		type;
    boolean			delayed;
    apr_time_t			time;
    void			*data;
    int				priority;
    nx_job_t			*job;
};


nx_event_t *nx_event_new();
void nx_event_free(nx_event_t *event);
void nx_event_to_jobqueue(nx_event_t *event);
void nx_event_add(nx_event_t *event);
void nx_event_dedupe(nx_job_t *job);
void nx_event_remove(nx_event_t *event);
void nx_event_process(nx_event_t *event);
nx_event_t *nx_event_next(nx_event_t *event);
const char *nx_event_type_to_string(nx_event_type_t type);
void nx_lock();
void nx_unlock();

// TODO this function is defined in module.c so this is a hack for nx_event_type_t
// declaration error workaround
void nx_module_remove_events_by_type(nx_module_t *module, nx_event_type_t type);

#endif	/* __NX_EVENT_H */
