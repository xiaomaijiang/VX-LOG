/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "patterndb.h"
#include "../../../common/exception.h"
#include "../../../common/alloc.h"
#include "../../../common/expr-parser.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define NX_PATTERNDB_MAX_CAPTURED_FIELDS 100

nx_patterndb_t *nx_patterndb_new(apr_pool_t *pool)
{
    nx_patterndb_t *retval = NULL;
    apr_pool_t *tmppool = NULL;

    log_debug("nx_patterndb_new");

    tmppool = nx_pool_create_child(pool);
    retval = apr_pcalloc(tmppool, sizeof(nx_patterndb_t));
    retval->groups = apr_pcalloc(tmppool, sizeof(nx_patterngroups_t));
    ASSERT(retval->groups != NULL);
    NX_DLIST_INIT(retval->groups, nx_patterngroups_t, link);
    retval->pool = tmppool;

    return ( retval );
}



nx_patterngroup_t *nx_patterngroup_new(nx_patterndb_t *patterndb)
{
    nx_patterngroup_t *retval;

    ASSERT(patterndb != NULL);

    retval = apr_pcalloc(patterndb->pool, sizeof(nx_patterngroup_t));
    retval->patterns = apr_pcalloc(patterndb->pool, sizeof(nx_patterns_t));
    retval->matchfields = apr_pcalloc(patterndb->pool, sizeof(nx_pattern_matchfields_t));
    NX_DLIST_INIT(retval->matchfields, nx_pattern_matchfields_t, link);
    retval->patterndb = patterndb;
    NX_DLIST_INIT(retval->patterns, nx_patterns_t, link);

    return ( retval );
}



nx_pattern_t *nx_pattern_new(nx_patterndb_t *patterndb)
{
    nx_pattern_t *retval;

    ASSERT(patterndb != NULL);

    retval = apr_pcalloc(patterndb->pool, sizeof(nx_pattern_t));
    retval->matchfields = apr_pcalloc(patterndb->pool, sizeof(nx_pattern_matchfields_t));
    NX_DLIST_INIT(retval->matchfields, nx_pattern_matchfields_t, link);

    return ( retval );
}



nx_pattern_matchfield_t *nx_pattern_matchfield_new(nx_patterndb_t *patterndb)
{
    nx_pattern_matchfield_t *retval;

    ASSERT(patterndb != NULL);

    retval = apr_pcalloc(patterndb->pool, sizeof(nx_pattern_matchfield_t));
    retval->capturedfields = apr_pcalloc(patterndb->pool, sizeof(nx_pattern_capturedfields_t));
    NX_DLIST_INIT(retval->capturedfields, nx_pattern_capturedfields_t, link);

    return ( retval );
}



void nx_patterngroup_add_pattern(nx_patterngroup_t *group, nx_pattern_t *pattern)
{
    ASSERT(group != NULL);
    ASSERT(pattern != NULL);

    if ( NX_DLIST_FIRST(pattern->matchfields) == NULL )
    {
	throw_msg("pattern has no matchfields");
    }
    pattern->group = group;
    NX_DLIST_INSERT_TAIL(group->patterns, pattern, link);

}



void nx_patterndb_add_group(nx_patterndb_t *patterndb, nx_patterngroup_t *group)
{
    ASSERT(patterndb != NULL);
    ASSERT(group != NULL);

    group->patterndb = patterndb;
    NX_DLIST_INSERT_TAIL(patterndb->groups, group, link);
}



