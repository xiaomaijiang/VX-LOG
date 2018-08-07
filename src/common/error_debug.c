/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <stdlib.h>
#include "../core/nxlog.h"
#include "../modules/input/internal/im_internal.h"
#include "error_debug.h"
#include "exception.h"
#include "backtrace.h"
#include "date.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static boolean _foreground = TRUE;

static nx_keyval_t logmodules[] =
{
    { NX_LOGMODULE_NONE, "none" },
    { NX_LOGMODULE_CORE, "core" },
    { NX_LOGMODULE_SSL, "ssl" },
    { NX_LOGMODULE_MODULE, "module" },
    { -1, NULL },
};


static nx_keyval_t loglevels[] =
{
    { NX_LOGLEVEL_DEBUG, "DEBUG" },
    { NX_LOGLEVEL_INFO, "INFO" },
    { NX_LOGLEVEL_WARNING, "WARNING" },
    { NX_LOGLEVEL_ERROR, "ERROR" },
    { NX_LOGLEVEL_CRITICAL, "CRITICAL" },
    { -1, NULL },
};



void nx_logger_disable_foreground()
{
    _foreground = FALSE;
}


static apr_thread_mutex_t *_logger_mutex = NULL;


void nx_logger_mutex_set(apr_thread_mutex_t *mutex)
{
    ASSERT(mutex != NULL);

    _logger_mutex = mutex;
}



const char *nx_logmodule_to_string(nx_logmodule_t module)
{
    unsigned int i = 0;

    while ( logmodules[i].value != NULL )
    {
	if ( logmodules[i].key == (int) module )
	{
	    return ( logmodules[i].value );
	}
    }

    return ( "INVALID" );
}




const char *nx_loglevel_to_string(nx_loglevel_t type)
{
    switch ( type )
    {
	case NX_LOGLEVEL_DEBUG:
	    return "DEBUG";
	case NX_LOGLEVEL_INFO:
	    return "INFO";
	case NX_LOGLEVEL_WARNING:
	    return "WARNING";
	case NX_LOGLEVEL_ERROR:
	    return "ERROR";
	case NX_LOGLEVEL_CRITICAL:
	    return "CRITICAL";
	default:
	    break;
    }

    return ( "UNKNOWN" );
}



nx_loglevel_t nx_loglevel_from_string(const char *str)
{
    int i;

    for ( i = 0; loglevels[i].value != NULL; i++ )
    {
	if ( strcasecmp(loglevels[i].value, str) == 0 )
	{
	    return ( loglevels[i].key );
	}
    }
    return ( 0 );
}



void nx_assertion_failed(nx_logmodule_t	logmodule,
			 const char	*file,	///< file
			 int		line,	///< line
			 const char	*func,	///< function
			 const char	*exprstr)
{
    nx_ctx_t *ctx;
    nx_exception_t e;

    ctx = nx_ctx_get();
    if ( ctx == NULL )
    {
	_nx_abort(file, line, func, logmodule, "### ASSERTION FAILED: \"%s\" ###", exprstr);
    }
    else
    {
	if ( ctx->panic_level == NX_PANIC_LEVEL_HARD )
	{
	    _nx_abort(file, line, func, logmodule, "### ASSERTION FAILED: \"%s\" ###", exprstr);
	}
	else if ( ctx->panic_level == NX_PANIC_LEVEL_SOFT )
	{
	    if ( the_exception_context->penv == NULL )
	    {
		_nx_abort(file, line, func, logmodule,
			  "### ASSERTION FAILED: \"%s\" ### (aborted due to uncaught exception).", exprstr);
	    }
	    nx_exception_init(&e, NULL, file, line, func, APR_SUCCESS,
			      "### ASSERTION FAILED at line %d in %s/%s(): \"%s\" ###",
			      line, file, func, exprstr);
	    Throw(e);
	}
	else if ( ctx->panic_level == NX_PANIC_LEVEL_OFF )
	{
	    nx_log(APR_SUCCESS, NX_LOGLEVEL_ERROR, logmodule, 
		   "### ASSERTION FAILED at line %d in %s/%s(): \"%s\" ### (ignored)",
		   line, file, func, exprstr);
	}
    }
}



