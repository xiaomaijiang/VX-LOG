/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_JOB_H
#define __NX_JOB_H

#include <apr_tables.h>
#include <apr_thread_proc.h>

#include "../common/types.h"
#include "../common/dlist.h"

typedef struct nx_job_events_t nx_job_events_t;
NX_DLIST_HEAD(nx_job_events_t, nx_event_t);

typedef struct nx_jobs_t nx_jobs_t;
NX_DLIST_HEAD(nx_jobs_t, nx_job_t);

struct nx_job_t
{
    NX_DLIST_ENTRY(nx_job_t)	link;		///< jobs are linked together in a jobgroup when their priorities equal
    boolean			busy;		///< a worker thread is processing this job
    nx_job_events_t		events;		///< pending events for this job
//    apr_thread_mutex_t		*mutex;
    apr_uint32_t		event_cnt;      ///< number of events in the list
};


typedef struct nx_jobgroup_t
{
    NX_DLIST_ENTRY(nx_jobgroup_t) link;		///< jobgroups are linked together in priority order
    nx_job_t		*last;			///< last job which was processed.
    int			priority;		///< priority of the jobs in this jobgroup
    nx_jobs_t		jobs;			///< list of jobs in this jobgroup
} nx_jobgroup_t;

//#define nx_job_lock(job) CHECKERR(apr_thread_mutex_lock(job->mutex))
//#define nx_job_unlock(job) CHECKERR(apr_thread_mutex_unlock(job->mutex))

#endif	/* __NX_JOB_H */
