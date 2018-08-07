/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "syslog.h"
#include "../../../common/date.h"
#include "../../../common/module.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static nx_keyval_t syslog_facilities[] =
{
    { NX_SYSLOG_FACILITY_USER, "USER" },
    { NX_SYSLOG_FACILITY_SYSLOG, "SYSLOG" },
    { NX_SYSLOG_FACILITY_DAEMON, "DAEMON" },
    { NX_SYSLOG_FACILITY_KERN, "KERN" },
    { NX_SYSLOG_FACILITY_MAIL, "MAIL" },
    { NX_SYSLOG_FACILITY_AUTH, "AUTH" },
    { NX_SYSLOG_FACILITY_LPR, "LPR" },
    { NX_SYSLOG_FACILITY_NEWS, "NEWS" },
    { NX_SYSLOG_FACILITY_UUCP, "UCP" },
    { NX_SYSLOG_FACILITY_CRON, "CRON" },
    { NX_SYSLOG_FACILITY_AUTHPRIV, "AUTHPRIV" },
    { NX_SYSLOG_FACILITY_FTP, "FTP" },
    { NX_SYSLOG_FACILITY_NTP, "NTP" },
    { NX_SYSLOG_FACILITY_AUDIT, "AUDIT" },
    { NX_SYSLOG_FACILITY_ALERT, "FACALERT" },
    { NX_SYSLOG_FACILITY_CRON2, "CRON2" },
    { NX_SYSLOG_FACILITY_LOCAL0, "LOCAL0" },
    { NX_SYSLOG_FACILITY_LOCAL1, "LOCAL1" },
    { NX_SYSLOG_FACILITY_LOCAL2, "LOCAL2" },
    { NX_SYSLOG_FACILITY_LOCAL3, "LOCAL3" },
    { NX_SYSLOG_FACILITY_LOCAL4, "LOCAL4" },
    { NX_SYSLOG_FACILITY_LOCAL5, "LOCAL5" },
    { NX_SYSLOG_FACILITY_LOCAL6, "LOCAL6" },
    { NX_SYSLOG_FACILITY_LOCAL7, "LOCAL7" },
    { 0, NULL }
};



static nx_keyval_t syslog_severities[] =
{
    { NX_SYSLOG_SEVERITY_INFO, "INFO" },
    { NX_SYSLOG_SEVERITY_ERR, "ERR" },
    { NX_SYSLOG_SEVERITY_ERR, "ERROR" },
    { NX_SYSLOG_SEVERITY_NOTICE, "NOTICE" },
    { NX_SYSLOG_SEVERITY_ALERT, "ALERT" },
    { NX_SYSLOG_SEVERITY_CRIT, "CRIT" },
    { NX_SYSLOG_SEVERITY_CRIT, "CRITICAL" },
    { NX_SYSLOG_SEVERITY_DEBUG, "DEBUG" },
    { NX_SYSLOG_SEVERITY_EMERG, "EMERG" },
    { NX_SYSLOG_SEVERITY_EMERG, "EMERGENT" },
    { NX_SYSLOG_SEVERITY_EMERG, "PANIC" },
    { NX_SYSLOG_SEVERITY_NOPRI, "NONE" },
    { NX_SYSLOG_SEVERITY_WARNING, "WARNING" },
    { NX_SYSLOG_SEVERITY_WARNING, "WARN" },
    { 0, NULL }
};



nx_syslog_facility_t nx_syslog_facility_from_string(const char *str)
{
    int i;

    for ( i = 0; syslog_facilities[i].value != NULL; i++ )
    {
	if ( strcasecmp(syslog_facilities[i].value, str) == 0 )
	{
	    return ( syslog_facilities[i].key );
	}
    }
    return ( 0 );
}



const char *nx_syslog_facility_to_string(nx_syslog_facility_t facility)
{
    int i;

    for ( i = 0; syslog_facilities[i].value != NULL; i++ )
    {
	if ( syslog_facilities[i].key == (int) facility )
	{
	    return ( syslog_facilities[i].value );
	}
    }
    return ( NULL );
}



nx_syslog_severity_t nx_syslog_severity_from_string(const char *str)
{
    int i;

    for ( i = 0; syslog_severities[i].value != NULL; i++ )
    {
	if ( strcasecmp(syslog_severities[i].value, str) == 0 )
	{
	    return ( syslog_severities[i].key );
	}
    }
    return ( 0 );
}



const char *nx_syslog_severity_to_string(nx_syslog_severity_t severity)
{
    int i;

    for ( i = 0; syslog_severities[i].value != NULL; i++ )
    {
	if ( syslog_severities[i].key == (int) severity )
	{
	    return ( syslog_severities[i].value );
	}
    }
    return ( NULL );
}



static void set_syslog_hostname(nx_logdata_t *logdata,
				const char *hoststart,
				const char *hostend)
{
    nx_value_t *hostname = NULL;
    int len;

    if ( (hoststart != NULL) && (hostend != NULL) && (hostend > hoststart) )
    {
	hostname = malloc(sizeof(nx_value_t));
	ASSERT(hostname != NULL);
	hostname->type = NX_VALUE_TYPE_STRING;
	hostname->defined = TRUE;
	len = (int) (hostend - hoststart);
	hostname->string = nx_string_create(hoststart, len);
	nx_logdata_set_field_value(logdata, "Hostname", hostname);
    }
    else
    {
	nx_value_t recv_from;

	if ( nx_logdata_get_field_value(logdata, "MessageSourceAddress", &recv_from) != TRUE )
	{
	    nx_value_t *val;
	    const nx_string_t *hoststr;
	    
	    hoststr = nx_get_hostname();
	    val = nx_value_new(NX_VALUE_TYPE_STRING);
	    val->string = nx_string_clone(hoststr);
	    ASSERT(val->string != NULL);
	    nx_logdata_set_field_value(logdata, "Hostname", val);
	}
	else
	{ // default hostname will be the IP in recv_from field
	    // FIXME: hostname can be string only
	    nx_logdata_set_field_value(logdata, "Hostname", nx_value_clone(NULL, &recv_from));
	}
    }
}



