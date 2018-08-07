/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>
#include <time.h>

#include "expr-core-funcproc.h"
#include "date.h"
#include "statvar.h"
#include "../core/nxlog.h"
#include "../core/ctx.h"
#include "alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

void nx_expr_func__lc(nx_expr_eval_ctx_t *eval_ctx UNUSED,
		      nx_module_t *module UNUSED,
		      nx_value_t *retval,
		      int32_t num_arg,
		      nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    int i;

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }
    retval->string = nx_string_clone(args[0].string);
    retval->defined = TRUE;

    //FIXME: utf8 tolower
    for ( i = 0; args[0].string->buf[i] != '\0'; i++ )
    {
	retval->string->buf[i] = (char) apr_tolower(args[0].string->buf[i]);
    }
}



void nx_expr_func__uc(nx_expr_eval_ctx_t *eval_ctx UNUSED,
		      nx_module_t *module UNUSED,
		      nx_value_t *retval,
		      int32_t num_arg,
		      nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    int i;

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }
    retval->string = nx_string_clone(args[0].string);
    retval->defined = TRUE;

    //FIXME: utf8 toupper
    for ( i = 0; args[0].string->buf[i] != '\0'; i++ )
    {
	retval->string->buf[i] = (char) apr_toupper(args[0].string->buf[i]);
    }
}



void nx_expr_func__now(nx_expr_eval_ctx_t *eval_ctx UNUSED,
		       nx_module_t *module UNUSED,
		       nx_value_t *retval,
		       int32_t num_arg,
		       nx_value_t *args UNUSED)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    retval->type = NX_VALUE_TYPE_DATETIME;
    retval->datetime = apr_time_now();
    retval->defined = TRUE;
}



void nx_expr_func__type(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			nx_module_t *module UNUSED,
			nx_value_t *retval,
			int32_t num_arg,
			nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    retval->string = nx_string_create(nx_value_type_to_string(args[0].type), -1);
    retval->defined = TRUE;
}



void nx_expr_func__microsecond(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			    nx_module_t *module UNUSED,
			    nx_value_t *retval,
			    int32_t num_arg,
			    nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_usec;
    retval->defined = TRUE;
}



void nx_expr_func__second(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			  nx_module_t *module UNUSED,
			  nx_value_t *retval,
			  int32_t num_arg,
			  nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_sec;
    retval->defined = TRUE;
}



void nx_expr_func__minute(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			  nx_module_t *module UNUSED,
			  nx_value_t *retval,
			  int32_t num_arg,
			  nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_min;
    retval->defined = TRUE;
}



void nx_expr_func__hour(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			nx_module_t *module UNUSED,
			nx_value_t *retval,
			int32_t num_arg,
			nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_hour;
    retval->defined = TRUE;
}



void nx_expr_func__day(nx_expr_eval_ctx_t *eval_ctx UNUSED,
		       nx_module_t *module UNUSED,
		       nx_value_t *retval,
		       int32_t num_arg,
		       nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_mday;
    retval->defined = TRUE;
}



void nx_expr_func__month(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			 nx_module_t *module UNUSED,
			 nx_value_t *retval,
			 int32_t num_arg,
			 nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_mon + 1;
    retval->defined = TRUE;
}



void nx_expr_func__year(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			nx_module_t *module UNUSED,
			nx_value_t *retval,
			int32_t num_arg,
			nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_year;
    if ( retval->integer < 1900 )
    {
	retval->integer += 1900;
    }
    retval->defined = TRUE;
}



void nx_expr_func__fix_year(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			    nx_module_t *module UNUSED,
			    nx_value_t *retval,
			    int32_t num_arg,
			    nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_DATETIME;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    retval->datetime = args[0].datetime;
    nx_date_fix_year(&(retval->datetime));
    retval->defined = TRUE;
}



void nx_expr_func__dayofyear(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			     nx_module_t *module UNUSED,
			     nx_value_t *retval,
			     int32_t num_arg,
			     nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_yday + 1;
    retval->defined = TRUE;
}



void nx_expr_func__dayofweek(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			     nx_module_t *module UNUSED,
			     nx_value_t *retval,
			     int32_t num_arg,
			     nx_value_t *args)
{
    apr_time_exp_t ds;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    CHECKERR_MSG(apr_time_exp_lt(&ds, args[0].datetime), "couldn't convert datetime");

    retval->integer = ds.tm_wday + 1;
    retval->defined = TRUE;
}



void nx_expr_func__to_string(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			     nx_module_t *module UNUSED,
			     nx_value_t *retval,
			     int32_t num_arg,
			     nx_value_t *args)
{
    char *str;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    str = nx_value_to_string(&(args[0]));
    if ( str != NULL )
    {
	retval->string = nx_string_create_owned(str, -1);
	retval->defined = TRUE;
    }
    else
    {
	retval->defined = FALSE;
    }
}



void nx_expr_func__to_integer(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			      nx_module_t *module UNUSED,
			      nx_value_t *retval,
			      int32_t num_arg,
			      nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    retval->type = NX_VALUE_TYPE_INTEGER;
    retval->defined = FALSE;
    if ( args[0].defined == FALSE )
    {
	return;
    }

    if ( args[0].type == NX_VALUE_TYPE_STRING )
    {
	retval->integer = nx_value_parse_int(args[0].string->buf);
    }
    else if ( args[0].type == NX_VALUE_TYPE_DATETIME )
    {
	retval->integer = args[0].datetime;
    }
    else
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }
    retval->defined = TRUE;
}



