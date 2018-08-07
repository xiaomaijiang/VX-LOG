/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/core/nxlog.h"
#include "../../src/common/date.h"
#include "../../src/core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_TEST

nxlog_t nxlog;


int main(int argc UNUSED, const char * const *argv, const char * const *env UNUSED)
{
    nx_value_t *value;
    nx_value_t *value2;
    char buf[128];
    apr_time_t datetime;
    apr_size_t bytes;
    apr_size_t sersize;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    value = nx_value_new_string("test");
    sersize = nx_value_serialized_size(value);
    ASSERT(nx_value_to_membuf(value, buf, sizeof(buf)) == sersize);
    ASSERT(nx_value_to_membuf(value, buf, sersize - 1) == 0);
    value2 = nx_value_from_membuf(buf, sizeof(buf), &bytes);
    ASSERT(value2 != NULL);
    ASSERT(bytes == sersize);
    ASSERT(nx_value_eq(value, value2) == TRUE);
    nx_value_free(value);
    nx_value_free(value2);

    value = nx_value_new_integer(42);
    sersize = nx_value_serialized_size(value);
    ASSERT(nx_value_to_membuf(value, buf, sizeof(buf)) == sersize);
    ASSERT(nx_value_to_membuf(value, buf, sersize - 1) == 0);
    value2 = nx_value_from_membuf(buf, sizeof(buf), &bytes);
    ASSERT(value2 != NULL);
    ASSERT(bytes == sersize);
    ASSERT(nx_value_eq(value, value2) == TRUE);
    nx_value_free(value);
    nx_value_free(value2);

    nx_date_parse(&datetime, "1977-09-06 01:02:03", NULL);
    value = nx_value_new_datetime(datetime);
    sersize = nx_value_serialized_size(value);
    ASSERT(nx_value_to_membuf(value, buf, sizeof(buf)) == sersize);
    ASSERT(nx_value_to_membuf(value, buf, sersize - 1) == 0);
    value2 = nx_value_from_membuf(buf, sizeof(buf), &bytes);
    ASSERT(value2 != NULL);
    ASSERT(bytes == sersize);
    ASSERT(nx_value_eq(value, value2) == TRUE);
    nx_value_free(value);
    nx_value_free(value2);

    value = nx_value_new(NX_VALUE_TYPE_BOOLEAN);
    value->boolean = TRUE;
    sersize = nx_value_serialized_size(value);
    ASSERT(nx_value_to_membuf(value, buf, sizeof(buf)) == sersize);
    ASSERT(nx_value_to_membuf(value, buf, sersize - 1) == 0);
    value2 = nx_value_from_membuf(buf, sizeof(buf), &bytes);
    ASSERT(value2 != NULL);
    ASSERT(bytes == sersize);
    ASSERT(nx_value_eq(value, value2) == TRUE);
    nx_value_free(value);
    nx_value_free(value2);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