static void write_log(const char	*message,
		      apr_size_t	bytes,
		      nx_loglevel_t	loglevel,
		      apr_file_t	*logfile)
{
    apr_status_t rv;

    if ( _foreground == TRUE )
    {
	if ( loglevel > NX_LOGLEVEL_INFO )
	{
	    fprintf(stderr, "%s", message);
	}
	else
	{
	    printf("%s", message);
	}
    }

    if ( logfile != NULL )
    {
		
	if ( (rv = apr_file_write_full(logfile, message, bytes, NULL)) != APR_SUCCESS )
	{
	    if ( _foreground == TRUE )
	    {
		char errmsg[200];

		apr_strerror(rv, errmsg, sizeof(errmsg));
		fprintf(stderr, "couldn't write LogFile: %s\n", errmsg);
	    }
	}
    }
}



void nx_log(apr_status_t	code,
	    nx_loglevel_t	loglevel,
	    nx_logmodule_t	logmodule,	///< module
	    const char		*fmt,		///< message formatter
	    ...)
{
    nx_ctx_t *ctx;
    nx_internal_log_t log;

    ctx = nx_ctx_get();
    if ( ctx == NULL )
    {
	char message[NX_LOGBUF_SIZE];
	va_list ap;

	va_start(ap, fmt);
	apr_vsnprintf(message, NX_LOGBUF_SIZE, fmt, ap);
	va_end(ap);

	if ( loglevel > NX_LOGLEVEL_INFO )
	{
	    fprintf(stderr, "%s\n", message);
	}
	else
	{
	    printf("%s\n", message);
	}
	return;
    }

    if ( ((_foreground == TRUE) && (loglevel >= ctx->loglevel)) ||
	 (loglevel >= NX_LOGLEVEL_INFO) ||
	 (ctx->logfile != NULL) )
    {
	if ( loglevel >= NX_LOGLEVEL_INFO )
	{ // send to im_internal 
	    nx_module_t *module;
	    char message[NX_LOGBUF_SIZE];
	    va_list ap;

	    va_start(ap, fmt);
	    apr_vsnprintf(message, NX_LOGBUF_SIZE, fmt, ap);
	    va_end(ap);

	    log.msg = message;
	    log.level = loglevel;
	    log.module = logmodule;
	    log.errorcode = code;

	    // FIXME: race condition, module list is modified in nx_ctx_shutdown_modules
	    for ( module = NX_DLIST_FIRST(ctx->modules);
		  module != NULL;
		  module = NX_DLIST_NEXT(module, link) )
	    {
		if ( strcmp(module->dsoname, "im_internal") == 0 )
		{
		    module->decl->event(module, (void *) &log);
		}
	    }
	}

	if ( (loglevel >= ctx->loglevel) && 
	     ((_foreground == TRUE) || (ctx->logfile != NULL)) )
	{
	    char message[NX_LOGBUF_SIZE];
	    va_list ap;
	    apr_size_t offs = 0;
	    apr_size_t msgoffs = 0;
	    const char *loglevelstr;
	    int i;
	    apr_time_t now;

	    now = apr_time_now();

	    if ( ctx->formatlog == TRUE )
	    {
		nx_date_to_iso(message, sizeof(message), now);
		message[19] = ' ';
		offs = 20;
		loglevelstr = nx_loglevel_to_string(loglevel);
		for ( i = 0; loglevelstr[i] != '\0'; i++ )
		{
		    message[offs] = loglevelstr[i];
		    offs++;
		    ASSERT(offs < NX_LOGBUF_SIZE);
		}
		message[offs] = ' ';
		offs++;
	    }
	    msgoffs = offs;
	    va_start(ap, fmt);
	    offs += (apr_size_t) apr_vsnprintf(message + offs, NX_LOGBUF_SIZE - offs, fmt, ap);
	    va_end(ap);

	    // We don't want multi line entries in nxlog.log
	    for ( i = 0; i < NX_LOGBUF_SIZE; i++ )
	    {
		if ( message[i] == '\0' )
		{
		    break;
		}
		if ( (message[i] == '\r') && (message[i + 1] == '\n') )
		{
		    message[i] = ';';
		    message[i + 1] = ' ';
		}
		if ( (message[i] == '\n') || (message[i] == '\r') )
		{
		    message[i] = ';';
		}
	    }
    
#ifdef WIN32
	    if ( offs >= NX_LOGBUF_SIZE - 2 )
	    {
		offs = NX_LOGBUF_SIZE - 3;
	    }
	    message[offs] = '\r';
	    offs++;
	    message[offs] = '\n';
	    offs++;
#else 
	    if ( offs >= NX_LOGBUF_SIZE - 1 )
	    {
		offs = NX_LOGBUF_SIZE - 2;
	    }
	    message[offs] = '\n';
	    offs++;
#endif
	    message[offs] = '\0';

	    if ( (ctx->norepeat == TRUE) && (loglevel >= NX_LOGLEVEL_INFO) )
	    {
		//ASSERT(apr_thread_mutex_lock(_logger_mutex) == APR_SUCCESS);
		if ( strcmp(ctx->norepeat_buf + ctx->norepeat_offs, message + msgoffs) == 0 )
		{
		    (ctx->norepeat_cnt)++;
		    if ( ctx->norepeat_time == 0 )
		    {
			ctx->norepeat_time = now;
		    }
		    if ( ctx->norepeat_time + APR_USEC_PER_SEC * 3 <= now )
		    {
			ctx->norepeat_time = now;
			if ( ctx->norepeat_cnt > 1 )
			{
			    msgoffs += (apr_size_t) apr_snprintf(message + msgoffs, NX_LOGBUF_SIZE - msgoffs,
								 "last message repeated %d times"NX_LINEFEED,
								 ctx->norepeat_cnt);
			    offs = msgoffs;
			    ctx->norepeat_cnt = -1;
			}
		        else
			{ //print original
			    ctx->norepeat_cnt = 0;
			}
		    }
		    else
		    {
			if ( ctx->norepeat_cnt > 0 )
			{
			    return; //we suppress here
			}

			// TODO: add an event to flush logs after 1 sec , see pm_norepeat
		    }
		}
		else
		{ // we have a different log message
		    ctx->norepeat_offs = msgoffs;
		    if ( ctx->norepeat_cnt > 1 )
		    {
			char tmpmsg[NX_LOGBUF_SIZE];

			if ( ctx->formatlog == TRUE )
			{
			    nx_date_to_iso(tmpmsg, sizeof(tmpmsg), now);
			    msgoffs = strlen(tmpmsg);
			    msgoffs += (apr_size_t) apr_snprintf(tmpmsg + msgoffs, NX_LOGBUF_SIZE - msgoffs,
								 " %s last message repeated %d times"NX_LINEFEED,
								 nx_loglevel_to_string(ctx->norepeat_loglevel),
								 ctx->norepeat_cnt);
			}
			else
			{
			    msgoffs = (apr_size_t) apr_snprintf(tmpmsg, NX_LOGBUF_SIZE,
								"last message repeated %d times"NX_LINEFEED,
								ctx->norepeat_cnt);
			}
			write_log(tmpmsg, msgoffs, NX_LOGLEVEL_INFO, ctx->logfile);
		    }
		    else if ( ctx->norepeat_cnt == 1 )
		    {
			write_log(ctx->norepeat_buf, strlen(ctx->norepeat_buf), NX_LOGLEVEL_INFO, ctx->logfile);
		    }

		    ctx->norepeat_cnt = 0;
		    ctx->norepeat_time = now;
		    memcpy(ctx->norepeat_buf, message, NX_LOGBUF_SIZE);
		    ctx->norepeat_loglevel = loglevel;
		}
		//ASSERT(apr_thread_mutex_unlock(_logger_mutex) == APR_SUCCESS);
	    }

	    write_log(message, offs, loglevel, ctx->logfile);
	}
    }
}