static const char *parse_syslog_priority(const char *string,
					 nx_logdata_t *logdata,
					 boolean *retval)
{
    const char *ptr = string;
    int priority = -1;
    nx_syslog_facility_t fac = NX_SYSLOG_FACILITY_USER;
    nx_syslog_severity_t sev = NX_SYSLOG_SEVERITY_NOTICE;

    nx_value_t *facility = NULL;
    nx_value_t *severity = NULL;
    nx_loglevel_t loglevel = NX_LOGLEVEL_INFO;

    ASSERT(string != NULL);

    // parse priority
    if ( *ptr == '<' )
    {
	ptr++;
	priority = 0;
	while ( apr_isdigit(*ptr) )
	{
	    if ( ptr - string > 6 )
	    {
		*retval = FALSE;
		ptr = string;
		goto badpri;
	    }
	    priority = priority * 10 + (*ptr - '0');
	    ptr++;
	}
	if ( *ptr != '>' )
	{
	    priority = -1;
	}
	else
	{
	    ptr++;
	    if ( *ptr == ' ' )
	    {
		ptr++;
	    }
	}
    }
    else
    {
	*retval = FALSE;
	goto badpri;
    }

    if ( priority != -1 )
    {
	fac = (priority >> 3);
	if ( fac > NX_SYSLOG_FACILITY_LOCAL7 )
	{
	    fac = NX_SYSLOG_FACILITY_USER;
	    goto badpri;
	}
	sev = priority & 0x07;
	if ( sev > NX_SYSLOG_SEVERITY_NOPRI )
	{
	    sev = NX_SYSLOG_SEVERITY_NOTICE;
	    goto badpri;
	}
    }

  badpri:
    facility = malloc(sizeof(nx_value_t));
    ASSERT(facility != NULL);
    facility->type = NX_VALUE_TYPE_INTEGER;
    facility->defined = TRUE;
    facility->integer = fac;
    nx_logdata_set_field_value(logdata, "SyslogFacilityValue", facility);
    nx_logdata_set_field_value(logdata, "SyslogFacility", 
			       nx_value_new_string(nx_syslog_facility_to_string(fac)));

    severity = malloc(sizeof(nx_value_t));
    ASSERT(severity != NULL);
    severity->integer = sev;
    severity->type = NX_VALUE_TYPE_INTEGER;
    severity->defined = TRUE;
    nx_logdata_set_field_value(logdata, "SyslogSeverityValue", severity);
    nx_logdata_set_field_value(logdata, "SyslogSeverity", 
			       nx_value_new_string(nx_syslog_severity_to_string(sev)));

    // normalize syslog severity
    switch ( sev )
    {
	case NX_SYSLOG_SEVERITY_INFO:
	    loglevel = NX_LOGLEVEL_INFO;
	    break;
	case NX_SYSLOG_SEVERITY_ERR:
	    loglevel = NX_LOGLEVEL_ERROR;
	    break;
	case NX_SYSLOG_SEVERITY_NOTICE:
	    loglevel = NX_LOGLEVEL_INFO;
	    break;
	case NX_SYSLOG_SEVERITY_ALERT:
	    loglevel = NX_LOGLEVEL_WARNING;
	    break;
	case NX_SYSLOG_SEVERITY_CRIT:
	    loglevel = NX_LOGLEVEL_CRITICAL;
	    break;
	case NX_SYSLOG_SEVERITY_DEBUG:
	    loglevel = NX_LOGLEVEL_DEBUG;
	    break;
	case NX_SYSLOG_SEVERITY_EMERG:
	    loglevel = NX_LOGLEVEL_ERROR;
	    break;
	case NX_SYSLOG_SEVERITY_NOPRI:
	    loglevel = NX_LOGLEVEL_INFO;
	    break;
	case NX_SYSLOG_SEVERITY_WARNING:
	    loglevel = NX_LOGLEVEL_WARNING;
	    break;
	default:
	    loglevel = NX_LOGLEVEL_INFO;
	    break;
    }
    nx_logdata_set_integer(logdata, "SeverityValue", loglevel);
    nx_logdata_set_string(logdata, "Severity", nx_loglevel_to_string(loglevel));

    return ( ptr );
}



static void set_syslog_appname(nx_logdata_t *logdata,
			       const char *start,
			       const char *end)
{
    int len;
    nx_value_t *application = NULL;

    if ( (start != NULL) && (end != NULL) && (end > start) )
    {
	application = malloc(sizeof(nx_value_t));
	application->type = NX_VALUE_TYPE_STRING;
	application->defined = TRUE;
	ASSERT(application != NULL);
	len = (int) (end - start);
	application->string = nx_string_create(start, len);
	nx_logdata_set_field_value(logdata, "SourceName", application);
    }
}



static void set_syslog_timestamp(nx_logdata_t *logdata,
				 apr_time_t date)
{
    nx_value_t *timestamp = NULL;

    timestamp = malloc(sizeof(nx_value_t));
    ASSERT(timestamp != NULL);
    timestamp->type = NX_VALUE_TYPE_DATETIME;
    timestamp->defined = TRUE;
    timestamp->datetime = date;
    nx_logdata_set_field_value(logdata, "EventTime", timestamp);
}



static void set_syslog_procid(nx_logdata_t *logdata,
			      const char *start,
			      const char *end)
{
    int len;
    nx_value_t *procid = NULL;

    if ( (start != NULL) && (end != NULL) && (end > start) )
    {
	procid = malloc(sizeof(nx_value_t));
	procid->type = NX_VALUE_TYPE_STRING;
	procid->defined = TRUE;
	ASSERT(procid != NULL);
	len = (int) (end - start);
	procid->string = nx_string_create(start, len);
	nx_logdata_set_field_value(logdata, "ProcessID", procid);
    }
}



