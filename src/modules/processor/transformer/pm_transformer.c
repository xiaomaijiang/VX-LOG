/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../extension/syslog/syslog.h"
#include "../../extension/csv/csv.h"
#include "../../../core/ctx.h"

#include "pm_transformer.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static nx_logdata_t *pm_transformer_process(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_pm_transformer_conf_t *modconf;
    nx_value_t *val;

    ASSERT(logdata != NULL);
    ASSERT(logdata->raw_event != NULL);

    modconf = (nx_pm_transformer_conf_t *) module->config;

    log_debug("nx_pm_transformer_process_data()");

    //log_debug("processing: [%s]", logdata->data);

    switch ( modconf->inputformat )
    {
	case NX_PM_TRANSFORMER_FORMAT_AS_RECEIVED:
	    break;
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_BSD:
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC3164:
	    nx_syslog_parse_rfc3164(logdata, logdata->raw_event->buf,
				    logdata->raw_event->len);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_IETF:
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC5424:
	    nx_syslog_parse_rfc5424(logdata, logdata->raw_event->buf,
				    logdata->raw_event->len);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_CSV:
	    nx_csv_parse(logdata, &(modconf->csv_in_ctx), logdata->raw_event->buf,
			 logdata->raw_event->len);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_JSON:
	    modconf->json_in_ctx.logdata = logdata;
	    nx_json_parse(&(modconf->json_in_ctx), logdata->raw_event->buf,
			  logdata->raw_event->len);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_XML:
	    modconf->xml_in_ctx.logdata = logdata;
	    nx_xml_parse(&(modconf->xml_in_ctx), logdata->raw_event->buf,
			 logdata->raw_event->len);
	    break;
	default:
	    nx_panic("invalid inputformat: %d", modconf->inputformat);
    }
    
    switch ( modconf->outputformat )
    {
	case NX_PM_TRANSFORMER_FORMAT_AS_RECEIVED:
	    break;
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_BSD:
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC3164:
	    nx_logdata_to_syslog_rfc3164(logdata);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_IETF:
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC5424:
	    nx_logdata_to_syslog_rfc5424(logdata, FALSE);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_SNARE:
	    nx_logdata_to_syslog_snare(logdata, module->evt_recvd, '\t', ' ');
	    break;
	case NX_PM_TRANSFORMER_FORMAT_CSV:
	    val = nx_value_new(NX_VALUE_TYPE_STRING);
	    val->string = nx_logdata_to_csv(&(modconf->csv_out_ctx), logdata);
	    nx_logdata_set_field_value(logdata, "raw_event", val);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_JSON:
	    val = nx_value_new(NX_VALUE_TYPE_STRING);
	    modconf->json_out_ctx.logdata = logdata;
	    val->string = nx_logdata_to_json(&(modconf->json_out_ctx));
	    nx_logdata_set_field_value(logdata, "raw_event", val);
	    break;
	case NX_PM_TRANSFORMER_FORMAT_XML:
	    val = nx_value_new(NX_VALUE_TYPE_STRING);
	    modconf->xml_out_ctx.logdata = logdata;
	    val->string = nx_logdata_to_xml(&(modconf->xml_out_ctx));
	    nx_logdata_set_field_value(logdata, "raw_event", val);
	    break;
	default:
	    nx_panic("invalid inputformat: %d", modconf->inputformat);
    }
 
    return ( logdata );
}



static void pm_transformer_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;

    log_debug("nx_pm_transformer_data_available()");
    
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    logdata = pm_transformer_process(module, logdata);

    // add logdata to the next module's queue
    nx_module_progress_logdata(module, logdata);
}



static void pm_transformer_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_transformer_data_available(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



static nx_pm_transformer_format get_format(const char *format)
{
    ASSERT(format != NULL);

    if ( strcasecmp(format, "syslog_rfc3164") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC3164 );
    }
    else if ( strcasecmp(format, "syslog_bsd") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_SYSLOG_BSD );
    }
    else if ( strcasecmp(format, "syslog_ietf") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_SYSLOG_IETF );
    }
    else if ( strcasecmp(format, "syslog_rfc5424") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_SYSLOG_RFC5424 );
    }
    else if ( strcasecmp(format, "syslog_snare") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_SYSLOG_SNARE );
    }
    else if ( strcasecmp(format, "csv") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_CSV );
    }
    else if ( strcasecmp(format, "json") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_JSON );
    }
    else if ( strcasecmp(format, "xml") == 0 )
    {
	return ( NX_PM_TRANSFORMER_FORMAT_XML );
    }

    return ( 0 );
}



static void nx_csv_ctx_set_default_fields(nx_csv_ctx_t *ctx)
{
    ASSERT(ctx != NULL);

    ctx->fields[0] = "SyslogFacility";
    ctx->fields[1] = "SyslogSeverity";
    ctx->fields[2] = "EventTime";
    ctx->fields[3] = "Hostname";
    ctx->fields[4] = "SourceName";
    ctx->fields[5] = "ProcessID";
    ctx->fields[6] = "Message";

    ctx->num_field = 7;
}



