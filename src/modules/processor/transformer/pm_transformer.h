/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_PM_TRANSFORMER_H
#define __NX_PM_TRANSFORMER_H

#include "../../../common/types.h"
#include "../../extension/csv/csv.h"
#include "../../extension/json/json.h"
#include "../../extension/xml/xml.h"


typedef enum nx_pm_transformer_format
{
    NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC3164 = 1,
    NX_PM_TRANSFORMER_FORMAT_SYSLOG_BSD,
    NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC5424,
    NX_PM_TRANSFORMER_FORMAT_SYSLOG_IETF,
    NX_PM_TRANSFORMER_FORMAT_SYSLOG_SNARE,
    NX_PM_TRANSFORMER_FORMAT_CSV,
    NX_PM_TRANSFORMER_FORMAT_AS_RECEIVED,
    NX_PM_TRANSFORMER_FORMAT_XML,
    NX_PM_TRANSFORMER_FORMAT_JSON,
} nx_pm_transformer_format;

typedef struct nx_pm_transformer_conf_t
{
    nx_pm_transformer_format inputformat;
    const char *inputcharset;

    nx_pm_transformer_format outputformat;
    const char *outputcharset;

    nx_csv_ctx_t csv_in_ctx;
    nx_csv_ctx_t csv_out_ctx;

    nx_json_parser_ctx_t json_in_ctx;
    nx_json_parser_ctx_t json_out_ctx;

    nx_xml_parser_ctx_t xml_in_ctx;
    nx_xml_parser_ctx_t xml_out_ctx;

} nx_pm_transformer_conf_t;



#endif	/* __NX_PM_TRANSFORMER_H */