static void set_syslog_msgid(nx_logdata_t *logdata,
			     const char *start,
			     const char *end)
{
    int len;
    nx_value_t *msgid = NULL;

    if ( (start != NULL) && (end != NULL) && (end > start) )
    {
	msgid = malloc(sizeof(nx_value_t));
	msgid->type = NX_VALUE_TYPE_STRING;
	msgid->defined = TRUE;
	ASSERT(msgid != NULL);
	len = (int) (end - start);
	msgid->string = nx_string_create(start, len);
	nx_logdata_set_field_value(logdata, "MessageID", msgid);
    }
}



static void set_syslog_message(nx_logdata_t *logdata,
			       const char *start,
			       const char *end)
{
    int len;
    nx_value_t *message = NULL;

    if ( (start != NULL) && (end != NULL) && (end > start) )
    {
	len = (int) (end - start);
	if ( start[len - 1] == '\n' )
	{
	    len--;
	    if ( len == 0 )
	    {
		return;
	    }
	}
	message = malloc(sizeof(nx_value_t));
	message->type = NX_VALUE_TYPE_STRING;
	message->defined = TRUE;
	ASSERT(message != NULL);
	message->string = nx_string_create(start, len);
	nx_logdata_set_field_value(logdata, "Message", message);
    }
}


#define IS_HOSTCHAR(c) ( ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || \
  ((c >= 'A') && (c <= 'Z')) || (c == '.') || (c == '_') || (c == '-') || (c == '/') )
#define IS_TAGCHAR(c) ( ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || \
  ((c >= 'A') && (c <= 'Z')) || (c == '_') || (c == '-') || (c == '/') || (c == '.') )

boolean nx_syslog_parse_rfc3164(nx_logdata_t *logdata,
				const char *string,
				size_t stringlen)
{
    boolean retval = TRUE;
    const char *ptr, *hoststart = NULL, *hostend = NULL;
    const char *appstart = NULL, *append = NULL;
    const char *msgstart = NULL, *msgend = NULL;
    const char *pidstart = NULL, *pidend = NULL;
    apr_time_t date;
    boolean got_date = FALSE;
    boolean got_pri = FALSE;

    ASSERT(logdata != NULL);
    ASSERT(string != NULL);

    if ( stringlen <= 0 )
    {
	stringlen = strlen(string);
    }

    msgend = string + stringlen;
    ptr = parse_syslog_priority(string, logdata, &retval);
    got_pri = retval;
    msgstart = ptr;
    
    if ( nx_date_parse(&date, ptr, &ptr) != APR_SUCCESS )
    {
	nx_logdata_set_datetime(logdata, "EventTime", apr_time_now());
	for ( appstart = ptr; IS_TAGCHAR(*ptr); ptr++ );
	append = ptr;
    }
    else
    { // date ok
	got_date = TRUE;
	for ( ; *ptr == ' '; ptr++ ); // skip space
	msgstart = ptr;
	for ( hoststart = ptr; IS_HOSTCHAR(*ptr); ptr++ );

	if ( hoststart == ptr )
	{ //no host
	    hoststart = NULL;
	}
	else if ( *ptr == '\0' )
	{ // line ends after host
	    msgstart = NULL;
	    hostend = ptr;
	}
	else if ( *ptr == '[' )
	{ //app instead of host
	    hoststart = NULL;
	    appstart = msgstart;
	}
	else if ( (*ptr != ' ') && (*ptr != ':') )
	{
	    msgstart = hoststart;
	    hoststart = NULL;
	}
	else
	{ // got host
	    hostend = ptr;
	    msgstart = ptr;
	    
	    if ( *ptr == ':' )
	    { // no host
		appstart = hoststart;
		hoststart = NULL;
		append = hostend;
		hostend = NULL;
	    }
	    else
	    {
		for ( ; *ptr == ' '; ptr++ ); // skip space
		for ( appstart = ptr; IS_TAGCHAR(*ptr); ptr++ );
		msgstart = ptr;
	    }
	}
    }

    if ( (got_date == TRUE) || (got_pri = TRUE) )
    {
	if ( (appstart != NULL) && (*appstart == '[') )
	{
	    appstart = NULL;
	}
	else if ( appstart == NULL )
	{
	    // ignore pid
	}
	else if ( *ptr == '[' )
	{ // pid
	    append = ptr;
	    
	    ptr++;
	    pidstart = ptr;
	    for ( ; *ptr != '\0'; ptr++ )
	    {
		if ( (*ptr == ']') || (*ptr == ' ') )
		{
		    break;
		}
	    }
	    
	    msgstart = appstart;
	    
	    if ( *ptr == ']' )
	    {
		pidend = ptr;
		ptr++;
	    }
	    else
	    {
		pidend = NULL;
	    }

	    if ( *ptr == ':' )
	    {
		ptr++;
	    }
	    else
	    {
		pidend = NULL;
	    }
	    if ( *ptr == ' ' )
	    {
		ptr++;
	    }
	    if ( pidend == NULL )
	    {
		appstart = NULL;
		append = NULL;
	    }
	    if ( appstart != NULL )
	    {
		msgstart = ptr;
	    }
	}
	else if ( *ptr == ':')
	{
	    append = ptr;
	    ptr++;
	    if ( *ptr == ' ' )
	    {
		ptr++;
	    }
	    if ( appstart != NULL )
	    {
		msgstart = ptr;
	    }
	}
	else
	{
	    msgstart = appstart;
	    appstart = NULL;
	}
    }
    else
    {
	appstart = NULL;
    }

    set_syslog_hostname(logdata, hoststart, hostend);
    if ( got_date == TRUE )
    {
	nx_date_fix_year(&date);
	set_syslog_timestamp(logdata, date);
    }
    set_syslog_appname(logdata, appstart, append);
    set_syslog_procid(logdata, pidstart, pidend);
    set_syslog_message(logdata, msgstart, msgend);

    return ( retval );
}