void nx_expr_func__to_datetime(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			       nx_module_t *module UNUSED,
			       nx_value_t *retval,
			       int32_t num_arg,
			       nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    retval->type = NX_VALUE_TYPE_DATETIME;
    retval->defined = FALSE;
    if ( args[0].defined == FALSE )
    {
	return;
    }

    if ( args[0].type != NX_VALUE_TYPE_INTEGER )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }
    retval->defined = TRUE;

    retval->datetime = args[0].integer;
}



void nx_expr_func__parsedate(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			     nx_module_t *module UNUSED,
			     nx_value_t *retval,
			     int32_t num_arg,
			     nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_DATETIME;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    if ( nx_date_parse(&(retval->datetime), args[0].string->buf, NULL) == APR_SUCCESS )
    {
	retval->defined = TRUE;
    }
    else
    {
	log_debug("couldn't parse date: %s", args[0].string->buf);
	retval->defined = FALSE;
    }
}



void nx_expr_func__strftime(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			    nx_module_t *module UNUSED,
			    nx_value_t *retval,
			    int32_t num_arg,
			    nx_value_t *args)
{
    apr_size_t retsize;
    apr_time_exp_t tm;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 2);

    if ( args[0].type != NX_VALUE_TYPE_DATETIME )
    {
	throw_msg("got '%s' for first argument of function 'strftime(datetime, string)'",
		  nx_value_type_to_string(args[0].type));
    }

    if ( args[1].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("got '%s' for second argument of function 'strftime(datetime, string)'",
		  nx_value_type_to_string(args[1].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }
    retval->string = nx_string_new();
    retval->defined = TRUE;

    if ( args[1].defined == FALSE )
    {
	ASSERT(retval->string->bufsize > 20);
	nx_date_to_iso(retval->string->buf, retval->string->len, args[0].datetime);
	retval->string->len = 20;
	return;
    }

    CHECKERR(apr_time_exp_lt(&tm, args[0].datetime));
    CHECKERR_MSG(apr_strftime(retval->string->buf, &retsize, retval->string->bufsize,
			      args[1].string->buf, &tm),
		 "strftime() couldn't convert datetime to string");
    retval->string->len = (uint32_t) retsize;
}

#ifndef HAVE_STRPTIME
char *strptime(const char *buf, const char *fmt, struct tm *tm);
#endif

void nx_expr_func__strptime(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			    nx_module_t *module UNUSED,
			    nx_value_t *retval,
			    int32_t num_arg,
			    nx_value_t *args)
{
    struct tm tm;
    apr_time_t t;
    time_t tval;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 2);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("string required for first argument of function 'strptime(string, string)', got %s",
		  nx_value_type_to_string(args[0].type));
    }

    if ( args[1].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("string required for second argument of function 'strptime(datetime, string)', got %s",
		  nx_value_type_to_string(args[1].type));
    }

    retval->type = NX_VALUE_TYPE_DATETIME;
    retval->defined = FALSE;
    if ( (args[0].defined == FALSE) || (args[1].defined == FALSE) )
    {
	return;
    }

    memset(&tm, 0, sizeof(struct tm));

    if ( strptime(args[0].string->buf, args[1].string->buf, &tm) == NULL )
    { // return undef if it couldn't parse
	//log_warn("strptime('%s', '%s') failed", args[0].string->buf, args[1].string->buf);
	return;
    }

    tm.tm_isdst = -1;

    if ( (tval = mktime(&tm)) == -1 )
    {
	throw_msg("mktime failed");
    }
    CHECKERR(apr_time_ansi_put(&t, tval));

    retval->defined = TRUE;
    retval->datetime = t;
}



void nx_expr_func__hostname(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			    nx_module_t *module UNUSED,
			    nx_value_t *retval,
			    int32_t num_arg,
			    nx_value_t *args UNUSED)
{
    nxlog_t *nxlog;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    retval->type = NX_VALUE_TYPE_STRING;
    retval->string = nx_string_new_size(255);
    retval->defined = TRUE;

    nxlog = nxlog_get();
    apr_cpystrn(retval->string->buf, nxlog->hostname.buf, retval->string->bufsize);

    retval->string->len = (uint32_t) strlen(retval->string->buf);
}



void nx_expr_func__hostname_fqdn(nx_expr_eval_ctx_t *eval_ctx UNUSED,
				 nx_module_t *module UNUSED,
				 nx_value_t *retval,
				 int32_t num_arg,
				 nx_value_t *args UNUSED)
{
    nxlog_t *nxlog;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    retval->type = NX_VALUE_TYPE_STRING;
    retval->string = nx_string_new_size(255);
    retval->defined = TRUE;

    nxlog = nxlog_get();
    apr_cpystrn(retval->string->buf, nxlog->hostname_fqdn.buf, retval->string->bufsize);

    retval->string->len = (uint32_t) strlen(retval->string->buf);
}



