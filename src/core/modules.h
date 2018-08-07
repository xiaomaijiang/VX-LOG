/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_MODULES_H
#define __NX_MODULES_H

#include "../common/types.h"
#include "ctx.h"

nx_module_t *nx_module_new(nx_module_type_t type, const char *name, apr_size_t bufsize);
void nx_ctx_config_modules(nx_ctx_t *ctx);
void nx_ctx_init_modules(nx_ctx_t *ctx);
void nx_ctx_start_modules(nx_ctx_t *ctx);
void nx_ctx_stop_modules(nx_ctx_t *ctx, nx_module_type_t type);
void nx_ctx_shutdown_modules(nx_ctx_t *ctx, nx_module_type_t type);
void nx_ctx_save_queues(nx_ctx_t *ctx);
void nx_ctx_restore_queues(nx_ctx_t *ctx);

#endif	/* __NX_MODULES_H */

