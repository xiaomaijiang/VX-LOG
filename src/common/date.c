/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * some routines are based on apr_date.c from apr-util
 */

#include <time.h>
#include <apr_lib.h>

#include "types.h"
#include "error_debug.h"
#include "date.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

/*
// FIXME: this is a hack to get the local timezone
static apr_int32_t get_gmtoff_localtime()
{
    static apr_int32_t _gmtoff_localtime = -1;
    apr_time_t t;
    apr_time_exp_t ds;

    if ( _gmtoff_localtime == -1 )
    {
	t = apr_time_now();
	ASSERT(apr_time_exp_lt(&ds, t) == APR_SUCCESS);
	_gmtoff_localtime = ds.tm_gmtoff;
    }

    return ( _gmtoff_localtime );
}
*/


struct tz_offs
{
    const char	*zone;
    int		offs;
} tz_offs;

// return the offset in minutes
// FIXME: the offset also depends on isdst in some cases, e.g. CET!!
static size_t get_tz_offset(const char *zone, int *offs)
{
    size_t i;

    static struct tz_offs offsets[] =
    {
	{ "UTC", 0 },		// Universal Coordinated Time
	{ "GMT", 0 },		// Greenwich Mean Time
	{ "MET", 1 * 60 },	// Middle European Time
	{ "CET", 1 * 60 },	// Central European Time
	{ "EET", 2 * 60 },	// Eastern European Time
	{ "WST", 8 * 60 },	// Western Standard Time
	{ "JST", 9 * 60 },	// Japan Standard Time
	{ "NZT", 12 * 60 },	// New Zealand Time

	{ "AST", -4 * 60 },	// Atlantic Standard Time
	{ "ADT", -3 * 60 },	// Atlantic Daylight Time
	{ "EST", -5 * 60 },	// Eastern Standard Time
	{ "EDT", -4 * 60 },	// Eastern Daylight Time
	{ "CST", -6 * 60 },	// Central Standard Time
	{ "CDT", -5 * 60 },	// Central Daylight Time
	{ "MST", -7 * 60 },	// Mountain Standard Time
	{ "MDT", -6 * 60 },	// Mountain Daylight Time
	{ "PST", -8 * 60 },	// Pacific Standard Time
	{ "PDT", -7 * 60 },	// Pacific Daylight Time
	{ NULL, 0},
    };

    if ( (zone[0] == '+') ||  zone[0] == '-' )
    {
	int mul = 1;
	int offset = 0;
	if ( zone[0] == '-' )
	{
	    mul = -1;
	}
	if ( apr_isdigit(zone[1]) && apr_isdigit(zone[2]) )
	{
	    offset = ((zone[1] - '0') * 10 + (zone[2] - '0')) * 60;
	}
	else
	{
	    return ( 0 );
	}
	i = 3;
	if ( zone[i] == ':' )
	{
	    i++;
	}
	if ( apr_isdigit(zone[i]) && apr_isdigit(zone[i + 1]) )
	{
	    offset += (zone[i] - '0') * 10 + (zone[i + 1] - '0');
	}
	else
	{
	    return ( 0 );
	}
	offset *= mul;
	*offs = offset;

	return ( i + 2 );
    }
    else
    {
	for ( i = 0; offsets[i].zone != NULL; i++ )
	{
	    if ( strncasecmp(zone, offsets[i].zone, 3) == 0 )
	    {
		*offs = offsets[i].offs;
		return ( 3 );
	    }
	}
    }
    
    return ( 0 );
}



/*
 * Compare a string to a mask
 * Mask characters (arbitrary maximum is 256 characters, just in case):
 *   @ - uppercase letter
 *   $ - lowercase letter
 *   & - hex digit
 *   # - digit
 *   ~ - digit or space
 *   % - NUL or space
 *   * - swallow remaining characters 
 *  <x> - exact match for any other character
 */

static int _date_checkmask(const char *data, const char *mask)
{
    int i;
    char d;

    for ( i = 0; i < 256; i++ )
    {
        d = data[i];
        switch (mask[i])
	{
	    case '\0':
		return (d == '\0');
	    case '*':
		return 1;
	    case '@':
		if ( !apr_isupper(d) )
		{
		    return 0;
		}
		break;
	    case '$':
		if ( !apr_islower(d) )
		{
		    return 0;
		}
		break;
	    case '#':
		if ( !apr_isdigit(d) )
		{
		    return 0;
		}
		break;
	    case '&':
		if ( !apr_isxdigit(d) )
		{
		    return 0;
		}
		break;
	    case '~':
		if ( (d != ' ') && !apr_isdigit(d) )
		{
		    return 0;
		}
		break;
	    case '%':
		if ( (d != '\0') && (d != ' ') )
		{
		    return 0;
		}
		if ( d == '\0' )
		{
		    return 1;
		}
		break;
	    default:
		if ( mask[i] != d )
		{
		    return 0;
		}
		break;
        }
    }

    return 0;          /* We only get here if mask is corrupted (exceeds 256) */
}


#define TIMEPARSE(ds, hr10, hr1, min10, min1, sec10, sec1)  \
    {                                                       \
        ds.tm_hour = ((hr10 - '0') * 10) + (hr1 - '0');     \
        ds.tm_min = ((min10 - '0') * 10) + (min1 - '0');    \
        ds.tm_sec = ((sec10 - '0') * 10) + (sec1 - '0');    \
    }
#define TIMEPARSE_STD(ds, timstr)                           \
    {                                                       \
        TIMEPARSE(ds, timstr[0],timstr[1],                  \
                      timstr[3],timstr[4],                  \
                      timstr[6],timstr[7]);                 \
    }


/*
static apr_int32_t get_lt_gmtoff()
{
    apr_time_exp_t ds;

    apr_time_exp_lt(&ds, APR_INT64_C(0));

    return ( ds.tm_gmtoff );
}
*/


static int _get_curr_year()
{
    apr_time_exp_t exp;

    ASSERT(apr_time_exp_gmt(&exp, apr_time_now()) == APR_SUCCESS);

    return ( exp.tm_year );
}


/*
 * Parses a string resembling an RFC 3164 date.
 *
 * Formats supported:
 *
 *     Sun 6 Nov 08:49:37             ; RFC 3164
 *
 */

