/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/date.h"

#include <apr_lib.h>

#include "om_dbi.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define OM_DBI_RECONNECT_INTERVAL 5

static apr_thread_mutex_t *connect_lock = NULL; // this is to prevent the race in mysql_init

static void om_dbi_add_reconnect_event(nx_module_t *module)
{
    nx_event_t *event;

    log_warn("om_dbi detected a disconnection, attempting to reconnect in %d seconds",
	     OM_DBI_RECONNECT_INTERVAL);
    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->type = NX_EVENT_RECONNECT;
    event->time = apr_time_now() + APR_USEC_PER_SEC * OM_DBI_RECONNECT_INTERVAL;
    event->priority = module->priority;
    nx_event_add(event);
}



static void om_dbi_error(nx_module_t *module, 
			 const char *fmt,
			 ...) PRINTF_FORMAT(2,3) NORETURN;


static void om_dbi_error(nx_module_t *module,
			 const char *fmt,
			 ...)
{
    char buf[NX_LOGBUF_SIZE];
    char dbibuf[NX_LOGBUF_SIZE];
    const char *dbimsg = NULL;
    va_list ap;
    int status;
    dbi_conn conn;
    nx_om_dbi_conf_t *omconf;

    omconf = (nx_om_dbi_conf_t *) module->config;
    conn = omconf->conn;

    va_start(ap, fmt);
    apr_vsnprintf(buf, NX_LOGBUF_SIZE, fmt, ap);
    va_end(ap);

    status = dbi_conn_error(conn, &dbimsg);

    // module_stop will cause dbimsg to be invalidated by libdbi so we copy the error message here.
    apr_snprintf(dbibuf, sizeof(dbibuf), " %s[errorcode: %d]", dbimsg == NULL ? "" : dbimsg, status);

    if ( dbimsg != NULL )
    {
	// this is a hack for postgresql disconnection which cannot be detected properly
	if ( strstr(dbimsg, "connection has been closed unexpectedly") != NULL )
	{
	    nx_module_stop_self(module);
	    om_dbi_add_reconnect_event(module);
	}
    }

    if ( (strcasecmp(omconf->driver, "mysql") == 0) && ((status >= 2000) && (status < 3000)) )
    { // mysql client errors
	nx_module_stop_self(module);
	om_dbi_add_reconnect_event(module);
    }

    throw_msg("%s.%s", buf, dbibuf);
}



static char *om_dbi_get_logdata_value(nx_module_t *module,
				      nx_logdata_t *logdata,
				      const char *varstart, 
				      const char *varend)
{
    char *retval = NULL;
    nx_value_t value;
    size_t len;
    dbi_conn conn;
    nx_om_dbi_conf_t *omconf;

    omconf = (nx_om_dbi_conf_t *) module->config;
    conn = omconf->conn;

    ASSERT(varstart <= varend);

    len = (size_t) (varend - varstart);

    if ( len <= 1 )
    {
	log_error("invalid variable in om_dbi SQL %s", varstart);
	return ( NULL );
    }
    else
    {
	char varname[len + 1];
	
	apr_cpystrn(varname, varstart + 1, len);
	varname[len - 1] = '\0';
	
	if ( (nx_logdata_get_field_value(logdata, varname, &value) != TRUE) ||
	     (value.defined == FALSE) )
	{
	    log_debug("logdata missing or undef '%s', setting to NULL", varname);
	    retval = strdup("NULL");
	}
	else
	{
	    ASSERT(value.defined == TRUE);
	    switch ( value.type )
	    {
		case NX_VALUE_TYPE_STRING:
		    retval = nx_value_to_string(&value);
		    if ( dbi_conn_quote_string(conn, &retval) <= 0 )
		    {
			om_dbi_error(module, "om_dbi couldn't quote string");
			free(retval);
			retval = strdup("NULL");
		    }
		    break;
		default:
		    retval = nx_value_to_string(&value);
	    }
	}
    }

    ASSERT(retval != NULL);
    return ( retval );
}



