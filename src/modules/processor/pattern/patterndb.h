/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_PATTERNDB_H
#define __NX_PATTERNDB_H

#include <pcre.h>
#include "../../../common/types.h"
#include "../../../common/dlist.h"
#include "../../../common/value.h"
#include "../../../common/logdata.h"
#include "../../../common/expr.h"

typedef enum nx_pattern_match_type_t
{
    NX_PATTERN_MATCH_TYPE_NONE = 0,
    NX_PATTERN_MATCH_TYPE_EXACT,
    NX_PATTERN_MATCH_TYPE_REGEXP,
} nx_pattern_match_type_t;

typedef struct nx_patterns_t nx_patterns_t;
NX_DLIST_HEAD(nx_patterns_t, nx_pattern_t);

typedef struct nx_patterngroups_t nx_patterngroups_t;
NX_DLIST_HEAD(nx_patterngroups_t, nx_patterngroup_t);

typedef struct nx_pattern_matchfields_t nx_pattern_matchfields_t;
NX_DLIST_HEAD(nx_pattern_matchfields_t, nx_pattern_matchfield_t);

typedef struct nx_pattern_capturedfields_t nx_pattern_capturedfields_t;
NX_DLIST_HEAD(nx_pattern_capturedfields_t, nx_pattern_capturedfield_t);

typedef struct nx_pattern_capturedfield_t nx_pattern_capturedfield_t;
typedef struct nx_pattern_matchfield_t nx_pattern_matchfield_t;
typedef struct nx_pattern_t nx_pattern_t;
typedef struct nx_patterngroup_t nx_patterngroup_t;
typedef struct nx_patterndb_t nx_patterndb_t;
typedef struct nx_pattern_field_t nx_pattern_field_t;


struct nx_pattern_capturedfield_t
{
    NX_DLIST_ENTRY(nx_pattern_capturedfield_t)	link;
    const char 					*name;
    nx_value_type_t 				type;
};



struct nx_pattern_matchfield_t
{
    NX_DLIST_ENTRY(nx_pattern_matchfield_t) 	link;
    const char 					*name;
    nx_pattern_match_type_t			type;
    const char					*value;
    pcre 					*regexp; ///< if type is REGEXP
    int						capturedfield_cnt; // number for captured fields
    nx_pattern_capturedfields_t			*capturedfields;
};



struct nx_pattern_t
{
    NX_DLIST_ENTRY(nx_pattern_t)	link;
    const char 				*name;
    int64_t 				id;
    nx_pattern_matchfields_t		*matchfields;
    int64_t				matchcnt;
    nx_patterngroup_t			*group; ///< pointer to the group this belongs to
    nx_logdata_field_list_t		*setfields;
    nx_expr_statement_list_t		*exec;	///< Statement blocks to execute
};



struct nx_patterngroup_t
{
    NX_DLIST_ENTRY(nx_patterngroup_t) 	link;
    const char 				*name;
    int64_t 				id;
    nx_patterns_t			*patterns;
    nx_patterndb_t			*patterndb; ///< pointer to the patterndb this belongs to
    nx_pattern_matchfields_t		*matchfields;
};



struct nx_patterndb_t
{
    apr_pool_t		*pool;
    nx_patterngroups_t	*groups;
    // FIXME testcases
};



// temp structure converted to nx_logdata_field_t later
struct nx_pattern_field_t
{
    char 				*name;
    nx_value_type_t			type;
    char				*value;
};


nx_patterndb_t *nx_patterndb_parse(apr_pool_t *pool, const char *filename);
nx_patterndb_t *nx_patterndb_new(apr_pool_t *pool);
nx_patterngroup_t *nx_patterngroup_new(nx_patterndb_t *patterndb);
nx_pattern_t *nx_pattern_new(nx_patterndb_t *patterndb);
nx_pattern_matchfield_t *nx_pattern_matchfield_new(nx_patterndb_t *patterndb);
void nx_patterngroup_add_pattern(nx_patterngroup_t *group, nx_pattern_t *pattern);
void nx_patterndb_add_group(nx_patterndb_t *patterndb, nx_patterngroup_t *group);
void nx_pattern_add_matchfield(apr_pool_t *pool,
			       nx_pattern_t *pattern,
			       nx_pattern_matchfield_t *matchfield);
void nx_patterngroup_add_matchfield(apr_pool_t *pool,
				    nx_patterngroup_t *group,
				    nx_pattern_matchfield_t *matchfield);
void nx_pattern_field_list_insert_field(apr_pool_t *pool,
					nx_logdata_field_list_t *fields,
					const char *key,
					const char *value,
					nx_value_type_t type);
void nx_pattern_field_free(nx_pattern_field_t *field);
void nx_pattern_matchfield_add_capturedfield(nx_pattern_matchfield_t *matchfield,
					     nx_pattern_capturedfield_t *capturedfield);
nx_pattern_match_type_t nx_pattern_match_type_from_string(const char *typestr);
nx_logdata_t *nx_patterndb_match_logdata(nx_module_t *module,
					 nx_logdata_t *logdata,
					 nx_patterndb_t *patterndb,
					 const nx_pattern_t **matched);
void nx_patterndb_parse_exec_block(nx_module_t *module,
				   nx_patterndb_t *patterndb,
				   nx_pattern_t *pattern,
				   const char *execstr,
				   const char *filename,
				   int currline,
				   int currpos);
#endif	/* __NX_PATTERNDB_H */
