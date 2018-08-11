/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_MARK_H
#define __NX_IM_MARK_H

#include "../../../common/types.h"

typedef struct nx_im_mark_conf_t
{
    const char *mark;
    int		mark_interval;
    int		pid;
    nx_event_t *event;
    sigar_t *sigar;
    sigar_pid_t sigar_pid;
    sigar_proc_mem_t proc_mem;
    sigar_proc_cpu_t proc_cpu;
    sigar_sys_info_t sys_info;
} nx_im_mark_conf_t;



#endif	/* __NX_IM_MARK_H */
