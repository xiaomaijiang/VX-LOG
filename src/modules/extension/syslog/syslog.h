/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_SYSLOG_H
#define __NX_SYSLOG_H

#include "types.h"
#include "logdata.h"

// real + 1
typedef enum nx_syslog_facility_t
{
    NX_SYSLOG_FACILITY_KERN = 0,
    NX_SYSLOG_FACILITY_USER,
    NX_SYSLOG_FACILITY_MAIL,
    NX_SYSLOG_FACILITY_DAEMON,
    NX_SYSLOG_FACILITY_AUTH,
    NX_SYSLOG_FACILITY_SYSLOG,
    NX_SYSLOG_FACILITY_LPR,
    NX_SYSLOG_FACILITY_NEWS,
    NX_SYSLOG_FACILITY_UUCP,
    NX_SYSLOG_FACILITY_CRON,
    NX_SYSLOG_FACILITY_AUTHPRIV,
    NX_SYSLOG_FACILITY_FTP,
    NX_SYSLOG_FACILITY_NTP,
    NX_SYSLOG_FACILITY_AUDIT,
    NX_SYSLOG_FACILITY_ALERT,
    NX_SYSLOG_FACILITY_CRON2,
    NX_SYSLOG_FACILITY_LOCAL0,
    NX_SYSLOG_FACILITY_LOCAL1,
    NX_SYSLOG_FACILITY_LOCAL2,
    NX_SYSLOG_FACILITY_LOCAL3,
    NX_SYSLOG_FACILITY_LOCAL4,
    NX_SYSLOG_FACILITY_LOCAL5,
    NX_SYSLOG_FACILITY_LOCAL6,
    NX_SYSLOG_FACILITY_LOCAL7,
} nx_syslog_facility_t;


typedef enum nx_syslog_severity_t
{
    NX_SYSLOG_SEVERITY_EMERG = 0,
    NX_SYSLOG_SEVERITY_ALERT,
    NX_SYSLOG_SEVERITY_CRIT,
    NX_SYSLOG_SEVERITY_ERR,
    NX_SYSLOG_SEVERITY_WARNING,
    NX_SYSLOG_SEVERITY_NOTICE,
    NX_SYSLOG_SEVERITY_INFO,
    NX_SYSLOG_SEVERITY_DEBUG,
    NX_SYSLOG_SEVERITY_NOPRI,
} nx_syslog_severity_t;


nx_syslog_facility_t nx_syslog_facility_from_string(const char *str);
const char *nx_syslog_facility_to_string(nx_syslog_facility_t facility);
nx_syslog_severity_t nx_syslog_severity_from_string(const char *str);
const char *nx_syslog_severity_to_string(nx_syslog_severity_t severity);
boolean nx_syslog_parse_rfc3164(nx_logdata_t *logdata,
				const char *string,
				size_t stringlen);
boolean nx_syslog_parse_rfc5424(nx_logdata_t *logdata,
				const char *string,
				size_t stringlen);
void nx_logdata_to_syslog_rfc3164(nx_logdata_t *logdata);
void nx_logdata_to_syslog_rfc5424(nx_logdata_t *logdata, boolean gmt);
void nx_logdata_to_syslog_snare(nx_logdata_t *logdata,
				uint64_t evtcnt,
				char delimiter,
				char replacement);

#endif	/* __NX_SYSLOG_H */
   