static int nx_syslog_get_priority(nx_logdata_t *logdata)
{
    int retval = 0;
    nx_value_t facility;
    nx_value_t severity;
    int64_t fac = NX_SYSLOG_FACILITY_USER;
    int64_t sev = NX_SYSLOG_SEVERITY_NOTICE;
    nx_loglevel_t loglevel = NX_LOGLEVEL_INFO;

    if ( nx_logdata_get_field_value(logdata, "SyslogFacilityValue", &facility) == TRUE )
    {
	if ( (facility.defined == TRUE) && (facility.type == NX_VALUE_TYPE_INTEGER) )
	{
	    fac = facility.integer;
	}
    }
    else if ( nx_logdata_get_field_value(logdata, "SyslogFacility", &facility) == TRUE )
    {
	if ( (facility.defined == TRUE) && (facility.type == NX_VALUE_TYPE_STRING) )
	{
	    fac = nx_syslog_facility_from_string(facility.string->buf);
	    if ( fac == 0 )
	    {
		fac = NX_SYSLOG_FACILITY_USER;
	    }
	}
    }

    if ( nx_logdata_get_field_value(logdata, "SyslogSeverityValue", &severity) == TRUE )
    {
	if ( (severity.defined == TRUE) && (severity.type == NX_VALUE_TYPE_INTEGER) )
	{
	    sev = severity.integer;
	}
    }
    else if ( nx_logdata_get_field_value(logdata, "SyslogSeverity", &severity) == TRUE )
    {
	if ( (severity.defined == TRUE) && (severity.type == NX_VALUE_TYPE_STRING) )
	{
	    sev = nx_syslog_severity_from_string(severity.string->buf);
	    if ( sev == 0 )
	    {
		sev = NX_SYSLOG_SEVERITY_NOTICE;
	    }
	}
    }
    else if (( nx_logdata_get_field_value(logdata, "SeverityValue", &severity) == TRUE ) ||
	     ( nx_logdata_get_field_value(logdata, "Severity", &severity) == TRUE ) )
    {
	if ( (severity.defined == TRUE) && (severity.type == NX_VALUE_TYPE_INTEGER) )
	{
	    loglevel = severity.integer;
	}
	else if ( (severity.defined == TRUE) && (severity.type == NX_VALUE_TYPE_STRING) )
	{
	    loglevel = nx_loglevel_from_string(severity.string->buf);
	    if ( loglevel == 0 )
	    {
		loglevel = NX_LOGLEVEL_INFO;
	    }
	}
	switch ( loglevel )
	{
	    case NX_LOGLEVEL_DEBUG:
		sev = NX_SYSLOG_SEVERITY_DEBUG;
		break;
	    case NX_LOGLEVEL_INFO:
		sev = NX_SYSLOG_SEVERITY_INFO;
		break;
	    case NX_LOGLEVEL_WARNING:
		sev = NX_SYSLOG_SEVERITY_WARNING;
		break;
	    case NX_LOGLEVEL_ERROR:
		sev = NX_SYSLOG_SEVERITY_ERR;
		break;
	    case NX_LOGLEVEL_CRITICAL:
		sev = NX_SYSLOG_SEVERITY_CRIT;
		break;
	    default:
		sev = NX_SYSLOG_SEVERITY_NOTICE;
		break;
	}
    }

    retval = ((int) fac << 3) + (int) sev;

    return ( retval );
}



void nx_logdata_to_syslog_rfc3164(nx_logdata_t *logdata)
{
    int pri = 0;
    nx_value_t hostname;
    nx_value_t timestamp;
    nx_value_t application;
    nx_value_t pid;
    nx_value_t msg;
    nx_string_t *tmpmsg = NULL;
    size_t len;
    char tmpstr[20];
    int i;

    ASSERT(logdata != NULL);
    ASSERT(logdata->raw_event != NULL);

    if ( (nx_logdata_get_field_value(logdata, "Message", &msg) == TRUE) &&
	 (msg.type == NX_VALUE_TYPE_STRING) && (msg.defined == TRUE) )
    {
	// we have a Message field
    }
    else
    { // otherwise use raw_event
	tmpmsg = nx_string_clone(logdata->raw_event);
	msg.string = tmpmsg;
	msg.type = NX_VALUE_TYPE_STRING;
    }

    pri = nx_syslog_get_priority(logdata);

    if ( (nx_logdata_get_field_value(logdata, "EventTime", &timestamp) == TRUE) &&
	 (timestamp.type == NX_VALUE_TYPE_DATETIME) && (timestamp.defined == TRUE) )
    {
	nx_date_to_rfc3164(tmpstr, sizeof(tmpstr), timestamp.datetime);
    }
    else
    {
	nx_date_to_rfc3164(tmpstr, sizeof(tmpstr), apr_time_now());
    }

    nx_string_sprintf(logdata->raw_event, "<%d>%s ", pri, tmpstr);

    if ( (nx_logdata_get_field_value(logdata, "Hostname", &hostname) == TRUE) &&
	 (hostname.type == NX_VALUE_TYPE_STRING) && (hostname.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, hostname.string->buf, (int) hostname.string->len);
    }
    else
    {
	const nx_string_t *hoststr;

	hoststr = nx_get_hostname();
	nx_string_append(logdata->raw_event, hoststr->buf, (int) hoststr->len);
    }

    if ( (nx_logdata_get_field_value(logdata, "SourceName", &application) == TRUE) &&
	 (application.type == NX_VALUE_TYPE_STRING)  && (application.defined == TRUE) &&
	 (application.string->len > 0) )
    {
	nx_string_append(logdata->raw_event, " ", 1);
	i = (int) logdata->raw_event->len;
	nx_string_append(logdata->raw_event, application.string->buf, (int) application.string->len);
	for ( ; i < (int) logdata->raw_event->len; i++ )
	{ // replace space with underscore
	    if ( (logdata->raw_event->buf[i] == ' ') || (logdata->raw_event->buf[i] == '\t') )
	    {
		logdata->raw_event->buf[i] = '_';
	    }
	}

	if ( (nx_logdata_get_field_value(logdata, "ProcessID", &pid) == TRUE) &&
	     (pid.defined == TRUE) )
	{
	    if ( pid.type == NX_VALUE_TYPE_INTEGER )
	    {
		len = (size_t) apr_snprintf(tmpstr, sizeof(tmpstr), "[%"APR_INT64_T_FMT"]", pid.integer);
		nx_string_append(logdata->raw_event, tmpstr, (int) len);
	    }
	    else if ( pid.type == NX_VALUE_TYPE_STRING )
	    {
		nx_string_append(logdata->raw_event, "[", 1);
		nx_string_append(logdata->raw_event, pid.string->buf, (int) pid.string->len);
		nx_string_append(logdata->raw_event, "]", 1);
	    }
	}
	nx_string_append(logdata->raw_event, ":", 1);
    }

    // Append message
    i = (int) logdata->raw_event->len;
    nx_string_append(logdata->raw_event, " ", 1);
    nx_string_append(logdata->raw_event, msg.string->buf, (int) msg.string->len);
    for ( ; i < (int) logdata->raw_event->len; i++ )
    { // replace linebreaks with space
	if ( (logdata->raw_event->buf[i] == '\n') || (logdata->raw_event->buf[i] == '\r') )
	{
	    logdata->raw_event->buf[i] = ' ';
	}
    }

    if (tmpmsg != NULL)
    { // clean up temp copy
	nx_string_free(tmpmsg);
    }
}