void nx_log_aprerror(apr_status_t	rv,
		     nx_loglevel_t	loglevel,
		     nx_logmodule_t	module,	///< module
		     const char		*fmt,	///< message formatter
		     ...)
{
    char errmsg[200];
    char buf[NX_LOGBUF_SIZE];
    va_list ap;

    apr_strerror(rv, errmsg, sizeof(errmsg));
    va_start(ap, fmt);
    apr_vsnprintf(buf, NX_LOGBUF_SIZE, fmt, ap);
    va_end(ap);
    if ( loglevel >= NX_LOGLEVEL_INFO )
    {
	nx_log(rv, loglevel, module, "%s: %s", buf, errmsg);
    }
    else
    {
	nx_log(rv, loglevel, module, "%s: %s (%u)", buf, errmsg, (unsigned int) rv);
    }
}



void _nx_panic(const char	*file,	///< file
	       int		line,	///< line
	       const char	*func,	///< function
	       nx_logmodule_t	module,	///< module
	       const char	*fmt,	///< message formatter
	       ...)
{
    static boolean already_in_panic = FALSE;
    char buf[NX_LOGBUF_SIZE];
    int buflen;
    va_list ap;
    nx_panic_level_t panic_level = NX_PANIC_LEVEL_HARD;
    nx_ctx_t *ctx;
    nx_string_t *backtrace;

    if ( already_in_panic == TRUE )
    {
	return;
    }

    already_in_panic = TRUE;
    ctx = nx_ctx_get();
    if ( ctx != NULL )
    {
	panic_level = ctx->panic_level;
    }

    va_start(ap, fmt);
    buflen = apr_vsnprintf(buf, NX_LOGBUF_SIZE, fmt, ap);
    va_end(ap);

    backtrace = nx_string_new_size(300);
    nx_string_append(backtrace, " ", 1);
    nx_append_backtrace(backtrace);
    apr_cpystrn(buf + buflen, backtrace->buf, (apr_size_t) (NX_LOGBUF_SIZE - buflen));
    nx_string_free(backtrace);

    if ( panic_level == NX_PANIC_LEVEL_HARD )
    {
	_nx_abort(file, line, func, module, "%s", buf);
    }

    if ( panic_level == NX_PANIC_LEVEL_SOFT )
    {
	nx_exception_t e;

	already_in_panic = FALSE;

	if ( the_exception_context->penv == NULL )
	{
	    _nx_abort(file, line, func, module,
		      "%s (aborted due to uncaught exception).", buf);
	}

	nx_exception_init(&e, NULL, file, line, func, APR_SUCCESS,
			  "### PANIC at line %d in %s/%s(): \"%s\" ###",
			  line, file, func, buf);
	Throw(e);
    }
    else
    { // OFF
	nx_log(APR_SUCCESS, NX_LOGLEVEL_CRITICAL, module,
	       "### PANIC at line %d in %s/%s(): \"%s\" ###",
	       line, file, func, buf);
    }
  
    already_in_panic = FALSE;
    fflush(stderr);
    fflush(stdout);
}