apr_status_t nx_date_parse_rfc3164(apr_time_t  *t, 
				   const char *date,
				   const char **dateend)
{
    //apr_time_exp_t ds;
    struct tm ds;
    time_t tval;
    apr_status_t rv;
    int mint, mon;
    const char *timstr;
    static const int months[12] =
    {
	('J' << 16) | ('a' << 8) | 'n', ('F' << 16) | ('e' << 8) | 'b',
	('M' << 16) | ('a' << 8) | 'r', ('A' << 16) | ('p' << 8) | 'r',
	('M' << 16) | ('a' << 8) | 'y', ('J' << 16) | ('u' << 8) | 'n',
	('J' << 16) | ('u' << 8) | 'l', ('A' << 16) | ('u' << 8) | 'g',
	('S' << 16) | ('e' << 8) | 'p', ('O' << 16) | ('c' << 8) | 't',
	('N' << 16) | ('o' << 8) | 'v', ('D' << 16) | ('e' << 8) | 'c' 
    };

    if ( !date )
    {
        return ( APR_EBADDATE );
    }

    /* Not all dates have text days at the beginning. */
    if ( !apr_isdigit(date[0]) )
    {
        while ( *date && apr_isspace(*date) ) /* Find first non-whitespace char */
	{
            ++date;
	}
        if ( *date == '\0' )
	{ 
            return ( APR_EBADDATE );
	}
    }

    if ( (date[1] == '\0') || (date[2] == '\0') )
    {
	return ( APR_EBADDATE );
    }
    mint = (date[0] << 16) | (date[1] << 8) | date[2];
    for ( mon = 0; mon < 12; mon++ )
    {
        if ( mint == months[mon] )
	{
            break;
	}
    }

    if ( mon == 12 )
    { // Date does not start with month
	return ( APR_EBADDATE );
    }

    if ( _date_checkmask(date, "@$$ ## ##:##:##%*") )
    {   /* RFC 3164 format: Mon dd mm:hh:ss */
	ds.tm_mday = ((date[4] - '0') * 10) + (date[5] - '0');
        timstr = date + 7;

        TIMEPARSE_STD(ds, timstr);
    }
    else if ( _date_checkmask(date, "@$$  # ##:##:##%*") )
    {   /* RFC 3164 format: Mon  d mm:hh:ss */
	ds.tm_mday = date[5] - '0';
        timstr = date + 7;

        TIMEPARSE_STD(ds, timstr);
    }
    else if ( _date_checkmask(date, "@$$ # ##:##:##%*") )
    {   /* RFC 3164 format: Mon d mm:hh:ss */
	ds.tm_mday = date[4] - '0';
        timstr = date + 6;

        TIMEPARSE_STD(ds, timstr);
    }
    else
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday <= 0) || (ds.tm_mday > 31) )
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61) )
    { 
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday == 31) && (mon == 3 || mon == 5 || mon == 8 || mon == 10) )
    {
        return ( APR_EBADDATE );
    }

    ds.tm_mon = mon;

    //ds.tm_usec = 0;
    ds.tm_year = _get_curr_year();
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    ds.tm_gmtoff = 0;
#endif
    ds.tm_isdst = -1;

    if ( (tval = mktime(&ds)) == -1 )
    {
        return ( APR_EBADDATE );
    }
    
    if ( (rv = apr_time_ansi_put(t, tval)) != APR_SUCCESS )
    {
	return ( rv );
    }

/* this doesn't work properly: we have to use mktime
    if ( (rv = apr_time_exp_gmt_get(t, &ds)) != APR_SUCCESS )
    {
        return ( rv );
    }
*/
    if ( dateend != NULL )
    {
	*dateend = date + 15;
    }
    
    return ( APR_SUCCESS );
}



/*
 * Parses a string resembling an RFC 1123 date.
 *
 * Formats supported:
 *
 *     Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
 *     Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
 *     Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format
 *     Sun, 6 Nov 1994 08:49:37 GMT   ; RFC 822, updated by RFC 1123
 *     Sun, 06 Nov 94 08:49:37 GMT    ; RFC 822
 *     Sun,  6 Nov 94 08:49:37 GMT    ; RFC 822
 *     Sun, 6 Nov 94 08:49:37 GMT     ; RFC 822
 *     Sun, 06 Nov 94 08:49 GMT       ; Unknown [drtr@ast.cam.ac.uk] 
 *     Sun, 6 Nov 94 08:49 GMT        ; Unknown [drtr@ast.cam.ac.uk]
 *     Sun, 06 Nov 94 8:49:37 GMT     ; Unknown [Elm 70.85]
 *     Sun, 6 Nov 94 8:49:37 GMT      ; Unknown [Elm 70.85] 
 *     Mon,  7 Jan 2002 07:21:22 GMT  ; Unknown [Postfix]
 *     Sun, 06-Nov-1994 08:49:37 GMT  ; RFC 850 with four digit years
 *
 */

