/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_XM_CHARCONV_H
#define __NX_XM_CHARCONV_H

#include "charconv.h"

#define NX_XM_CHARCONV_MAX_CHARSETS 50

typedef struct nx_xm_charconv_conf_t
{
    iconv_t	*iconv_ctx;
    //const char	*srccharset;
    //const char	*dstcharset;
    const char *autocharsets[NX_XM_CHARCONV_MAX_CHARSETS];
    int num_charsets;
    //apr_thread_mutex_t *mutex; // lock for iconv ctx
} nx_xm_charconv_conf_t;

#endif	/* __NX_XM_CHARCONV_H */
