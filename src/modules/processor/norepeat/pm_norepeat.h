/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_PM_NOREPEAT_H
#define __NX_PM_NOREPEAT_H

#include "../../../common/types.h"

typedef struct nx_pm_norepeat_conf_t
{
    nx_event_t		*event;
    int			repeatcnt;
    nx_logdata_t	*logdata;
    int			pid;
    apr_array_header_t	*fields;
} nx_pm_norepeat_conf_t;



#endif	/* __NX_PM_NOREPEAT_H */
