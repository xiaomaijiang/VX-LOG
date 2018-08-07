/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_OM_TCP_H
#define __NX_OM_TCP_H

#include "../../../common/types.h"

typedef struct nx_om_tcp_conf_t
{
    const char		*host;
    apr_port_t		port;
    apr_socket_t	*sock;
    boolean		connected;
    int			reconnect;

    boolean		listen; ///< listen for connections (a.k.a. pull-mode)
    boolean		queueinlistenmode; ///< do not drop messages in listen mode
    apr_socket_t	*listensock;
    nx_module_input_list_t *connections; 
} nx_om_tcp_conf_t;


#endif	/* __NX_OM_TCP_H */
