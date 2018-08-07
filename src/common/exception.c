/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "exception.h"
#include "backtrace.h"
#include "../core/ctx.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static void nx_exception_add_message(nx_exception_t	*e,
				     const char		*file,
				     int		line,
				     const char		*func,
				     const char		*fmt,
				     va_list		ap)
{
    apr_size_t n, bufsize;
    char *buf;

    if ( e->num_throw >= NX_EXCEPTION_THROWLIST_SIZE )
    {
	e->num_throw = NX_EXCEPTION_THROWLIST_SIZE - 1;
	if ( e->throwlist[e->num_throw].msgoffs >= 0 )
	{
	    e->msglen = (apr_size_t) e->throwlist[e->num_throw].msgoffs;
	}
    }

    e->throwlist[e->num_throw].file = file;
    e->throwlist[e->num_throw].line = line;
    e->throwlist[e->num_throw].func = func;
    e->throwlist[e->num_throw].msgoffs	= -1;
    e->num_throw++;

    if ( fmt == NULL )
    {
	return;
    }

    if ( e->msglen + 1 >= (int32_t) sizeof(e->msgbuf) )
    {
	return;
    }

    buf = e->msgbuf + e->msglen;
    bufsize = sizeof(e->msgbuf) - e->msglen;
    n = (apr_size_t) apr_vsnprintf((char *) buf, bufsize, fmt, ap);
    ASSERT(n != 0);

    e->throwlist[e->num_throw - 1].msgoffs = (int32_t) e->msglen;

    if ( n < bufsize )
    {
	e->msglen += n + 1;
    }
    else
    {
	e->msglen = sizeof(e->msgbuf);
    }
}



void nx_exception_init(nx_exception_t	*e,
		       const char	*caused_by,
		       const char	*file,
		       int		line,
		       const char	*func,
		       apr_status_t	code,
		       const char	*fmt,
		       ...)
{
    va_list ap;
    int32_t i;

    ASSERT(e != NULL);
   
    memset(e, 0, sizeof(nx_exception_t));
    for ( i = 0; i < NX_EXCEPTION_THROWLIST_SIZE; i++ )
    {
	e->throwlist[i].msgoffs	= -1;
    }
    e->code = code;

    if ( caused_by == NULL )
    {
	(e->caused_by)[0] = '\0';
    }
    else
    {
	apr_cpystrn(e->caused_by, caused_by, sizeof(e->caused_by));
    }
    
    if ( fmt != NULL )
    {
	va_start(ap, fmt);
	nx_exception_add_message(e, file, line, func, fmt, ap);
	va_end(ap);
    }
    else
    {
	va_start(ap, fmt);
	nx_exception_add_message(e, file, line, func, NULL, ap);
	va_end(ap);
    }
}



void nx_exception_rethrow(nx_exception_t	*e,
			   const char		*file,
			   int			line,
			   const char		*func,
			   const char		*fmt,
			   ...)
{
    va_list ap;

    ASSERT(e != NULL);

    va_start(ap, fmt);
    nx_exception_add_message(e, file, line, func, fmt, ap);
    va_end(ap);

    nx_exception_check_uncaught(e, NX_LOGMODULE);
    
    Throw(*e);
}



const char *nx_exception_get_message(const nx_exception_t *e, apr_size_t idx)
{
    ASSERT(e != NULL);
    ASSERT(idx < NX_EXCEPTION_THROWLIST_SIZE);

    return ( (e->throwlist[idx].msgoffs >= 0) ? (const char *)(e->msgbuf + e->throwlist[idx].msgoffs) : NULL );
}



void nx_log_exception(nx_logmodule_t		logmodule,
		      const nx_exception_t	*e,
		      const char		*fmt,
		      ...)
{
    int i;
    char errmsg[1024];
    nx_loglevel_t loglevel = NX_LOGLEVEL_ERROR;
    nx_ctx_t *ctx;
    nx_string_t *tmpstr;
    boolean empty = TRUE;
    int size;

    ASSERT(e != NULL);
    ASSERT(e->num_throw > 0);

    ctx = nx_ctx_get();
    if ( ctx != NULL )
    {
	loglevel = ctx->loglevel;
    }

    tmpstr = nx_string_new();

    if ( fmt != NULL )
    {
	va_list ap;

	va_start(ap, fmt);
	size = apr_vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
	va_end(ap);
	nx_string_append(tmpstr, errmsg, size);
	empty = FALSE;
    }

    if ( loglevel == NX_LOGLEVEL_DEBUG )
    {
        if ( (e->caused_by)[0] != '\0' )
	{
	    ASSERT(e->num_throw > 0);
	    i = (int) (e->num_throw) - 1;
	    if ( empty != TRUE )
	    {
		nx_string_append(tmpstr, NX_LINEFEED, -1);
	    }
	    size = apr_snprintf(errmsg, sizeof(errmsg), 
				"Exception was caused by \"%s\" at %s:%d/%s()", 
				e->caused_by, e->throwlist[i].file,
				e->throwlist[i].line, e->throwlist[i].func);
	    nx_string_append(tmpstr, errmsg, size);
	    empty = FALSE;
	}
    }
    for ( i = ((int) e->num_throw) - 1; i >= 0; i-- )
    {
	if ( loglevel == NX_LOGLEVEL_DEBUG )
	{
	    if ( e->throwlist[i].msgoffs < 0 )
	    {
		if ( empty != TRUE )
		{
		    nx_string_append(tmpstr, NX_LINEFEED, -1);
		}
		size = apr_snprintf(errmsg, sizeof(errmsg), 
				    "[%s:%d/%s()] -",
				    e->throwlist[i].file, e->throwlist[i].line,
				    e->throwlist[i].func);
		nx_string_append(tmpstr, errmsg, size);
		empty = FALSE;
	    }
	    else
	    {
		if ( empty != TRUE )
		{
		    nx_string_append(tmpstr, NX_LINEFEED, -1);
		}
		size = apr_snprintf(errmsg, sizeof(errmsg), 
				    "[%s:%d/%s()] %s",
				    e->throwlist[i].file, e->throwlist[i].line,
				    e->throwlist[i].func, e->msgbuf + e->throwlist[i].msgoffs);
		nx_string_append(tmpstr, errmsg, size);
		empty = FALSE;
	    }
	}
	else
	{
	    if ( e->throwlist[i].msgoffs < 0 )
	    {
	    }
	    else
	    {
		if ( empty != TRUE )
		{
		    nx_string_append(tmpstr, NX_LINEFEED, -1);
		}
		nx_string_append(tmpstr, e->msgbuf + e->throwlist[i].msgoffs, -1);
		empty = FALSE;
	    }
	}
    }

    if ( e->code != APR_SUCCESS )
    {
	apr_strerror(e->code, errmsg, sizeof(errmsg));
	if ( empty != TRUE )
	{
	    nx_string_append(tmpstr, NX_LINEFEED, -1);
	}
	nx_string_append(tmpstr, errmsg, -1);
	empty = FALSE;
    }

    nx_log(e->code, NX_LOGLEVEL_ERROR, logmodule, "%s", tmpstr->buf);
    nx_string_free(tmpstr);
}
	       