static const char *syslog_parse_sd_params(nx_logdata_t *logdata,
					  const char *sd_name,
					  size_t sd_namelen,
					  const char *string)
{
    nx_string_t *param_value = NULL;
    char param_name[80];
    const char *ptr = string;
    int i;
    nx_value_t *value;

    if ( sd_namelen > 0 )
    {
	ASSERT(sd_namelen < sizeof(param_name) / 2);
	memcpy(param_name, sd_name, sd_namelen);
	param_name[sd_namelen] = '.';
	sd_namelen++;
    }
    for ( ; *ptr != '\0'; ptr++ )
    {
	for ( ; *ptr == ' '; ptr++ ); // skip space

	if ( *ptr == ']' )
	{
	    break;
	}

	// parse param-name
	for ( i = (int) sd_namelen; *ptr != '\0'; i++, ptr++ )
	{
	    if ( i >= (int) sizeof(param_name) - 1 )
	    {
		break;
	    }
	    if ( (*ptr == '=') || (*ptr == ' ') || (*ptr == '"') || (*ptr == ']') )
	    {
		break;
	    }
	    if ( IS_HOSTCHAR(*ptr) )
	    {
		param_name[i] = *ptr;
	    }
	    else
	    {
		param_name[i] = '_';
	    }
	}
	param_name[i] = '\0';
	if ( *ptr != '=' )
	{
	    return ( NULL );
	}
	ptr++;
	if ( *ptr != '"' )
	{
	    return ( NULL );
	}
	ptr++;

	param_value = nx_string_new_size(50);

	// parse param-value
	for ( i = 0; *ptr != '\0'; i++, ptr++ )
	{
	    //FIXME: this needs to be utf-8 aware
	    if ( *ptr == '\\' )
	    {
		if ( (ptr[1] == '\\') || (ptr[1] == '"') || (ptr[1] == ']') )
		{
		    ptr++;
		    nx_string_append(param_value, ptr, 1);
		}
	    }
	    else if ( *ptr == '"' )
	    {
		break;
	    }
	    else
	    {
		nx_string_append(param_value, ptr, 1);
	    }
	    if ( param_value->len >= 1024*64 )
	    { // 64K limit
		nx_string_free(param_value);
		return ( NULL );
	    }
	}

	if ( *ptr != '"' )
	{
	    if ( param_value != NULL )
	    {
		nx_string_free(param_value);
	    }
	    return ( NULL );
	}

	value = nx_value_new(NX_VALUE_TYPE_STRING);
	value->string = param_value;
	nx_logdata_set_field_value(logdata, param_name, value);
    }
    
    return ( ptr );
}



static const char *syslog_parse_structured_data(nx_logdata_t *logdata, const char *string)
{
    const char *ptr = string;
    char sd_name[40];
    int i;
    int sd_namelen;

    for ( ; *ptr != '\0'; ptr++ )
    {
	for ( ; *ptr == ' '; ptr++ ); // skip space
	if ( *ptr != '[' )
	{ // no SD-ELEMENT found
	    return ( ptr );
	}
	ptr++;
	for ( ; *ptr == ' '; ptr++ ); // skip space

	sd_namelen = 0;
	for ( i = 0; *ptr != '\0'; i++, ptr++ )
	{
	    if ( i >= (int) sizeof(sd_name) - 1 )
	    {
		break;
	    }
	    if ( (*ptr == '=') || (*ptr == ' ') || (*ptr == '"') || (*ptr == ']') )
	    {
		if ( sd_namelen == 0 )
		{
		    sd_namelen = i;
		}
		break;
	    }
	    if ( *ptr == '@' )
	    { // ignore enterprise number and the at mark 
		sd_namelen = i;
		sd_name[i] = '\0';
	    }
	    else if ( IS_HOSTCHAR(*ptr) )
	    {
		sd_name[i] = *ptr;
	    }
	    else
	    {
		sd_name[i] = '_';
	    }
	}
	sd_name[i] = '\0';

	if ( *ptr == '\0' )
	{
	    return ( string );
	}
	ptr++;

	if ( strcmp(sd_name, "NXLOG") == 0 )
	{
	    sd_name[0] = '\0';
	    sd_namelen = 0;
	}

	if ( (ptr = syslog_parse_sd_params(logdata, sd_name, (size_t) sd_namelen, ptr)) == NULL )
	{
	    return ( string );
	}
	for ( ; *ptr == ' '; ptr++ ); // skip space
	if ( *ptr != ']' )
	{ // missing closing bracket, invalid SD-ELEMENT
	    return ( string );
	}
    }

    return ( ptr );
}



#define IS_NILVALUE(PTR) ((PTR[0] == '-') && ((PTR[1] == ' ') || (PTR[1] == '\0')))

