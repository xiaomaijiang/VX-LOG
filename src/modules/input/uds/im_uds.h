/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_UDS_H
#define __NX_IM_UDS_H

#include "../../../common/types.h"

typedef struct nx_im_uds_conf_t
{
    const char *sun_path;
    nx_module_input_func_decl_t *inputfunc;
} nx_im_uds_conf_t;



#endif	/* __NX_IM_UDS_H */
