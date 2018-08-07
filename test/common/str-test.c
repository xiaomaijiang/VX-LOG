/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/common/str.h"
#include "../../src/core/nxlog.h"
#include "../../src/core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_TEST

nxlog_t nxlog;

const char *esc[] = {
    "x\n", "x\\n",
    "x\r", "x\\r",
    "x\t", "x\\t",
    "x\b", "x\\b",
    "x\\", "x\\\\",
    "\"", "\\\"",
    "x", "x",
    "\\x", "\\\\x",
    "xxx\\xxx", "xxx\\\\xxx",
    "\'" "\\" "\n" "\r" "\t", "\\\'" "\\\\" "\\n" "\\r" "\\t",

    NULL, NULL,
};

static void test_escape()
{
    int i;
    nx_string_t *str;

    for ( i = 0; esc[i] != NULL; i += 2 )
    {
	str = nx_string_create(esc[i], -1);
	nx_string_escape(str);
	if ( strcmp(str->buf, esc[i + 1]) != 0 )
	{
	    nx_abort("expected [%s], got [%s]", esc[i + 1], str->buf);
	}
	nx_string_free(str);
    }
}



static void test_ascii()
{
    unsigned int size = 200;
    char bin[size];
    char bin2[size];
    char ascii[size * 2 + 1];

    memset(bin, 'a', size);
    nx_bin2ascii(bin, size, ascii);
    nx_ascii2bin(ascii, size * 2, bin2);
    ASSERT(memcmp(bin, bin2, size) == 0);
}



static void test_strip_crlf()
{
    nx_string_t *str;

    str = nx_string_create("ab\r\n", 4);
    nx_string_strip_crlf(str);
    ASSERT(strcmp(str->buf, "ab") == 0);
    nx_string_free(str);

    str = nx_string_create("ab\r\n\r\n", -1);
    nx_string_strip_crlf(str);
    ASSERT(strcmp(str->buf, "ab\r\n") == 0);
    nx_string_free(str);

    str = nx_string_create("ab\n", -1);
    nx_string_strip_crlf(str);
    ASSERT(strcmp(str->buf, "ab") == 0);
    nx_string_free(str);

    str = nx_string_create("ab", -1);
    nx_string_strip_crlf(str);
    ASSERT(strcmp(str->buf, "ab") == 0);
    nx_string_free(str);

    str = nx_string_create("\r\n", -1);
    nx_string_strip_crlf(str);
    ASSERT(strcmp(str->buf, "") == 0);
    nx_string_free(str);

    str = nx_string_create("\n", -1);
    nx_string_strip_crlf(str);
    ASSERT(strcmp(str->buf, "") == 0);
    nx_string_free(str);
}



static void test_utf()
{
    //static const char *nihongo = "日本語";
    static const char *arviz_utf8 = "\xC3\xA1\x72\x76\xC3\xAD\x7A";
    static const char *arviz_iso8859_2 = "\xE1\x72\x76\xED\x7A";
    static const char *nihongo_shift_jis = "\x93\xFA\x96\x7B\x8C\xEA\x83\x65\x83\x58\x83\x67";
    static const char *nihongo_euc_jp = "\xC6\xFC\xCB\xDC\xB8\xEC\xA5\xC6\xA5\xB9\xA5\xC8";
    static const char *nihongo_utf8 = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88";
    nx_string_t *str;

    str = nx_string_create(arviz_utf8, -1);
    nx_string_validate_utf8(str, TRUE, FALSE);
    ASSERT(strcmp(arviz_utf8, str->buf) == 0);
    nx_string_free(str);

    str = nx_string_create(nihongo_utf8, -1);
    nx_string_validate_utf8(str, TRUE, FALSE);
    ASSERT(strcmp(nihongo_utf8, str->buf) == 0);
    nx_string_free(str);

    str = nx_string_create(arviz_iso8859_2, -1);
    nx_string_validate_utf8(str, TRUE, FALSE);
    ASSERT(strcmp("?rv?z", str->buf) == 0);
    nx_string_free(str);

    str = nx_string_create(nihongo_shift_jis, -1);
    nx_string_validate_utf8(str, TRUE, FALSE);
    nx_string_free(str);

    str = nx_string_create(nihongo_euc_jp, -1);
    nx_string_validate_utf8(str, TRUE, FALSE);
    nx_string_free(str);
}



int main(int argc UNUSED, const char * const *argv, const char * const *env UNUSED)
{
    nx_string_t *str, *str2;
    int i;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    str = nx_string_new();
    nx_string_free(str);

    str = nx_string_new();
    nx_string_kill(str);
    nx_string_free(str);

    str = nx_string_create("test", -1);
    ASSERT(strcmp(str->buf, "test") == 0);
    nx_string_free(str);

    str = nx_string_create("testtest", 4);
    ASSERT(strcmp(str->buf, "test") == 0);
    nx_string_free(str);

    str = nx_string_create("test", -1);
    nx_string_append(str, "test2", -1);
    ASSERT(strcmp(str->buf, "testtest2") == 0);
    nx_string_free(str);

    str = nx_string_create("test", 0);
    nx_string_append(str, "TEST", -1);
    ASSERT(strcmp(str->buf, "TEST") == 0);
    nx_string_free(str);

    str = nx_string_create("test", -1);
    nx_string_append(str, "test2", 4);
    ASSERT(strcmp(str->buf, "testtest") == 0);
    nx_string_free(str);

    str = nx_string_new();
    for ( i = 0; i < 1024; i++ )
    {
	nx_string_append(str, "test", 4);
    }
    nx_string_free(str);

    str = nx_string_create("test", -1);
    str2 = nx_string_clone(str);
    nx_string_free(str);
    ASSERT(strcmp(str2->buf, "test") == 0);
    nx_string_free(str2);

    str = nx_string_sprintf(NULL, "%s%s", "test", "TEST");
    ASSERT(strcmp(str->buf, "testTEST") == 0);
    nx_string_free(str);

    str = nx_string_new();
    nx_string_sprintf(str, "%s%s", "test", "TEST");
    ASSERT(strcmp(str->buf, "testTEST") == 0);
    nx_string_free(str);

    str = nx_string_new();
    nx_string_set(str, "test", -1);
    ASSERT(strcmp(str->buf, "test") == 0);
    nx_string_free(str);

    str = nx_string_new();
    nx_string_set(str, "testtest", 4);
    ASSERT(strcmp(str->buf, "test") == 0);
    nx_string_free(str);

    str = nx_string_create("test", -1);
    nx_string_set(str, "", -1);
    ASSERT(strcmp(str->buf, "") == 0);
    nx_string_free(str);

    str = nx_string_create("test", 4);
    nx_string_sprintf_append(str, "%s", "TEST");
    ASSERT(strcmp(str->buf, "testTEST") == 0);
    nx_string_free(str);

    str = nx_string_new();
    nx_string_sprintf_append(str, "%s", "TEST");
    ASSERT(strcmp(str->buf, "TEST") == 0);
    nx_string_free(str);

    test_escape();
    test_ascii();
    test_utf();
    test_strip_crlf();

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
