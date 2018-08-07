/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_INTERNAL_H
#define __NX_IM_INTERNAL_H

#include "../../../common/types.h"

typedef struct nx_internal_log_t
{
    const char 		*msg;
    nx_loglevel_t	level;
    nx_logmodule_t	module;
    apr_status_t	errorcode;
} nx_internal_log_t;


typedef struct nx_im_internal_conf_t
{
    const char *string;
    int		pid;
} nx_im_internal_conf_t;



#endif	/* __NX_IM_INTERNAL_H */