static void pm_transformer_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_pm_transformer_conf_t *modconf;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_pm_transformer_conf_t));
    module->config = modconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "InputFormat") == 0 )
	{
	    if ( modconf->inputformat != 0 )
	    {
		nx_conf_error(curr, "InputFormat is already defined");
	    }
	    modconf->inputformat = get_format(curr->args);
	    if ( modconf->inputformat == 0 )
	    {
		nx_conf_error(curr, "invalid InputFormat '%s'", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "OutputFormat") == 0 )
	{
	    if ( modconf->outputformat != 0 )
	    {
		nx_conf_error(curr, "OutputFormat is already defined");
	    }
	    modconf->outputformat = get_format(curr->args);
	    if ( modconf->outputformat == 0 )
	    {
		nx_conf_error(curr, "invalid OutputFormat '%s'", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "CSVInputFields") == 0 )
	{
	    nx_csv_ctx_init(&(modconf->csv_in_ctx));
	    modconf->csv_in_ctx.quote_method = NX_CSV_QUOTE_METHOD_ALL;
	    modconf->csv_in_ctx.escape_control = TRUE;
	    nx_csv_ctx_set_quotechar(&(modconf->csv_in_ctx), '\"');
	    nx_csv_ctx_set_delimiter(&(modconf->csv_in_ctx), ',');
	    nx_csv_ctx_set_escapechar(&(modconf->csv_in_ctx), '\\');
	    modconf->csv_in_ctx.num_field = nx_module_parse_fields(modconf->csv_in_ctx.fields, curr->args);
	}
	else if ( strcasecmp(curr->directive, "CSVInputFieldTypes") == 0 )
	{
	    modconf->csv_in_ctx.num_type = nx_module_parse_types(modconf->csv_in_ctx.types, curr->args);
	}
	else if ( strcasecmp(curr->directive, "CSVOutputFields") == 0 )
	{
	    nx_csv_ctx_init(&(modconf->csv_out_ctx));
	    modconf->csv_out_ctx.quote_method = NX_CSV_QUOTE_METHOD_ALL;
	    modconf->csv_out_ctx.escape_control = TRUE;
	    nx_csv_ctx_set_quotechar(&(modconf->csv_out_ctx), '\"');
	    nx_csv_ctx_set_delimiter(&(modconf->csv_out_ctx), ',');
	    nx_csv_ctx_set_escapechar(&(modconf->csv_out_ctx), '\\');
	    modconf->csv_out_ctx.num_field = nx_module_parse_fields(modconf->csv_out_ctx.fields, curr->args);
	}
	else
	{
	    nx_conf_error(curr, "invalid pm_transformer keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( modconf->inputformat == 0 )
    {
	modconf->inputformat = NX_PM_TRANSFORMER_FORMAT_AS_RECEIVED;
    }

    switch ( modconf->inputformat )
    {
	case NX_PM_TRANSFORMER_FORMAT_SYSLOG_SNARE:
	    nx_conf_error(module->directives, "syslog_snare format is only supported for output");
	    break;
	case NX_PM_TRANSFORMER_FORMAT_CSV:
	    if ( modconf->csv_in_ctx.fields[0] == NULL )
	    {
		nx_csv_ctx_init(&(modconf->csv_in_ctx));
		nx_csv_ctx_set_default_fields(&(modconf->csv_in_ctx));
	    }
	    if ( (modconf->csv_in_ctx.num_type > 0) && (modconf->csv_in_ctx.num_type != modconf->csv_in_ctx.num_field) )
	    {
		nx_conf_error(module->directives, "Number of 'CSVInputFields' must equal to 'CSVInputFieldTypes' if both directives are defined");
	    }
	    break;
	default:
	    if ( modconf->csv_in_ctx.fields[0] != NULL )
	    {
		nx_conf_error(module->directives, "CSVInputFields set but InputFormat is not 'csv'");
	    }
	    break;
    }

    if ( modconf->outputformat == 0 )
    {
	modconf->outputformat = NX_PM_TRANSFORMER_FORMAT_AS_RECEIVED;
    }

    switch ( modconf->outputformat )
    {
	case NX_PM_TRANSFORMER_FORMAT_CSV:
	    if ( modconf->csv_out_ctx.fields[0] == NULL )
	    {
		nx_csv_ctx_init(&(modconf->csv_out_ctx));
		nx_csv_ctx_set_default_fields(&(modconf->csv_out_ctx));
	    }
	    break;
	default:
	    if ( modconf->csv_out_ctx.fields[0] != NULL )
	    {
		nx_conf_error(module->directives, "CSVOutputFields set but OutputFormat is not 'csv'");
	    }
	    break;
    }
}



NX_MODULE_DECLARATION nx_pm_transformer_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    pm_transformer_config,	// config
    NULL,			// start
    NULL,		 	// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_transformer_event,	// event
    NULL,			// info
    NULL,			// exports
};
