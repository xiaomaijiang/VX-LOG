/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_MODULE_H
#define __NX_MODULE_H

#include "types.h"

#include <apr_dso.h>
#include <apr_file_io.h>
#include <apr_network_io.h>
#include <apr_poll.h>
#include <apr_tables.h>
#include <apr_hash.h>

#include "logdata.h"
#include "route.h"
#include "cfgfile.h"
#include "dlist.h"
#include "resource.h"
#include "expr.h"
#include "schedule.h"
#include "statvar.h"

#define NX_MODULE_API_VERSION 4

#define NX_MODULE_DEFAULT_OUTPUT_BUFSIZE 65000
#define NX_MODULE_DEFAULT_INPUT_BUFSIZE 65000

#ifdef DLL_EXPORT 
# define NX_MODULE_DECLARATION nx_module_declaration_t __declspec(dllexport)
#else
# define NX_MODULE_DECLARATION nx_module_declaration_t
#endif

#ifdef WIN32
# define NX_MODULE_DSO_EXTENSION ".dll"
#else
# define NX_MODULE_DSO_EXTENSION ".so"
#endif

// this is a limit for the text based field parser
// also used by the csv parser
#define NX_MODULE_MAX_FIELDS 200

/*
 TODO:
 error messages in nx_module_t: linked list (ringbuffer) with message and datetime
*/
 

typedef enum nx_input_source_t
{
    NX_INPUT_SOURCE_FILE = 1,
    NX_INPUT_SOURCE_SOCKET,
} nx_input_source_t;



typedef enum nx_module_type_t
{
    NX_MODULE_TYPE_INPUT = 1,
    NX_MODULE_TYPE_PROCESSOR,
    NX_MODULE_TYPE_OUTPUT,
    NX_MODULE_TYPE_EXTENSION,
} nx_module_type_t;



typedef enum nx_module_flag_t
{
    NX_MODULE_FLAG_NONE	= 0,
    NX_MODULE_FLAG_NOSHARE	= 1 << 1, ///< module has non shareable resources
} nx_module_flag_t;



typedef enum nx_module_status_t
{
    NX_MODULE_STATUS_UNINITIALIZED = 0,
    NX_MODULE_STATUS_STOPPED,
    NX_MODULE_STATUS_PAUSED,
    NX_MODULE_STATUS_RUNNING,
} nx_module_status_t;


typedef void (nx_module_config_func_t)(nx_module_t *module);
typedef void (nx_module_start_func_t)(nx_module_t *module);
typedef void (nx_module_stop_func_t)(nx_module_t *module);
typedef void (nx_module_pause_func_t)(nx_module_t *module);
typedef void (nx_module_resume_func_t)(nx_module_t *module);
typedef void (nx_module_init_func_t)(nx_module_t *module);
typedef void (nx_module_shutdown_func_t)(nx_module_t *module);
typedef void (nx_module_event_func_t)(nx_module_t *module, nx_event_t *event);
typedef char *(nx_module_info_func_t)(nx_module_t *module);
typedef nx_logdata_t *(nx_module_process_func_t)(nx_module_t *module, nx_logdata_t *logdata);

typedef struct nx_module_exports_t
{
    int			num_func;
    nx_expr_func_t	*funcs;
    int			num_proc;
    nx_expr_proc_t	*procs;
} nx_module_exports_t;

typedef struct nx_module_declaration_t
{
    int16_t			api_version;		///< module api version
    nx_module_type_t		type;
    const char 			*capabilities;          ///< space separated list of capabilities for linux
    nx_module_config_func_t	*config;
    nx_module_start_func_t	*start;
    nx_module_stop_func_t	*stop;
    nx_module_pause_func_t	*pause;
    nx_module_resume_func_t	*resume;
    nx_module_init_func_t	*init;
    nx_module_shutdown_func_t	*shutdown;
    nx_module_event_func_t	*event;
    nx_module_info_func_t	*info;
    nx_module_exports_t		*exports;
    /* FIXME move these here
       dso
       dsoname
       flags
    */
} nx_module_declaration_t;


