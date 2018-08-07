/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_RESOURCE_H
#define __NX_RESOURCE_H

#include "dlist.h"

/*
 Resources can be used to pass data between, to and from modules.
 Resources are non-persistent across restarts.
*/

typedef enum nx_resource_type_t
{
    NX_RESOURCE_TYPE_FILE = 1,
    NX_RESOURCE_TYPE_SOCKET,
    NX_RESOURCE_TYPE_PIPE,
    NX_RESOURCE_TYPE_FD,
    NX_RESOURCE_TYPE_PORT,
    NX_RESOURCE_TYPE_STRING,
    NX_RESOURCE_TYPE_PASSWORD,
    NX_RESOURCE_TYPE_KEY,
    NX_RESOURCE_TYPE_CERT,
    NX_RESOURCE_TYPE_OTHER,
} nx_resource_type_t;


typedef struct nx_resource_t
{
    NX_DLIST_ENTRY(nx_resource_t) link;
    const char *name;
    nx_resource_type_t	type;
    const void *data; 		///< pointer to the resource
} nx_resource_t;

void nx_resource_put(const char *name, nx_resource_type_t type, const void *data);
const void *nx_resource_get(const char *name, nx_resource_type_t type);

#endif /* __NX_RESOURCE_H */