static void nx_pattern_matchfield_compile(apr_pool_t *pool,
					  nx_pattern_matchfield_t *matchfield)
{
    const char *error = NULL;
    int erroroffs = 0;
    size_t size;
    pcre *tmppcre;
    int rc;
    int capturecount;

    matchfield->regexp = pcre_compile(matchfield->value, 0, &error,  &erroroffs, NULL);
    if ( matchfield->regexp == NULL )
    {
	throw_msg("failed to compile regular expression '%s', error at position %d: %s",
		  matchfield->value, erroroffs, error);
    }
    rc = pcre_fullinfo(matchfield->regexp, NULL, PCRE_INFO_SIZE, &size); 
    if ( rc < 0 )
    {
	pcre_free(matchfield->regexp);
	throw_msg("failed to get compiled regexp size");
    }

    rc = pcre_fullinfo(matchfield->regexp, NULL, PCRE_INFO_CAPTURECOUNT, &capturecount); 
    if ( rc < 0 )
    {
	pcre_free(matchfield->regexp);
	throw_msg("failed to get regexp captured count");
    }
    if ( capturecount >= NX_PATTERNDB_MAX_CAPTURED_FIELDS )
    {
	pcre_free(matchfield->regexp);
	throw_msg("maximum number of captured substrings is limited to %d",
		  NX_PATTERNDB_MAX_CAPTURED_FIELDS);
    }

    ASSERT(size > 0);
    ASSERT(pool != NULL);
    tmppcre = apr_palloc(pool, (apr_size_t) size);
    memcpy(tmppcre, matchfield->regexp, (apr_size_t) size);
    pcre_free(matchfield->regexp);
    matchfield->regexp = tmppcre;
    //TODO: use pcre_study for further speed optimization
}



void nx_patterngroup_add_matchfield(apr_pool_t *pool,
				    nx_patterngroup_t *group,
				    nx_pattern_matchfield_t *matchfield)
{

    ASSERT(group != NULL);
    ASSERT(matchfield != NULL);

    if ( matchfield->type == NX_PATTERN_MATCH_TYPE_REGEXP )
    {
	nx_pattern_matchfield_compile(pool, matchfield);
	NX_DLIST_INSERT_TAIL(group->matchfields, matchfield, link);
    }
    else
    { // for optimiization purposes we insert EXACT matches in front because it is faster to match
	NX_DLIST_INSERT_HEAD(group->matchfields, matchfield, link);
    }
}



void nx_pattern_field_free(nx_pattern_field_t *field)
{
    ASSERT(field != NULL);

    free(field->name);
    free(field->value);
    free(field);
}



void nx_pattern_field_list_insert_field(apr_pool_t *pool,
					nx_logdata_field_list_t *fields,
					const char *key,
					const char *value,
					nx_value_type_t type)
{
    nx_logdata_field_t *field;
    nx_value_t *val;

    ASSERT(pool != NULL);
    ASSERT(fields != NULL);
    ASSERT(key != NULL);
    ASSERT(value != NULL);
    
    val = nx_value_from_string(value, type); //FIXME: use pool alloc here
    if ( val != NULL )
    {
	field = apr_pcalloc(pool, sizeof(nx_logdata_field_t));
	field->value = val;
	field->key = apr_pstrdup(pool, key);

	NX_DLIST_INSERT_TAIL(fields, field, link);
    }
}



void nx_pattern_add_matchfield(apr_pool_t *pool,
			       nx_pattern_t *pattern,
			       nx_pattern_matchfield_t *matchfield)
{
    int rc;
    int capturedcnt = 0, cnt;
    nx_pattern_capturedfield_t *capturedfield;

    ASSERT(pattern != NULL);
    ASSERT(matchfield != NULL);

    if ( matchfield->type == NX_PATTERN_MATCH_TYPE_REGEXP )
    {
	nx_pattern_matchfield_compile(pool, matchfield);
        NX_DLIST_INSERT_TAIL(pattern->matchfields, matchfield, link);

	for ( capturedfield = NX_DLIST_FIRST(matchfield->capturedfields);
	      capturedfield != NULL;
	      capturedfield = NX_DLIST_NEXT(capturedfield, link) )
	{
	    capturedcnt++;
	}
	rc = pcre_fullinfo(matchfield->regexp, NULL, PCRE_INFO_CAPTURECOUNT, &cnt); 
	if ( rc < 0 )
	{
	    pcre_free(matchfield->regexp);
	    throw_msg("failed to get captured count");
	}
	if ( capturedcnt != cnt )
	{
	    throw_msg("number of captured fields (%d) does no match value (%d) reported by regexp engine for pattern %ld",
		      capturedcnt, cnt, pattern->id);
	}
	matchfield->capturedfield_cnt = cnt;
    }
    else
    { // for optimazition purposes we insert EXACT matches in front because it is faster to match
        NX_DLIST_INSERT_HEAD(pattern->matchfields, matchfield, link);
    }
}