void nx_expr_func__host_ip(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			   nx_module_t *module UNUSED,
			   nx_value_t *retval,
			   int32_t num_arg,
			   nx_value_t *args)
{
    nxlog_t *nxlog;
    nx_exception_t e;
    static apr_sockaddr_t *sa = NULL; // this holds a cached sa so that we don't perform a DNS lookup every time
    volatile uint8_t ip4addr[4];
    uint32_t ip4;
    apr_sockaddr_t *_sa;
    volatile int64_t nth = 1;
    int64_t i = 1;

    ASSERT(retval != NULL);

    if ( num_arg == 1 )
    {
	if ( args[0].type != NX_VALUE_TYPE_INTEGER )
	{
	    throw_msg("got '%s' for first argument of function 'host_ip(integer)'",
		      nx_value_type_to_string(args[0].type));
	}
	if ( args[0].defined == FALSE )
	{
	    retval->defined = FALSE;
	    return;
	}
	nth = args[0].integer;
	if ( nth <= 0 )
	{
	    throw_msg("Positive non-zero number required for argument nth in function 'host_ip(integer)', got %d", nth);
	}
    }
    else
    {
	ASSERT(num_arg == 0);
    }


    retval->type = NX_VALUE_TYPE_IP4ADDR;
    retval->defined = FALSE;

    if ( sa == NULL )
    { 
	nx_lock();
	try
	{
	    nxlog = nxlog_get();

	    CHECKERR_MSG(apr_sockaddr_info_get(&_sa, nxlog->hostname.buf, APR_INET, 0, 0, nxlog->pool), 
			 "cannot resolve hostname to ip address");
	    sa = _sa;
	}
	catch(e)
	{
	    nx_unlock();
	    rethrow(e);
	}
	nx_unlock();
    }
    else
    {
	_sa = sa;
    }
    while ( _sa != NULL )
    {
	if ( _sa->family == AF_INET )
	{ // we are only interested in ipv4 addresses
	    ip4 = ntohl(_sa->sa.sin.sin_addr.s_addr);
	    ip4addr[3] = (uint8_t) (ip4 % 256);
	    ip4addr[2] = (uint8_t) ((ip4 / 256) % 256);
	    ip4addr[1] = (uint8_t) ((ip4 / (256 * 256)) % 256);
	    ip4addr[0] = (uint8_t) ((ip4 / (256 * 256 * 256)) % 256);
	    if ( ip4addr[0] != 127 )
	    { // ignore loopback address
		if (i == nth)
		{
		    retval->ip4addr[0] = ip4addr[0];
		    retval->ip4addr[1] = ip4addr[1];
		    retval->ip4addr[2] = ip4addr[2];
		    retval->ip4addr[3] = ip4addr[3];
		    retval->defined = TRUE;
		    break; // return nth
		}
		i++;
	    }
	}
	else
	{
	    log_info("non AF_INET");
	}
	_sa = _sa->next;
    }
}



static void nx_expr_proc__log(nx_expr_eval_ctx_t *eval_ctx,
			      nx_module_t *module UNUSED,
			      nx_expr_list_t *args,
			      nx_loglevel_t loglevel)
{
    int i, j;
    nx_expr_list_elem_t *arg;
    char *buf;
    char **argvalues;
    char *tmpstr;
    nx_value_t value;
    size_t len = 0;

    ASSERT(args != NULL);

    for ( i = 0, arg = NX_DLIST_FIRST(args);
	  arg != NULL;
	  arg = NX_DLIST_NEXT(arg, link), i++ );

    argvalues = malloc((size_t) i * sizeof(char *));

    for ( i = 0, arg = NX_DLIST_FIRST(args);
	  arg != NULL;
	  arg = NX_DLIST_NEXT(arg, link), i++ )
    {
	ASSERT(arg->expr != NULL);
	nx_expr_evaluate(eval_ctx, &value, arg->expr);
	argvalues[i] = nx_value_to_string(&value);
	nx_value_kill(&value);
	if ( argvalues[i] == NULL )
	{
	    argvalues[i] = strdup("");
	}
	len += strlen(argvalues[i]);
    }

    buf = malloc(len + 1);
    tmpstr = buf;
    for ( j = 0; j < i; j++ )
    {
	ASSERT(argvalues[j] != NULL);
	memcpy(tmpstr, argvalues[j], strlen(argvalues[j]));
	tmpstr += strlen(argvalues[j]);
	free(argvalues[j]);
    }
    ASSERT(tmpstr == buf + len);
    buf[len] = '\0';
    nx_log(APR_SUCCESS, loglevel, NX_LOGMODULE_CORE, "%s", buf);
    free(buf);
    free(argvalues);
}



void nx_expr_proc__log_debug(nx_expr_eval_ctx_t *eval_ctx,
			     nx_module_t *module,
			     nx_expr_list_t *args)
{
    nx_expr_proc__log(eval_ctx, module, args, NX_LOGLEVEL_DEBUG);
}



void nx_expr_proc__debug(nx_expr_eval_ctx_t *eval_ctx,
			 nx_module_t *module,
			 nx_expr_list_t *args)
{
    nx_expr_proc__log(eval_ctx, module, args, NX_LOGLEVEL_DEBUG);
}



void nx_expr_proc__log_info(nx_expr_eval_ctx_t *eval_ctx,
			     nx_module_t *module,
			     nx_expr_list_t *args)
{
    nx_expr_proc__log(eval_ctx, module, args, NX_LOGLEVEL_INFO);
}



void nx_expr_proc__log_warning(nx_expr_eval_ctx_t *eval_ctx,
			     nx_module_t *module,
			     nx_expr_list_t *args)
{
    nx_expr_proc__log(eval_ctx, module, args, NX_LOGLEVEL_WARNING);
}



void nx_expr_proc__log_error(nx_expr_eval_ctx_t *eval_ctx,
			     nx_module_t *module,
			     nx_expr_list_t *args)
{
    nx_expr_proc__log(eval_ctx, module, args, NX_LOGLEVEL_ERROR);
}



