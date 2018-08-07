/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_TYPES_H
#define __NX_TYPES_H

#ifdef WIN32
# ifndef WINVER
#  define WINVER 0x0600
# endif
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT WINVER
# endif
# if(_WIN32_WINNT >= 0x0400)
#  include <winsock2.h>
#  include <mswsock.h>
# endif
#endif

#include <apr.h>
#include <apr_strings.h>
#include <apr_time.h>

//#include <openssl/ssl.h>

#include "config.h"


#ifndef WIN32
typedef enum boolean
{
#  ifndef false
    false = 0,
#  endif
#  ifndef true
    true = 1,
#  endif

# ifndef __APPLE__
#  ifndef FALSE
    FALSE = 0,
#  endif
#  ifndef TRUE
    TRUE = 1,
#  endif
# endif

#  ifndef no
    no = 0,
#  endif
#  ifndef yes
    yes = 1,
#  endif

#  ifndef NO
    NO = 0,
#  endif
#  ifndef YES
    YES = 1,
#  endif
} boolean;
#endif


typedef struct nx_keyval_t
{
    int key;
    const char *value;
} nx_keyval_t;


typedef struct nx_module_input_t nx_module_input_t;
typedef struct nx_module_t nx_module_t;
typedef struct nx_event_t nx_event_t;
typedef struct nx_job_t nx_job_t;

#ifdef __GNUC__
# define NORETURN __attribute__ ((noreturn))
# define PRINTF_FORMAT(ARG1, ARG2)  __attribute ((format (printf, ARG1, ARG2)))
# define UNUSED __attribute__ ((unused))
#endif

#ifdef WIN32
# define NX_DIR_SEPARATOR "\\"
# define NX_LINEFEED "\r\n"
#else
# define NX_DIR_SEPARATOR "/"
# define NX_LINEFEED "\n"
#endif

#endif	/* __NX_TYPES_H */
   
