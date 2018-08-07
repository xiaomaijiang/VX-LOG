/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_XML_H
#define __NX_XML_H

#include "../../../common/str.h"
#include "../../../common/logdata.h"
#include "xm_xml.h"


#define NS_SEPARATOR '|'

typedef struct nx_xml_parser_ctx_t
{
    nx_logdata_t *logdata;
    const char *key;
    nx_string_t *value;
    int depth;
} nx_xml_parser_ctx_t;


void nx_xml_parse(nx_xml_parser_ctx_t *ctx,
		  const char *xml, size_t len);
nx_string_t *nx_logdata_to_xml(nx_xml_parser_ctx_t *ctx);

#endif	/* __NX_XML_H */
