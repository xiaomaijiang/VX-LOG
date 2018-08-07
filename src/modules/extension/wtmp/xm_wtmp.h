/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Balazs Boza <balazsboza@gmail.com>
 */

#ifndef __NX_XM_WTMP_H
#define __NX_XM_WTMP_H

#include "../../../common/types.h"
#include "../../../common/logdata.h"

typedef struct nx_xm_wtmp_conf_t {
} nx_xm_wtmp_conf_t;

typedef struct nx_xm_wtmp_ctx_t {
    int len;
    char* buf;
} nx_xm_wtmp_ctx_t;

#endif	/* __NX_XM_wtmp_H */