apr_status_t nx_date_parse_rfc1123(apr_time_t *t,
				   const char *date,
				   const char **dateend)
{
    apr_status_t rv;
    //apr_time_exp_t ds;
    time_t tval;
    struct tm ds;
    int mint, mon;
    const char *monstr, *timstr, *tzstr;
    int gmtoff = 0;
    static const int months[12] =
    {
	('J' << 16) | ('a' << 8) | 'n', ('F' << 16) | ('e' << 8) | 'b',
	('M' << 16) | ('a' << 8) | 'r', ('A' << 16) | ('p' << 8) | 'r',
	('M' << 16) | ('a' << 8) | 'y', ('J' << 16) | ('u' << 8) | 'n',
	('J' << 16) | ('u' << 8) | 'l', ('A' << 16) | ('u' << 8) | 'g',
	('S' << 16) | ('e' << 8) | 'p', ('O' << 16) | ('c' << 8) | 't',
	('N' << 16) | ('o' << 8) | 'v', ('D' << 16) | ('e' << 8) | 'c' 
    };

    if ( !date )
    {
        return ( APR_EBADDATE );
    }

    /* Not all dates have text days at the beginning. */
    if ( !apr_isdigit(date[0]) )
    {
        while ( *date && apr_isspace(*date) ) /* Find first non-whitespace char */
	{
            ++date;
	}

        if ( *date == '\0' ) 
	{
            return ( APR_EBADDATE );
	}    

        if ( (date = strchr(date, ' ')) == NULL )   /* Find space after weekday */
	{
	    return ( APR_EBADDATE );
	}

	++date;    /* Now pointing to first char after space, which should be mday */    
    }

    if ( _date_checkmask(date, "## @$$ #### ##:##:## *") ) 
    {   /* RFC 1123 format */
        ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;

        if ( ds.tm_year < 0 )
	{
            return ( APR_EBADDATE );
	}

        ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

        ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

        monstr = date + 3;
        timstr = date + 12;
        tzstr = date + 21;

        TIMEPARSE_STD(ds, timstr);
    }
    else if ( _date_checkmask(date, "##-@$$-## ##:##:## *") ) /* RFC 850 format  */
    {
        ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

        if ( ds.tm_year < 70 )
	{
            ds.tm_year += 100;
	}

        ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

        monstr = date + 3;
        timstr = date + 10;
        tzstr = date + 19;

        TIMEPARSE_STD(ds, timstr);
    }
    else if (_date_checkmask(date, "@$$ ~# ##:##:## ####%*"))  /* asctime format */
    {
        ds.tm_year = ((date[16] - '0') * 10 + (date[17] - '0') - 19) * 100;

        if ( ds.tm_year < 0 ) 
	{
            return ( APR_EBADDATE );
	}

        ds.tm_year += ((date[18] - '0') * 10) + (date[19] - '0');

        if ( date[4] == ' ' )
	{
            ds.tm_mday = 0;
	}
        else
	{
            ds.tm_mday = (date[4] - '0') * 10;
	}

        ds.tm_mday += (date[5] - '0');

        monstr = date;
        timstr = date + 7;
        tzstr = NULL;

	if ( dateend != NULL )
	{
	    *dateend = date + 20;
	}
        TIMEPARSE_STD(ds, timstr);
    }
    else if ( _date_checkmask(date, "# @$$ #### ##:##:## *") )
    { /* RFC 1123 format*/
        ds.tm_year = ((date[6] - '0') * 10 + (date[7] - '0') - 19) * 100;

        if ( ds.tm_year < 0 )
	{
            return ( APR_EBADDATE );
	}

        ds.tm_year += ((date[8] - '0') * 10) + (date[9] - '0');
        ds.tm_mday = (date[0] - '0');

        monstr = date + 2;
        timstr = date + 11;
        tzstr = date + 20;

        TIMEPARSE_STD(ds, timstr);
    }
    else if ( _date_checkmask(date, "## @$$ ## ##:##:## *") )
    {
        /* This is the old RFC 1123 date format - many many years ago, people
         * used two-digit years.  Oh, how foolish.
         *
         * Two-digit day, two-digit year version. */
        ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

        if (ds.tm_year < 70)
            ds.tm_year += 100;

        ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

        monstr = date + 3;
        timstr = date + 10;
        tzstr = date + 19;

        TIMEPARSE_STD(ds, timstr);
    } 
    else if ( _date_checkmask(date, " # @$$ ## ##:##:## *") )
    {
        /* This is the old RFC 1123 date format - many many years ago, people
         * used two-digit years.  Oh, how foolish.
         *
         * Space + one-digit day, two-digit year version.*/
        ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

        if (ds.tm_year < 70)
            ds.tm_year += 100;

        ds.tm_mday = (date[1] - '0');

        monstr = date + 3;
        timstr = date + 10;
        tzstr = date + 19;

        TIMEPARSE_STD(ds, timstr);
    } 
    else if ( _date_checkmask(date, "# @$$ ## ##:##:## *") )
    {
        /* This is the old RFC 1123 date format - many many years ago, people
         * used two-digit years.  Oh, how foolish.
         *
         * One-digit day, two-digit year version. */
        ds.tm_year = ((date[6] - '0') * 10) + (date[7] - '0');

        if ( ds.tm_year < 70 )
	{
            ds.tm_year += 100;
	}
        ds.tm_mday = (date[0] - '0');

        monstr = date + 2;
        timstr = date + 9;
        tzstr = date + 18;

        TIMEPARSE_STD(ds, timstr);
    } 
    else if ( _date_checkmask(date, "## @$$ ## ##:##%*") )
    {
        /* Loser format.  This is quite bogus.  */
        ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

        if ( ds.tm_year < 70 )
	{
            ds.tm_year += 100;
	}

        ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

        monstr = date + 3;
        timstr = date + 10;
        tzstr = NULL;

	if ( dateend != NULL )
	{
	    *dateend = date + 16;
	}
        TIMEPARSE(ds, timstr[0],timstr[1], timstr[3],timstr[4], '0','0');
    } 
    else if (_date_checkmask(date, "# @$$ ## ##:##%*")) {
        /* Loser format.  This is quite bogus.  */
        ds.tm_year = ((date[6] - '0') * 10) + (date[7] - '0');

        if ( ds.tm_year < 70 )
	{
            ds.tm_year += 100;
	}
        ds.tm_mday = (date[0] - '0');

        monstr = date + 2;
        timstr = date + 9;
        tzstr = NULL;

	if ( dateend != NULL )
	{
	    *dateend = date + 15;
	}
        TIMEPARSE(ds, timstr[0],timstr[1], timstr[3],timstr[4], '0','0');
    }
    else if ( _date_checkmask(date, "## @$$ ## #:##:## *") )
    {
        /* Loser format.  This is quite bogus.  */
        ds.tm_year = ((date[7] - '0') * 10) + (date[8] - '0');

        if ( ds.tm_year < 70 )
	{
            ds.tm_year += 100;
	}
        ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

        monstr = date + 3;
        timstr = date + 9;
        tzstr = date + 18;

        TIMEPARSE(ds, '0',timstr[1], timstr[3],timstr[4], timstr[6],timstr[7]);
    }
    else if ( _date_checkmask(date, "# @$$ ## #:##:## *") )
    {
         /* Loser format.  This is quite bogus.  */
        ds.tm_year = ((date[6] - '0') * 10) + (date[7] - '0');

        if ( ds.tm_year < 70 )
	{
            ds.tm_year += 100;
	}
        ds.tm_mday = (date[0] - '0');

        monstr = date + 2;
        timstr = date + 8;
        tzstr = date + 17;

        TIMEPARSE(ds, '0',timstr[1], timstr[3],timstr[4], timstr[6],timstr[7]);
    }
    else if ( _date_checkmask(date, " # @$$ #### ##:##:## *") )
    {   
        /* RFC 1123 format with a space instead of a leading zero. */
        ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;

        if ( ds.tm_year < 0 )
	{
            return ( APR_EBADDATE );
	}

        ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

        ds.tm_mday = (date[1] - '0');

        monstr = date + 3;
        timstr = date + 12;
        tzstr = date + 21;

        TIMEPARSE_STD(ds, timstr);
    }
    else if ( _date_checkmask(date, "##-@$$-#### ##:##:## *") )
    {
       /* RFC 1123 with dashes instead of spaces between date/month/year
        * This also looks like RFC 850 with four digit years.
        */
        ds.tm_year = ((date[7] - '0') * 10 + (date[8] - '0') - 19) * 100;
        if ( ds.tm_year < 0 )
	{
            return ( APR_EBADDATE );
	}
        ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

        ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

        monstr = date + 3;
        timstr = date + 12;
        tzstr = date + 21;

        TIMEPARSE_STD(ds, timstr);
    }
    else
    {
        return ( APR_EBADDATE );
    }

    if ( ds.tm_mday <= 0 || ds.tm_mday > 31 )
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61) )
    {
        return ( APR_EBADDATE );
    }

    mint = (monstr[0] << 16) | (monstr[1] << 8) | monstr[2];
    for ( mon = 0; mon < 12; mon++ )
    {
        if ( mint == months[mon] )
	{
            break;
	}
    }

    if ( mon == 12 )
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday == 31) && (mon == 3 || mon == 5 || mon == 8 || mon == 10) )
    {
        return ( APR_EBADDATE );
    }

    /* February gets special check for leapyear */

    if ( (mon == 1) &&
	 ((ds.tm_mday > 29) || 
	  ((ds.tm_mday == 29) && 
	   ((ds.tm_year & 3) ||
	    (((ds.tm_year % 100) == 0) &&
	     (((ds.tm_year % 400) != 100)))))) )
    {
        return ( APR_EBADDATE );
    }

    ds.tm_mon = mon;

    /* Do we have a timezone ? */
    if ( tzstr != NULL ) 
    {
        int offset;
	size_t len = 0;

	len = get_tz_offset(tzstr, &offset);
	if ( len > 0 )
	{
	    if ( dateend != NULL )
	    {
		*dateend += len;
	    }
	    gmtoff = offset * 60;
	}
    }

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    ds.tm_gmtoff = gmtoff * 60;
#endif

    ds.tm_isdst = -1;

    if ( (tval = mktime(&ds)) == -1 )
    {
        return ( APR_EBADDATE );
    }
    
    if ( (rv = apr_time_ansi_put(t, tval)) != APR_SUCCESS )
    {
	return ( rv );
    }
