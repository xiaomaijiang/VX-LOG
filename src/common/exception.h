/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_EXCEPTION_H
#define __NX_EXCEPTION_H

#include <apr.h>
#include "cexcept.h"
#include "../../src/common/types.h"
#include "../../src/common/error_debug.h"

#define	NX_EXCEPTION_MSGBUF_SIZE	2048	///< size of the message buffer
#define	NX_EXCEPTION_THROWLIST_SIZE	8	///< number of rethrows possible

struct nx_exception_t
{
    apr_status_t		code;		///< APR error code
    apr_size_t			msglen;		///< actual length of the message in bytes
    char			msgbuf[NX_EXCEPTION_MSGBUF_SIZE]; ///< message buffer
    char			caused_by[128]; ///< code which caused the error
    apr_size_t			num_throw;	///< number of throws in this exception
    struct
    {
	const char		*file;		///< file
	int			line;		///< line
	const char		*func;		///< function
	int32_t			msgoffs;	///< exception text offset in the message buffer
    } throwlist[NX_EXCEPTION_THROWLIST_SIZE];
};

typedef struct nx_exception_t nx_exception_t;

define_exception_type(nx_exception_t);
#define the_exception_context nx_get_exception_context()

struct exception_context *nx_get_exception_context();


void nx_exception_init(nx_exception_t	*e,
		       const char	*caused_by,
		       const char	*file,
		       int		line,
		       const char	*func,
		       apr_status_t	code,
		       const char	*fmt,
		       ...);

void nx_exception_rethrow(nx_exception_t	*e,
			  const char		*file,
			  int			line,
			  const char		*func,
			  const char		*fmt,
			  ...) NORETURN;

const char *nx_exception_get_message(const nx_exception_t 	*e,
				     apr_size_t			idx);

#define log_exception(e) nx_log_exception(NX_LOGMODULE, &(e), NULL)
#define log_exception_msg(e, fmt, args...) nx_log_exception(NX_LOGMODULE, &(e), fmt, ##args)

void nx_log_exception(nx_logmodule_t logmodule,
		      const nx_exception_t *e,
		      const char *fmt,
		      ...) PRINTF_FORMAT(3,4);

#define try Try
#define catch Catch

#define nx_exception_check_uncaught(e, logmodule)		\
    if ( the_exception_context->penv == NULL )			\
    {								\
	nx_log_exception(logmodule, e,				\
                         "FATAL: Uncaught exception.");		\
	_nx_panic(__FILE__, __LINE__, __FUNCTION__, 		\
		  logmodule, "aborting.");			\
    }


#define	throw_cause(status, caused_by, fmt, args...)		\
do {								\
    nx_exception_t _nx_e;					\
								\
    /*log_debug("Throwing exception from %s at %s:%d",*/	\
    /*	      __FUNCTION__, __FILE__, __LINE__);*/		\
    nx_exception_init(&_nx_e, caused_by, __FILE__, __LINE__,	\
		      __FUNCTION__, status, fmt, ##args);	\
    nx_exception_check_uncaught(&(_nx_e), NX_LOGMODULE);	\
    Throw(_nx_e);						\
} while (0)

#define throw(status, fmt, args...) throw_cause(status, NULL, fmt, ##args)
#define throw_msg(fmt, args...) throw(APR_SUCCESS, fmt, ##args)

#define	rethrow(e)						 \
    nx_exception_rethrow(&(e), __FILE__, __LINE__, __FUNCTION__, \
			 NULL)

#define	rethrow_msg(e, fmt, args...)				 \
    nx_exception_rethrow(&(e), __FILE__, __LINE__, __FUNCTION__, \
			 fmt, ##args)

#define	throw_errno(fmt, args...) throw(APR_FROM_OS_ERROR(errno), fmt, ##args)

#define CHECKERR(code)						 \
do {								 \
     apr_status_t _rv = code;					 \
     if ( _rv != APR_SUCCESS )					 \
     {					 			 \
	 throw_cause(_rv, #code, NULL);				 \
     }					 			 \
} while (0)

#define CHECKERR_MSG(code, fmt, args...)                         \
do {								 \
     apr_status_t _rv = code;					 \
     if ( _rv != APR_SUCCESS )					 \
     {					 			 \
	 throw_cause(_rv, #code, fmt, ##args);			 \
     }					 			 \
} while (0)

#endif	/* __NX_EXCEPTION_H */