void _nx_abort(const char	*file,	///< file
	       int		line,	///< line
	       const char	*func,	///< function
	       nx_logmodule_t	module,	///< module
	       const char	*fmt,	///< message formatter
	       ...)
{
    static boolean already_in_panic = FALSE;
    char buf[NX_LOGBUF_SIZE];
    char msg[NX_LOGBUF_SIZE];
    va_list ap;
#ifdef WIN32
    DWORD dispmode;
#endif

    va_start(ap, fmt);
    apr_vsnprintf(buf, NX_LOGBUF_SIZE, fmt, ap);
    va_end(ap);
    
    apr_snprintf(msg, NX_LOGBUF_SIZE, "### PANIC at line %d in %s/%s(): \"%s\" ###", line, file, func, buf);
#ifdef WIN32
    fprintf(stderr, "%s\n", msg);
    if ( GetConsoleDisplayMode(&dispmode) != 0 )
    {
	MessageBox(NULL, msg, "nxlog aborted",
		   MB_OK | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION);
	apr_sleep(2 * APR_USEC_PER_SEC); // FIXME
    }
#endif
    if ( already_in_panic == FALSE )
    {
	already_in_panic = TRUE;
	nx_log(APR_SUCCESS, NX_LOGLEVEL_CRITICAL, module, "%s", msg);
    }
    fflush(stderr);
    fflush(stdout);

#ifdef WIN32
    {
	struct
	{
	    char *ptr;
	} *dummy = NULL;

	dummy->ptr = NULL; // this is a real ugly hack for forcing a gdb backtrace on windows
	abort();
    }	
#else
    abort();
#endif
}

