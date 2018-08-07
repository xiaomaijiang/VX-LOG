/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_SCHEDULE_H
#define __NX_SCHEDULE_H

#include "types.h"
#include "expr.h"

#define NX_SCHEDULE_MIN_FIRST 0
#define NX_SCHEDULE_MIN_LAST 59
#define NX_SCHEDULE_MIN_COUNT (NX_SCHEDULE_MIN_LAST - NX_SCHEDULE_MIN_FIRST + 1)

#define NX_SCHEDULE_HOUR_FIRST 0
#define NX_SCHEDULE_HOUR_LAST 23
#define NX_SCHEDULE_HOUR_COUNT (NX_SCHEDULE_HOUR_LAST - NX_SCHEDULE_HOUR_FIRST + 1)

#define NX_SCHEDULE_DAY_FIRST 1
#define NX_SCHEDULE_DAY_LAST 31
#define NX_SCHEDULE_DAY_COUNT (NX_SCHEDULE_DAY_LAST - NX_SCHEDULE_DAY_FIRST + 1)

#define NX_SCHEDULE_MONTH_FIRST 1
#define NX_SCHEDULE_MONTH_LAST 12
#define NX_SCHEDULE_MONTH_COUNT (NX_SCHEDULE_MONTH_LAST - NX_SCHEDULE_MONTH_FIRST + 1)

// Sunday is 0 or 7
#define NX_SCHEDULE_WDAY_FIRST 0
#define NX_SCHEDULE_WDAY_LAST 7
#define NX_SCHEDULE_WDAY_COUNT (NX_SCHEDULE_WDAY_LAST - NX_SCHEDULE_WDAY_FIRST + 1)

typedef struct nx_schedule_entry_list_t nx_schedule_entry_list_t;
NX_DLIST_HEAD(nx_schedule_entry_list_t, nx_schedule_entry_t);


typedef enum nx_schedule_entry_type_t
{
    NX_SCHEDULE_ENTRY_TYPE_EVERY = 1,
    NX_SCHEDULE_ENTRY_TYPE_CRONTAB,
} nx_schedule_entry_type_t;


typedef struct nx_schedule_entry_t
{
    nx_schedule_entry_type_t type;
    NX_DLIST_ENTRY(nx_schedule_entry_t) link;
    union
    {
	struct
	{
	    int64_t	every;	///< interval in seconds
	    apr_time_t	first;	///< first run, otherwise 0 to start immediately
	};
	struct
	{
	    int8_t	min[NX_SCHEDULE_MIN_COUNT];
	    int8_t	hour[NX_SCHEDULE_HOUR_COUNT];
	    int8_t	day[NX_SCHEDULE_DAY_COUNT];
	    int8_t	month[NX_SCHEDULE_MONTH_COUNT];
	    int8_t	wday[NX_SCHEDULE_WDAY_COUNT];
	};
    };
    nx_expr_statement_list_t *exec;
} nx_schedule_entry_t;

void nx_schedule_entry_parse_crontab(nx_schedule_entry_t *entry, const char *str);
void nx_schedule_entry_parse_every(nx_schedule_entry_t *entry, const char *str);
apr_time_t nx_schedule_entry_next_run(nx_schedule_entry_t *entry, apr_time_t now);

#endif	/* __NX_SCHEDULE_H */