typedef struct nx_module_data_t nx_module_data_t;
typedef nx_logdata_t *(nx_module_input_func_t)(nx_module_input_t *input, void *data);
typedef struct nx_module_input_func_decl_t nx_module_input_func_decl_t;
struct nx_module_data_t
{
    const char *key;
    void *data;
    nx_module_data_t *next;
};
struct nx_module_input_func_decl_t
{
    NX_DLIST_ENTRY(nx_module_input_func_decl_t) link;
    const nx_module_t *module;
    const char *name;
    nx_module_input_func_t *func;  ///< return logdata to caller
    nx_module_input_func_t *flush; ///< return logdata to caller forcibly
    void *data;
};
struct nx_module_input_t
{
    NX_DLIST_ENTRY(nx_module_input_t) link;
    apr_pool_t		*pool;
    apr_datatype_e	desc_type;
    apr_descriptor	desc;
    nx_module_t		*module;
    char		*buf;		///< input buffer
    int			bufsize;	///< size of the input buffer
    int			bufstart;	///< first unused/unread byte in the buffer
    int			buflen;		///< less than or equal to bufsize depending how much could be read
    // TODO we need to store the size of the data that is stored in incomplete_data in order to calculate the
    // correct file position that should be saved
    // We have filepos retuned by apr_file_seek() but that also includes data in buf which has not been fully read
    // It would lose this data on restart. Currently buflen is subtracted from filepos in im_file but this is not
    // enough if we have incomplete data.
    //int		incomplete_size;
    void		*ctx;		///< parser context
    nx_module_data_t	*data;	 	///< custom data for the input function, linked list
    const char		*name;		///< name of the input (cert cn, remote IP, filename)
    nx_module_input_func_decl_t *inputfunc;
};


typedef struct nx_module_input_list_t nx_module_input_list_t;
NX_DLIST_HEAD(nx_module_input_list_t, nx_module_input_t);

typedef struct nx_module_output_t nx_module_output_t;
typedef struct nx_module_output_func_decl_t nx_module_output_func_decl_t;
typedef void (nx_module_output_func_t)(nx_module_output_t *output, void *data);

struct nx_module_output_func_decl_t
{
    NX_DLIST_ENTRY(nx_module_output_func_decl_t) link;
    const nx_module_t *module;
    const char *name;
    nx_module_output_func_t *func;
    void *data;
};
struct nx_module_output_t
{
    apr_pool_t		*pool;
    nx_module_t		*module;
    char		*buf;		///< output buffer
    apr_size_t		bufsize;	///< size of the output buffer
    apr_size_t		bufstart;	///< first unused byte in the buffer
    apr_size_t		buflen;		///< if the buffer is not full this marks the end, otherwise equal to bufsize
    nx_module_data_t 	*data; 		///< custom data for the output function, linked list
    nx_logdata_t	*logdata;	///< logdata which is used to generate the output buffer
    nx_module_output_func_decl_t *outputfunc;
};


struct nx_module_t
{
    nx_module_type_t	type;
    nx_module_declaration_t *decl;
    apr_pool_t 		*pool;
    NX_DLIST_ENTRY(nx_module_t) link;
    boolean		has_config_errors;
    boolean		flowcontrol;	///< TRUE if flow-control is in effect
    nx_module_status_t	status;
    apr_array_header_t	*routes;	///< array of routes the module belongs to
    nx_logqueue_t	*queue; 	///< the queue for the module
    int			refcount;	///< if module is referenced from a route
    const char		*name;
    const char		*dsoname;
    apr_dso_handle_t 	*dso;
    const nx_directive_t *directives;
    nx_module_flag_t   flags;
    void 		*config;
    apr_thread_mutex_t	*mutex;
    uint64_t		evt_recvd;	///< events received
    uint64_t		evt_fwd;	///< events sent
    int			priority;	///< the highest priority of all routes this input/output module is part of
    nx_job_t		*job;		///< job for input and output modules, NULL for processors
    nx_module_data_t 	*data; 		///< custom data for the module, linked list
    union 
    {
	nx_module_input_t input;
	nx_module_output_t output;
    };
    nx_expr_statement_list_t *exec;	///< Statement blocks to execute
    nx_schedule_entry_list_t *schedule;	///< Scheduled exec blocks

    apr_hash_t		*vars;		///< variables
    apr_hash_t		*stats;		///< statistical counters
    
    volatile apr_uint32_t in_poll;
    apr_pollset_t	*pollset;
};

const char *nx_module_type_to_string(nx_module_type_t type);
const char *nx_module_status_to_string(nx_module_status_t status);
void nx_module_remove_events(nx_module_t *module);
void nx_module_remove_events_by_data(nx_module_t *module, void *data);
apr_status_t nx_module_input_fill_buffer_from_socket(nx_module_input_t *input);
void nx_module_add_logdata_input(nx_module_t *module,
				 nx_module_input_t *input,
				 nx_logdata_t *logdata);
void nx_module_add_logdata_to_route(nx_module_t *module,
				    nx_route_t *route,
				    nx_logdata_t *logdata);
