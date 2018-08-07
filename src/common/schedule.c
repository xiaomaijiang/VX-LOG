/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>
#include "schedule.h"
#include "exception.h"
#include "date.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

#define skip_space(p) for ( ; (*p == ' ') || (*p == '\t'); p++)


static const char *parse_int(const char *ptr, int *dst)
{
    int retval = 0;

    if ( !apr_isdigit(*ptr) )
    {
	throw_msg("integer expected, found [%s]", ptr);
    }
    for ( ; apr_isdigit(*ptr); ptr++ )
    {
	retval *= 10;
	retval += *ptr - '0';
    }

    *dst = retval;

    return ( ptr );
}



static const char *parse_field(const char *ptr,
			       const char *fieldname,
			       int8_t *dst,
			       int dstsize,
			       int first,
			       int last)
{
    int num1 = 0;
    int num2 = 0;
    int num3 = 0;

    if ( *ptr == '*' )
    {
	ptr++;
	if ( *ptr == '/' )
	{
	    ptr++;
	    ptr = parse_int(ptr, &num2);
	    for ( num1 = 0; num1 < dstsize; num1++ )
	    {
		if ( (num1 + first) % num2 == 0 )
		{
		    dst[num1] = 1;
		}
	    }
	}
	else
	{
	    for ( num1 = 0; num1 < dstsize; num1++ )
	    {
		dst[num1] = 1;
	    }
	}
    }
    else if ( apr_isdigit(*ptr) )
    {
	while ( (*ptr != '\0') && (!apr_isspace(*ptr)) )
	{
	    ptr = parse_int(ptr, &num1);
	    if ( (num1 > last) || (num1 < first) )
	    {
		throw_msg("invalid crontab format, value out of range for %s: %d",
			  fieldname, num1);
	    }
	    num1 -= first;
	    ASSERT(num1 < dstsize);
	    dst[num1] = 1;
	    if ( *ptr == '-' )
	    {
		ptr++;
		ptr = parse_int(ptr, &num2);
		if ( (num2 > last) || (num2 < first) )
		{
		    throw_msg("invalid crontab format, value out of range for %s: %d",
			      fieldname, num2);
		}
		num2 -= first;
		if ( *ptr == '/' )
		{ // X-Y/Z
		    ptr++;
		    ptr = parse_int(ptr, &num3);
		    for ( ; num1 < num2; num1++ )
		    {
			if ( (num1 + first) % num3 == 0 )
			{
			    dst[num1] = 1;
			}
		    }
		}
		else
		{ //X-Y
		    for ( ; num1 <= num2; num1++ )
		    {
			dst[num1] = 1;
		    }
		}
	    }
	    if ( *ptr == ',' )
	    {
		ptr++;
	    }
	}
    }
    else
    {
	throw_msg("invalid crontab format, char not valid in field %s: [%s]",
		  fieldname, ptr);
    }
    
    return ( ptr );
}



static void parse_macro(nx_schedule_entry_t *entry, const char *str)
{
    int i;

    if ( strcasecmp("reboot", str) == 0 )
    { // Run once, at startup.
	throw_msg("@reboot not supported");
    }
    else if ( (strcasecmp("yearly", str) == 0) || (strcasecmp("annually", str) == 0) )
    { // Run once a year, "0 0 1 1 *"
	entry->min[0] = 1;
	entry->hour[0] = 1;
	entry->day[0] = 1;
	entry->month[0] = 1;
	for ( i = 0; i < NX_SCHEDULE_WDAY_COUNT; i++ )
	{
	    entry->wday[i] = 1;
	}
    }
    else if ( strcasecmp("monthly", str) == 0 )
    { // Run once a month, "0 0 1 * *"
	entry->min[0] = 1;
	entry->hour[0] = 1;
	entry->day[0] = 1;
	for ( i = 0; i < NX_SCHEDULE_MONTH_COUNT; i++ )
	{
	    entry->month[i] = 1;
	}
	for ( i = 0; i < NX_SCHEDULE_WDAY_COUNT; i++ )
	{
	    entry->wday[i] = 1;
	}
    }
    else if ( strcasecmp("weekly", str) == 0 )
    { // Run once a week, "0 0 * * 0"
	entry->min[0] = 1;
	entry->hour[0] = 1;
	for ( i = 0; i < NX_SCHEDULE_DAY_COUNT; i++ )
	{
	    entry->day[i] = 1;
	}
	for ( i = 0; i < NX_SCHEDULE_MONTH_COUNT; i++ )
	{
	    entry->month[i] = 1;
	}
	entry->wday[0] = 1;
    }
    else if ( (strcasecmp("daily", str) == 0) || (strcasecmp("midnight", str) == 0) )
    { // Run once a day, "0 0 * * *"
	entry->min[0] = 1;
	entry->hour[0] = 1;
	for ( i = 0; i < NX_SCHEDULE_DAY_COUNT; i++ )
	{
	    entry->day[i] = 1;
	}
	for ( i = 0; i < NX_SCHEDULE_MONTH_COUNT; i++ )
	{
	    entry->month[i] = 1;
	}
	for ( i = 0; i < NX_SCHEDULE_WDAY_COUNT; i++ )
	{
	    entry->wday[i] = 1;
	}
    }
    else if ( strcasecmp("hourly", str) == 0 )
    { // Run once an hour, "0 * * * *"
	entry->min[0] = 1;
	for ( i = 0; i < NX_SCHEDULE_HOUR_COUNT; i++ )
	{
	    entry->hour[i] = 1;
	}
	for ( i = 0; i < NX_SCHEDULE_DAY_COUNT; i++ )
	{
	    entry->day[i] = 1;
	}
	for ( i = 0; i < NX_SCHEDULE_MONTH_COUNT; i++ )
	{
	    entry->month[i] = 1;
	}
	for ( i = 0; i < NX_SCHEDULE_WDAY_COUNT; i++ )
	{
	    entry->wday[i] = 1;
	}
    }
    else
    {
	throw_msg("invalid crontab macro: @%s", str);
    }
}