void nx_expr_func__replace(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			   nx_module_t *module UNUSED,
			   nx_value_t *retval,
			   int32_t num_arg,
			   nx_value_t *args)
{
    int64_t count = 0;
    nx_string_t *result = NULL;
    const char *str, *ptr;
    const char *dstptr;
    int dstlen;
    int i;

    ASSERT(retval != NULL);
    ASSERT((num_arg == 3) || (num_arg == 4));

    retval->type = NX_VALUE_TYPE_STRING;

    if ( args[0].defined == FALSE )
    {
	return;
    }
    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid for argument 'subject', string required",
		  nx_value_type_to_string(args[0].type));
    }

    if ( args[1].defined != TRUE )
    { // if 'src' is undef we don't modify 'subject'
	retval->string = nx_string_clone(args[0].string);
	retval->defined = TRUE;
	return;
    }
    if ( args[1].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid for argument 'src', string required",
		  nx_value_type_to_string(args[1].type));
    }

    if ( args[2].defined != TRUE ) 
    {
	dstptr = "";
	dstlen = 0;
    }
    else if ( args[2].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid for argument 'dst', string required",
		  nx_value_type_to_string(args[2].type));
    }
    else
    { // NX_VALUE_TYPE_STRING 
	dstptr = args[2].string->buf;
	dstlen = (int) args[2].string->len;
    }

    if ( num_arg == 4 )
    {
	if ( args[3].type != NX_VALUE_TYPE_INTEGER )
	{
	    throw_msg("'%s' type argument is invalid for argument 'count', integer required",
		      nx_value_type_to_string(args[3].type));
	}
	if ( args[3].defined == TRUE )
	{
	    count = args[3].integer;
	    if ( count < 0 )
	    {
		throw_msg("'positive integer required for argument 'count'");
	    }
	}
    }
    
    if ( args[0].string->len == 0 )
    {
	return;
    }
    if ( args[1].string->len == 0 )
    {
	return;
    }

    for ( i = 0, str = args[0].string->buf; ; i++ )
    {
	if ( (count > 0) && (i >= count) )
	{
	    break;
	}
	ptr = strstr(str, args[1].string->buf);
	if ( ptr == NULL )
	{
	    break;
	}
	if ( result == NULL )
	{
	    result = nx_string_new_size(args[0].string->len);
	}
	nx_string_append(result, str, (int) (ptr - str));
	nx_string_append(result, dstptr, dstlen);
	str = ptr + args[1].string->len;
    }
    if ( str - args[0].string->buf < args[0].string->len )
    {
	if ( result == NULL )
	{
	    result = nx_string_new_size(args[0].string->len);
	}
	nx_string_append(result, str, -1);
    }

    if ( result != NULL )
    {
	retval->defined = TRUE;
	retval->string = result;
    }
}



void nx_expr_proc__drop(nx_expr_eval_ctx_t *eval_ctx,
			nx_module_t *module UNUSED,
			nx_expr_list_t *args UNUSED)
{
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to drop(), already dropped?");
    }
    // must not free it here because it is still linked in the queue
    eval_ctx->logdata = NULL;
    eval_ctx->dropped = TRUE; // don't continue executing further statements
}



void nx_expr_proc__delete(nx_expr_eval_ctx_t *eval_ctx,
			  nx_module_t *module UNUSED,
			  nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;

    ASSERT(args != NULL);

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata to delete() from, possibly dropped");
    }

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    if ( arg->expr->type != NX_EXPR_TYPE_FIELD )
    {
	throw_msg("field type required as argument for delete($field)");
    }

    while ( nx_logdata_delete_field(eval_ctx->logdata, arg->expr->field) == TRUE );
    if ( strcmp(arg->expr->field, "raw_event") == 0 )
    {
	eval_ctx->logdata->raw_event = NULL;
    }
}



void nx_expr_proc__create_var(nx_expr_eval_ctx_t *eval_ctx,
			      nx_module_t *module,
			      nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t argval1, argval2;
    apr_time_t expiry = 0;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);
    module = eval_ctx->module;

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    nx_expr_evaluate(eval_ctx, &argval1, arg->expr);
    if ( (argval1.defined != TRUE) || (argval1.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval1);
	throw_msg("string argument required for create_var(varname)");
    }
    if ( argval1.string->len == 0 )
    {
	nx_value_kill(&argval1);
	throw_msg("zero sized string argument is invalid for create_var(varname)");
    }

    arg = NX_DLIST_NEXT(arg, link);


    if ( arg == NULL )
    {	//no lifetime parameter
	nx_module_lock(module);
	nx_module_var_create(module, argval1.string->buf, argval1.string->len, 0);
	nx_module_unlock(module);
	nx_value_kill(&argval1);
    }
    else
    {
	nx_expr_evaluate(eval_ctx, &argval2, arg->expr);
	if ( (argval2.defined != TRUE) ||
	     !((argval2.type == NX_VALUE_TYPE_INTEGER) ||
	       (argval2.type == NX_VALUE_TYPE_DATETIME)) )
	{
	    nx_value_kill(&argval1);
	    throw_msg("integer or datetime argument required for create_var(varname)");
	}
	if ( argval2.type == NX_VALUE_TYPE_INTEGER )
	{
	    expiry = apr_time_now() + argval2.integer * APR_USEC_PER_SEC;
	}
	else if ( argval2.type == NX_VALUE_TYPE_DATETIME )
	{
	    if ( argval2.datetime < apr_time_now() )
	    {
		log_warn("expiry given for create_var('%s') is in the past, created variable will never expire",
			 argval1.string->buf);
	    }
	    else
	    {
		expiry = argval2.datetime;
	    }
	}
	nx_module_lock(module);
	nx_module_var_create(module, argval1.string->buf, argval1.string->len, expiry);
	nx_module_unlock(module);
	nx_value_kill(&argval1);
    }
}



