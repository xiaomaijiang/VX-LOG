/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_SSL_H
#define __NX_IM_SSL_H

#include "../../../common/ssl.h"


typedef struct nx_im_ssl_conf_t
{
    const char			*host;
    apr_port_t			port;
    apr_socket_t		*listensock;
    nx_module_input_list_t	*connections; ///< contains nx_module_input_t structures
    nx_ssl_ctx_t		ssl_ctx;
    nx_module_input_func_decl_t *inputfunc;
} nx_im_ssl_conf_t;



#endif	/* __NX_IM_SSL_H */