#ifndef HAVE_STRUCT_TM_TM_GMTOFF
    *t -= gmtoff * 60 * APR_USEC_PER_SEC;
#endif

/* this doesn't work properly: we have to use mktime
    if ( (rv = apr_time_exp_gmt_get(t, &ds)) != APR_SUCCESS )
    {
        return ( rv );
    }
*/

    if ( dateend != NULL )
    {
	ASSERT ( *dateend != NULL );
    }

    return ( APR_SUCCESS );
}


/*
 * Parses an apache date.
 * Example: 24/Aug/2009:16:08:57 +0200
 *
 */

apr_status_t nx_date_parse_apache(apr_time_t *t,
				  const char *date,
				  const char **dateend)
{
    apr_status_t rv;
    time_t tval;
    struct tm ds;
    int mint, mon;
    const char *timstr, *tzstr;
    int gmtoff = 0;
    size_t offs = 0;

    static const int months[12] =
    {
	('J' << 16) | ('a' << 8) | 'n', ('F' << 16) | ('e' << 8) | 'b',
	('M' << 16) | ('a' << 8) | 'r', ('A' << 16) | ('p' << 8) | 'r',
	('M' << 16) | ('a' << 8) | 'y', ('J' << 16) | ('u' << 8) | 'n',
	('J' << 16) | ('u' << 8) | 'l', ('A' << 16) | ('u' << 8) | 'g',
	('S' << 16) | ('e' << 8) | 'p', ('O' << 16) | ('c' << 8) | 't',
	('N' << 16) | ('o' << 8) | 'v', ('D' << 16) | ('e' << 8) | 'c' 
    };

    if ( !date )
    {
        return ( APR_EBADDATE );
    }

    if ( _date_checkmask(date, "##/@$$/####:##:##:## *") ) 
    {
        ds.tm_mday = ((date[0] - '0') * 10) + (date[1] - '0');

	mint = (date[3] << 16) | (date[4] << 8) | date[5];
	for ( mon = 0; mon < 12; mon++ )
	{
	    if ( mint == months[mon] )
	    {
		break;
	    }
	}
	if ( mon == 12 )
	{ // Invalid month
	    return ( APR_EBADDATE );
	}
	ds.tm_mon = mon;

        ds.tm_year = (date[7] - '0') * 1000 + (date[8] - '0') * 100;

        if ( ds.tm_year < 0 )
	{
            return ( APR_EBADDATE );
	}

        ds.tm_year += ((date[9] - '0') * 10) + (date[10] - '0');

        timstr = date + 12;
        tzstr = date + 21;

        TIMEPARSE_STD(ds, timstr);
	offs += get_tz_offset(tzstr, &gmtoff);

    }
    else
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday <= 0) || (ds.tm_mday > 31) )
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61) )
    { 
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday == 31) && (ds.tm_mon == 3 || ds.tm_mon == 5 || ds.tm_mon == 8 || ds.tm_mon == 10) )
    {
        return ( APR_EBADDATE );
    }

    if ( ds.tm_year < 1970 )
    {
        return ( APR_EBADDATE );
    }	
    ds.tm_year -= 1900;

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    ds.tm_gmtoff = gmtoff * 60;
#endif
    ds.tm_isdst = -1;

    if ( (tval = mktime(&ds)) == -1 )
    {
        return ( APR_EBADDATE );
    }
    
    if ( (rv = apr_time_ansi_put(t, tval)) != APR_SUCCESS )
    {
	return ( rv );
    }
#ifndef HAVE_STRUCT_TM_TM_GMTOFF
    *t -= gmtoff * 60 * APR_USEC_PER_SEC;
#endif

    if ( dateend != NULL )
    {
	*dateend = date + offs;
    }
    
    return ( APR_SUCCESS );
}



/*
 * Parses an iso date (rfc3339)
 *
 * Formats supported:
 *
 *     1977-09-06 01:02:03
 *     1977-09-06 01:02:03.004
 *     1977-09-06T01:02:03.004Z
 *     1977-09-06T01:02:03.004+02:00
 *     2011-5-29 0:3:21
 *     2011-5-29 0:3:21+02:00
 *     2011-5-29 0:3:21.004
 *     2011-5-29 0:3:21.004+02:00
 *
 */