// PRI VERSION SP TIMESTAMP SP HOSTNAME SP APP-NAME SP PROCID SP MSGID SP STRUCTURED-DATA [SP MSG]
boolean nx_syslog_parse_rfc5424(nx_logdata_t *logdata,
				const char *string,
				size_t stringlen)
{
    boolean retval = TRUE;
    const char *ptr, *hoststart = NULL, *hostend = NULL;
    const char *appstart = NULL, *append = NULL;
    const char *msgstart = NULL, *msgend = NULL;
    const char *procidstart = NULL, *procidend = NULL;
    const char *msgidstart = NULL, *msgidend = NULL;
    apr_time_t date = 0;

    ASSERT(logdata != NULL);
    ASSERT(string != NULL);

    if ( stringlen <= 0 )
    {
	stringlen = strlen(string);
    }

    msgend = string + stringlen;
    // PRIORITY
    ptr = parse_syslog_priority(string, logdata, &retval);
    if ( (ptr[0] == '1') && (ptr[1] == ' ') )
    { // VERSION
	ptr += 2;
    }
    else
    { // fall back to bsd syslog
	return ( nx_syslog_parse_rfc3164(logdata, string, stringlen) );
    }
    msgstart = ptr;

    // TIMESTAMP
    if ( IS_NILVALUE(ptr) )
    {
	nx_logdata_set_datetime(logdata, "EventTime", apr_time_now());
	ptr++;
    }
    else
    {
	if ( nx_date_parse_iso(&date, ptr, &ptr) != APR_SUCCESS )
	{
	    nx_logdata_set_datetime(logdata, "EventTime", apr_time_now());
	    set_syslog_hostname(logdata, NULL, NULL);
	    set_syslog_message(logdata, msgstart, msgend);
	    return ( FALSE );
	}
	set_syslog_timestamp(logdata, date);
    }
    for ( ; *ptr == ' '; ptr++ ); // skip space

    // HOSTNAME
    if ( IS_NILVALUE(ptr) )
    {
	ptr++;
	set_syslog_hostname(logdata, NULL, NULL);
    }
    else
    {
	hoststart = ptr;
	for ( ; (*ptr != ' ') && (*ptr != '\0'); ptr++ );
	hostend = ptr;
	set_syslog_hostname(logdata, hoststart, hostend);
    }
    for ( ; *ptr == ' '; ptr++ ); // skip space

    // APP-NAME
    if ( IS_NILVALUE(ptr) )
    {
	ptr++;
    }
    else
    {
	appstart = ptr;
	for ( ; (*ptr != ' ') && (*ptr != '\0'); ptr++ );
	append = ptr;
	set_syslog_appname(logdata, appstart, append);
    }
    for ( ; *ptr == ' '; ptr++ ); // skip space

    // PROCID
    if ( IS_NILVALUE(ptr) )
    {
	ptr++;
    }
    else
    {
	procidstart = ptr;
	for ( ; (*ptr != ' ') && (*ptr != '\0'); ptr++ );
	procidend = ptr;
	set_syslog_procid(logdata, procidstart, procidend);
    }
    for ( ; *ptr == ' '; ptr++ ); // skip space

    // MSGID
    if ( IS_NILVALUE(ptr) )
    {
	ptr++;
    }
    else
    {
	msgidstart = ptr;
	for ( ; (*ptr != ' ') && (*ptr != '\0'); ptr++ );
	msgidend = ptr;
	set_syslog_msgid(logdata, msgidstart, msgidend);
    }
    for ( ; *ptr == ' '; ptr++ ); // skip space

    // STRUCTURED-DATA
    if ( IS_NILVALUE(ptr) )
    {
	ptr++;
    }
    else
    {
	ptr = syslog_parse_structured_data(logdata, ptr);
    }
    if ( *ptr == ' ' ) ptr++; // skip space

    // MESSAGE
    if ( (ptr[0] == 0xEF) && (ptr[1] == 0xBB) && (ptr[2] == 0xBF) )
    { //Skip UTF8 BOM
	ptr += 3;
    }
    msgstart = ptr;
    set_syslog_message(logdata, msgstart, msgend);

    return ( retval );
}



static void nx_syslog_add_structured_data(nx_logdata_t *logdata)
{
    nx_string_t *sd;
    nx_logdata_field_t *field;
    int cnt = 0;
    char *value = NULL;
    int i;
    uint32_t sdlen;
    static const char *ignorelist[] = 
    {
	"raw_event",
	"EventTime",
	"Hostname",
	"SourceName",
	"Message",
	"MessageID",
	"ProcessID",
	"Severity",
	"SeverityValue",
	"SyslogSeverity",
	"SyslogSeverityValue",
	"SyslogFacility",
	"SyslogFacilityValue",
	NULL,
    };
    boolean ignore;

    sd = nx_string_new();

    nx_string_append(sd, "[NXLOG@14506 ", -1);

    for ( field = NX_DLIST_FIRST(&(logdata->fields));
	  field != NULL;
	  field = NX_DLIST_NEXT(field, link) )
    {
	ignore = FALSE;
	for ( i = 0; ignorelist[i] != NULL; i++ )
	{
	    if ( strcasecmp(field->key, ignorelist[i]) == 0 )
	    {
		ignore = TRUE;
	    }
	}

	if ( (ignore == FALSE) && (field->value->defined == TRUE) )
	{
	    if ( cnt > 0 )
	    {
		nx_string_append(sd, " ", 1);
	    }
	    cnt++;
	    
	    sdlen = sd->len;
	    nx_string_append(sd, field->key, -1);
	    for ( ;  sdlen < sd->len; sdlen++ )
	    { // replace space, ], " with underscore in field names
		if ( (sd->buf[sdlen] == ' ') || (sd->buf[sdlen] == ']') || (sd->buf[sdlen] == '"') )
		{
		    sd->buf[sdlen] = '_';
		}
	    }

	    nx_string_append(sd, "=\"", 2);
	    if ( field->value->type == NX_VALUE_TYPE_STRING )
	    {
		for ( i = 0; i < (int) field->value->string->len; i++ )
		{
		    switch ( field->value->string->buf[i] )
		    {
			case '\\':
			case '"':
			case ']':
			    nx_string_append(sd, "\\", 1);
			    break;
			default:
			    break;
		    }
		    nx_string_append(sd, field->value->string->buf + i, 1);
		}
	    }
	    else
	    {
		value = nx_value_to_string(field->value);
		nx_string_append(sd, value, -1);
		free(value);
	    }
	    nx_string_append(sd, "\"", 1);
	}
    }

    nx_string_append(sd, "]", 1);

    if ( cnt > 0 )
    {
	nx_string_append(logdata->raw_event, sd->buf, (int) sd->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "-", 1);
    }
    nx_string_free(sd);
}



