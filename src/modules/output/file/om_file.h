/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_OM_FILE_H
#define __NX_OM_FILE_H

#include "../../../common/types.h"
#include "../../../common/expr.h"
#include <apr_file_io.h>

typedef struct nx_om_file_conf_t
{
    nx_expr_t   *filename_expr;
    char	filename[APR_PATH_MAX];
    apr_pool_t  *fpool;
    apr_file_t  *file;
    boolean	truncate; ///< truncate output?
    boolean	sync; ///< sync on write
    boolean	createdir; ///< CreateDir
    boolean	in_pollset; ///< true if added to pollset
} nx_om_file_conf_t;

void om_file_open(nx_module_t *module);
void om_file_close(nx_module_t *module);

#endif	/* __NX_OM_FILE_H */
