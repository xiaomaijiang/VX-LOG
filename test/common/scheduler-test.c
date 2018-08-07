/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/common/schedule.h"
#include "../../src/common/date.h"
#include "../../src/core/nxlog.h"
#include "../../src/core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_TEST

nxlog_t nxlog;

/*
#  .---------------- minute (0 - 59) 
#  |   .------------- hour (0 - 23)
#  |   |   .---------- day of month (1 - 31)
#  |   |   |   .------- month (1 - 12) OR jan,feb,mar,apr ... 
#  |   |   |   |  .----- day of week (0 - 6) (Sunday=0 or 7)  OR sun,mon,tue,wed,thu,fri,sat 
#  |   |   |   |  |
#  *   *   *   *  * 
*/

const char *valids[] =
{
    "* * * * *",

    "0 * * * *",
    "59 * * * *",
    "* 0 * * *",
    "* 23 * * *",
    "* * 1 * *",
    "* * 31 * *",
    "* * * 1 *",
    "* * * 12 *",
    "* * * * 0",
    "* * * * 7",

    "*/2 * * * *",
    "* */2 * * *",
    "* * */2 * *",
    "* * * */2 *",
    "* * * * */2",

    "0-59 * * * *",
    "* 0-23 * * *",
    "* * 1-31 * *",
    "* * * 1-12 *",
    "* * * * 0-6",

    "1,2 * * * *",
    "* 1,2 * * *",
    "* * 1,2 * *",
    "* * * 1,2 *",
    "* * * * 1,2",

    "1-2,3 * * * *",
    "* 1-2,3 * * *",
    "* * 1-2,3 * *",
    "* * * 1-2,3 *",
    "* * * * 1-2,3",
    NULL,
};

const char *invalids[] =
{
    "60 * * * *",
    "* 24 * * *",
    "* * 0 * *",
    "* * 32 * *",
    "* * * 0 *",
    "* * * 13 *",
    "* * * * 8",

    "1- * * * *",
    "* 1- * * *",
    "* * 1- * *",
    "* * * 1- *",
    "* * * * 1-",

    "*/ * * * *",
    "*/a * * * *",
    "a * * * *",
    "1-a * * * *",
    ", * * * *",
    "1,a * * * *",

    "@month",
    "@year",

    NULL,
};

// now = 2010-12-13 10:00:00
const char *scheduled[] =
{
    // min hour day month wday
    "* 20-22 * * *", "2010-12-13 20:00:00",
    "* 21,22 * * *", "2010-12-13 21:00:00",
    "0 * * * * *", "2010-12-13 11:00:00",
    "0 */2 * * *", "2010-12-13 12:00:00",
    "*/2 * * * *", "2010-12-13 10:02:00",
    "0 10 */2 * *", "2010-12-14 10:00:00",
    "0 10 */3 * *", "2010-12-15 10:00:00",
    "0 9 10 * *", "2011-01-10 09:00:00",
    "* * * * *", "2010-12-13 10:01:00",

    NULL, NULL,
};


const char *macros[] =
{
    "@yearly", "0 0 1 1 *",
    "@annually", "0 0 1 1 *",
    "@monthly", "0 0 1 * *",
    "@weekly", "0 0 * * 0",
    "@daily", "0 0 * * *",
    "@midnight", "0 0 * * *",
    "@hourly", "0 * * * *",
    NULL, NULL,
};

int main(int argc UNUSED, const char * const *argv, const char * const *env UNUSED)
{
    apr_time_t timeval, next;
    char tmpbuf[30];
    nx_schedule_entry_t sched, sched2;
    const char *nowstr = "2010-12-13 10:00:00";
    nx_exception_t e;
    int i;
    volatile boolean failed;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    ASSERT(nx_date_parse_iso(&timeval, nowstr, NULL) == APR_SUCCESS);

    for ( i = 0; valids[i] != NULL; i++ )
    {
	try
	{
	    nx_schedule_entry_parse_crontab(&sched, valids[i]);
	}
	catch(e)
	{
	    log_exception(e);
	    nx_abort("Test failed for crontab entry: %s", valids[i]);
	}
    }

    for ( i = 0; invalids[i] != NULL; i++ )
    {
	failed = TRUE;
	try
	{
	    nx_schedule_entry_parse_crontab(&sched, invalids[i]);
	    failed = FALSE;
	}
	catch(e)
	{
	}
	if ( failed != TRUE )
	{
	    nx_abort("Invalid crontab entry passed: %s", invalids[i]);
	}
    }

    for ( i = 0; valids[i] != NULL; i++ )
    {
	memset(&sched, 0, sizeof(nx_schedule_entry_t));
	nx_schedule_entry_parse_crontab(&sched, valids[i]);
	sched.type = NX_SCHEDULE_ENTRY_TYPE_CRONTAB;
	next = nx_schedule_entry_next_run(&sched, timeval);
    }

    for ( i = 0; scheduled[i] != NULL; i += 2 )
    {
	memset(&sched, 0, sizeof(nx_schedule_entry_t));
	nx_schedule_entry_parse_crontab(&sched, scheduled[i]);
	sched.type = NX_SCHEDULE_ENTRY_TYPE_CRONTAB;
	next = nx_schedule_entry_next_run(&sched, timeval);
	ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), next) == APR_SUCCESS);
	if ( strcmp(tmpbuf, scheduled[i + 1]) != 0 )
	{
	    nx_abort("test of '%s' failed, expected %s, got %s for next run (diff is %ld min)",
		     scheduled[i], scheduled[i + 1], tmpbuf,
		     (next - timeval) / (APR_USEC_PER_SEC * 60));
	}
    }

    for ( i = 0; macros[i] != NULL; i += 2 )
    {
	memset(&sched, 0, sizeof(nx_schedule_entry_t));
	nx_schedule_entry_parse_crontab(&sched, macros[i]);
	memset(&sched2, 0, sizeof(nx_schedule_entry_t));
	nx_schedule_entry_parse_crontab(&sched2, macros[i + 1]);
	if ( memcmp(&sched, &sched2, sizeof(nx_schedule_entry_t)) != 0 )
	{
	    nx_abort("\"%s\" != %s", macros[i], macros[i + 1]);
	}
    }

    memset(&sched, 0, sizeof(nx_schedule_entry_t));
    sched.type = NX_SCHEDULE_ENTRY_TYPE_EVERY;
    nx_schedule_entry_parse_every(&sched, "42 sec");
    ASSERT(sched.every == 42);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
