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
    char *str;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    value = nx_value_new_string("test");
    ASSERT(strcmp(value->string->buf, "test") == 0);
    nx_value_free(value);

    value = nx_value_new_integer(42);
    ASSERT(value->integer == 42);
    nx_value_free(value);

    value = nx_value_from_string("127.0.1.2", NX_VALUE_TYPE_IP4ADDR);
    ASSERT(value->ip4addr[0] == 127);
    ASSERT(value->ip4addr[1] == 0);
    ASSERT(value->ip4addr[2] == 1);
    ASSERT(value->ip4addr[3] == 2);
    
    str = nx_value_to_string(value);
    ASSERT(strcmp(str, "127.0.1.2") == 0);
    free(str);
    nx_value_free(value);

    value = nx_value_from_string("2001:0db8:85a3:0000:0000:8a2e:0370:7334", NX_VALUE_TYPE_IP6ADDR);
    str = nx_value_to_string(value);
    ASSERT(strcmp(str, "2001:db8:85a3::8a2e:370:7334") == 0);
    free(str);
    nx_value_free(value);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