apr_status_t nx_date_parse_iso(apr_time_t  *t, 
			       const char *date,
			       const char **dateend)
{
    apr_time_exp_t ds;
    struct tm tm;
    apr_status_t rv;
    const char *timstr;
    time_t tval;
    size_t offs = 0;
    int i;
    int mul = 100000;
    int32_t usec = 0;
    int gmtoff = 0;
    boolean have_tz = FALSE;

    if ( !date )
    {
        return ( APR_EBADDATE );
    }

    if ( _date_checkmask(date, "####-##-## ##:##:##*") ||
	 _date_checkmask(date, "####-##-##T##:##:##*") )
    {
	ds.tm_year = ((date[0] - '0') * 1000) + ((date[1] - '0') * 100) +
	    ((date[2] - '0') * 10) + (date[3] - '0');
	ds.tm_mon = ((date[5] - '0') * 10) + (date[6] - '0') - 1;
	ds.tm_mday = ((date[8] - '0') * 10) + (date[9] - '0');

        timstr = date + 11;
	
        TIMEPARSE_STD(ds, timstr);
	offs += 19;

	if ( (date[offs] == '.') || (date[offs] == ',') )
	{
	    offs++;
	    i = 0;
	    for ( ; apr_isdigit(date[offs]); offs++, i++ )
	    {
		if ( i >= 6 )
		{
		    return ( APR_EBADDATE );
		}
		usec += (date[offs] - '0') * mul;
		mul /= 10;
	    }
	}

	if ( (date[offs] == '+') || (date[offs] == '-') )
	{
	    offs += get_tz_offset(date + offs, &gmtoff);
	    have_tz = TRUE;
	}
	else if ( date[offs] == 'Z'  )
	{
	    offs++;
	    gmtoff = 0;
	    have_tz = TRUE;
	}
    }
    // Loser format with single digit in month/day/hour/min/sec
    // 2011-5-29 0:3:21
    else if ( _date_checkmask(date, "####-#*") )
    {
	ds.tm_year = ((date[0] - '0') * 1000) + ((date[1] - '0') * 100) +
	    ((date[2] - '0') * 10) + (date[3] - '0');
	offs = 5;

	// mon
	if ( apr_isdigit(date[offs + 1]) )
	{
	    ds.tm_mon = ((date[offs] - '0') * 10) + (date[offs + 1] - '0') - 1;
	    offs += 2;
	}
	else if ( date[offs + 1] == '-' )
	{
	    ds.tm_mon = (date[offs] - '0') - 1;
	    offs++;
	}
	else
	{
	    return ( APR_EBADDATE );
	}
	if ( date[offs] != '-' )
	{
	    return ( APR_EBADDATE );
	}
	offs++;

	// day
	if ( apr_isdigit(date[offs]) && apr_isdigit(date[offs + 1]) )
	{
	    ds.tm_mday = ((date[offs] - '0') * 10) + (date[offs + 1] - '0');
	    offs += 2;
	}
	else if ( apr_isdigit(date[offs]) && (date[offs + 1] == ' ') )
	{
	    ds.tm_mday = date[offs] - '0';
	    offs++;
	}
	else
	{
	    return ( APR_EBADDATE );
	}
	if ( date[offs] != ' ' )
	{
	    return ( APR_EBADDATE );
	}
	offs++;

	// hour
	if ( apr_isdigit(date[offs]) && apr_isdigit(date[offs + 1]) )
	{
	    ds.tm_hour = ((date[offs] - '0') * 10) + (date[offs + 1] - '0');
	    offs += 2;
	}
	else if ( apr_isdigit(date[offs]) && (date[offs + 1] == ':') )
	{
	    ds.tm_hour = date[offs] - '0';
	    offs++;
	}
	else
	{
	    return ( APR_EBADDATE );
	}
	if ( date[offs] != ':' )
	{
	    return ( APR_EBADDATE );
	}
	offs++;

	// min
	if ( apr_isdigit(date[offs]) && apr_isdigit(date[offs + 1]) )
	{
	    ds.tm_min = ((date[offs] - '0') * 10) + (date[offs + 1] - '0');
	    offs += 2;
	}
	else if ( apr_isdigit(date[offs]) && (date[offs + 1] == ':') )
	{
	    ds.tm_min = date[offs] - '0';
	    offs++;
	}
	else
	{
	    return ( APR_EBADDATE );
	}
	if ( date[offs] != ':' )
	{
	    return ( APR_EBADDATE );
	}
	offs++;

	// sec
	if ( apr_isdigit(date[offs]) && apr_isdigit(date[offs + 1]) )
	{
	    ds.tm_sec = ((date[offs] - '0') * 10) + (date[offs + 1] - '0');
	    offs += 2;
	}
	else if ( apr_isdigit(date[offs]) )
	{
	    ds.tm_sec = date[offs] - '0';
	    offs++;
	}
	else
	{
	    return ( APR_EBADDATE );
	}

	if ( (date[offs] == '.') || (date[offs] == ',') )
	{
	    offs++;
	    i = 0;
	    for ( ; apr_isdigit(date[offs]); offs++, i++ )
	    {
		if ( i >= 6 )
		{
		    return ( APR_EBADDATE );
		}
		usec += (date[offs] - '0') * mul;
		mul /= 10;
	    }
	}
	if ( (date[offs] == '+') || (date[offs] == '-') )
	{
	    offs += get_tz_offset(date + offs, &gmtoff);
	    have_tz = TRUE;
	}
	else if ( date[offs] == 'Z'  )
	{
	    offs++;
	    gmtoff = 0;
	    have_tz = TRUE;
	}
    }
    else
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday <= 0) || (ds.tm_mday > 31) )
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61) )
    { 
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday == 31) && (ds.tm_mon == 3 || ds.tm_mon == 5 || ds.tm_mon == 8 || ds.tm_mon == 10) )
    {
        return ( APR_EBADDATE );
    }

    if ( ds.tm_year < 1970 )
    {
        return ( APR_EBADDATE );
    }	
    ds.tm_year -= 1900;
    ds.tm_isdst = -1;
    ds.tm_usec = usec;

    if ( have_tz == TRUE )
    {
	ds.tm_gmtoff = gmtoff * 60;
	if ( (rv = apr_time_exp_gmt_get(t, &ds)) != APR_SUCCESS )
	{
	    return ( rv );
	}
    }
    else 
    {   // no timezone
	// apr_time_exp_gmt_get() doesn't work properly so we have to use mktime
	tm.tm_year = ds.tm_year;
	tm.tm_mon = ds.tm_mon;
	tm.tm_mday = ds.tm_mday;
	tm.tm_hour = ds.tm_hour;
	tm.tm_min = ds.tm_min;
	tm.tm_sec = ds.tm_sec;
	tm.tm_isdst = -1;

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
	ds.tm_gmtoff = 0;
#endif
	if ( (tval = mktime(&tm)) == -1 )
	{
	    return ( APR_EBADDATE );
	}
	
	if ( (rv = apr_time_ansi_put(t, tval)) != APR_SUCCESS )
	{
	    return ( rv );
	}
	
	*t += usec;
	//ds.tm_gmtoff = get_gmtoff_localtime();
    }


    if ( dateend != NULL )
    {
	*dateend = date + offs;
    }
    
    return ( APR_SUCCESS );
}



/*
 * Parses a cisco timestamp
 *
 * Formats supported:
 *  * Nov 3 14:50:30.403
 *  * Oct 12 2004 21:54:47 
 */

