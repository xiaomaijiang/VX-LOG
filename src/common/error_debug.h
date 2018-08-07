/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_ERROR_DEBUG_H
#define __NX_ERROR_DEBUG_H

#include "types.h"

#define NX_LOGBUF_SIZE 1024


typedef enum nx_loglevel_t
{
    NX_LOGLEVEL_DEBUG		= 1,
    NX_LOGLEVEL_INFO,
    NX_LOGLEVEL_WARNING,
    NX_LOGLEVEL_ERROR,
    NX_LOGLEVEL_CRITICAL,
} nx_loglevel_t;



typedef enum nx_panic_level_t
{
    NX_PANIC_LEVEL_SOFT	= 0,
    NX_PANIC_LEVEL_HARD,
    NX_PANIC_LEVEL_OFF,
} nx_panic_level_t;



typedef enum nx_logmodule_t
{
    NX_LOGMODULE_NONE	 	= 0,
    NX_LOGMODULE_CONFIG,
    NX_LOGMODULE_CORE,
    NX_LOGMODULE_MODULE,
    NX_LOGMODULE_SSL,
    NX_LOGMODULE_TEST,
} nx_logmodule_t;

#define log_aprerror(rv, fmt, args...) nx_log_aprerror(rv, NX_LOGLEVEL_ERROR, NX_LOGMODULE, fmt , ## args )
#define log_aprwarn(rv, fmt, args...) nx_log_aprerror(rv, NX_LOGLEVEL_WARNING, NX_LOGMODULE, fmt , ## args )
#define log_aprinfo(rv, fmt, args...) nx_log_aprerror(rv, NX_LOGLEVEL_INFO, NX_LOGMODULE, fmt , ## args )
#define log_errno(fmt, args...) nx_log_aprerror(APR_FROM_OS_ERROR(errno), NX_LOGLEVEL_ERROR, NX_LOGMODULE, fmt , ## args )
#define log_error(fmt, args...) nx_log(APR_SUCCESS, NX_LOGLEVEL_ERROR, NX_LOGMODULE, fmt , ## args )
#define log_warn(fmt, args...) nx_log(APR_SUCCESS, NX_LOGLEVEL_WARNING, NX_LOGMODULE, fmt , ## args )
#define log_info(fmt, args...) nx_log(APR_SUCCESS, NX_LOGLEVEL_INFO, NX_LOGMODULE, fmt , ## args )
#define log_debug(fmt, args...) nx_log(APR_SUCCESS, NX_LOGLEVEL_DEBUG, NX_LOGMODULE, fmt , ## args )

#define nx_panic(fmt, args...) _nx_panic(__FILE__, __LINE__, __FUNCTION__, NX_LOGMODULE, fmt , ## args )
#define nx_abort(fmt, args...) _nx_panic(__FILE__, __LINE__, __FUNCTION__, NX_LOGMODULE, fmt , ## args )

void nx_logger_mutex_set(apr_thread_mutex_t *mutex);
const char *nx_loglevel_to_string(nx_loglevel_t type);
nx_loglevel_t nx_loglevel_from_string(const char *str);
const char *nx_logmodule_to_string(nx_logmodule_t module);

void  _nx_panic(const char *file,
		int line,
		const char *func,
		nx_logmodule_t module,
		const char *fmt,
		...) PRINTF_FORMAT(5, 6);

void  _nx_abort(const char *file,
		int line,
		const char *func,
		nx_logmodule_t module,
		const char *fmt,
		...) NORETURN PRINTF_FORMAT(5, 6);

void  nx_log(apr_status_t	code,
	     nx_loglevel_t	loglevel,
	     nx_logmodule_t	module,
	     const char		*fmt,
	     ...) PRINTF_FORMAT(4,5);


void nx_log_aprerror(apr_status_t	rv,
		     nx_loglevel_t	loglevel,
		     nx_logmodule_t	module,
		     const char		*fmt,
		     ...) PRINTF_FORMAT(4,5);

void nx_log_errno(nx_loglevel_t		loglevel,
		  nx_logmodule_t	module,
		  const char		*fmt,
		  ...) PRINTF_FORMAT(3,4);

void nx_assertion_failed(nx_logmodule_t	logmodule,
			 const char	*file,
			 int		line,
			 const char	*func,
			 const char	*expstr);

#define ASSERT(expr) _NX_ASSERT(expr, #expr)

#define _NX_ASSERT(expr, exprstr) 		                                 \
    ({                                                                           \
        if ( !(expr) )                          	                         \
        {                                                                        \
	    nx_assertion_failed(NX_LOGMODULE, __FILE__, __LINE__,                \
                                __FUNCTION__, exprstr);                          \
        }                                                                        \
    })

#endif	/* __NX_ERROR_DEBUG_H */
