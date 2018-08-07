/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <unistd.h>
#include <apr_lib.h>

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/config_cache.h"


#include "im_dbi.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_DBI_DEFAULT_POLL_INTERVAL 1

static apr_thread_mutex_t *connect_lock = NULL; // this is to prevent the race in mysql_init


static void im_dbi_error(dbi_conn conn, const char *errormsg) NORETURN;
static void im_dbi_error(dbi_conn conn, const char *errormsg)
{
    const char *dbimsg = NULL;

    dbi_conn_error(conn, &dbimsg);
    if ( dbimsg != NULL )
    {
	throw_msg("%s. %s", errormsg, dbimsg);
    }
    else
    {
	throw_msg("%s.", errormsg);
    }
}



static boolean im_dbi_add_option(nx_module_t *module, char *optionstr)
{
    nx_im_dbi_option_t *option;
    nx_im_dbi_conf_t *imconf;
    char *ptr;

    imconf = (nx_im_dbi_conf_t *) module->config;

    for ( ptr = optionstr; (*ptr != '\0') && (!apr_isspace(*ptr)); ptr++ );
    while ( apr_isspace(*ptr) )
    {
	*ptr = '\0';
	ptr++;
    }

    if ( *ptr == '\0' )
    {
	return ( FALSE );
    }
    log_debug("im_dbi option %s = %s", optionstr, ptr);

    option = apr_pcalloc(module->pool, sizeof(nx_im_dbi_option_t));
    option->name = optionstr;
    option->value = ptr;

    *((nx_im_dbi_option_t **)apr_array_push(imconf->options)) = option;

    return ( TRUE );
}



static const char *dbi_type_to_string(int type)
{
    switch ( type )
    {
	case DBI_TYPE_INTEGER:
	    return "integer";
	case DBI_TYPE_STRING:
	    return "string";
	case DBI_TYPE_DATETIME:
	    return "datetime";
	case DBI_TYPE_DECIMAL:
	    return "decimal";
	case DBI_TYPE_BINARY:
	    return "binary";
	default:
	    return "unknown";
    }
}



static void im_dbi_set_logdata_field(nx_module_t *module,
				     nx_logdata_t *logdata,
				     dbi_result result,
				     unsigned int fieldidx)
{
    nx_im_dbi_conf_t *imconf;
    int fieldtype;
    const char *fieldname;
    nx_value_t *val;
    const char *str;

    if ( (fieldname = dbi_result_get_field_name(result, fieldidx)) == NULL )
    {
	throw_msg("im_dbi failed to get field name for %u", fieldidx);
    }

    if ( (fieldtype = dbi_result_get_field_type_idx(result, fieldidx)) == DBI_TYPE_ERROR )
    {
	throw_msg("im_dbi failed to get field type for %s", fieldname);
    }

    // TODO:
    // For integers we need to check the size with dbi_result_get_field_attribs_idx()
    // and test for these:
    // #define DBI_INTEGER_UNSIGNED    (1 << 0)
    // #define DBI_INTEGER_SIZE1               (1 << 1)
    // #define DBI_INTEGER_SIZE2               (1 << 2)
    // #define DBI_INTEGER_SIZE3               (1 << 3)
    // #define DBI_INTEGER_SIZE4               (1 << 4)
    // #define DBI_INTEGER_SIZE8               (1 << 5)

    if ( strcasecmp(fieldname, "id") == 0 )
    {
	if ( fieldtype != DBI_TYPE_INTEGER )
	{
	    throw_msg("dbi type 'integer' required for field '%s', got '%s'",
		      fieldname, dbi_type_to_string(fieldtype));
	}
	imconf = (nx_im_dbi_conf_t *) module->config;
	imconf->last_id = dbi_result_get_longlong_idx(result, fieldidx);

	return;
    }

    switch ( fieldtype )
    {
	case DBI_TYPE_INTEGER:
	    val = nx_value_new_integer(dbi_result_get_longlong_idx(result, fieldidx));
	    nx_logdata_append_field_value(logdata, fieldname, val);
	    break;
	case DBI_TYPE_STRING:
	    str = dbi_result_get_string_idx(result, fieldidx);
	    if ( str != NULL )
	    {
		val = nx_value_new_string(str);
		nx_logdata_append_field_value(logdata, fieldname, val);
	    }
	    break;
	case DBI_TYPE_BINARY:
	    str = (const char *) dbi_result_get_binary_idx(result, fieldidx);
	    if ( str != NULL )
	    {
		val = nx_value_new_string(str); //FIXME new_binary
		// FIXME: validate string
		nx_logdata_append_field_value(logdata, fieldname, val);
	    }
	    break;
	case DBI_TYPE_DATETIME:
	{
	    time_t timeval = dbi_result_get_datetime_idx(result, fieldidx);
	    if ( timeval != 0 )
	    {
		val = nx_value_new_datetime((apr_time_t) (timeval * APR_USEC_PER_SEC));
		nx_logdata_append_field_value(logdata, fieldname, val);
	    }
	    else
	    {
		log_error("im_dbi failed to retrieve datetime value for column '%s'", fieldname);
	    }
	    break;
	}
	default:
	    throw_msg("invalid/unsupported type for database column %s", fieldname);
    }
}



