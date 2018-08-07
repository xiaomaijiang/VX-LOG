/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_BACKTRACE_H
#define __NX_BACKTRACE_H

#include "../../src/common/types.h"
#include "../../src/common/str.h"

#define	NX_BACKTRACE_SIZE	24

boolean nx_append_backtrace(nx_string_t *str);

#endif	/* __NX_BACKTRACE_H */
