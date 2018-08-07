/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/core/nxlog.h"
#include "../../src/core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_TEST

nxlog_t nxlogd;

int main(int argc UNUSED, const char * const *argv, const char * const *env UNUSED)
{
    nx_logdata_t *logdata;
    nx_logdata_t *logdata2;
    nx_value_t *value;
    nx_logdata_field_t *field;
    const char *teststr;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);
    
    teststr = "test line";
    logdata = nx_logdata_new_logline(teststr, -1);
    ASSERT(strcmp(logdata->raw_event->buf, teststr) == 0);
    nx_logdata_free(logdata);

    teststr = "message only";
    logdata = nx_logdata_new_logline(teststr, (int) strlen(teststr));
    field = nx_logdata_get_field(logdata, "raw_event");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_STRING);
    ASSERT(strcmp(field->value->string->buf, teststr) == 0);

    value = nx_value_new_integer(42);
    nx_logdata_set_field_value(logdata, "integer", value);
    field = nx_logdata_get_field(logdata, "integer");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_INTEGER);
    ASSERT(field->value->integer == 42);

    value = nx_value_new_string("test string");
    nx_logdata_set_field_value(logdata, "string", value);
    field = nx_logdata_get_field(logdata, "string");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_STRING);
    ASSERT(strcmp(field->value->string->buf, "test string") == 0);

    value = nx_value_new_boolean(TRUE);
    nx_logdata_set_field_value(logdata, "boolean", value);
    field = nx_logdata_get_field(logdata, "boolean");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_BOOLEAN);
    ASSERT(field->value->boolean == TRUE);

    value = nx_value_new_datetime(42);
    nx_logdata_set_field_value(logdata, "datetime", value);
    field = nx_logdata_get_field(logdata, "datetime");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_DATETIME);
    ASSERT(field->value->datetime == 42);

    logdata2 = nx_logdata_clone(logdata);
    nx_logdata_free(logdata);
    nx_logdata_free(logdata2);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
