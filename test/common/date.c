/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/common/date.h"
#include "../../src/core/nxlog.h"

#include <sys/time.h>
#include <time.h>

#define NX_LOGMODULE NX_LOGMODULE_TEST

nxlog_t nxlog;

typedef struct timepair
{
    const char *t1;
    const char *iso;
    const char *result;
} timepair;


static timepair valid_times[] = 
{
    { "Nov  2 10:45:18", "1970-11-02 10:45:18", "Nov  2 10:45:18" },
    { "Nov 2 10:45:18", "1970-11-02 10:45:18", "Nov  2 10:45:18" },
    { "Nov 02 10:45:18", "1970-11-02 10:45:18", "Nov  2 10:45:18" },
    { "Nov 22 10:45:18", "1970-11-22 10:45:18", "Nov 22 10:45:18" },
    { NULL, NULL, NULL },
};



static void compare_timeval(apr_time_t timeval, const char *timestr)
{
    char tmpbuf[21];

    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    if ( strcmp(timestr, tmpbuf) != 0 )
    {
	nx_abort("%s != %s", timestr, tmpbuf);
    }
}



int main(int argc UNUSED, const char * const *argv, const char * const *env UNUSED)
{
    apr_status_t rv;
    apr_time_t timeval = 0;
    apr_time_t timeval2 = 0;
    const char *teststr = "May  7 17:39:13";
    const char *teststr2 = "May  7 17:39:13 ";
    const char *teststr3 = "May  7 17:39:13 host ";
    const char *isostr = "1970-05-07 17:39:13";
    char isostr_curryear[21];
    char tmpbuf[21];
    char tmpbuf2[21];
    char curryear[5];
    apr_time_t now;
    int i;
    apr_time_exp_t exp;

    apr_cpystrn(isostr_curryear, "XXXX-05-07 17:39:13", sizeof(isostr_curryear));
    ASSERT(apr_time_exp_gmt(&exp, apr_time_now()) == APR_SUCCESS);
    if ( exp.tm_year < 1900 )
    {
	exp.tm_year += 1900;
    }

    apr_snprintf(curryear, sizeof(curryear), "%d", exp.tm_year);
    memcpy(isostr_curryear, curryear, 4);

    // test iso
    ASSERT(nx_date_parse_iso(&timeval, "1970-05-07 17:39:13,42", NULL) == APR_SUCCESS);
    compare_timeval(timeval, "1970-05-07 17:39:13");
    ASSERT(timeval == 10946353420000LL);

    // localtime
    ASSERT(nx_date_parse_iso(&timeval, "1970-05-07 17:39:13.42", NULL) == APR_SUCCESS);
    ASSERT(timeval == 10946353420000LL);
    ASSERT(nx_date_parse_iso(&timeval, "1970-05-07T17:39:13.42", NULL) == APR_SUCCESS);
    ASSERT(timeval == 10946353420000LL);

    // UTC
    ASSERT(nx_date_parse_iso(&timeval, "1970-05-07T17:39:13.42Z", NULL) == APR_SUCCESS);
    ASSERT(nx_date_parse_iso(&timeval2, "1970-05-07 17:39:13.42+GMT", NULL) == APR_SUCCESS);
    ASSERT(timeval == timeval2);

    ASSERT(nx_date_parse_iso(&timeval, "1970-05-07T17:39:13.42+02:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 10942753420000LL);
    ASSERT(nx_date_parse_iso(&timeval, "2003-08-24T05:14:15.000003-01:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1061705655000003LL);
    ASSERT(nx_date_parse_iso(&timeval, "2003-08-24T05:14:15.000003+03:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1061691255000003LL);

    ASSERT(nx_date_parse_iso(&timeval, "2011-12-06T19:14:15-01:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1323202455000000LL);
    ASSERT(nx_date_parse_iso(&timeval, "2011-12-06T19:14:15+00:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1323198855000000LL);
    ASSERT(nx_date_parse_iso(&timeval, "2011-12-06T19:14:15+01:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1323195255000000LL);
    ASSERT(nx_date_parse_iso(&timeval, "2011-12-06T19:14:15+02:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1323191655000000LL);
    ASSERT(nx_date_parse_iso(&timeval, "2011-12-06T19:14:15+03:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1323188055000000LL);
    ASSERT(nx_date_parse_iso(&timeval, "2011-12-06T19:14:15+04:00", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1323184455000000LL);

    ASSERT(nx_date_parse_iso(&timeval, "2011-12-06T19:14:15", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1323195255000000LL);

    // wrong format, nanosecond precision not accepted:
    ASSERT(nx_date_parse_iso(&timeval, "2003-08-24T05:14:15.000000003-07:00", NULL) == APR_EBADDATE);

    // test apache
    ASSERT(nx_date_parse_apache(&timeval, "07/May/1970:17:39:13 +0200", NULL) == APR_SUCCESS);
    ASSERT(timeval == 10946353000000LL);
    compare_timeval(timeval, "1970-05-07 17:39:13");
    ASSERT(nx_date_parse(&timeval, "26/Jan/2011:20:30:45 +0100", NULL) == APR_SUCCESS);
    compare_timeval(timeval, "2011-01-26 20:30:45");

    // test cisco
    ASSERT(nx_date_parse_cisco(&timeval, "Nov 3 14:50:30.403", NULL) == APR_SUCCESS);
    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strcmp("-11-03 14:50:30", tmpbuf + 4) == 0);

    ASSERT(nx_date_parse_cisco(&timeval, "Nov  3 14:50:30.403", NULL) == APR_SUCCESS);
    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strcmp("-11-03 14:50:30", tmpbuf + 4) == 0);

    ASSERT(nx_date_parse_cisco(&timeval, "Nov 13 14:50:30.403", NULL) == APR_SUCCESS);
    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strcmp("-11-13 14:50:30", tmpbuf + 4) == 0);

    // test win
    ASSERT(nx_date_parse_win(&timeval, "19700507173913.420000-000", NULL) == APR_SUCCESS);
    ASSERT(timeval == 10946353420000LL);

    // test timestamp
    ASSERT(nx_date_parse_timestamp(&timeval, "1258531221.650359", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1258531221650359LL);
    ASSERT(nx_date_parse_timestamp(&timeval, "1258531221", NULL) == APR_SUCCESS);
    ASSERT(timeval == 1258531221000000LL);

    now = apr_time_now();
    //printf("GMT now: %lu\n", now / APR_USEC_PER_SEC);
    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), now) == APR_SUCCESS);
    ASSERT(nx_date_parse_iso(&timeval, tmpbuf, NULL) == APR_SUCCESS);

    ASSERT(nx_date_to_iso(tmpbuf2, sizeof(tmpbuf2), timeval) == APR_SUCCESS);
    //printf("%s <=> %s\n", tmpbuf, tmpbuf2);
    ASSERT(strcmp(tmpbuf, tmpbuf2) == 0);
   
    timeval = now;
    for ( i = 0; i < 60 * 24 * 370; i += 2 )
    {
	ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
	ASSERT(nx_date_parse_iso(&timeval2, tmpbuf, NULL) == APR_SUCCESS);
	ASSERT(nx_date_to_iso(tmpbuf2, sizeof(tmpbuf2), timeval2) == APR_SUCCESS);
	if ( strcmp(tmpbuf, tmpbuf2) != 0 )
	{
	    nx_abort("%s != %s", tmpbuf, tmpbuf2);
	}
	timeval += APR_USEC_PER_SEC * 60; // 1 min increment
    }

    ASSERT(nx_date_parse_iso(&timeval, isostr, NULL) == APR_SUCCESS);
    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strcmp(isostr, tmpbuf) == 0);

    timeval2 = timeval;
    ASSERT(nx_date_fix_year(&timeval2) == APR_SUCCESS);

    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval2) == APR_SUCCESS);
    //printf("%s\n", isostr);
    //printf("%s\n", tmpbuf);
    ASSERT(strcmp(isostr + 4, tmpbuf + 4) == 0);

    ASSERT(nx_date_parse_iso(&timeval, "2011-05-07 07:09:3", NULL) == APR_SUCCESS);
    compare_timeval(timeval, "2011-05-07 07:09:03");
    ASSERT(nx_date_parse_iso(&timeval, "2011-05-07 07:9:03", NULL) == APR_SUCCESS);
    compare_timeval(timeval, "2011-05-07 07:09:03");
    ASSERT(nx_date_parse_iso(&timeval, "2011-05-07 7:9:03", NULL) == APR_SUCCESS);
    compare_timeval(timeval, "2011-05-07 07:09:03");
    ASSERT(nx_date_parse_iso(&timeval, "2011-5-7 07:09:03", NULL) == APR_SUCCESS);
    compare_timeval(timeval, "2011-05-07 07:09:03");
    ASSERT(nx_date_parse_iso(&timeval, "2011-5-7 7:9:3", NULL) == APR_SUCCESS);
    compare_timeval(timeval, "2011-05-07 07:09:03");

    // parse_rfc3164
    rv = nx_date_parse_rfc3164(&timeval, teststr, NULL);
    ASSERT(rv == APR_SUCCESS);
    ASSERT(timeval != 0);
    ASSERT(nx_date_to_rfc3164(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strncmp(teststr, tmpbuf, 15) == 0);

    rv = nx_date_parse_rfc3164(&timeval, teststr2, NULL);
    ASSERT(rv == APR_SUCCESS);
    ASSERT(timeval != 0);
    ASSERT(nx_date_to_rfc3164(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strncmp(teststr, tmpbuf, 15) == 0);

    rv = nx_date_parse_rfc3164(&timeval, teststr3, NULL);
    ASSERT(rv == APR_SUCCESS);
    ASSERT(timeval != 0);
    ASSERT(nx_date_to_rfc3164(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strncmp(teststr, tmpbuf, 15) == 0);

    ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
    ASSERT(strcmp(isostr_curryear, tmpbuf) == 0);

    for ( i = 0; valid_times[i].t1 != NULL; i++ )
    {
	rv = nx_date_parse_rfc3164(&timeval, valid_times[i].t1, NULL);
	ASSERT(rv == APR_SUCCESS);
	ASSERT(timeval != 0);
	ASSERT(nx_date_to_iso(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
	apr_cpystrn(isostr_curryear, valid_times[i].iso, sizeof(isostr_curryear));
	memcpy(isostr_curryear, curryear, 4);
	ASSERT(strcmp(isostr_curryear, tmpbuf) == 0);
	ASSERT(nx_date_to_rfc3164(tmpbuf, sizeof(tmpbuf), timeval) == APR_SUCCESS);
	ASSERT(strcmp(valid_times[i].result, tmpbuf) == 0);
    }

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
