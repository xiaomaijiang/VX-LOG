/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_XM_MULTILINE_H
#define __NX_XM_MULTILINE_H

#include "../../../common/types.h"
#include "../../../common/value.h"
#include "../../../common/logdata.h"
#include "../../../common/expr.h"

typedef struct nx_xm_multiline_conf_t
{
    nx_value_t *headerline;
    nx_expr_t *headerline_expr;
    nx_value_t *endline;
    nx_expr_t *endline_expr;
    int fixedlinecount;
} nx_xm_multiline_conf_t;


typedef struct nx_xm_multiline_ctx_t
{
    nx_logdata_t *tmpline;
    nx_logdata_t *logdata;
    int	linecount;
} nx_xm_multiline_ctx_t;

#endif	/* __NX_XM_MULTILINE_H */