void nx_pattern_matchfield_add_capturedfield(nx_pattern_matchfield_t *matchfield,
					     nx_pattern_capturedfield_t *capturedfield)
{
    ASSERT(matchfield != NULL);
    ASSERT(capturedfield != NULL);

    NX_DLIST_INSERT_TAIL(matchfield->capturedfields, capturedfield, link);
}



nx_pattern_match_type_t nx_pattern_match_type_from_string(const char *typestr)
{
    ASSERT(typestr != NULL);
    
    if ( strcasecmp(typestr, "EXACT") == 0 )
    {
	return ( NX_PATTERN_MATCH_TYPE_EXACT );
    }
    if ( strcasecmp(typestr, "REGEXP") == 0 )
    {
	return ( NX_PATTERN_MATCH_TYPE_REGEXP );
    }

    throw_msg("invalid match type '%s'", typestr);
}



static boolean patterndb_regexp_match(const char *subject,
				      nx_pattern_matchfield_t *matchfield,
				      nx_logdata_field_list_t *addfields,
				      boolean isgroup,
				      const char *name)
{
    int result;
    nx_pattern_capturedfield_t * volatile capturedfield;
    int ovector[NX_PATTERNDB_MAX_CAPTURED_FIELDS * 3];
    volatile int i;
    nx_value_t *value;
    nx_logdata_field_t *setfield;
    nx_exception_t e;

    result = pcre_exec(matchfield->regexp, NULL, subject, (int) strlen(subject), 0, 0,
		       ovector, NX_PATTERNDB_MAX_CAPTURED_FIELDS * 3);

    if ( result < 0 )
    {
	switch ( result )
	{
	    case PCRE_ERROR_NOMATCH:
		break;
	    case PCRE_ERROR_PARTIAL:
		// The subject string did not match, but it did match partially.
		// Treat the same as NOMATCH
		break;
	    case PCRE_ERROR_NULL:
		nx_panic("invalid arguments (code, ovector or ovecsize are invalid)");
	    case PCRE_ERROR_BADOPTION:
		nx_panic("invalid option in options parameter");
	    case PCRE_ERROR_BADMAGIC:
		nx_panic("invalid pcre magic value");
	    case PCRE_ERROR_UNKNOWN_NODE:
	    case PCRE_ERROR_INTERNAL:
		nx_panic("pcre bug or buffer overflow error");
	    case PCRE_ERROR_NOMEMORY:
		nx_panic("pcre_malloc() failed");
	    case PCRE_ERROR_MATCHLIMIT:
		log_error("pcre match_limit reached in pattern '%s' for regexp '%s'",
			  name, matchfield->value);
		break;
	    case PCRE_ERROR_BADUTF8:
		log_error("invalid pcre utf-8 byte sequence in pattern '%s' for regexp '%s'",
			  name, matchfield->value);
		break;
	    case PCRE_ERROR_BADUTF8_OFFSET:
		log_error("invalid pcre utf-8 byte sequence offset in pattern '%s' for regexp '%s'",
			  name, matchfield->value);
		break;
	    case PCRE_ERROR_BADPARTIAL:
		nx_panic("PCRE_ERROR_BADPARTIAL");
	    case PCRE_ERROR_BADCOUNT:
		nx_panic("negative ovecsize");
	    default:
		log_error("unknown pcre error in pcre_exec(): %d", result);
		break;
	}
	log_debug("regexp '%s' did not match against '%s'", matchfield->value, subject);
	return ( FALSE );
    }

    log_debug("regexp '%s' matched on '%s'", matchfield->value, subject);

    if ( matchfield->capturedfield_cnt > 0 )
    {
	if ( result != matchfield->capturedfield_cnt + 1 )
	{
	    throw_msg("regexp match returned %d captured substrings, matchfield in pattern has %d defined",
		      result - 1, matchfield->capturedfield_cnt);
	}
	// add captured fields into setfields variable, these will be added to the
	// logdata if all matchfields match (=the pattern matches)
	for ( capturedfield = NX_DLIST_FIRST(matchfield->capturedfields), i = 2;
	      capturedfield != NULL;
	      capturedfield = NX_DLIST_NEXT(capturedfield, link), i += 2 )
	{
	    ASSERT(matchfield->capturedfield_cnt * 2 >= i);

	    try
	    {
		size_t len = (size_t) (ovector[i + 1] - ovector[i]);
		if ( capturedfield->type == NX_VALUE_TYPE_STRING )
		{
		    value = nx_value_new(NX_VALUE_TYPE_STRING);
		    value->string = nx_string_create(subject + ovector[i], (int) len);
		}
		else
		{
		    char tmpstr[len + 1];
		    
		    memcpy(tmpstr, subject + ovector[i], len);
		    tmpstr[len] = '\0';
		    value = nx_value_from_string(tmpstr, capturedfield->type);
		}
		setfield = malloc(sizeof(nx_logdata_field_t));
		setfield->value = value;
		setfield->key = strdup(capturedfield->name);
		ASSERT(addfields != NULL);
		NX_DLIST_INSERT_TAIL(addfields, setfield, link);
		//log_debug("added %s to logdata", capturedfield->name);
	    }
	    catch(e)
	    {
		log_exception_msg(e, "error setting field '%s' in %s '%s'",		  
				  capturedfield->name,
				  isgroup == TRUE ? "pattern group" : "pattern",
				  name);
	    }
	}
    }

    return ( TRUE );
}