void nx_expr_proc__delete_var(nx_expr_eval_ctx_t *eval_ctx,
			      nx_module_t *module UNUSED,
			      nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t argval;
    nx_exception_t e;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    nx_expr_evaluate(eval_ctx, &argval, arg->expr);
    if ( (argval.defined != TRUE) || (argval.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval);
	throw_msg("string argument required for delete_var(varname)");
    }
    
    nx_module_lock(eval_ctx->module);
    try
    {
	nx_module_var_delete(eval_ctx->module, argval.string->buf, argval.string->len);
    }
    catch(e)
    {
	nx_module_unlock(eval_ctx->module);
	rethrow(e);
    }
    nx_module_unlock(eval_ctx->module);
}



void nx_expr_proc__set_var(nx_expr_eval_ctx_t *eval_ctx,
			   nx_module_t *module,
			   nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg1;
    nx_expr_list_elem_t *arg2;
    nx_value_t argval;
    nx_module_var_t *var;
    nx_value_t tmpval;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);
    module = eval_ctx->module;

    arg1 = NX_DLIST_FIRST(args);
    ASSERT(arg1 != NULL);
    arg2 = NX_DLIST_NEXT(arg1, link);
    ASSERT(arg2 != NULL);

    nx_expr_evaluate(eval_ctx, &argval, arg1->expr);
    if ( (argval.defined != TRUE) || (argval.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval);
	throw_msg("string argument required for set_var(varname, value)");
    }

    nx_module_lock(module);
    if ( module->vars == NULL )
    { // create variable
	var = nx_module_var_create(module, argval.string->buf, argval.string->len, 0);
    }
    else
    {
	var = apr_hash_get(module->vars, argval.string->buf, argval.string->len);
    }
    if ( var == NULL )
    { // create variable
	var = nx_module_var_create(module, argval.string->buf, argval.string->len, 0);
    }
    nx_module_unlock(module);

    nx_value_kill(&argval);
    nx_expr_evaluate(eval_ctx, &tmpval, arg2->expr);

    nx_module_lock(module);
    if ( var->value.defined == TRUE )
    { // free old value
	nx_value_kill(&(var->value));
    }
    var->value = tmpval;
    nx_module_unlock(module);
}



void nx_expr_func__get_var(nx_expr_eval_ctx_t *eval_ctx,
			   nx_module_t *module,
			   nx_value_t *retval,
			   int32_t num_arg,
			   nx_value_t *args)
{
    nx_module_var_t *var;
    nx_exception_t e;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);
    ASSERT(eval_ctx->module != NULL);
    module = eval_ctx->module;

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    nx_module_lock(module);
    try
    {
	if ( module->vars == NULL )
	{
	    log_debug("get_var(): no module variables, var '%s' doesn't exist", args[0].string->buf);
	    retval->defined = FALSE;
	    retval->type = NX_VALUE_TYPE_UNKNOWN;
	}
	else
	{
	    var = apr_hash_get(module->vars, args[0].string->buf, args[0].string->len);
	    if ( var == NULL )
	    {
		log_debug("var %s doesn't exist", args[0].string->buf);
		retval->defined = FALSE;
		retval->type = NX_VALUE_TYPE_UNKNOWN;
	    }
	    else
	    {
		nx_value_clone(retval, &(var->value));
	    }
	}
    }
    catch(e)
    {
	nx_module_unlock(module);
	rethrow(e);
    }
    nx_module_unlock(module);
}



