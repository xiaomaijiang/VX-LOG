/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Balazs Boza <balazsboza@gmail.com>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "../../../common/date.h"
#include "xm_wtmp.h"
#include <utmp.h>

#define NX_LOGMODULE NX_LOGMODULE_MODULE
#define UTMP_SIZE sizeof(struct utmp)

static nx_logdata_t* xm_wtmp_parser(struct utmp *u)
{
    if (strlen(u->ut_user) <= 0 || strcmp(u->ut_user, "runlevel") == 0)
    {
        return (NULL);
    }

    nx_string_t* raw_input_str;
    nx_logdata_t* logdata = nx_logdata_new();

    time_t t = u->ut_tv.tv_sec;
    apr_time_t at;
    apr_time_ansi_put(&at, t);
    nx_logdata_set_datetime(logdata, "EventTime", at);
    char tmpstr[30];
    nx_date_to_iso(tmpstr, sizeof (tmpstr), at);
    raw_input_str = nx_string_create(tmpstr, (int) strlen(tmpstr));

    nx_logdata_set_string(logdata, "AccountName", u->ut_user);
    nx_string_append(raw_input_str, "    ", 4);
    nx_string_append(raw_input_str, u->ut_user, -1);
    nx_string_append(raw_input_str, "    ", 4);

    if (strlen(u->ut_line) >= 3)
    {
        nx_logdata_set_string(logdata, "Device", u->ut_line);
        nx_string_append(raw_input_str, u->ut_line, -1);
        nx_string_append(raw_input_str, "    ", 4);
    }
    else
    {
        nx_logdata_set_string(logdata, "Device", "no device");
        nx_string_append(raw_input_str, "no device", -1);
        nx_string_append(raw_input_str, "    ", 4);
    }

    const char* type;

    switch (u->ut_type)
    {
        case RUN_LVL:
            type = "shutdown";
            break;
        case BOOT_TIME:
            type = "reboot(boot)";
            break;
        case NEW_TIME:
            type = "new time";
            break;
        case OLD_TIME:
            type = "old time";
            break;
        case INIT_PROCESS:
            type = "init process";
            break;
        case LOGIN_PROCESS:
            type = "login process(user logout)";
            break;
        case USER_PROCESS:
            type = "login";
            break;
        case DEAD_PROCESS:
            type = "logout";
            break;
        case ACCOUNTING:
            type = "accounting";
            break;
        default:
            type = "no valid information";
            break;

    }

    nx_logdata_set_string(logdata, "LoginType", type);
    nx_string_append(raw_input_str, type, (int) strlen(type));

    nx_string_append(logdata->raw_event, raw_input_str->buf, (int) raw_input_str->len);

    return (logdata);
}



static nx_logdata_t *xm_wtmp_input_func(nx_module_input_t *input, void *data)
{   
    nx_logdata_t *retval = NULL;
    struct utmp u;
    nx_xm_wtmp_ctx_t *incomplete_logdata;
    unsigned int seek = 0;

    ASSERT(input != NULL);
    ASSERT(input->buflen >= 0);
    ASSERT(data != NULL);

    if (input->buflen == 0)
    {
        return (NULL);
    }

    if (input->ctx != NULL)
    {
	incomplete_logdata = input->ctx;
    }
    else
    {
        incomplete_logdata = NULL;
    }
    
    if (incomplete_logdata == NULL)
    {
        incomplete_logdata = apr_palloc(input->pool, sizeof (struct nx_xm_wtmp_ctx_t));
        incomplete_logdata->buf = apr_palloc(input->pool, (apr_size_t) UTMP_SIZE);
    }
    
    if (incomplete_logdata->len <= 0)
    {
        if (input->buflen >= (int) UTMP_SIZE)
        {
            memcpy(&u, input->buf + input->bufstart, (size_t) UTMP_SIZE);
            input->buflen -= (int) UTMP_SIZE;
            input->bufstart += (int) UTMP_SIZE;
        }
        else
        {
            memcpy(incomplete_logdata->buf, input->buf + input->bufstart, (size_t) input->buflen);
            incomplete_logdata->len = input->buflen;
            input->ctx = (void*) incomplete_logdata;
            input->buflen = 0;
            input->bufstart = 0;
        }
    }
    else
    {
        if (input->buflen >= (int) UTMP_SIZE)
        {
            seek = (unsigned int) ((int) UTMP_SIZE - incomplete_logdata->len);
            memcpy(incomplete_logdata->buf + incomplete_logdata->len, input->buf, seek);
            memcpy(&u, incomplete_logdata->buf, UTMP_SIZE);
            input->buflen -= (int) seek;
            input->bufstart += (int) seek;
            incomplete_logdata->len = 0;
            input->ctx = NULL;
        }
        else
        {
            if (incomplete_logdata->len + input->buflen < (int) UTMP_SIZE)
            {
                memcpy(incomplete_logdata->buf + incomplete_logdata->len,
                       input->buf + input->bufstart, (size_t) input->buflen);
                incomplete_logdata->len += input->buflen;
                input->ctx = (void*) incomplete_logdata;
                input->buflen = 0;
                input->bufstart = 0;
            }
            else
            {
                int s = (int) UTMP_SIZE - incomplete_logdata->len;
                memcpy(incomplete_logdata->buf + incomplete_logdata->len, input->buf, (size_t) s);
                memcpy(&u, incomplete_logdata->buf, UTMP_SIZE);
                input->buflen -= (int) s;
                input->bufstart = input->buflen == 0 ? 0 : (int) s;
                input->ctx = NULL;
            }
        }
    }

    if (&u != NULL)
    {
        retval = xm_wtmp_parser(&u);
        return (retval);
    }

    return (NULL);

}



static void xm_wtmp_config(nx_module_t *module)
{
    nx_xm_wtmp_conf_t *modconf;
    const nx_directive_t * volatile curr;

    modconf = apr_pcalloc(module->pool, sizeof (nx_xm_wtmp_conf_t));
    module->config = modconf;

    curr = module->directives;

    while (curr != NULL)
    {
        if (nx_module_common_keyword(curr->directive) == TRUE)
        {
        }
        curr = curr->next;
    }

    if (nx_module_input_func_lookup(module->name) == NULL)
    {
        nx_module_input_func_register(NULL, module->name, &xm_wtmp_input_func, NULL, module);
        log_debug("Inputreader '%s' registered", module->name);
    }
}



NX_MODULE_DECLARATION nx_xm_wtmp_module = {
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL, // capabilities
    xm_wtmp_config, // config
    NULL, // start
    NULL, // stop
    NULL, // pause
    NULL, // resume
    NULL, // init
    NULL, // shutdown
    NULL, // event
    NULL, // info
    NULL, //exports
};