void nx_module_progress_logdata(nx_module_t *module, nx_logdata_t *logdata);
void nx_module_data_available(nx_module_t *module);
boolean nx_module_can_send(nx_module_t *module, double multiplier);
nx_logdata_t *nx_module_logqueue_peek(nx_module_t *module);
void nx_module_logqueue_pop(nx_module_t *module, nx_logdata_t *logdata);
void nx_module_logqueue_drop(nx_module_t *module, nx_logdata_t *logdata);

boolean nx_module_common_keyword(const char *keyword);
void nx_module_lock(nx_module_t *module);
void nx_module_unlock(nx_module_t *module);
const char *nx_module_get_cachedir();
void nx_module_process_schedule_event(nx_module_t *module, nx_event_t *event);
nx_expr_statement_list_t *nx_module_parse_exec_block(nx_module_t *module,
						     apr_pool_t *pool,
						     const nx_directive_t *curr);
void nx_module_data_set(nx_module_t *module, const char *key, void *data);
void *nx_module_data_get(nx_module_t *module, const char *key);
void nx_module_start(nx_module_t *module);
void nx_module_start_self(nx_module_t *module);
void nx_module_stop(nx_module_t *module);
void nx_module_pause(nx_module_t *module);
void nx_module_resume(nx_module_t *module);
void nx_module_shutdown(nx_module_t *module);
void nx_module_config(nx_module_t *module);
void nx_module_init(nx_module_t *module);
void nx_module_stop_self(nx_module_t *module);
void nx_module_pause_self(nx_module_t *module);
void nx_module_resume_self(nx_module_t *module);
void nx_module_shutdown_self(nx_module_t *module);
char *nx_module_info(nx_module_t *module);
const void *nx_module_get_resource(nx_module_t *module,
				   const char *name,
				   nx_resource_type_t type);
nx_module_input_t *nx_module_input_new(nx_module_t *module, apr_pool_t *pool);
void nx_module_input_free(nx_module_input_t *input);
void *nx_module_input_data_get(nx_module_input_t *input, const char *key);
void nx_module_input_name_set(nx_module_input_t *input, const char *name);
const char *nx_module_input_name_get(nx_module_input_t *input);
void nx_module_input_data_set(nx_module_input_t *input, const char *key, void *data);
nx_module_input_func_decl_t *nx_module_input_func_lookup(const char *fname);
void nx_module_input_func_register(const nx_module_t *module,
				   const char *fname,
				   nx_module_input_func_t *func,
				   nx_module_input_func_t *flush,
				   void *data);
nx_logdata_t *nx_module_input_func_dgramreader(nx_module_input_t *input,
					       void *data);
nx_logdata_t *nx_module_input_func_linereader(nx_module_input_t *input,
					      void *data);
nx_logdata_t *nx_module_input_func_binaryreader(nx_module_input_t *input,
						void *data);
//nx_module_output_t *nx_module_output_new(nx_module_t *module, apr_pool_t *pool);
void nx_module_output_free(nx_module_output_t *output);
void *nx_module_output_data_get(nx_module_output_t *output, const char *key);
void nx_module_output_data_set(nx_module_output_t *output, const char *key, void *data);
nx_module_output_func_decl_t *nx_module_output_func_lookup(const char *fname);
void nx_module_output_func_register(const nx_module_t *module,
				    const char *fname,
				    nx_module_output_func_t *func,
				    void *data);

void nx_module_set_status(nx_module_t *module, nx_module_status_t status);
nx_module_status_t nx_module_get_status(nx_module_t *module);
nx_module_status_t nx_module_get_status_full(nx_module_t *module);

void nx_module_output_func_linewriter(nx_module_output_t *output,
				      void *data);
void nx_module_output_func_dgramwriter(nx_module_output_t *output,
				       void *data);
void nx_module_output_func_binarywriter(nx_module_output_t *output,
					void *data);
int nx_module_parse_fields(const char **fields, char *string);
int nx_module_parse_types(nx_value_type_t *types, char *string);


void nx_module_pollset_init(nx_module_t *module);
void nx_module_pollset_wakeup(nx_module_t *module);
void nx_module_pollset_add_file(nx_module_t *module,
				apr_file_t *file,
				apr_int16_t reqevents);
void nx_module_pollset_remove_file(nx_module_t *module,
				   apr_file_t *file);
void nx_module_pollset_add_socket(nx_module_t *module,
				  apr_socket_t *sock,
				  apr_int16_t reqevents);
void nx_module_pollset_remove_socket(nx_module_t *module,
				     apr_socket_t *sock);
void nx_module_add_poll_event(nx_module_t *module);
void nx_module_pollset_poll(nx_module_t *module, boolean readd);

const nx_string_t *nx_get_hostname();

#endif	/* __NX_MODULE_H */
