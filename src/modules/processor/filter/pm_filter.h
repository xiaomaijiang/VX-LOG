/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_PM_FILTER_H
#define __NX_PM_FILTER_H

#include "../../../common/types.h"

typedef struct nx_pm_filter_conf_t
{
    nx_expr_t *condition;
} nx_pm_filter_conf_t;



#endif	/* __NX_PM_FILTER_H */
