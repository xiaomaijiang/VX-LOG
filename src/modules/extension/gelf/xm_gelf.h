/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_XM_GELF_H
#define __NX_XM_GELF_H

#include "../../../common/types.h"

typedef struct nx_xm_gelf_conf_t
{
    boolean usenulldelimiter;
    unsigned int shortmessagelength;
} nx_xm_gelf_conf_t;

#endif	/* __NX_XM_GELF_H */
