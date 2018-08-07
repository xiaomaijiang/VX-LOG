/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_DBI_H
#define __NX_IM_DBI_H

#include "../../../common/types.h"
#include <dbi/dbi.h>

#define NX_IM_DBI_DEFAULT_SQL_TEMPLATE "SELECT id, facility, severity, hostname, timestamp, application, message, hmac FROM log"
#define NX_IM_DBI_WHERE "WHERE id > %d"

typedef struct nx_im_dbi_option_t
{
    const char *name;
    const char *value;
} nx_im_dbi_option_t;

typedef struct nx_im_dbi_conf_t
{
#ifdef HAVE_DBI_INITIALIZE_R
    dbi_inst dbi_inst;
#endif
    const char *driver;
    apr_array_header_t *options;
    boolean savepos;
    int64_t last_id;
    float poll_interval;
    const char *sql;
    char *_sql;
    size_t _sql_bufsize;
    dbi_conn conn;
    nx_event_t *event;
} nx_im_dbi_conf_t;



#endif	/* __NX_IM_DBI_H */