void nx_expr_proc__create_stat(nx_expr_eval_ctx_t *eval_ctx,
			       nx_module_t *module,
			       nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t argval1, argval2, argval3;
    apr_time_t expiry = 0;
    int64_t interval = 0;
    nx_module_stat_type_t type;
    apr_time_t timeval = 0;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);
    module = eval_ctx->module;

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    nx_expr_evaluate(eval_ctx, &argval1, arg->expr);
    if ( (argval1.defined != TRUE) || (argval1.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval1);
	throw_msg("string required for the first argument of create_stat()");
    }
    if ( argval1.string->len == 0 )
    {
	nx_value_kill(&argval1);
	throw_msg("zero sized string statname argument is invalid for create_stat()");
    }

    arg = NX_DLIST_NEXT(arg, link);
    ASSERT(arg != NULL);
    nx_expr_evaluate(eval_ctx, &argval2, arg->expr);
    if ( (argval2.defined != TRUE) || (argval2.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	throw_msg("string required for second argument of create_stat()");
    }
    type = nx_module_stat_type_from_string(argval2.string->buf);
    if ( type == 0 )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	throw_msg("invalid stat type given for second argument of create_stat()");
    }

    arg = NX_DLIST_NEXT(arg, link);
    if ( arg == NULL )
    {	// no interval parameter
	if ( strcasecmp(argval2.string->buf, "count") != 0 )
	{
	    nx_value_kill(&argval2);
	    nx_value_kill(&argval1);
	    throw_msg("create_stat(name, type) can only be used with \"COUNT\", "
		      "for other types the interval parameter is required");
	}
	nx_module_lock(module);
	nx_module_stat_create(module, type, argval1.string->buf, argval1.string->len,
			      interval, timeval, expiry);
	nx_module_unlock(module);
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	return;
    }

    nx_expr_evaluate(eval_ctx, &argval3, arg->expr);
    if ( (argval3.defined != TRUE) || (argval3.type != NX_VALUE_TYPE_INTEGER) )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	nx_value_kill(&argval3);
	throw_msg("integer type required for third argument of create_stat()");
    }
    interval = argval3.integer;
    if ( interval <= 0 )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	throw_msg("interval passed to create_stat() must be greater than zero");
    }

    arg = NX_DLIST_NEXT(arg, link);
    if ( arg == NULL )
    {	// no time parameter
	nx_module_lock(module);
	nx_module_stat_create(module, type, argval1.string->buf, argval1.string->len,
			      interval, timeval, expiry);
	nx_module_unlock(module);
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	return;
    }
    nx_expr_evaluate(eval_ctx, &argval3, arg->expr);
    if ( argval3.defined != TRUE )
    { // treat undef as 0
    }
    else if ( argval3.type != NX_VALUE_TYPE_DATETIME )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	nx_value_kill(&argval3);
	throw_msg("datetime type required for time argument of create_stat()");
    }
    timeval = argval3.datetime;
    
    arg = NX_DLIST_NEXT(arg, link);
    if ( arg == NULL )
    {	// no lifetime parameter
	nx_module_lock(module);
	nx_module_stat_create(module, type, argval1.string->buf, argval1.string->len,
			      interval, timeval, expiry);
	nx_module_unlock(module);
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	return;
    }

    nx_expr_evaluate(eval_ctx, &argval3, arg->expr);
    if ( (argval3.defined != TRUE) ||
	 !((argval3.type == NX_VALUE_TYPE_INTEGER) ||
	   (argval3.type == NX_VALUE_TYPE_DATETIME)) )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	nx_value_kill(&argval3);
	throw_msg("integer or datetime required for lifetime/expiry argument of create_stat()");
    }
    if ( argval3.type == NX_VALUE_TYPE_INTEGER )
    {
	expiry = apr_time_now() + argval3.integer * APR_USEC_PER_SEC;
    }
    else if ( argval3.type == NX_VALUE_TYPE_DATETIME )
    {
	if ( argval3.datetime < apr_time_now() )
	{
	    log_warn("expiry given for create_stat('%s') is in the past, created stat will never expire",
		     argval1.string->buf);
	}
	else
	{
	    expiry = argval3.datetime;
	}
    }

    nx_module_lock(module);
    nx_module_stat_create(module, type, argval1.string->buf,
			  argval1.string->len, interval, timeval, expiry);
    nx_module_unlock(module);
    nx_value_kill(&argval1);
    nx_value_kill(&argval2);
    nx_value_kill(&argval3);
}



void nx_expr_proc__add_stat(nx_expr_eval_ctx_t *eval_ctx,
			    nx_module_t *module,
			    nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg1;
    nx_expr_list_elem_t *arg2;
    nx_expr_list_elem_t *arg3;
    nx_value_t argval1, argval2;
    nx_module_stat_t *stat;
    apr_time_t timeval = 0;
    int64_t val;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);
    module = eval_ctx->module;

    arg1 = NX_DLIST_FIRST(args);
    ASSERT(arg1 != NULL);
    arg2 = NX_DLIST_NEXT(arg1, link);
    ASSERT(arg2 != NULL);

    nx_expr_evaluate(eval_ctx, &argval1, arg1->expr);
    if ( (argval1.defined != TRUE) || (argval1.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval1);
	throw_msg("string argument required for add_stat(statname, value)");
    }

    if ( module->stats == NULL )
    {
	nx_value_kill(&argval1);
	throw_msg("no stats exist, cannot call add_stat() on nonexistent stat");
    }
    
    nx_module_lock(module);
    stat = apr_hash_get(module->stats, argval1.string->buf, argval1.string->len);
    nx_module_unlock(module);
    if ( stat == NULL )
    {
	nx_value_kill(&argval1);
	throw_msg("cannot call add_stat() on nonexistent stat");
    }
    nx_value_kill(&argval1);

    nx_expr_evaluate(eval_ctx, &argval2, arg2->expr);
    if ( (argval2.defined != TRUE) || (argval2.type != NX_VALUE_TYPE_INTEGER) )
    {
	nx_value_kill(&argval2);
	throw_msg("integer required for second argument of add_stat()");
    }
    val = argval2.integer;
	
    arg3 = NX_DLIST_NEXT(arg2, link);
    if ( arg3 != NULL )
    {
	nx_expr_evaluate(eval_ctx, &argval2, arg3->expr);
	if ( (argval2.defined != TRUE) || (argval2.type != NX_VALUE_TYPE_DATETIME) )
	{
	    nx_value_kill(&argval2);
	    throw_msg("datetime required for third argument of add_stat()");
	}
	timeval = argval2.datetime;
    }
    if ( timeval == 0 )
    {
	timeval = apr_time_now();
    }

    nx_module_lock(module);
    nx_module_stat_add(stat, val, timeval);
    nx_module_unlock(module);
}