static char *om_dbi_get_sql(nx_module_t *module, const char *sqltemplate, nx_logdata_t *logdata)
{
    char *sql;
    unsigned int i, j;
    size_t len = NX_OM_DBI_DEFAULT_SQL_LENGTH;
    const char *varstart = NULL;
    const char *varend = NULL;
    char *value;
    size_t valuelen;

    sql = malloc(len);
    ASSERT(sql != NULL);

    for ( i = 0, j = 0; i < strlen(sqltemplate); i++ )
    {
	if ( j >= len )
	{
	    len = (len * 3) / 2;
	    sql = realloc(sql, len);
	    ASSERT(sql != NULL);
	}
	if ( varstart != NULL )
	{
	    if ( ! (apr_isalpha(sqltemplate[i]) || (sqltemplate[i] == '_')) )
	    {
		varend = &(sqltemplate[i]);
		value = om_dbi_get_logdata_value(module, logdata, varstart, varend);
		varstart = NULL;
		varend = NULL;
		valuelen = strlen(value);
		if ( j + valuelen + 1 >= len )
		{
		    len = ((len + valuelen) * 3) / 2;
		    sql = realloc(sql, len);
		    ASSERT(sql != NULL);
		}
		apr_cpystrn(sql + j, value, valuelen + 1);
		j += (unsigned int) valuelen;
		free(value);
		sql[j] = sqltemplate[i];
		j++;
	    }
	}
	else
	{
	    if ( sqltemplate[i] == '$' )
	    {
		varstart = &(sqltemplate[i]);
	    }
	    else
	    {
		sql[j] = sqltemplate[i];
		j++;
	    }
	}
    }

    sql[j] = '\0';

    ASSERT(len > j);

    return ( sql );
}



static void om_dbi_write(nx_module_t *module)
{
    nx_om_dbi_conf_t *omconf;
    nx_logdata_t *logdata = NULL;
    char * volatile sql;
    volatile dbi_result result;
    nx_exception_t e;

    log_debug("om_dbi_write");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not writing any more data", module->name);
	return;
    }

    omconf = (nx_om_dbi_conf_t *) module->config;
    ASSERT(omconf->sql != NULL);

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    sql = om_dbi_get_sql(module, omconf->sql, logdata);
    log_debug("om_dbi SQL: %s", sql);

    try
    {
	if ( (result = dbi_conn_query(omconf->conn, sql)) == NULL )
	{
	    om_dbi_error(module, "om_dbi failed to execute SQL statement \"%s\"", sql);
	}
    }
    catch(e)
    {
	free(sql);
	dbi_result_free(result);
	rethrow(e);
    }
    dbi_result_free(result);
    free(sql);

    nx_module_logqueue_pop(module, logdata);
    nx_logdata_free(logdata);
}



static boolean om_dbi_add_option(nx_module_t *module, char *optionstr)
{
    nx_om_dbi_option_t *option;
    nx_om_dbi_conf_t *omconf;
    char *ptr;

    omconf = (nx_om_dbi_conf_t *) module->config;

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
    log_debug("om_dbi option %s = %s", optionstr, ptr);

    option = apr_pcalloc(module->pool, sizeof(nx_om_dbi_option_t));
    option->name = optionstr;
    option->value = ptr;
/*
    if ( (strcmp(ptr, "\"\"") == 0) || (strcmp(ptr, "''") == 0) )
    {
	option->value = "";
    }
*/
    *((nx_om_dbi_option_t **)apr_array_push(omconf->options)) = option;

    return ( TRUE );
}



