/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_OM_UDP_H
#define __NX_OM_UDP_H

#include "../../../common/types.h"

typedef struct nx_om_udp_conf_t
{
    const char		*host;
    apr_port_t		port;
    apr_socket_t	*sock;
    int			sockbufsize;
    int			reconnect;
} nx_om_udp_conf_t;


#endif	/* __NX_OM_UDP_H */