void nx_expr_func__get_stat(nx_expr_eval_ctx_t *eval_ctx,
			    nx_module_t *module UNUSED,
			    nx_value_t *retval,
			    int32_t num_arg,
			    nx_value_t *args)
{
    apr_time_t volatile timeval = 0;
    nx_module_stat_t *stat;
    nx_exception_t e;

    ASSERT(retval != NULL);
    ASSERT(num_arg >= 1);
    ASSERT(eval_ctx->module != NULL);

    if ( (args[0].defined != TRUE) || (args[0].type != NX_VALUE_TYPE_STRING) )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }
    if ( num_arg == 2 )
    {
	if ( args[1].defined == FALSE )
	{
	}
	else if ( args[1].type != NX_VALUE_TYPE_DATETIME )
	{
	    throw_msg("got '%s' argument, but datetime is required for function 'get_stat(string, datetime)'",
		      nx_value_type_to_string(args[1].type));
	}
	else
	{
	    timeval = args[1].datetime;
	}
    }

    nx_module_lock(eval_ctx->module);
    try
    {
	if ( eval_ctx->module->stats == NULL )
	{
	    //log_debug("get_stat(): no module stats, stat '%s' doesn't exist", args[0].string->buf);
	    retval->defined = FALSE;
	    retval->type = NX_VALUE_TYPE_UNKNOWN;
	}
	else
	{
	    stat = apr_hash_get(eval_ctx->module->stats, args[0].string->buf, args[0].string->len);
	    if ( stat == NULL )
	    {
		retval->defined = FALSE;
		retval->type = NX_VALUE_TYPE_UNKNOWN;
	    }
	    else
	    {
		if ( timeval == 0 )
		{
		    timeval = apr_time_now();
		}
		nx_module_stat_update(stat, timeval);
		nx_value_clone(retval, &(stat->value));
	    }
	}
    }
    catch(e)
    {
	nx_module_unlock(eval_ctx->module);
	rethrow(e);
    }
    nx_module_unlock(eval_ctx->module);
}



void nx_expr_proc__sleep(nx_expr_eval_ctx_t *eval_ctx,
			      nx_module_t *module UNUSED,
			      nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t argval;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    nx_expr_evaluate(eval_ctx, &argval, arg->expr);
    if ( (argval.defined != TRUE) || (argval.type != NX_VALUE_TYPE_INTEGER) )
    {
	nx_value_kill(&argval);
	throw_msg("integer argument required for sleep(interval)");
    }

    apr_sleep(argval.integer);
}



void nx_expr_proc__rename_field(nx_expr_eval_ctx_t *eval_ctx,
				nx_module_t *module UNUSED,
				nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t argval1, argval2;
    nx_exception_t e;

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to rename_field(), already dropped?");
    }

    ASSERT(args != NULL);

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    nx_expr_evaluate(eval_ctx, &argval1, arg->expr);
    if ( (argval1.defined != TRUE) || (argval1.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval1);
	throw_msg("string required for first argument of rename_field()");
    }
    if ( argval1.string->len == 0 )
    {
	nx_value_kill(&argval1);
	throw_msg("zero sized string invalid for first arg of rename_field()");
    }

    arg = NX_DLIST_NEXT(arg, link);
    ASSERT(arg != NULL);
    nx_expr_evaluate(eval_ctx, &argval2, arg->expr);
    if ( (argval2.defined != TRUE) || (argval2.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	throw_msg("string required for second argument of rename_field()");
    }
    if ( argval2.string->len == 0 )
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	throw_msg("zero sized string is invalid for second arg of rename_field()");
    }

    try
    {
	nx_logdata_rename_field(eval_ctx->logdata, argval1.string->buf, argval2.string->buf);
    }
    catch(e)
    {
	nx_value_kill(&argval1);
	nx_value_kill(&argval2);
	rethrow(e);
    }

    nx_value_kill(&argval1);
    nx_value_kill(&argval2);
}



void nx_expr_proc__reroute(nx_expr_eval_ctx_t *eval_ctx,
			   nx_module_t *module,
			   nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t routeval;
    nx_logdata_t *logdata;
    nx_ctx_t *ctx;
    nx_route_t *route;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);
    module = eval_ctx->module;

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to reroute(), already dropped?");
    }
    logdata = eval_ctx->logdata;

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);

    nx_expr_evaluate(eval_ctx, &routeval, arg->expr);
    if ( (routeval.defined != TRUE) || (routeval.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&routeval);
	throw_msg("string argument required for reroute(routename)");
    }

    ctx = nx_ctx_get();
    for ( route = NX_DLIST_FIRST(ctx->routes);
	  route != NULL;
	  route = NX_DLIST_NEXT(route, link) )
    {
	if ( strcmp(route->name, routeval.string->buf) == 0 )
	{
	    break;
	}
    }

    if ( route == NULL )
    {
	char tmpstr[200];

	apr_cpystrn(tmpstr, routeval.string->buf, sizeof(tmpstr));
	nx_value_kill(&routeval);
	throw_msg("invalid route name specified for reroute(): '%s'", tmpstr);
    }

    nx_value_kill(&routeval);
    nx_module_add_logdata_to_route(module, route, logdata);

    // must not free it here because it is still linked in the queue
    eval_ctx->logdata = NULL;
}