static void om_dbi_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_om_dbi_conf_t *omconf;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    omconf = apr_pcalloc(module->pool, sizeof(nx_om_dbi_conf_t));
    module->config = omconf;

    omconf->options = apr_array_make(module->pool, 5, sizeof(const nx_om_dbi_option_t *)); 

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "driver") == 0 )
	{
	    if ( omconf->driver != NULL )
	    {
		nx_conf_error(curr, "driver is already defined");
	    }
	    omconf->driver = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "option") == 0 )
	{
	    if ( om_dbi_add_option(module, curr->args) == FALSE )
	    {
		nx_conf_error(curr, "invalid option %s", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "SQL") == 0 )
	{
	    if ( omconf->sql != NULL )
	    {
		nx_conf_error(curr, "SQL is already defined");
	    }
	    omconf->sql = apr_pstrdup(module->pool, curr->args);
	    log_debug("SQL: %s", omconf->sql);
	}
	else
	{
	    nx_conf_error(curr, "invalid om_dbi keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( omconf->driver == NULL )
    {
	nx_conf_error(module->directives, "'Driver' missing for module om_dbi");
    }

    if ( omconf->sql == NULL )
    {
	omconf->sql = NX_OM_DBI_DEFAULT_SQL_TEMPLATE;
    }
}



static void om_dbi_init(nx_module_t *module)
{
    int retval;

    ASSERT(module != NULL);
    
#ifdef HAVE_DBI_INITIALIZE_R
    nx_om_dbi_conf_t *omconf;

    omconf = (nx_om_dbi_conf_t *) module->config;
    retval = dbi_initialize_r(NULL, &(omconf->dbi_inst));
#else
    retval = dbi_initialize(NULL);
#endif
    if ( retval == -1 )
    {
	throw_msg("dbi_initialize failed, no drivers present?");
    }

    if ( connect_lock == NULL )
    {
	CHECKERR(apr_thread_mutex_create(&connect_lock, APR_THREAD_MUTEX_UNNESTED, module->pool));
    }
}



static void om_dbi_connect(nx_module_t *module)
{
    nx_om_dbi_conf_t *omconf;
    int i;
    nx_om_dbi_option_t *option;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    omconf = (nx_om_dbi_conf_t *) module->config;

    ASSERT(omconf->driver != NULL);

#ifdef HAVE_DBI_INITIALIZE_R
    if ( (omconf->conn = dbi_conn_new_r(omconf->driver, omconf->dbi_inst)) == NULL )
    {
	throw_msg("om_dbi couldn't initialize libdbi driver '%s'", omconf->driver);
    }
#else
    if ( (omconf->conn = dbi_conn_new(omconf->driver)) == NULL )
    {
	throw_msg("om_dbi couldn't initialize libdbi driver '%s'", omconf->driver);
    }
#endif
    for ( i = 0; i < omconf->options->nelts; i++ )
    {
	option = ((nx_om_dbi_option_t **) omconf->options->elts)[i];
	if ( dbi_conn_set_option(omconf->conn, option->name, option->value) < 0 )
	{
	    throw_msg("couldn't set om_dbi option %s = %s", option->name, option->value);
	}
    }

    if ( dbi_conn_connect(omconf->conn) < 0 )
    {
	om_dbi_error(module, "om_dbi couldn't connect to the database, check the om_dbi Options");
    }
    log_info("successfully connected to database");
}



static void om_dbi_start(nx_module_t *module)
{
    nx_exception_t e;
    nx_om_dbi_conf_t *omconf;

    ASSERT(module != NULL);
    omconf = (nx_om_dbi_conf_t *) module->config;

    if ( strcasecmp(omconf->driver, "mysql") == 0 )
    { // this is a workaround for mysql_init not being reentrant
	CHECKERR(apr_thread_mutex_lock(connect_lock));
	try
	{
	    om_dbi_connect(module);
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
	om_dbi_connect(module);
    }
}



static void om_dbi_stop(nx_module_t *module)
{
    nx_om_dbi_conf_t *omconf;

    ASSERT(module != NULL);

    omconf = (nx_om_dbi_conf_t *) module->config;

    dbi_conn_close(omconf->conn);
}



static void om_dbi_shutdown(nx_module_t *module UNUSED)
{
#ifdef HAVE_DBI_INITIALIZE_R
    nx_om_dbi_conf_t *omconf;

    ASSERT(module != NULL);

    omconf = (nx_om_dbi_conf_t *) module->config;

    dbi_shutdown_r(omconf->dbi_inst);
#else
    dbi_shutdown();
#endif
}



static void om_dbi_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    om_dbi_write(module);
	    break;
	case NX_EVENT_RECONNECT:
	    nx_module_start_self(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_om_dbi_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_dbi_config,		// config
    om_dbi_start,		// start
    om_dbi_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_dbi_init,		// init
    om_dbi_shutdown,		// shutdown
    om_dbi_event,		// event
    NULL,			// info
    NULL,			// exports
};

