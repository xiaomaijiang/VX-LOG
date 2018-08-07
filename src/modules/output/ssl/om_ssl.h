/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_OM_SSL_H
#define __NX_OM_SSL_H

#include "../../../common/ssl.h"

typedef struct nx_om_ssl_conf_t
{
    const char		*host;
    apr_port_t		port;
    apr_socket_t	*sock;
    boolean		connected;
    int			reconnect;
    nx_ssl_ctx_t	ssl_ctx;
    SSL			*ssl;
} nx_om_ssl_conf_t;


#endif	/* __NX_OM_SSL_H */