static void im_dbi_read(nx_module_t *module)
{
    nx_event_t *event;
    nx_im_dbi_conf_t *imconf;
    nx_logdata_t *logdata;
    unsigned long long i;
    volatile unsigned long long numrows;
    unsigned int j;
    unsigned int numfields;
    volatile dbi_result result;
    const char *limit = "";
    nx_exception_t e;

    ASSERT(module != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;
    imconf->event = NULL;

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    if ( (strcasecmp(imconf->driver, "mysql") == 0) || 
	 (strcasecmp(imconf->driver, "pgsql") == 0) )
    {
	limit = " LIMIT 10";
    }
    apr_snprintf(imconf->_sql, imconf->_sql_bufsize, "%s WHERE id > %"APR_INT64_T_FMT"%s", 
		 imconf->sql, imconf->last_id, limit);

    log_debug("im_dbi sql: %s", imconf->_sql);

    try
    {
	if ( (result = dbi_conn_query(imconf->conn, imconf->_sql)) == NULL )
	{
	    im_dbi_error(imconf->conn, "im_dbi failed to execute SQL statement");
	}

	numrows = dbi_result_get_numrows(result);
	log_debug("im_dbi read %lu rows", (long unsigned int) numrows);

	if ( numrows > 0 )
	{
	    if ( dbi_result_first_row(result) != 1 )
	    {
		im_dbi_error(imconf->conn, "im_dbi failed to get first row");
	    }
	}

	for ( i = 0; i < numrows; i++, dbi_result_next_row(result) )
	{
	    if ( (numfields = dbi_result_get_numfields(result)) == DBI_FIELD_ERROR )
	    {
		im_dbi_error(imconf->conn, "im_dbi failed to query field number");
	    }
	    //log_debug("im_dbi got %d fields", numfields);
	    logdata = nx_logdata_new();
	    for ( j = 1; j <= numfields; j++ )
	    {
		im_dbi_set_logdata_field(module, logdata, result, j);
	    }
	    //FIXME: enable templates
	    nx_module_add_logdata_input(module, NULL, logdata);
	}
    }
    catch(e)
    {
	dbi_result_free(result);
	rethrow(e);
    }

    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->delayed = TRUE;
    event->type = NX_EVENT_READ;
    event->priority = module->priority;
    if ( numrows >= 10 ) // = LIMIT
    {
	event->time = apr_time_now();
    }
    else
    {
	event->time = apr_time_now() + (apr_time_t) (APR_USEC_PER_SEC * imconf->poll_interval);
    }
    nx_event_add(event);
}



static void im_dbi_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_dbi_conf_t *imconf;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_dbi_conf_t));
    module->config = imconf;

    imconf->options = apr_array_make(module->pool, 5, sizeof(const nx_im_dbi_option_t *)); 

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "driver") == 0 )
	{
	    if ( imconf->driver != NULL )
	    {
		nx_conf_error(curr, "driver is already defined");
	    }
	    imconf->driver = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "option") == 0 )
	{
	    if ( im_dbi_add_option(module, curr->args) == FALSE )
	    {
		nx_conf_error(curr, "invalid option %s", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "SQL") == 0 )
	{
	    if ( imconf->sql != NULL )
	    {
		nx_conf_error(curr, "SQL is already defined");
	    }
	    imconf->sql = apr_pstrdup(module->pool, curr->args);
	    log_debug("SQL: %s", imconf->sql);
	}
	else if ( strcasecmp(curr->directive, "savepos") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "PollInterval") == 0 )
	{
	    if ( sscanf(curr->args, "%f", &(imconf->poll_interval)) != 1 )
	    {
		nx_conf_error(curr, "invalid PollInterval: %s", curr->args);
            }
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( imconf->driver == NULL )
    {
	nx_conf_error(module->directives, "'Driver' missing for module im_dbi");
    }

    imconf->savepos = TRUE;
    nx_cfg_get_boolean(module->directives, "savepos", &(imconf->savepos));

    if ( imconf->sql == NULL )
    {
	imconf->sql = NX_IM_DBI_DEFAULT_SQL_TEMPLATE;
    }
    imconf->_sql_bufsize = strlen(imconf->sql) + 100;
    imconf->_sql = apr_palloc(module->pool, imconf->_sql_bufsize);

    if ( imconf->poll_interval == 0 )
    {
	imconf->poll_interval = IM_DBI_DEFAULT_POLL_INTERVAL;
    }
}