static boolean nx_patterndb_match_matchfields(nx_logdata_t *logdata,
					      nx_pattern_matchfields_t *matchfields,
					      nx_logdata_field_list_t *addfields,
					      boolean isgroup,
					      const char *name)
{
    nx_pattern_matchfield_t *matchfield;
    nx_logdata_field_t *logfield;
    nx_exception_t e;

    ASSERT(matchfields != NULL);

    for ( matchfield = NX_DLIST_FIRST(matchfields);
	  matchfield != NULL;
	  matchfield = NX_DLIST_NEXT(matchfield, link) )
    {
	logfield = nx_logdata_get_field(logdata, matchfield->name);
	if ( logfield == NULL )
	{ // logdata does not have field
	    //log_debug("logdata does not have '%s' field", matchfield->name);
	    return ( FALSE );
	}

	if ( logfield->value->defined == FALSE )
	{ // undef does not match
	    return ( FALSE );
	}

	if ( logfield->value->type == NX_VALUE_TYPE_STRING )
	{ 
	    switch ( matchfield->type )
	    {
		case NX_PATTERN_MATCH_TYPE_EXACT:
		    //log_debug("exact match for '%s' against '%s'", matchfield->value, logfield->value->string->buf);
		    if ( strcmp(matchfield->value, logfield->value->string->buf) != 0 )
		    {
			return ( FALSE );
		    }
		    break;
		case NX_PATTERN_MATCH_TYPE_REGEXP:
		    if ( patterndb_regexp_match(logfield->value->string->buf, matchfield,
						addfields, isgroup, name) == FALSE )
		    {
			//log_debug("logdata field didn't match regexp");
			return ( FALSE );
		    }
		    break;
		default:
		    nx_panic("invalid match type: %d", matchfield->type);
	    }
	}
	else if ( logfield->value->type == NX_VALUE_TYPE_INTEGER )
	{ 
	    char intstr[32];
	    char *strval;
	    volatile boolean retval = FALSE;

	    switch ( matchfield->type )
	    {
		case NX_PATTERN_MATCH_TYPE_EXACT:
		    //log_debug("exact match for '%s' against '%ld'", matchfield->value, logfield->value->integer);
		    apr_snprintf(intstr, sizeof(intstr), "%"APR_INT64_T_FMT, logfield->value->integer);
		    if ( strcmp(matchfield->value, intstr) != 0 )
		    {
			return ( FALSE );
		    }
		    break;
		case NX_PATTERN_MATCH_TYPE_REGEXP:
		    strval = nx_value_to_string(logfield->value);
		    try
		    {
			if ( patterndb_regexp_match(strval, matchfield,
						    addfields, isgroup, name) == TRUE )
			{
			    retval = TRUE;
			}
		    }
		    catch(e)
		    {
			free(strval);
			rethrow(e);
		    }
		    free(strval);
		    return ( retval );
		default:
		    nx_panic("invalid match type: %d", matchfield->type);
	    }
	}
	else if ( logfield->value->type == NX_VALUE_TYPE_BOOLEAN )
	{ 
	    switch ( matchfield->type )
	    {
		case NX_PATTERN_MATCH_TYPE_EXACT:
		    if ( (strcasecmp(matchfield->value, "true") == 0) &&
			 (logfield->value->boolean == FALSE) )
		    {
			return ( FALSE );
		    }
		    else if ( (strcasecmp(matchfield->value, "false") == 0) &&
			 (logfield->value->boolean == TRUE) )
		    {
			return ( FALSE );
		    }
		    break;
		case NX_PATTERN_MATCH_TYPE_REGEXP:
		    log_error("invalid use of REGEXP match with BOOLEAN type %s", logfield->key);
		    return ( FALSE );
		default:
		    nx_panic("invalid match type: %d", matchfield->type);
	    }
	}
	else 
	{ // other logdata field type
	    char *strval;
	    boolean retval = FALSE;

	    strval = nx_value_to_string(logfield->value);
	    try
	    {
		switch ( matchfield->type )
		{
		    case NX_PATTERN_MATCH_TYPE_EXACT:
			if ( strcmp(matchfield->value, strval) == 0 )
			{
			    retval = TRUE;
			}
			break;
		    case NX_PATTERN_MATCH_TYPE_REGEXP:
			if ( patterndb_regexp_match(strval, matchfield,
						    addfields, isgroup, name) == TRUE )
			{
			    retval = TRUE;
			}
			break;
		    default:
			nx_panic("invalid match type: %d", matchfield->type);
		}
	    }
	    catch(e)
	    {
		free(strval);
		rethrow(e);
	    }
	    free(strval);

	    return ( retval );
	}
    }
    return ( TRUE );
}



