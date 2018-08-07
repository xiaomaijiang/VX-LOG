/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/core/nxlog.h"
#include "../../src/common/logdata.h"
#include "../../src/core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_TEST

nxlog_t nxlog;

int main(int argc UNUSED, const char * const *argv, const char * const *env UNUSED)
{
    nx_logdata_t *logdata, *logdata2;
    nx_value_t *value;
    apr_size_t memsize;
    char *buf;
    apr_size_t bytes;
    const char *teststr = "test msg";
    nx_logdata_field_t *field;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    logdata = nx_logdata_new_logline(teststr, -1);
    memsize = nx_logdata_serialized_size(logdata);
    buf = malloc(memsize);
    ASSERT(nx_logdata_to_membuf(logdata, buf, memsize) == memsize);
    logdata2 = nx_logdata_from_membuf(buf, memsize, &bytes);
    ASSERT(logdata2 != NULL);
    ASSERT(memsize == bytes);
    ASSERT(strcmp(logdata->fields.first->key, logdata2->fields.first->key) == 0);
    ASSERT(nx_value_eq(logdata->fields.first->value, logdata2->fields.first->value) == TRUE);
    nx_logdata_free(logdata);
    nx_logdata_free(logdata2);
    free(buf);
    
    logdata = nx_logdata_new_logline(teststr, -1);
    value = nx_value_new_integer(42);
    nx_logdata_set_field_value(logdata, "integer", value);

    value = nx_value_new_string(teststr);
    nx_logdata_set_field_value(logdata, "string", value);

    value = nx_value_new_boolean(TRUE);
    nx_logdata_set_field_value(logdata, "boolean", value);

    value = nx_value_new_datetime(42);
    nx_logdata_set_field_value(logdata, "datetime", value);

    memsize = nx_logdata_serialized_size(logdata);
    buf = malloc(memsize);
    ASSERT(nx_logdata_to_membuf(logdata, buf, memsize) == memsize);
    logdata2 = nx_logdata_from_membuf(buf, memsize, &bytes);
    ASSERT(logdata2 != NULL);
    ASSERT(memsize == bytes);

    nx_logdata_free(logdata);

    field = nx_logdata_get_field(logdata2, "integer");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_INTEGER);
    ASSERT(field->value->integer == 42);

    field = nx_logdata_get_field(logdata2, "string");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_STRING);
    ASSERT(strcmp(field->value->string->buf, teststr) == 0);

    field = nx_logdata_get_field(logdata2, "boolean");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_BOOLEAN);
    ASSERT(field->value->boolean == TRUE);

    field = nx_logdata_get_field(logdata2, "datetime");
    ASSERT(field != NULL);
    ASSERT(field->value->type == NX_VALUE_TYPE_DATETIME);
    ASSERT(field->value->datetime == 42);

    nx_logdata_free(logdata2);
    

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
