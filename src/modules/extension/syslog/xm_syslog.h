/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_XM_SYSLOG_H
#define __NX_XM_SYSLOG_H


typedef struct nx_xm_syslog_conf_t
{
    char snaredelimiter;
    char snarereplacement;
    boolean ietftimestampingmt;
} nx_xm_syslog_conf_t;

#endif	/* __NX_XM_SYSLOG_H */