void nx_logdata_to_syslog_rfc5424(nx_logdata_t *logdata, boolean gmt)
{
    int pri = 0;
    nx_value_t hostname;
    nx_value_t timestamp;
    nx_value_t application;
    nx_value_t messageid;
    nx_value_t pid;
    nx_value_t msg;
    nx_string_t *tmpmsg = NULL;
    size_t len;
    char tmpstr[33];
    int i;

    ASSERT(logdata != NULL);
    ASSERT(logdata->raw_event != NULL);
    
    if ( (nx_logdata_get_field_value(logdata, "Message", &msg) == TRUE) &&
	 (msg.type == NX_VALUE_TYPE_STRING) && (msg.defined == TRUE) )
    {
	// we have a Message field
    }
    else
    { // otherwise use raw_event
	tmpmsg = nx_string_clone(logdata->raw_event);
	msg.string = tmpmsg;
	msg.type = NX_VALUE_TYPE_STRING;
    }

    pri = nx_syslog_get_priority(logdata);

    if ( (nx_logdata_get_field_value(logdata, "EventTime", &timestamp) == TRUE) &&
	 (timestamp.type == NX_VALUE_TYPE_DATETIME) && (timestamp.defined == TRUE) )
    {
	nx_date_to_rfc5424(tmpstr, sizeof(tmpstr), gmt, timestamp.datetime);
    }
    else
    {
	nx_date_to_rfc5424(tmpstr, sizeof(tmpstr), gmt, apr_time_now());
    }
    nx_string_sprintf(logdata->raw_event, "<%d>1 %s ", pri, tmpstr);

    if ( (nx_logdata_get_field_value(logdata, "Hostname", &hostname) == TRUE) &&
	 (hostname.type == NX_VALUE_TYPE_STRING) && (hostname.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, hostname.string->buf, (int) hostname.string->len);
    }
    else
    {
	const nx_string_t *hoststr;

	hoststr = nx_get_hostname();
	nx_string_append(logdata->raw_event, hoststr->buf, (int) hoststr->len);
    }
    nx_string_append(logdata->raw_event, " ", 1);

    if ( (nx_logdata_get_field_value(logdata, "SourceName", &application) == TRUE) &&
	 (application.type == NX_VALUE_TYPE_STRING) && (application.defined == TRUE) &&
	 (application.string->len > 0) )
    {
	i = (int) logdata->raw_event->len;
	nx_string_append(logdata->raw_event, application.string->buf, (int) application.string->len);
	for ( ; i < (int) logdata->raw_event->len; i++ )
	{ // replace space with underscore
	    if ( (logdata->raw_event->buf[i] == ' ') || (logdata->raw_event->buf[i] == '\t') )
	    {
		logdata->raw_event->buf[i] = '_';
	    }
	}
    }
    else
    {
	nx_string_append(logdata->raw_event, "-", 1);
    }
    nx_string_append(logdata->raw_event, " ", 1);

    if ( (nx_logdata_get_field_value(logdata, "ProcessID", &pid) == TRUE) &&
	 (pid.defined == TRUE) )
    {
	if ( pid.type == NX_VALUE_TYPE_INTEGER )
	{
	    len = (size_t) apr_snprintf(tmpstr, sizeof(tmpstr), "%"APR_INT64_T_FMT, pid.integer);
	    nx_string_append(logdata->raw_event, tmpstr, (int) len);
	}
	else if ( (pid.type == NX_VALUE_TYPE_STRING) && (pid.string->len > 0) )
	{
	    nx_string_append(logdata->raw_event, pid.string->buf, (int) pid.string->len);
	}
	else
	{
	    nx_string_append(logdata->raw_event, "-", 1);
	}
    }
    else
    {
	nx_string_append(logdata->raw_event, "-", 1);
    }
    nx_string_append(logdata->raw_event, " ", 1);
    
    if ( (nx_logdata_get_field_value(logdata, "MessageID", &messageid) == TRUE) &&
	 (messageid.type == NX_VALUE_TYPE_STRING) && (messageid.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, messageid.string->buf, (int) messageid.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "-", 1);
    }
    nx_string_append(logdata->raw_event, " ", 1);

    nx_syslog_add_structured_data(logdata);

    // Append message
    i = (int) logdata->raw_event->len;
    nx_string_append(logdata->raw_event, " ", 1);
    nx_string_append(logdata->raw_event, msg.string->buf, (int) msg.string->len);
    for ( ; i < (int) logdata->raw_event->len; i++ )
    { // replace linebreaks with space
	if ( (logdata->raw_event->buf[i] == '\n') || (logdata->raw_event->buf[i] == '\r') )
	{
	    logdata->raw_event->buf[i] = ' ';
	}
    }

    if (tmpmsg != NULL)
    { // clean up temp copy
	nx_string_free(tmpmsg);
    }
}