apr_status_t nx_date_parse_cisco(apr_time_t  *t, 
				 const char *date,
				 const char **dateend)
{
    //apr_time_exp_t ds;
    struct tm ds;
    apr_status_t rv;
    const char *timstr;
    time_t tval;
    size_t offs = 0;
    size_t i;
    int32_t usec = 0;
    int gmtoff = 0;
    int mint, mon;
    int mul = 100000;
    static const int months[12] =
    {
	('J' << 16) | ('a' << 8) | 'n', ('F' << 16) | ('e' << 8) | 'b',
	('M' << 16) | ('a' << 8) | 'r', ('A' << 16) | ('p' << 8) | 'r',
	('M' << 16) | ('a' << 8) | 'y', ('J' << 16) | ('u' << 8) | 'n',
	('J' << 16) | ('u' << 8) | 'l', ('A' << 16) | ('u' << 8) | 'g',
	('S' << 16) | ('e' << 8) | 'p', ('O' << 16) | ('c' << 8) | 't',
	('N' << 16) | ('o' << 8) | 'v', ('D' << 16) | ('e' << 8) | 'c' 
    };

    if ( !date )
    {
        return ( APR_EBADDATE );
    }

    if ( _date_checkmask(date, "@$$ # ##:##:##*") ||
	 _date_checkmask(date, "@$$  # ##:##:##*") ||
	 _date_checkmask(date, "@$$ ## ##:##:##*") )
    {
	mint = (date[0] << 16) | (date[1] << 8) | date[2];
	for ( mon = 0; mon < 12; mon++ )
	{
	    if ( mint == months[mon] )
	    {
		break;
	    }
	}
	if ( mon == 12 )
	{ // Invalid month
	    return ( APR_EBADDATE );
	}
	ds.tm_mon = mon;

	for ( i = 4; date[i] == ' '; i++ );

	if ( date[i + 1] == ' ' )
	{ // one digit day
	    ds.tm_mday = date[i] - '0';
	    i += 2;
	}
	else
	{
	    ds.tm_mday = ((date[i] - '0') * 10) + (date[i + 1] - '0');
	    i += 3;
	}

        timstr = date + i;
	
        TIMEPARSE_STD(ds, timstr);
	offs = i + 8;

	if ( date[offs] == '.' )
	{
	    offs++;
	    i = 0;
	    for ( ; apr_isdigit(date[offs]); offs++, i++ )
	    {
		if ( i >= 6 )
		{
		    return ( APR_EBADDATE );
		}
		usec += (date[offs] - '0') * mul;
		mul /= 10;
	    }
	}
	// no year, set it to current
	ds.tm_year = _get_curr_year();
    }
    else if ( _date_checkmask(date, "@$$ # #### ##:##:##*") ||
	      _date_checkmask(date, "@$$  # #### ##:##:##*") ||
	      _date_checkmask(date, "@$$ ## #### ##:##:##*") )
    {
	mint = (date[0] << 16) | (date[1] << 8) | date[2];
	for ( mon = 0; mon < 12; mon++ )
	{
	    if ( mint == months[mon] )
	    {
		break;
	    }
	}
	if ( mon == 12 )
	{ // Invalid month
	    return ( APR_EBADDATE );
	}
	ds.tm_mon = mon;

	for ( i = 4; date[i] == ' '; i++ );

	if ( date[i + 1] == ' ' )
	{ // one digit day
	    ds.tm_mday = date[i] - '0';
	    i += 2;
	}
	else
	{
	    ds.tm_mday = ((date[i] - '0') * 10) + (date[i + 1] - '0');
	    i += 3;
	}

        ds.tm_year = ((date[i] - '0') * 10 + (date[i + 1] - '0') - 19) * 100;

        if ( ds.tm_year < 0 )
	{
            return ( APR_EBADDATE );
	}

        ds.tm_year += ((date[i + 2] - '0') * 10) + (date[i + 3] - '0');

	i += 5;
        timstr = date + i;
	
        TIMEPARSE_STD(ds, timstr);
	offs = i + 8;

	if ( date[offs] == '.' )
	{
	    offs++;
	    i = 0;
	    for ( ; apr_isdigit(date[offs]); offs++, i++ )
	    {
		if ( i >= 6 )
		{
		    return ( APR_EBADDATE );
		}
		usec += (date[offs] - '0') * mul;
		mul /= 10;
	    }
	}
    }
    else
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday <= 0) || (ds.tm_mday > 31) )
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61) )
    { 
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday == 31) && (ds.tm_mon == 3 || ds.tm_mon == 5 || ds.tm_mon == 8 || ds.tm_mon == 10) )
    {
        return ( APR_EBADDATE );
    }

    if ( ds.tm_year == 0 )
    {
	ds.tm_year = _get_curr_year();
    }

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    ds.tm_gmtoff = gmtoff * 60;
#endif
    ds.tm_isdst = -1;

    if ( (tval = mktime(&ds)) == -1 )
    {
        return ( APR_EBADDATE );
    }
    
    if ( (rv = apr_time_ansi_put(t, tval)) != APR_SUCCESS )
    {
	return ( rv );
    }
    *t += usec;
#ifndef HAVE_STRUCT_TM_TM_GMTOFF
    *t += gmtoff * 60 * APR_USEC_PER_SEC;
#endif

    if ( dateend != NULL )
    {
	*dateend = date + offs;
    }
    
    return ( APR_SUCCESS );
}