static nx_logdata_t *nx_patterndb_pattern_exec(nx_module_t *module,
					       nx_logdata_t *logdata,
					       nx_pattern_t *pattern)
{
    nx_expr_eval_ctx_t eval_ctx;
    nx_exception_t e;

    nx_expr_eval_ctx_init(&eval_ctx, logdata, module, NULL);
    try
    {
	nx_expr_statement_list_execute(&eval_ctx, pattern->exec);
    }
    catch(e)
    {
	log_exception(e);
    }
    if ( eval_ctx.logdata == NULL )
    { // dropped
	nx_module_logqueue_pop(module, logdata);
	nx_logdata_free(logdata);
    }
    logdata = eval_ctx.logdata;
    nx_expr_eval_ctx_destroy(&eval_ctx);

    return ( logdata );
}



nx_logdata_t *nx_patterndb_match_logdata(nx_module_t *module,
					 nx_logdata_t *logdata,
					 nx_patterndb_t *patterndb,
					 const nx_pattern_t **matched)
{
    nx_pattern_t *pattern, *prevpattern;
    nx_patterngroup_t *group;
    nx_logdata_field_t *tmpfield;
    nx_logdata_field_list_t addfields;
    nx_logdata_field_t *setfield;

    ASSERT(module != NULL);
    ASSERT(logdata != NULL);
    ASSERT(patterndb != NULL);
    ASSERT(matched != NULL);

    NX_DLIST_INIT(&addfields, nx_logdata_field_t, link);

    for ( group = NX_DLIST_FIRST(patterndb->groups);
	  group != NULL;
	  group = NX_DLIST_NEXT(group, link) )
    {
	//log_debug("matching group '%s'", group->name);

	if ( nx_patterndb_match_matchfields(logdata, group->matchfields, NULL, TRUE, group->name) != TRUE )
	{
	    continue;
	}

	for ( pattern = NX_DLIST_FIRST(group->patterns);
	      pattern != NULL;
	      pattern = NX_DLIST_NEXT(pattern, link) )
	{
	    //log_debug("matching pattern '%ld'", pattern->id);

	    if ( nx_patterndb_match_matchfields(logdata, pattern->matchfields, &addfields, FALSE, pattern->name) == TRUE )
	    {   // if we get here all fields matched, i.e. the pattern matches
		*matched = pattern;
		while ( (tmpfield = NX_DLIST_FIRST(&addfields)) != NULL )
		{ // now add captured fields
		    NX_DLIST_REMOVE(&addfields, tmpfield, link);
		    nx_logdata_set_field(logdata, tmpfield);
		}

		(pattern->matchcnt)++;
		if ( ((prevpattern = NX_DLIST_PREV(pattern, link)) != NULL) &&
		     (prevpattern->matchcnt < pattern->matchcnt) )
		{ // advance pattern in list for optimized matching
		    NX_DLIST_REMOVE(group->patterns, pattern, link);
		    NX_DLIST_INSERT_BEFORE(group->patterns, prevpattern, pattern, link);
		}

		nx_logdata_set_integer(logdata, "PatternID", pattern->id);
		nx_logdata_set_string(logdata, "PatternName", pattern->name);

		// add SET fields to logdata
		if ( pattern->setfields != NULL )
		{
		    for ( setfield = NX_DLIST_FIRST(pattern->setfields);
			  setfield != NULL;
			  setfield = NX_DLIST_NEXT(setfield, link) )
		    {
			//log_debug("setting %s (type: %s)", setfield->key, nx_value_type_to_string(setfield->value->type));
			nx_logdata_set_field_value(logdata, setfield->key, 
						   nx_value_clone(NULL, setfield->value));
		    }
		}

		if ( pattern->exec != NULL )
		{
		    logdata = nx_patterndb_pattern_exec(module, logdata, pattern);
		}
		return ( logdata );
	    }

	    while ( (tmpfield = NX_DLIST_FIRST(&addfields)) != NULL )
	    { // clear all setfields
		NX_DLIST_REMOVE(&addfields, tmpfield, link);
		nx_logdata_field_free(tmpfield);
	    }
	}
    }

    return ( logdata );
}



void nx_patterndb_parse_exec_block(nx_module_t *module,
				   nx_patterndb_t *patterndb,
				   nx_pattern_t *pattern,
				   const char *execstr,
				   const char *filename,
				   int currline,
				   int currpos)
{
    nx_exception_t e;
    nx_expr_statement_t *stmnt;
    nx_expr_statement_list_t * volatile statements = NULL;

    ASSERT(patterndb != NULL);
    ASSERT(pattern != NULL);
    ASSERT(execstr != NULL);

    try
    {
	statements = nx_expr_parse_statements(module, execstr, patterndb->pool,
					      filename, currline, currpos);
    }
    catch(e)
    {
	rethrow_msg(e, "couldn't parse Exec block in pattern");
    }
    if ( pattern->exec == NULL )
    {
	pattern->exec = statements;
    }
    else
    {
	while ( (stmnt = NX_DLIST_FIRST(statements)) != NULL )
	{
	    NX_DLIST_REMOVE(statements, stmnt, link);
	    NX_DLIST_INSERT_TAIL(pattern->exec, stmnt, link);
	}
    }
}

