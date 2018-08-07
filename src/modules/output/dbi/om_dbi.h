/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_OM_DBI_H
#define __NX_OM_DBI_H

#include "../../../common/types.h"
#include <dbi/dbi.h>

#define NX_OM_DBI_DEFAULT_SQL_LENGTH 1024

#define NX_OM_DBI_DEFAULT_SQL_TEMPLATE "INSERT INTO log (facility, severity, hostname, timestamp, application, message, hmac) VALUES ($facility, $severity, $hostname, '$recv_timestamp', $application, $message, $hmac)"

typedef struct nx_om_dbi_option_t
{
    const char *name;
    const char *value;
} nx_om_dbi_option_t;

typedef struct nx_om_dbi_conf_t
{
#ifdef HAVE_DBI_INITIALIZE_R
    dbi_inst dbi_inst;
#endif
    const char *driver;
    apr_array_header_t	*options;
    const char *sql;
    dbi_conn conn;
} nx_om_dbi_conf_t;


#endif	/* __NX_OM_DBI_H */
