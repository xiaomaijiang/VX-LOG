/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_TCP_H
#define __NX_IM_TCP_H

#include "../../../common/types.h"


typedef struct nx_im_tcp_conf_t
{
    const char			*host;
    apr_port_t			port;
    apr_socket_t		*listensock;
    nx_module_input_list_t	*connections; ///< contains nx_im_tcp_conn_t structures
    boolean			nodelay;
    nx_module_input_func_decl_t *inputfunc;
} nx_im_tcp_conf_t;



#endif	/* __NX_IM_TCP_H */
