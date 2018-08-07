/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_PM_PATTERN_H
#define __NX_PM_PATTERN_H

#include "../../../common/types.h"
#include "patterndb.h"

typedef struct nx_pm_pattern_conf_t
{
    const char *patternfile;
    nx_patterndb_t *patterndb;
} nx_pm_pattern_conf_t;



#endif	/* __NX_PM_PATTERN_H */
