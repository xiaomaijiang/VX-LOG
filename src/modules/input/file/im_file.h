/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_IM_FILE_H
#define __NX_IM_FILE_H

#include "../../../common/types.h"
#include "../../../common/expr.h"
#include "../../../common/module.h"


typedef struct nx_im_file_input_t
{
    NX_DLIST_ENTRY(nx_im_file_input_t) link;
    nx_module_input_t	*input;	///< Input structure, NULL if not open
    apr_pool_t		*pool;	///< Pool to allocate from
    const char		*name;	///< Name of the file
    apr_time_t		mtime;	///< Last modification time
    apr_time_t		new_mtime;///< Last modification time returned by stat()
    apr_ino_t		inode;	///< Inode to monitor if file was rotated
    apr_off_t		filepos;///< File position to resume at after a close
    apr_off_t		size;	///< Last file size
    apr_off_t		new_size;///< File size returned by stat
    int			num_eof;///< The number of EOFs since the last successful read
    apr_time_t		blacklist_until; ///< ignore this file until this time
    int			blacklist_interval; ///< seconds to blacklist the file, increased on failure
} nx_im_file_input_t;


typedef struct nx_im_file_input_list_t nx_im_file_input_list_t;
NX_DLIST_HEAD(nx_im_file_input_list_t, nx_im_file_input_t);

typedef struct nx_im_file_conf_t
{
    nx_expr_t   	*filename_expr;
    boolean		filename_const;	///< Set to TRUE if the filename is not a dynamic string
    char		filename[APR_PATH_MAX];
    boolean 		savepos;
    boolean		readfromlast;
    boolean		recursive;
    boolean		closewhenidle;
    boolean		renamecheck;
    float		poll_interval;
    float		dircheck_interval;
    nx_event_t 		*poll_event;
    nx_event_t 		*dircheck_event;
    nx_module_input_func_decl_t *inputfunc;
    int			non_active_modified;	///< file modified in the non-active file set

    int			active_files;	///< Max number of files to keep in open_files
    apr_hash_t		*files; 	///< Contains nx_file_input_t structures
    int			num_open_files; ///< The number of open files in the list
    nx_im_file_input_list_t *open_files;///< The list of open files
    nx_im_file_input_t	*currsrc; 	///< last successfull read from this input file
    boolean		warned_no_input_files;
    boolean		warned_no_directory;
    apr_time_t		lastcheck;	///< time of last check for new data in closed files
    
} nx_im_file_conf_t;



#endif	/* __NX_IM_FILE_H */