void nx_expr_proc__add_to_route(nx_expr_eval_ctx_t *eval_ctx,
				nx_module_t *module,
				nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t routeval;
    nx_logdata_t *logdata;
    nx_ctx_t *ctx;
    nx_route_t *route;

    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);
    module = eval_ctx->module;

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to add_to_route(), already dropped?");
    }
    logdata = eval_ctx->logdata;

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);

    nx_expr_evaluate(eval_ctx, &routeval, arg->expr);
    if ( (routeval.defined != TRUE) || (routeval.type != NX_VALUE_TYPE_STRING) )
    {
	nx_value_kill(&routeval);
	throw_msg("string argument required for add_to_route(routename)");
    }
    ctx = nx_ctx_get();
    for ( route = NX_DLIST_FIRST(ctx->routes);
	  route != NULL;
	  route = NX_DLIST_NEXT(route, link) )
    {
	if ( strcmp(route->name, routeval.string->buf) == 0 )
	{
	    break;
	}
    }

    if ( route == NULL )
    {
	char tmpstr[200];

	apr_cpystrn(tmpstr, routeval.string->buf, sizeof(tmpstr));
	nx_value_kill(&routeval);

	throw_msg("invalid route name specified for add_to_route(): '%s'", tmpstr);
    }
    nx_value_kill(&routeval);

    nx_module_lock(module);
    (module->evt_fwd)++;
    nx_module_unlock(module);

    nx_module_add_logdata_to_route(module, route, logdata);
}



void nx_expr_func__to_ip4addr(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			      nx_module_t *module UNUSED,
			      nx_value_t *retval,
			      int32_t num_arg,
			      nx_value_t *args)
{
    boolean ntoa = FALSE;

    ASSERT(retval != NULL);
    ASSERT((num_arg == 1) || (num_arg == 2));

    if ( args[0].type != NX_VALUE_TYPE_INTEGER )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_IP4ADDR;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    if ( num_arg == 2 )
    {
	if ( args[1].type != NX_VALUE_TYPE_BOOLEAN )
	{
	    throw_msg("'%s' type argument is invalid",
		      nx_value_type_to_string(args[0].type));
	}
	if ( args[1].defined == TRUE )
	{
	    ntoa = args[1].boolean;
	}
    }

    if ( ntoa == FALSE )
    {
	retval->ip4addr[0] = (uint8_t) ((args[0].integer / (256 * 256 * 256)) % 256);
	retval->ip4addr[1] = (uint8_t) ((args[0].integer / (256 * 256)) % 256);
	retval->ip4addr[2] = (uint8_t) ((args[0].integer / 256) % 256);
	retval->ip4addr[3] = (uint8_t) (args[0].integer % 256);
    }
    else
    {
	retval->ip4addr[3] = (uint8_t) ((args[0].integer / (256 * 256 * 256)) % 256);
	retval->ip4addr[2] = (uint8_t) ((args[0].integer / (256 * 256)) % 256);
	retval->ip4addr[1] = (uint8_t) ((args[0].integer / 256) % 256);
	retval->ip4addr[0] = (uint8_t) (args[0].integer % 256);
    }
    retval->defined = TRUE;
}



void nx_expr_func__substr(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			  nx_module_t *module UNUSED,
			  nx_value_t *retval,
			  int32_t num_arg,
			  nx_value_t *args)
{
    int64_t from, to = 0;

    ASSERT(retval != NULL);
    ASSERT((num_arg == 2) || (num_arg == 3));

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid for first argument, string required",
		  nx_value_type_to_string(args[0].type));
    }
    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	return;
    }
    if ( args[1].type != NX_VALUE_TYPE_INTEGER )
    {
	throw_msg("'%s' type argument is invalid for second argument, integer required",
		  nx_value_type_to_string(args[1].type));
    }
    if ( args[1].defined == FALSE )
    {
	return;
    }
    from = args[1].integer;
    if ( from < 0 )
    {
	throw_msg("negative integer invalid for second argument");
    }
    if (args[0].string->len < from )
    {
	return;
    }

    if ( num_arg == 3 )
    {
	if ( args[2].type != NX_VALUE_TYPE_INTEGER )
	{
	    throw_msg("'%s' type argument is invalid for third argument, integer required",
		      nx_value_type_to_string(args[2].type));
	}
	if ( args[2].defined == TRUE )
	{
	    to = args[2].integer;
	    if ( to < 0 )
	    {
		throw_msg("negative integer invalid for third argument");
	    }

	    if ( from > to )
	    {
		return;
	    }
	    if ( to > args[0].string->len )
	    {
		to = args[0].string->len;
	    }
	}
    }
    else
    {
	to = args[0].string->len;
    }

    ASSERT(to >= from);
    ASSERT(from <= args[0].string->len);
    retval->string = nx_string_create(args[0].string->buf + from, (int) (to - from));
    retval->defined = TRUE;
}


void nx_expr_func__str_size(nx_expr_eval_ctx_t *eval_ctx UNUSED,
			    nx_module_t *module UNUSED,
			    nx_value_t *retval,
			    int32_t num_arg,
			    nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid for first argument, string required",
		  nx_value_type_to_string(args[0].type));
    }
    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	return;
    }

    retval->integer = args[0].string->len;
    retval->defined = TRUE;
}



void nx_expr_func__dropped(nx_expr_eval_ctx_t *eval_ctx,
			   nx_module_t *module UNUSED,
			   nx_value_t *retval,
			   int32_t num_arg,
			   nx_value_t *args UNUSED)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    retval->type = NX_VALUE_TYPE_BOOLEAN;
    retval->defined = TRUE;
    if ( eval_ctx->logdata == NULL )
    {
	retval->boolean = TRUE;
    }
    else
    {
	retval->boolean = FALSE;
    }
}