void nx_logdata_to_syslog_snare(nx_logdata_t *logdata,
				uint64_t evtcnt,
				char delimiter,
				char replacement)
{
    int pri = 0;
    nx_value_t hostname;
    nx_value_t timestamp;
    apr_time_t eventtime;
    char tmpstr[25];
    int i;
    nx_value_t tmpval;
    char delimiterstr[2] = { delimiter, '\0' };

    ASSERT(logdata != NULL);
    ASSERT(logdata->raw_event != NULL);

    pri = nx_syslog_get_priority(logdata);

    if ( (nx_logdata_get_field_value(logdata, "EventTime", &timestamp) == TRUE) &&
	 (timestamp.type == NX_VALUE_TYPE_DATETIME) && (timestamp.defined == TRUE) )
    {
	eventtime = timestamp.datetime;
	nx_date_to_rfc3164(tmpstr, sizeof(tmpstr), eventtime);
    }
    else
    {
	eventtime = apr_time_now();
	nx_date_to_rfc3164(tmpstr, sizeof(tmpstr), eventtime);
    }

    nx_string_sprintf(logdata->raw_event, "<%d>%s ", pri, tmpstr);

    // 1. Hostname
    if ( (nx_logdata_get_field_value(logdata, "Hostname", &hostname) == TRUE) &&
	 (hostname.type == NX_VALUE_TYPE_STRING) && (hostname.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, hostname.string->buf, (int) hostname.string->len);
    }
    else
    {
	const nx_string_t *hoststr;

	hoststr = nx_get_hostname();
	nx_string_append(logdata->raw_event, hoststr->buf, (int) hoststr->len);
    }
    nx_string_append(logdata->raw_event, " ", 1);

    // 2. Event Log Type
    nx_string_append(logdata->raw_event, "MSWinEventLog", -1);
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 3. Criticality
    if ( (nx_logdata_get_field_value(logdata, "SeverityValue", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_INTEGER) && (tmpval.defined == TRUE) )
    {
	// SeverityValue starts from 1 and Criticality is from 0
	apr_snprintf(tmpstr, sizeof(tmpstr), "%d", (int) tmpval.integer - 1); 
	nx_string_append(logdata->raw_event, tmpstr, -1);
    }
    else
    {
	nx_string_append(logdata->raw_event, "1", 1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    i = (int) logdata->raw_event->len;

    // 4. SourceName
    if ( (nx_logdata_get_field_value(logdata, "Channel", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    { // This one is for im_msvistalog
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else if ( (nx_logdata_get_field_value(logdata, "FileName", &tmpval) == TRUE) &&
	      (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    { // This one is for im_mseventlog
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 5. Snare Event Counter
    // use module->evt_recvd 
    apr_snprintf(tmpstr, sizeof(tmpstr), "%lu", evtcnt); 
    nx_string_append(logdata->raw_event, tmpstr, -1);
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 6. DateTime
    nx_date_to_rfc3164_wday_year(tmpstr, sizeof(tmpstr), eventtime, FALSE);
    nx_string_append(logdata->raw_event, tmpstr, -1);
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 7. EventID
    if ( (nx_logdata_get_field_value(logdata, "EventID", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_INTEGER) && (tmpval.defined == TRUE) )
    {
	apr_snprintf(tmpstr, sizeof(tmpstr), "%d", (int) tmpval.integer);
	nx_string_append(logdata->raw_event, tmpstr, -1);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 8. SourceName
    if ( (nx_logdata_get_field_value(logdata, "SourceName", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 9. UserName
    if ( (nx_logdata_get_field_value(logdata, "AccountName", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 10. SIDType
    if ( (nx_logdata_get_field_value(logdata, "AccountType", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 11. EventLogType
    if ( (nx_logdata_get_field_value(logdata, "EventType", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    {
	if ( strcmp(tmpval.string->buf, "AUDIT_SUCCESS") == 0 )
	{
	    nx_string_append(logdata->raw_event, "Success Audit", 13);
	}
	else if ( strcmp(tmpval.string->buf, "AUDIT_FAILURE") == 0 )
	{
	    nx_string_append(logdata->raw_event, "Failure Audit", 13);
	}
	else if ( strcmp(tmpval.string->buf, "INFO") == 0 )
	{
	    nx_string_append(logdata->raw_event, "Information", 11);
	}
	else if ( strcmp(tmpval.string->buf, "CRITICAL") == 0 )
	{
	    nx_string_append(logdata->raw_event, "Critical", 8);
	}
	else if ( strcmp(tmpval.string->buf, "ERROR") == 0 )
	{
	    nx_string_append(logdata->raw_event, "Error", 5);
	}
	else if ( strcmp(tmpval.string->buf, "WARNING") == 0 )
	{
	    nx_string_append(logdata->raw_event, "Warning", 7);
	}
	else if ( strcmp(tmpval.string->buf, "VERBOSE") == 0 )
	{
	    nx_string_append(logdata->raw_event, "Verbose", 7);
	}
	else
	{
	    nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
	}
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 12. ComputerName
    if ( (nx_logdata_get_field_value(logdata, "Hostname", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 13. CategoryString
    if ( (nx_logdata_get_field_value(logdata, "Category", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    {
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 14. DataString
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // 15. ExpandedString
    if ( (nx_logdata_get_field_value(logdata, "Message", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_STRING) && (tmpval.defined == TRUE) )
    {
	for ( i = 0; i < (int) tmpval.string->len; i++ )
	{
	    if ( tmpval.string->buf[i] == delimiter )
	    {
		tmpval.string->buf[i] = replacement;
	    }
	}
	nx_string_append(logdata->raw_event, tmpval.string->buf, (int) tmpval.string->len);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }
    nx_string_append(logdata->raw_event, delimiterstr, 1);

    // Eventlog Counter
    if ( (nx_logdata_get_field_value(logdata, "RecordNumber", &tmpval) == TRUE) &&
	 (tmpval.type == NX_VALUE_TYPE_INTEGER) && (tmpval.defined == TRUE) )
    {
	apr_snprintf(tmpstr, sizeof(tmpstr), "%d", (int) tmpval.integer);
	nx_string_append(logdata->raw_event, tmpstr, -1);
    }
    else
    {
	nx_string_append(logdata->raw_event, "N/A", -1);
    }


    for ( i = 0; i < (int) logdata->raw_event->len; i++ )
    { // replace linebreaks with space
	if ( (logdata->raw_event->buf[i] == '\n') || (logdata->raw_event->buf[i] == '\r') )
	{
	    logdata->raw_event->buf[i] = ' ';
	}
    }
}
