/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_ROUTER_H
#define __NX_ROUTER_H

#include "../common/module.h"
#include "../common/cfgfile.h"
#include "../common/route.h"
#include "nxlog.h"

void nx_ctx_init_routes(nx_ctx_t *ctx);


#endif	/* __NX_ROUTER_H */