apr_status_t nx_date_parse_win(apr_time_t  *t, 
			       const char *date,
			       const char **dateend)
{
    struct tm ds;
    apr_status_t rv;
    const char *timstr;
    time_t tval;
    size_t offs = 0;
    int i;
    int mul = 100000;
    int32_t usec = 0;
    int gmtoff = 0;
    boolean negtz = FALSE;

    if ( !date )
    {
        return ( APR_EBADDATE );
    }

    //                          20100426151354.537875-000
    if ( _date_checkmask(date, "##############.######-###") ||
	 _date_checkmask(date, "##############.#########") )
    {
	ds.tm_year = ((date[0] - '0') * 1000) + ((date[1] - '0') * 100) +
	    ((date[2] - '0') * 10) + (date[3] - '0');
	ds.tm_mon = ((date[4] - '0') * 10) + (date[5] - '0') - 1;
	ds.tm_mday = ((date[6] - '0') * 10) + (date[7] - '0');

        timstr = date + 8;
	
        TIMEPARSE(ds, timstr[0], timstr[1], timstr[2], timstr[3], timstr[4], timstr[5]);
	offs += 14;

	if ( date[offs] == '.' )
	{
	    offs++;
	    i = 0;
	    for ( ; apr_isdigit(date[offs]); offs++, i++ )
	    {
		if ( i >= 6 )
		{
		    return ( APR_EBADDATE );
		}
		usec += (date[offs] - '0') * mul;
		mul /= 10;
	    }
	}

	if ( date[offs] == '+' )
	{
	    offs++;
	}
	if ( date[offs] == '-' )
	{
	    offs++;
	    negtz = TRUE;
	}
	gmtoff = (date[offs] - '0') * 100 + (date[offs + 1] - '0') * 10 + (date[offs + 2] - '0');
	if ( negtz == TRUE )
	{
	    gmtoff *= -1;
	}
    }
    else
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday <= 0) || (ds.tm_mday > 31) )
    {
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_hour > 23) || (ds.tm_min > 59) || (ds.tm_sec > 61) )
    { 
        return ( APR_EBADDATE );
    }

    if ( (ds.tm_mday == 31) && (ds.tm_mon == 3 || ds.tm_mon == 5 || ds.tm_mon == 8 || ds.tm_mon == 10) )
    {
        return ( APR_EBADDATE );
    }

    if ( ds.tm_year < 1970 )
    {
        return ( APR_EBADDATE );
    }	
    ds.tm_year -= 1900;

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    ds.tm_gmtoff = gmtoff * 60;
#endif
    ds.tm_isdst = -1;

    if ( (tval = mktime(&ds)) == -1 )
    {
        return ( APR_EBADDATE );
    }
    
    if ( (rv = apr_time_ansi_put(t, tval)) != APR_SUCCESS )
    {
	return ( rv );
    }
    *t += usec;
#ifndef HAVE_STRUCT_TM_TM_GMTOFF
    *t += gmtoff * 60 * APR_USEC_PER_SEC;
#endif

/* this doesn't work properly: we have to use mktime
    if ( (rv = apr_time_exp_gmt_get(t, &ds)) != APR_SUCCESS )
    {
        return ( rv );
    }
*/
    if ( dateend != NULL )
    {
	*dateend = date + offs;
    }
    
    return ( APR_SUCCESS );
}



/*
 * Parses integer representation with or without microsecond precision in UTC.
 * 1258531221.650359
 * 1258531221
 */

apr_status_t nx_date_parse_timestamp(apr_time_t  *t, 
				     const char *date,
				     const char **dateend)
{
    int i;
    size_t offs = 0;
    int32_t usec = 0;
    int64_t sec = 0;
    int mul = 100000;

    if ( !date )
    {
        return ( APR_EBADDATE );
    }

    for ( ; apr_isdigit(date[offs]); offs++, i++ )
    {
	sec *= 10;
	sec += (date[offs] - '0');
    }
    switch ( date[offs] )
    {
	case '.':
	    offs++;
	    i = 0;
	    for ( ; apr_isdigit(date[offs]); offs++, i++ )
	    {
		if ( i >= 6 )
		{
		    return ( APR_EBADDATE );
		}
		usec += (date[offs] - '0') * mul;
		mul /= 10;
	    }
	break;

	case '\0':
	case ' ':
	case '\t':
	    // considered as ok
	    break;
	default:
	    return ( APR_EBADDATE );
    }

    *t = sec * APR_USEC_PER_SEC + usec;

    if ( dateend != NULL )
    {
	*dateend = date + offs;
    }
    
    return ( APR_SUCCESS );
}



apr_status_t nx_date_parse(apr_time_t *t, const char *date, const char **dateend)
{
    apr_status_t rv;

    if ( (rv = nx_date_parse_rfc3164(t, date, dateend)) == APR_SUCCESS )
    {
	return ( rv );
    }

    if ( (rv = nx_date_parse_rfc1123(t, date, dateend)) == APR_SUCCESS )
    {
	return ( rv );
    }

    if ( (rv = nx_date_parse_iso(t, date, dateend)) == APR_SUCCESS )
    {
	return ( rv );
    }

    if ( (rv = nx_date_parse_apache(t, date, dateend)) == APR_SUCCESS )
    {
	return ( rv );
    }

    if ( (rv = nx_date_parse_cisco(t, date, dateend)) == APR_SUCCESS )
    {
	return ( rv );
    }

    if ( (rv = nx_date_parse_win(t, date, dateend)) == APR_SUCCESS )
    {
	return ( rv );
    }
    // parse_win must be called before parse_timestamp since the latter would match the 'win' format.
    if ( (rv = nx_date_parse_timestamp(t, date, dateend)) == APR_SUCCESS )
    {
	return ( rv );
    }

    return ( APR_EBADDATE );
}


/**
 * datestr must be able to hold at least 16 chars including NUL
 */

apr_status_t nx_date_to_rfc3164(char *datestr,
				apr_size_t dstsize,
				apr_time_t timeval)
{
    apr_status_t rv;
    apr_time_exp_t ds;
    char *ptr;
    static const char *months[] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        "Aug", "Sep", "Oct", "Nov", "Dec",
    };

    ASSERT(datestr != NULL);
    ASSERT(dstsize >= 16);

    if ( (rv = apr_time_exp_lt(&ds, timeval)) != APR_SUCCESS )
    {
	return ( rv );
    }

    ASSERT(ds.tm_mon < 12);
    
    ptr = datestr;
    // month
    *ptr++ = months[ds.tm_mon][0];
    *ptr++ = months[ds.tm_mon][1];
    *ptr++ = months[ds.tm_mon][2];
    *ptr++ = ' ';

    // day
    if ( ds.tm_mday >= 10 )
    {
	*ptr++ = (char) ((ds.tm_mday / 10) + '0');
	*ptr++ = (char) ((ds.tm_mday % 10) + '0');
    }
    else
    {
	*ptr++ = ' ';
	*ptr++ = (char) (ds.tm_mday + '0');
    }
    *ptr++ = ' ';

    // hour
    *ptr++ = (char) ((ds.tm_hour / 10) + '0');
    *ptr++ = (char) ((ds.tm_hour % 10) + '0');
    *ptr++ = ':';

    // min
    *ptr++ = (char) ((ds.tm_min / 10) + '0');
    *ptr++ = (char) ((ds.tm_min % 10) + '0');
    *ptr++ = ':';

    // sec
    *ptr++ = (char) ((ds.tm_sec / 10) + '0');
    *ptr++ = (char) ((ds.tm_sec % 10) + '0');

    *ptr = '\0';

    return ( APR_SUCCESS );
}



/*
 * Similar to nx_date_to_rfc3164
 * Creates this format:  Sun Nov  6 08:49:37 2011, 
 * spaceday set to false:  Sun Nov 06 08:49:37 2011
 * datestr must be at least 25 bytes
 */

