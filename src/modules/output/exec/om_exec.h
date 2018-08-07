/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_OM_EXEC_H
#define __NX_OM_EXEC_H

#include "../../../common/types.h"

#define NX_OM_EXEC_MAX_ARGC 64

typedef struct nx_om_exec_conf_t
{
    const char *cmd;
    const char *argv[NX_OM_EXEC_MAX_ARGC];
    apr_file_t *desc;
} nx_om_exec_conf_t;



#endif	/* __NX_OM_EXEC_H */
