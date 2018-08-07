/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_EXEC_H
#define __NX_IM_EXEC_H

#include "../../../common/types.h"

#define NX_IM_EXEC_MAX_ARGC 64

typedef struct nx_im_exec_conf_t
{
    const char *cmd;
    const char *argv[NX_IM_EXEC_MAX_ARGC];
    nx_module_input_func_decl_t *inputfunc;
    apr_proc_t proc;
    boolean running;
    nx_event_t *event;
    boolean restart;
    int delay;
} nx_im_exec_conf_t;



#endif	/* __NX_IM_EXEC_H */
