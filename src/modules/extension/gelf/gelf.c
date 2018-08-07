/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "gelf.h"
#include "../../../common/exception.h"
#include "../../../common/module.h"

#include "../../extension/json/yajl/api/yajl_gen.h"
#include "../../extension/json/yajl/api/yajl_parse.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


nx_string_t *nx_logdata_to_gelf(nx_gelf_ctx_t *ctx)
{

    const unsigned char *json;
    size_t jsonlen;
    yajl_gen gen;
    nx_logdata_field_t *field;
    nx_string_t *retval;
    char *value;
    const nx_value_t *host = NULL;
    const nx_value_t *message = NULL;
    const nx_value_t *short_message = NULL;
    const nx_value_t *full_message = NULL;
    const nx_value_t *rcvdtime = NULL;
    const nx_value_t *eventtime = NULL;
    const nx_value_t *severity = NULL;
    const nx_value_t *syslogseverity = NULL;
    //const nx_value_t *syslogfacility = NULL;
    //const nx_value_t *sourcename = NULL;
    char key[128];
    size_t keylen;
    apr_time_t timestamp;

    gen = yajl_gen_alloc(NULL);
    yajl_gen_map_open(gen);
    
    // version: GELF spec version – "1.0" (string); MUST be set by client library.
    ASSERT(yajl_gen_string(gen, (const unsigned char *) "version", 7) == yajl_gen_status_ok);
    ASSERT(yajl_gen_string(gen, (const unsigned char *) "1.1", 3) == yajl_gen_status_ok);

    for ( field = NX_DLIST_FIRST(&(ctx->logdata->fields));
	  field != NULL;
	  field = NX_DLIST_NEXT(field, link) )
    {
	if ( strcmp(field->key, "raw_event") == 0 )
	{
	    continue;
	}
	if ( (field->key[0] == '.') || (field->key[0] == '_') )
	{
	    continue;
	}

	// Libraries SHOULD not allow to send id as additional field (_id), server MUST skip
	// the field because it could ovveride the MongoDB _key field else.
	if ( strcmp(field->key, "id") == 0 )
	{
	    continue;
	}

	if ( field->value->defined == FALSE )
	{
	    continue;
	}

	if ( strcmp(field->key, "Hostname") == 0 )
	{
	    host = field->value;
	    continue;
	}
	else if ( strcmp(field->key, "Message") == 0 )
	{
	    message = field->value;
	    continue;
	}
	else if ( strcmp(field->key, "EventTime") == 0 )
	{
	    eventtime = field->value;
	    continue;
	}
	else if ( strcmp(field->key, "EventReceivedTime") == 0 )
	{
	    rcvdtime = field->value;
	}
	else if ( strcmp(field->key, "SeverityValue") == 0 )
	{
	    severity = field->value;
	}
	else if ( strcmp(field->key, "SyslogSeverityValue") == 0 )
	{
	    syslogseverity = field->value;
	}
/*
	else if ( strcmp(field->key, "SyslogFacility") == 0 )
	{
	    syslogfacility = field->value;
	}
	else if ( strcmp(field->key, "SourceName") == 0 )
	{
	    sourcename = field->value;
	}
*/
	else if ( strcasecmp(field->key, "ShortMessage") == 0 )
	{
	    short_message = field->value;
	    continue;
	}
	else if ( strcasecmp(field->key, "Short_Message") == 0 )
	{
	    short_message = field->value;
	    continue;
	}
	else if ( strcasecmp(field->key, "FullMessage") == 0 )
	{
	    full_message = field->value;
	    continue;
	}
	else if ( strcasecmp(field->key, "Full_Message") == 0 )
	{
	    full_message = field->value;
	    continue;
	}

	// Additional GELF fields are prepended with _ to avoid collisions. 
	// Client libraries are expected to automatically fill some of those fields.
	key[0] = '_';
	keylen = (size_t) (apr_cpystrn(key + 1, field->key, sizeof(key) - 1) - key);
	ASSERT(yajl_gen_string(gen, (const unsigned char *) key, keylen) == yajl_gen_status_ok);

	switch ( field->value->type )
	{
	    case NX_VALUE_TYPE_BOOLEAN:
		ASSERT(yajl_gen_bool(gen, (int) field->value->boolean) == yajl_gen_status_ok);
		break;
	    case NX_VALUE_TYPE_INTEGER:
		ASSERT(yajl_gen_integer(gen, (long long) field->value->integer) == yajl_gen_status_ok);
		break;
	    case NX_VALUE_TYPE_STRING:
		ASSERT(yajl_gen_string(gen, (const unsigned char *) field->value->string->buf,
				       field->value->string->len) == yajl_gen_status_ok);
		break;
	    default:
		value = nx_value_to_string(field->value);
		ASSERT(yajl_gen_string(gen, (const unsigned char *) value,
				       strlen(value)) == yajl_gen_status_ok);
		free(value);
		break;
	}
    }

    // host: the name of the host or application that sent this message (string); 
    // MUST be set by client library.
    ASSERT(yajl_gen_string(gen, (const unsigned char *) "host", 4) == yajl_gen_status_ok);
    if ( (host != NULL) && (host->type == NX_VALUE_TYPE_STRING) )
    {
	ASSERT(yajl_gen_string(gen, (const unsigned char *) host->string->buf,
			       host->string->len) == yajl_gen_status_ok);
    }
    else
    {
	const nx_string_t *hoststr = nx_get_hostname();

	ASSERT(yajl_gen_string(gen, (const unsigned char *) hoststr->buf,
			       hoststr->len) == yajl_gen_status_ok);
    }

    // short_message: a short descriptive message (string);
    // MUST be set by client library.
    
    if ( (short_message != NULL) && (short_message->type == NX_VALUE_TYPE_STRING) )
    { // if ShortMessage exists use that
	ASSERT(yajl_gen_string(gen, (const unsigned char *) "short_message", 13) == yajl_gen_status_ok);
	ASSERT(yajl_gen_string(gen, (const unsigned char *) short_message->string->buf, short_message->string->len) == yajl_gen_status_ok);
    }
    else
    { // use Message or raw_event and truncate as per ShortMessageLength
	char *shortmsg;
	uint32_t shortmsglen = ctx->shortmessagelength;

	if ( (message != NULL) && (message->type == NX_VALUE_TYPE_STRING) )
	{
	    shortmsg = message->string->buf;
	    if ( shortmsglen > message->string->len )
	    {
		shortmsglen = message->string->len;
	    }
	}
	else
	{
	    shortmsg = ctx->logdata->raw_event->buf;
	    if ( shortmsglen > ctx->logdata->raw_event->len )
	    {
		shortmsglen = ctx->logdata->raw_event->len;
	    }
	}

	ASSERT(ctx->logdata->raw_event != NULL);
	ASSERT(yajl_gen_string(gen, (const unsigned char *) "short_message", 13) == yajl_gen_status_ok);
	ASSERT(yajl_gen_string(gen, (const unsigned char *) shortmsg, shortmsglen) == yajl_gen_status_ok);
    }

    // full_message: a long message that can i.e. contain a backtrace and environment 
    // variables (string); optional.
    if ( full_message == NULL )
    { // Message is treated as FullMessage if the latter does not exist
	full_message = message;
    }
    if ( (full_message != NULL) && (full_message->type == NX_VALUE_TYPE_STRING) )
    {
	ASSERT(yajl_gen_string(gen, (const unsigned char *) "full_message", 12) == yajl_gen_status_ok);
	ASSERT(yajl_gen_string(gen, (const unsigned char *) full_message->string->buf,  full_message->string->len) == yajl_gen_status_ok);
    }

    // timestamp: UNIX microsecond timestamp (decimal); SHOULD be set by client library.
    if ( (eventtime != NULL) && (eventtime->type == NX_VALUE_TYPE_DATETIME) )
    {
	timestamp = eventtime->datetime;
    }
    else if ( (rcvdtime != NULL) && (rcvdtime->type == NX_VALUE_TYPE_DATETIME) )
    {
	timestamp = rcvdtime->datetime;
    }
    else
    {
	timestamp = apr_time_now();
    }
    timestamp /= 1000000;

    ASSERT(yajl_gen_string(gen, (const unsigned char *) "timestamp", 9) == yajl_gen_status_ok);
    ASSERT(yajl_gen_integer(gen, (long long) timestamp) == yajl_gen_status_ok);

    // level: the level equal to the standard syslog levels (decimal); optional, default is 1 (ALERT).
    ASSERT(yajl_gen_string(gen, (const unsigned char *) "level", 5) == yajl_gen_status_ok);
    if ( (syslogseverity != NULL) && (syslogseverity->type == NX_VALUE_TYPE_INTEGER) )
    {
	ASSERT(yajl_gen_integer(gen, (long long) syslogseverity->integer) == yajl_gen_status_ok);
    }
    else if ( (severity != NULL) && (severity->type == NX_VALUE_TYPE_INTEGER) )
    {
	switch ( severity->integer )
	{
	    case NX_LOGLEVEL_DEBUG:
		ASSERT(yajl_gen_integer(gen, (long long) 7) == yajl_gen_status_ok);
		break;
	    case NX_LOGLEVEL_INFO:
		ASSERT(yajl_gen_integer(gen, (long long) 6) == yajl_gen_status_ok);
		break;
	    case NX_LOGLEVEL_WARNING:
		ASSERT(yajl_gen_integer(gen, (long long) 4) == yajl_gen_status_ok);
		break;
	    case NX_LOGLEVEL_ERROR:
		ASSERT(yajl_gen_integer(gen, (long long) 3) == yajl_gen_status_ok);
		break;
	    case NX_LOGLEVEL_CRITICAL:
		ASSERT(yajl_gen_integer(gen, (long long) 2) == yajl_gen_status_ok);
		break;
	    default:
		// set level:6 (INFO) by default
		ASSERT(yajl_gen_integer(gen, (long long) 6) == yajl_gen_status_ok);
		break;
	}
    }
    else
    { // set level:6 (INFO) by default
	ASSERT(yajl_gen_integer(gen, (long long) 6) == yajl_gen_status_ok);
    }

/* Facility is deprecated in GELF 1.1
    // facility: (string or decimal) optional, MUST be set by server to GELF if empty.
    ASSERT(yajl_gen_string(gen, (const unsigned char *) "facility", 8) == yajl_gen_status_ok);
    if ( (sourcename != NULL) && (sourcename->type == NX_VALUE_TYPE_STRING) )
    {
	ASSERT(yajl_gen_string(gen, (const unsigned char *) sourcename->string->buf,
			       sourcename->string->len) == yajl_gen_status_ok);
    }
    else if ( (syslogfacility != NULL) && (syslogfacility->type == NX_VALUE_TYPE_STRING) )
    {
	ASSERT(yajl_gen_string(gen, (const unsigned char *) syslogfacility->string->buf,
			       syslogfacility->string->len) == yajl_gen_status_ok);
    }
    else
    {
	ASSERT(yajl_gen_string(gen, (const unsigned char *) "NXLOG", 5) == yajl_gen_status_ok);
    }
*/

    yajl_gen_map_close(gen);
    yajl_gen_get_buf(gen, &json, &jsonlen);
    
    retval = nx_string_create((const char *) json, (int) jsonlen);

    yajl_gen_free(gen);

    return ( retval );
}
