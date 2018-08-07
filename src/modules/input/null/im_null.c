/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"


#define NX_LOGMODULE NX_LOGMODULE_MODULE

NX_MODULE_DECLARATION nx_im_null_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    NULL,			// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
    NULL,			// exports
};