static void im_dbi_init(nx_module_t *module)
{
    int retval;
    nx_im_dbi_conf_t *imconf;
    int i;
    nx_im_dbi_option_t *option;

    ASSERT(module->config != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;

#ifdef HAVE_DBI_INITIALIZE_R
    retval = dbi_initialize_r(NULL, &(imconf->dbi_inst));
#else
    retval = dbi_initialize(NULL);
#endif
    if ( retval == -1 )
    {
	throw_msg("dbi_initialize failed, no drivers present?");
    }

    ASSERT(imconf->driver != NULL);
#ifdef HAVE_DBI_INITIALIZE_R
    if ( (imconf->conn = dbi_conn_new_r(imconf->driver, imconf->dbi_inst)) == NULL )
    {
	throw_msg("im_dbi couldn't initialize libdbi driver '%s'", imconf->driver);
    }
#else
    if ( (imconf->conn = dbi_conn_new(imconf->driver)) == NULL )
    {
	throw_msg("im_dbi couldn't initialize libdbi driver '%s'", imconf->driver);
    }
#endif

    for ( i = 0; i < imconf->options->nelts; i++ )
    {
	option = ((nx_im_dbi_option_t **) imconf->options->elts)[i];
	if ( dbi_conn_set_option(imconf->conn, option->name, option->value) < 0 )
	{
	    throw_msg("couldn't set im_dbi option %s = %s", option->name, option->value);
	}
    }

    if ( connect_lock == NULL )
    {
	CHECKERR(apr_thread_mutex_create(&connect_lock, APR_THREAD_MUTEX_UNNESTED, module->pool));
    }
}



static void im_dbi_test_savedpos(nx_module_t *module)
{
    nx_im_dbi_conf_t *imconf;
    unsigned long long numrows;
    dbi_result result;

    ASSERT(module->config != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;

    apr_snprintf(imconf->_sql, imconf->_sql_bufsize, "%s WHERE id = %" APR_INT64_T_FMT, 
		 imconf->sql, imconf->last_id);

    log_debug("im_dbi sql: %s", imconf->_sql);

    if ( (result = dbi_conn_query(imconf->conn, imconf->_sql)) == NULL )
    {
	imconf->last_id = -1;
	im_dbi_error(imconf->conn, "im_dbi failed to execute SQL statement");
    }

    numrows = dbi_result_get_numrows(result);

    if ( numrows == 0 )
    {
	log_warn("saved id %"APR_INT64_T_FMT" not found in database, restarting from 0",
		 imconf->last_id);
	imconf->last_id = -1;
    }

    dbi_result_free(result);
}



static void im_dbi_start(nx_module_t *module)
{
    nx_im_dbi_conf_t *imconf;
    nx_event_t *event;
    nx_exception_t e;

    ASSERT(module->config != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;
  
    if ( imconf->savepos == TRUE )
    {
	if ( nx_config_cache_get_int(module->name, "savepos", &(imconf->last_id)) == FALSE )
	{
	    imconf->last_id = -1;  
	}
    }
    else
    {
	imconf->last_id = -1;
    }

    if ( strcasecmp(imconf->driver, "mysql") == 0 )
    { // this is a workaround for mysql_init not being reentrant
	CHECKERR(apr_thread_mutex_lock(connect_lock));
	try
	{
	    if ( dbi_conn_connect(imconf->conn) < 0 )
	    {
		im_dbi_error(imconf->conn, "im_dbi couldn't connect to the database, check the im_dbi Options");
	    }
	}
	catch(e)
	{
	    CHECKERR(apr_thread_mutex_unlock(connect_lock));
	    rethrow(e);
	}
	CHECKERR(apr_thread_mutex_unlock(connect_lock));
    }
    else
    {
	if ( dbi_conn_connect(imconf->conn) < 0 )
	{
	    im_dbi_error(imconf->conn, "im_dbi couldn't connect to the database, check the im_dbi Options");
	}
    }

    log_debug("module %s started (libdbi version %s/%s)", module->name, dbi_version(),
	      dbi_driver_get_version(dbi_conn_get_driver(imconf->conn)));

    if ( imconf->last_id != -1 )
    {
	im_dbi_test_savedpos(module);
    }

    ASSERT(imconf->event == NULL);
    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->delayed = FALSE;
    event->type = NX_EVENT_READ;
    event->priority = module->priority;
    nx_event_add(event);
}



static void im_dbi_stop(nx_module_t *module)
{
    nx_im_dbi_conf_t *imconf;

    ASSERT(module != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;

    if ( imconf->savepos == TRUE )
    {
	nx_config_cache_set_int(module->name, "savepos", imconf->last_id);
    }
    dbi_conn_close(imconf->conn);
    imconf->conn = NULL;
}



static void im_dbi_shutdown(nx_module_t *module UNUSED)
{
#ifdef HAVE_DBI_INITIALIZE_R
    nx_im_dbi_conf_t *imconf;

    ASSERT(module != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;

    dbi_shutdown_r(imconf->dbi_inst);
#else
    dbi_shutdown();
#endif
}



static void im_dbi_pause(nx_module_t *module)
{
    nx_im_dbi_conf_t *imconf;

    ASSERT(module != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;
    
    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}



static void im_dbi_resume(nx_module_t *module)
{
    nx_im_dbi_conf_t *imconf;
    nx_event_t *event;

    ASSERT(module != NULL);

    imconf = (nx_im_dbi_conf_t *) module->config;
    
    if ( imconf->event == NULL )
    {
	event = nx_event_new();
	imconf->event = event;
	event->module = module;
	event->delayed = FALSE;
	event->type = NX_EVENT_READ;
	event->priority = module->priority;
	nx_event_add(event);
    }
}



static void im_dbi_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_dbi_read(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_im_dbi_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    im_dbi_config,		// config
    im_dbi_start,		// start
    im_dbi_stop, 		// stop
    im_dbi_pause,		// pause
    im_dbi_resume,		// resume
    im_dbi_init,		// init
    im_dbi_shutdown,		// shutdown
    im_dbi_event,		// event
    NULL,			// info
    NULL,			// exports
};