apr_status_t nx_date_to_rfc3164_wday_year(char *datestr,
					  apr_size_t dstsize,
					  apr_time_t timeval,
					  boolean spaceday)
{
    apr_status_t rv;
    apr_time_exp_t ds;
    char *ptr;
    static const char *months[] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
        "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    static const char *wdays[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

    ASSERT(datestr != NULL);
    ASSERT(dstsize >= 25);

    if ( (rv = apr_time_exp_lt(&ds, timeval)) != APR_SUCCESS )
    {
	return ( rv );
    }

    ASSERT(ds.tm_mon < 12);
    
    ptr = datestr;
    // wday
    ASSERT((ds.tm_wday >= 0) && (ds.tm_wday <= 6));
    *ptr++ = wdays[ds.tm_wday][0];
    *ptr++ = wdays[ds.tm_wday][1];
    *ptr++ = wdays[ds.tm_wday][2];
    *ptr++ = ' ';

    // month
    *ptr++ = months[ds.tm_mon][0];
    *ptr++ = months[ds.tm_mon][1];
    *ptr++ = months[ds.tm_mon][2];
    *ptr++ = ' ';

    // day
    if ( ds.tm_mday >= 10 )
    {
	*ptr++ = (char) ((ds.tm_mday / 10) + '0');
	*ptr++ = (char) ((ds.tm_mday % 10) + '0');
    }
    else
    {
	if ( spaceday == TRUE )
	{
	    *ptr++ = ' ';
	}
	else
	{
	    *ptr++ = '0';
	}
	*ptr++ = (char) (ds.tm_mday + '0');
    }
    *ptr++ = ' ';

    // hour
    *ptr++ = (char) ((ds.tm_hour / 10) + '0');
    *ptr++ = (char) ((ds.tm_hour % 10) + '0');
    *ptr++ = ':';

    // min
    *ptr++ = (char) ((ds.tm_min / 10) + '0');
    *ptr++ = (char) ((ds.tm_min % 10) + '0');
    *ptr++ = ':';

    // sec
    *ptr++ = (char) ((ds.tm_sec / 10) + '0');
    *ptr++ = (char) ((ds.tm_sec % 10) + '0');
    *ptr++ = ' ';

    // year
    if ( ds.tm_year < 1900 )
    {
	ds.tm_year += 1900;
    }
    *ptr++ = (char) ((ds.tm_year / 1000) + '0');
    ds.tm_year -= (ds.tm_year / 1000) * 1000;
    *ptr++ = (char) ((ds.tm_year / 100) + '0');
    ds.tm_year -= (ds.tm_year / 100) * 100;
    *ptr++ = (char) ((ds.tm_year / 10) + '0');
    *ptr++ = (char) ((ds.tm_year % 10) + '0');

    *ptr = '\0';

    return ( APR_SUCCESS );
}



/* dst requires at least 20 bytes
 * Format is: 2000-01-01 00:00:00
 */
apr_status_t nx_date_to_iso(char *dst,
			    apr_size_t dstsize,
			    apr_time_t t)
{
    apr_status_t rv;
    apr_time_exp_t exp;

    ASSERT(dst != NULL);
    ASSERT(dstsize >= 20);

    if ( (rv = apr_time_exp_lt(&exp, t)) != APR_SUCCESS )
    {
	return ( rv );
    }
    if ( exp.tm_year < 1900 )
    {
	exp.tm_year += 1900;
    }
    apr_snprintf(dst, 20, "%d-%02d-%02d %02d:%02d:%02d", exp.tm_year, exp.tm_mon + 1,
		 exp.tm_mday, exp.tm_hour, exp.tm_min, exp.tm_sec);

    return ( APR_SUCCESS );
}



/* dst requires at least 33 bytes
 * Format for GMT: 2000-01-01T00:00:00.000000Z
 * Format for localtime: 2000-01-01T00:00:00.000000+01:00
 */
apr_status_t nx_date_to_rfc5424(char *dst,
				apr_size_t dstsize,
				boolean gmt,
				apr_time_t t)
{
    apr_status_t rv;
    apr_time_exp_t exp;

    ASSERT(dst != NULL);
    ASSERT(dstsize >= 33);

    if ( gmt == TRUE )
    {
	if ( (rv = apr_time_exp_gmt(&exp, t)) != APR_SUCCESS )
	{
	    return ( rv );
	}
	if ( exp.tm_year < 1900 )
	{
	    exp.tm_year += 1900;
	}
    
	apr_snprintf(dst, 30, "%d-%02d-%02dT%02d:%02d:%02d.%06dZ", exp.tm_year, exp.tm_mon + 1,
		     exp.tm_mday, exp.tm_hour, exp.tm_min, exp.tm_sec, exp.tm_usec);
    }
    else
    { // localtime
	if ( (rv = apr_time_exp_lt(&exp, t)) != APR_SUCCESS )
	{
	    return ( rv );
	}
	if ( exp.tm_year < 1900 )
	{
	    exp.tm_year += 1900;
	}
	apr_snprintf(dst, 33, "%d-%02d-%02dT%02d:%02d:%02d.%06d%+03d:%02d",
		     exp.tm_year, exp.tm_mon + 1,
		     exp.tm_mday, exp.tm_hour, exp.tm_min, exp.tm_sec,
		     exp.tm_usec, exp.tm_gmtoff / 3600,
		     (((exp.tm_gmtoff > 0) ? exp.tm_gmtoff : -exp.tm_gmtoff) /
		      60) % 60);
    }
    
    return ( APR_SUCCESS );
}



apr_status_t nx_date_fix_year(apr_time_t *t)
{
    apr_status_t rv;
    apr_time_exp_t exp, exp2;
    char tmpstr[20];

    ASSERT(t != NULL);

    // FIXME: add heuristics to detect end-of-year skew

    if ( (rv = apr_time_exp_gmt(&exp, *t)) != APR_SUCCESS )
    {
	return ( rv );
    }

    if ( exp.tm_year <= 70 )
    {
	nx_date_to_iso(tmpstr, sizeof(tmpstr), *t);
	if ( (rv = apr_time_exp_gmt(&exp2, apr_time_now())) != APR_SUCCESS )
	{
	    return ( rv );
	}
	tmpstr[0] = '2';
	tmpstr[1] = (char) ('0' + (exp2.tm_year - 100) / 100);
	tmpstr[2] = (char) ('0' + (exp2.tm_year - 100) / 10);
	tmpstr[3] = (char) ('0' + exp2.tm_year % 10);

	return ( nx_date_parse_iso(t, tmpstr, NULL) );
    }

    return ( APR_SUCCESS );
}