void nx_schedule_entry_parse_crontab(nx_schedule_entry_t *entry, const char *str)
{
    const char *ptr;

    ASSERT(entry != NULL);
    ASSERT(str != NULL);

    ptr = str;

    if ( *ptr == '@' )
    {
	ptr++;
	parse_macro(entry, ptr);
	return;
    }

    skip_space(ptr);
    ptr = parse_field(ptr, "minute", entry->min, NX_SCHEDULE_MIN_COUNT,
		      NX_SCHEDULE_MIN_FIRST, NX_SCHEDULE_MIN_LAST);

    skip_space(ptr);
    ptr = parse_field(ptr, "hour", entry->hour, NX_SCHEDULE_HOUR_COUNT,
		      NX_SCHEDULE_HOUR_FIRST, NX_SCHEDULE_HOUR_LAST);

    skip_space(ptr);
    ptr = parse_field(ptr, "day", entry->day, NX_SCHEDULE_DAY_COUNT,
		      NX_SCHEDULE_DAY_FIRST, NX_SCHEDULE_DAY_LAST);

    skip_space(ptr);
    ptr = parse_field(ptr, "month", entry->month, NX_SCHEDULE_MONTH_COUNT,
		      NX_SCHEDULE_MONTH_FIRST, NX_SCHEDULE_MONTH_LAST);

    skip_space(ptr);
    ptr = parse_field(ptr, "weekday", entry->wday, NX_SCHEDULE_WDAY_COUNT,
		      NX_SCHEDULE_WDAY_FIRST, NX_SCHEDULE_WDAY_LAST);
    
    // Sunday is 0 or 7
    if ( entry->wday[7] == 1 )
    {
	entry->wday[0] = 1;
    }
}



void nx_schedule_entry_parse_every(nx_schedule_entry_t *entry, const char *str)
{
    const char *ptr;
    int64_t val = 0;

    ASSERT(str != NULL);

    for ( ptr = str; apr_isdigit(*ptr); ptr++ )
    {
	val *= 10;
	val += *ptr - '0';
    }
    entry->every = val;
    skip_space(ptr);

    if ( *ptr == '\0' )
    {
    }
    else if ( strcasecmp(ptr, "sec") == 0 )
    {
	// sec is the default
    }
    else if ( strcasecmp(ptr, "min") == 0 )
    {
	entry->every *= 60;
    }
    else if ( strcasecmp(ptr, "hour") == 0 )
    {
	entry->every *= 60 * 60;
    }
    else if ( strcasecmp(ptr, "day") == 0 )
    {
	entry->every *= 60 * 60 * 24;
    }
    else if ( strcasecmp(ptr, "week") == 0 )
    {
	entry->every *= 60 * 60 * 24 * 7;
    }
    else
    {
	throw_msg("invalid time specifier: %s", ptr);
    }
}



static boolean check_timeval(nx_schedule_entry_t *entry, apr_time_t t)
{
    apr_time_exp_t exp;

    CHECKERR(apr_time_exp_lt(&exp, t));

    ASSERT(exp.tm_min < NX_SCHEDULE_MIN_COUNT);
    ASSERT(exp.tm_hour < NX_SCHEDULE_HOUR_COUNT);
    ASSERT(exp.tm_mday <= NX_SCHEDULE_DAY_COUNT);
    ASSERT(exp.tm_mday > 0);
    ASSERT(exp.tm_mon < NX_SCHEDULE_MONTH_COUNT);
    ASSERT(exp.tm_wday < 7);

    if ( (entry->min[exp.tm_min] == 1) &&
	 (entry->hour[exp.tm_hour] == 1) &&
	 (entry->day[exp.tm_mday - 1] == 1) &&
	 (entry->month[exp.tm_mon] == 1) &&
	 (entry->wday[exp.tm_wday] == 1) )
    {
	return ( TRUE );
    }

    return ( FALSE );
}



apr_time_t nx_schedule_entry_next_run(nx_schedule_entry_t *entry, apr_time_t now)
{
    apr_time_t tmpval;

    ASSERT(entry != NULL);

    switch ( entry->type )
    {
	case NX_SCHEDULE_ENTRY_TYPE_EVERY:
	    ASSERT(entry->every > 0);
	    if ( entry->first != 0 ) 
	    {
		while ( entry->first <= now )
		{
		    entry->first += entry->every * APR_USEC_PER_SEC;
		}
	    }
	    else
	    {
		entry->first = now;
	    }
	    return ( entry->first );
	case NX_SCHEDULE_ENTRY_TYPE_CRONTAB:
	    // the current minute should not be taken into account,
	    // so we calculate the next rounded seconds to a whole minute
	    tmpval = ((now / (APR_USEC_PER_SEC * 60)) + 1) * APR_USEC_PER_SEC * 60;

	    // we start checking every minute from current time for a period of one year
	    // this is not very efficient but if the schedule is frequent it will find it fast.
	    // otherwise if it runs rarely (e.g. once a month), then the time to find it is longer,
	    // but since it runs rarely, we don't care much :-)
	    for ( ; tmpval < now + (APR_USEC_PER_SEC * 60 * 60 * 24 * 365);
		  tmpval += APR_USEC_PER_SEC * 60 )
	    {
		if ( check_timeval(entry, tmpval) == TRUE )
		{
		    return ( tmpval );
		}
	    }
	    break;
	default:
	    nx_panic("invalid Schedule type: %d", entry->type);
    }
    nx_panic("should not reach");
    return ( 0LL );
}
