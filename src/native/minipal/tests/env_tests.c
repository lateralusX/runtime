#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "../env.h"
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef _CRTDBG_MAP_ALLOC
static _CrtMemState eventpipe_memory_start_snapshot;
static _CrtMemState eventpipe_memory_end_snapshot;
static _CrtMemState eventpipe_memory_diff_snapshot;
#endif

static void env_put_s(const char* name, const char* value)
{
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

static void env_unset_s(const char* name)
{
#if defined(_WIN32)
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

int env_strncasecmp(const char* str1, const char* str2, size_t count)
{
    if (str1 == NULL || str2 == NULL || count == 0)
    {
        return 0;
    }

    for (size_t i = 0; i < count; ++i)
    {
        char c1 = str1[i];
        char c2 = str2[i];
        if (c1 == '\0' || c2 == '\0')
        {
            return (unsigned char)c1 - (unsigned char)c2;
        }

        if (c1 >= 'A' && c1 <= 'Z')
        {
            c1 += ('a' - 'A');
        }

        if (c2 >= 'A' && c2 <= 'Z')
        {
            c2 += ('a' - 'A');
        }

        if (c1 != c2)
        {
            return (unsigned char)c1 - (unsigned char)c2;
        }
    }

    return 0;
}

static int env_strcpy_s(char* dest, size_t destsz, const char* src)
{
    if (dest == NULL || src == NULL || destsz == 0)
    {
        if (dest && destsz > 0)
        {
            dest[0] = '\0';
        }

        return EINVAL;
    }

    size_t src_len = strlen(src);
    if (src_len + 1 > destsz)
    {
        dest[0] = '\0';
        return ERANGE;
    }

    memcpy(dest, src, src_len + 1);
    return 0;
}

static int env_strncpy_s(char* dest, size_t destsz, const char* src, size_t count)
{
    if (dest == NULL || src == NULL || destsz == 0)
    {
        if (dest && destsz > 0)
        {
            dest[0] = '\0';
        }

        return EINVAL;
    }

    if (count >= destsz)
    {
        dest[0] = '\0';
        return ERANGE;
    }

    size_t i = 0;
    for (; i < count && i < destsz - 1 && src[i] != '\0'; ++i)
    {
        dest[i] = src[i];
    }

    dest[i] = '\0';
    return 0;
}

static int env_strcat_s(char* dest, size_t destsz, const char* src)
{
    if (dest == NULL || src == NULL || destsz == 0)
    {
        if (dest && destsz > 0)
        {
            dest[0] = '\0';
        }

        return EINVAL;
    }

    size_t dest_len = 0;
    for (; dest_len < destsz; ++dest_len)
    {
        if (dest[dest_len] == '\0')
        {
            break;
        }
    }

    if (dest_len == destsz)
    {
        dest[0] = '\0';
        return ERANGE;
    }

    size_t src_len = strlen(src);
    if (dest_len + src_len + 1 > destsz)
    {
        dest[0] = '\0';
        return ERANGE;
    }

    memcpy(dest + dest_len, src, src_len + 1);
    return 0;
}

static int env_getenv_s(size_t *required_len, char *buffer, size_t buffer_len, const char *name)
{
    if (name == NULL || required_len == NULL || (buffer == NULL && buffer_len > 0))
    {
        return EINVAL;
    }

    char *value = getenv(name);
    size_t value_len = value ? strlen(value) : 0;

    *required_len = value ? (value_len + 1) : 0;

    if (buffer && buffer_len > 0)
    {
        if (!value)
        {
            buffer[0] = '\0';
            return 0;
        }

        if (buffer_len < value_len + 1)
        {
            buffer[0] = '\0';
            return ERANGE;
        }

        memcpy(buffer, value, value_len + 1);
    }

    return 0;
}

bool minipal_tests_env_load_environ(void)
{
    bool result = minipal_env_load_environ();
    assert(result);

    return true;
}

bool minipal_tests_env_unload_environ(void)
{
    minipal_env_unload_environ();
    return true;
}

bool minipal_tests_env_copy_free_environ(void)
{
    minipal_env_unload_environ();

    char** result = minipal_env_get_environ_copy();
    assert(result);

    minipal_env_free_environ(result);

    minipal_env_unload_environ();

    return true;
}

bool minipal_tests_env_get_environ(void)
{
    minipal_env_unload_environ();

    char** result = minipal_env_get_environ();
    assert(result);

    minipal_env_load_environ();

    char** result2 = minipal_env_get_environ();
    assert(result2);

    assert(result != result2);

    minipal_env_unload_environ();

    return true;
}

bool minipal_tests_env_exists(void)
{
    minipal_env_unload_environ();

    bool result = minipal_env_exists("PATH");
    assert(result);

    result = minipal_env_exists("minipal_tests_env_exists");
    assert(!result);

    minipal_env_set("minipal_tests_env_exists", "1", true);
    result = minipal_env_exists("minipal_tests_env_exists");
    assert(result);

    minipal_env_unset("minipal_tests_env_exists");
    result = minipal_env_exists("minipal_tests_env_exists");
    assert(!result);

    minipal_env_put("minipal_tests_env_exists=1");
    result = minipal_env_exists("minipal_tests_env_exists");
    assert(result);

    minipal_env_unload_environ();

    minipal_env_load_environ();

    result = minipal_env_exists("PATH");
    assert(result);

    result = minipal_env_exists("minipal_tests_env_exists");
    assert(!result);

    minipal_env_set("minipal_tests_env_exists", "1", true);
    result = minipal_env_exists("minipal_tests_env_exists");
    assert(result);

    minipal_env_unset("minipal_tests_env_exists");
    result = minipal_env_exists("minipal_tests_env_exists");
    assert(!result);

    minipal_env_put("minipal_tests_env_exists=1");
    result = minipal_env_exists("minipal_tests_env_exists");
    assert(result);

    minipal_env_unload_environ();

    return true;
}

bool minipal_tests_env_get_copy(void)
{
    minipal_env_unload_environ();

    char* path = minipal_env_get_copy("PATH");
    assert(path);

    char* env_path = getenv("PATH");
    assert(env_path);

    assert(strlen(path) == strlen(env_path));
    assert(!strcmp(path, env_path));

    free(path);

    minipal_env_unload_environ();

    env_put_s("minipal_tests_env_get", "1");

    char* value = minipal_env_get_copy("minipal_tests_env_get");
    assert(value);

    free(value);

    minipal_env_load_environ();

    value = minipal_env_get_copy("minipal_tests_env_get");
    assert(value);

    free(value);

    env_put_s("minipal_tests_env_get", "2");

    value = minipal_env_get_copy("minipal_tests_env_get");
    assert(value);
    assert(!strcmp(value, "1"));

    free(value);

    minipal_env_unload_environ();

    value = minipal_env_get_copy("minipal_tests_env_get");
    assert(value);
    assert(!strcmp(value, "2"));

    free(value);

    return true;
}

bool minipal_tests_env_get_s(void)
{
    size_t len = 0;
    bool result = false;

    minipal_env_unload_environ();

    result = minipal_env_get_s(&len, NULL, 0, "PATH");
    assert(result && len != 0);

    char* env_path = getenv("PATH");
    assert(env_path);

    assert(strlen(env_path) + 1 == len);

    char* path = malloc(len);
    assert(path);

    result = minipal_env_get_s(&len, path, len, "PATH");
    assert(result && len != 0);

    assert(strlen(path) == strlen(env_path));
    assert(!strcmp(path, env_path));

    free(path);

    result = minipal_env_get_s(NULL, NULL, 0, "PATH");
    assert(result);

    len = 0;
    result = minipal_env_get_s(&len, NULL, 0, "PATH");
    assert(result && len != 0);

    len = 0;
    result = minipal_env_get_s(&len, NULL, 0, "__PATH");
    assert(!result && len == 0);

    char buffer[10];
    len = 0;
    result = minipal_env_get_s(&len, buffer, 10, "PATH");
    assert(result && len > 10);

    result = minipal_env_set("minipal_tests_env_get_s", "123456789", true);
    assert(result);

    result = minipal_env_get_s(&len, buffer, 10, "minipal_tests_env_get_s");
    assert(result && len == 10 && strlen(buffer) == 9);
    assert(!strcmp(buffer, "123456789"));

    char path_buffer[1024];
    result = minipal_env_set("TMPDIR", "/tmp", true);
    assert(result);

    result = minipal_env_get_s(&len, path_buffer, 1024, "TMPDIR");
    assert(result && len > 0 && strlen(path_buffer) > 0);
    assert(!strcmp(path_buffer, "/tmp"));

    path_buffer[0] = '\0';
    result = minipal_env_get_s(&len, path_buffer, 4, "TMPDIR");
    assert(result && len == 5 && path_buffer[0] == '\0');

    minipal_env_unload_environ();

    return true;
}

static bool minipal_tests_env_get_s_cache_consistency(void)
{
    const char* varname = "MINIPAL_TEST_ENV_CACHE";
    const char* value = "cache_test_value";
    char buf_cache[64];
    char buf_nocache[64];
    size_t len_cache = 0, len_nocache = 0;
    bool result_cache, result_nocache;

    // Ensure a clean state
    minipal_env_unload_environ();

    // Set the environment variable using the system API
    env_put_s(varname, value);

    // Load the environment (this will cache it)
    minipal_env_load_environ();

    // Get value using cache
    result_cache = minipal_env_get_s(&len_cache, buf_cache, sizeof(buf_cache), varname);
    assert(result_cache);
    assert(len_cache == strlen(value) + 1);
    assert(strcmp(buf_cache, value) == 0);

    // Unload cache to force non-cached path
    minipal_env_unload_environ();

    // Get value without cache
    result_nocache = minipal_env_get_s(&len_nocache, buf_nocache, sizeof(buf_nocache), varname);
    assert(result_nocache);
    assert(len_nocache == strlen(value) + 1);
    assert(strcmp(buf_nocache, value) == 0);

    // Results must match
    assert(result_cache == result_nocache);
    assert(len_cache == len_nocache);
    assert(strcmp(buf_cache, buf_nocache) == 0);

    // Clean up
    env_unset_s(varname);

    minipal_env_unload_environ();

    return true;
}

static bool minipal_tests_env_get_s_api_consistency(void)
{
    const char* varname = "MINIPAL_TEST_ENV_API";
    const char* value = "api_test_value";
    char buf_cache[32], buf_nocache[32];
    size_t len_cache = 0, len_nocache = 0;
    bool result_cache, result_nocache;

    // Ensure a clean state
    minipal_env_unload_environ();

    env_put_s(varname, value);

    // --- Test: Variable exists, buffer large enough ---
    minipal_env_load_environ();
    result_cache = minipal_env_get_s(&len_cache, buf_cache, sizeof(buf_cache), varname);
    minipal_env_unload_environ();
    result_nocache = minipal_env_get_s(&len_nocache, buf_nocache, sizeof(buf_nocache), varname);

    assert(result_cache == result_nocache);
    assert(len_cache == len_nocache);
    assert(strcmp(buf_cache, buf_nocache) == 0);
    assert(strcmp(buf_cache, value) == 0);

    // --- Test: Variable exists, buffer too small ---
    minipal_env_load_environ();
    result_cache = minipal_env_get_s(&len_cache, buf_cache, 4, varname);
    minipal_env_unload_environ();
    result_nocache = minipal_env_get_s(&len_nocache, buf_nocache, 4, varname);

    assert(result_cache == result_nocache);
    assert(len_cache == len_nocache);
    assert(buf_cache[0] == '\0' && buf_nocache[0] == '\0');
    assert(len_cache == strlen(value) + 1);

    // --- Test: Variable does not exist ---
    const char* missing = "MINIPAL_TEST_ENV_API_MISSING";
    minipal_env_load_environ();
    result_cache = minipal_env_get_s(&len_cache, buf_cache, sizeof(buf_cache), missing);
    minipal_env_unload_environ();
    result_nocache = minipal_env_get_s(&len_nocache, buf_nocache, sizeof(buf_nocache), missing);

    assert(result_cache == result_nocache);
    assert(len_cache == len_nocache && len_cache == 0);
    assert(buf_cache[0] == '\0' && buf_nocache[0] == '\0');

    // --- Test: Buffer is NULL, get required length only ---
    minipal_env_load_environ();
    result_cache = minipal_env_get_s(&len_cache, NULL, 0, varname);
    minipal_env_unload_environ();
    result_nocache = minipal_env_get_s(&len_nocache, NULL, 0, varname);

    assert(result_cache == result_nocache);
    assert(len_cache == len_nocache);
    assert(len_cache == strlen(value) + 1);

    // --- Test: Buffer is NULL, variable does not exist ---
    minipal_env_load_environ();
    result_cache = minipal_env_get_s(&len_cache, NULL, 0, missing);
    minipal_env_unload_environ();
    result_nocache = minipal_env_get_s(&len_nocache, NULL, 0, missing);

    assert(result_cache == result_nocache);
    assert(len_cache == len_nocache && len_cache == 0);

    // --- Test: Buffer is NULL, len is NULL (should return true) ---
    minipal_env_load_environ();
    result_cache = minipal_env_get_s(NULL, NULL, 0, varname);
    minipal_env_unload_environ();
    result_nocache = minipal_env_get_s(NULL, NULL, 0, varname);

    assert(result_cache == result_nocache);

    // Clean up
    env_unset_s(varname);

    minipal_env_unload_environ();

    return true;
}

static bool minipal_tests_env_set(void)
{
    minipal_env_unload_environ();

    char* value = minipal_env_get_copy("__minipal_tests_env_set");
    assert(!value);

    bool result = minipal_env_set("__minipal_tests_env_set", "XYZ", true);
    assert(result);

    value = minipal_env_get_copy("__minipal_tests_env_set");
    assert(value);
    assert(!strcmp(value, "XYZ"));
    free(value);

    result = minipal_env_set("__minipal_tests_env_set", "1", false);
    assert(result);

    value = minipal_env_get_copy("__minipal_tests_env_set");
    assert(value);
    assert(!strcmp(value, "XYZ"));
    free(value);

    result = minipal_env_set("__minipal_tests_env_set", "1", true);
    assert(result);

    value = minipal_env_get_copy("__minipal_tests_env_set");
    assert(value);
    assert(!strcmp(value, "1"));
    free(value);

    result = minipal_env_set("__minipal_tests_env_set", "", true);
    assert(result);

    value = minipal_env_get_copy("__minipal_tests_env_set");
    assert(value);
    assert(!strcmp(value, ""));
    free(value);

    result = minipal_env_set("__minipal_tests_env_set", NULL, true);
    assert(result);

    value = minipal_env_get_copy("__minipal_tests_env_set");
    assert(value);
    assert(!strcmp(value, ""));
    free(value);

    minipal_env_unload_environ();

    return true;
}

static bool minipal_tests_env_put(void)
{
    minipal_env_unload_environ();

    char* value = minipal_env_get_copy("__minipal_tests_env_put");
    assert(!value);

    bool result = minipal_env_put("__minipal_tests_env_put=XYZ");
    assert(result);

    value = minipal_env_get_copy("__minipal_tests_env_put");
    assert(value);

    assert(!strcmp(value, "XYZ"));

    free(value);

    result = minipal_env_put("__minipal_tests_env_put=newValue");
    assert(result);

    value = minipal_env_get_copy("__minipal_tests_env_put");
    assert(value);

    assert(!strcmp(value, "newValue"));

    free(value);

    minipal_env_unload_environ();

    return true;
}

static bool minipal_tests_env_unset(void)
{
    minipal_env_unload_environ();

    char* value = minipal_env_get_copy("minipal_tests_env_unset");
    assert(!value);

    bool result = minipal_env_set("minipal_tests_env_unset", "123", true);
    assert(result);

    value = minipal_env_get_copy("minipal_tests_env_unset");
    assert(value);

    free(value);

    result = minipal_env_unset("minipal_tests_env_unset");
    assert(result);

    value = minipal_env_get_copy("minipal_tests_env_unset");
    assert(!value);

    result = minipal_env_unset("minipal_tests_env_unset");
    assert(result);

    free(value);

    minipal_env_unload_environ();

    return true;
}

bool minipal_tetst_env_foreach_callback(const char* env_s, void* cookie)
{
    assert(env_s != NULL && cookie != NULL);
    (*(size_t *)cookie)++;
    return true;
}

bool minipal_tetst_env_foreach_callback_break(const char* env_s, void* cookie)
{
    assert(env_s != NULL && cookie != NULL);
    (*(size_t *)cookie)++;

    if ((*(size_t*)cookie) >= 5)
    {
        return false;
    }

    return true;
}

static bool minipal_test_env_merge(void)
{
    minipal_env_unload_environ();

    bool result = minipal_env_set("minipal_test_env_merge_1", "1", true);
    assert(result);

    result = minipal_env_set("minipal_test_env_merge_2", "2", true);
    assert(result);

    result = minipal_env_set("minipal_test_env_merge_3", "3", true);
    assert(result);

    result = minipal_env_load_environ();
    assert(result);

    char* value = minipal_env_get_copy("minipal_test_env_merge_1");
    assert(value);
    assert(!strcmp(value, "1"));

    free(value);

    value = minipal_env_get_copy("minipal_test_env_merge_2");
    assert(value);
    assert(!strcmp(value, "2"));

    free(value);

    value = minipal_env_get_copy("minipal_test_env_merge_3");
    assert(value);
    assert(!strcmp(value, "3"));

    free(value);

    minipal_env_unload_environ();

    return true;
}

static bool minipal_test_env_cache(void)
{
    minipal_env_unload_environ();

    env_put_s("minipal_test_env_cache", "1");

    char* value = minipal_env_get_copy("minipal_test_env_cache");
    assert(value);
    assert(!strcmp(value, "1"));
    free(value);

    bool result = minipal_env_unset("minipal_test_env_cache");
    assert(result);

    value = minipal_env_get_copy("minipal_test_env_cache");
    assert(!value);

    result = minipal_env_set("minipal_test_env_cache", "", true);
    assert(result);

    value = minipal_env_get_copy("minipal_test_env_cache");
    assert(value);
    assert(value[0] == '\0');
    free(value);

    result = minipal_env_set("minipal_test_env_cache", NULL, true);
    assert(result);

    value = minipal_env_get_copy("minipal_test_env_cache");
    assert(value);
    assert(value[0] == '\0');
    free(value);

    result = minipal_env_unset("minipal_test_env_cache");
    assert(result);

    value = minipal_env_get_copy("minipal_test_env_cache");
    assert(!value);

    result = minipal_env_put("minipal_test_env_cache=1");
    assert(result);

    value = minipal_env_get_copy("minipal_test_env_cache");
    assert(value);
    assert(!strcmp(value, "1"));
    free(value);

    result = minipal_env_put("minipal_test_env_cache=");
    assert(result);

    value = minipal_env_get_copy("minipal_test_env_cache");
    assert(value);
    assert(value[0] == '\0');
    free(value);

    minipal_env_unload_environ();

    minipal_env_load_environ();

    result = minipal_env_unset("minipal_test_env_cache");
    assert(result);

    value = minipal_env_get_copy("minipal_test_env_cache");
    assert(!value);
    free(value);

    minipal_env_unload_environ();

    return true;
}

static bool minipal_test_env_resize(void)
{
    minipal_env_unload_environ();

    for (int i = 0; i < 1000; ++i)
    {
        char env_name[32];
        char env_value[32];
        snprintf(env_name, sizeof(env_name), "minipal_test_env_resize_%d", i);
        snprintf(env_value, sizeof(env_value), "%d", i);
        bool result = minipal_env_set(env_name, env_value, true);
        assert(result);
    }

    char** env = minipal_env_get_environ();
    assert(env);

    for (int i = 0; i < 1000; ++i)
    {
        char env_string[64];
        assert(env[i] != NULL);

        snprintf(env_string, sizeof(env_string), "minipal_test_env_resize_%d=%d", i, i);
        assert(strcmp(env[i], env_string) == 0);
    }

    minipal_env_unload_environ();

    return true;
}
static bool minipal_test_strncasecmp(void)
{
    // Equal strings, same case
    assert(env_strncasecmp("abc", "abc", 3) == 0);

    // Equal strings, different case
    assert(env_strncasecmp("abc", "ABC", 3) == 0);

    // Prefix match, but one is longer
    assert(env_strncasecmp("abc", "abcd", 3) == 0);
    assert(env_strncasecmp("abcd", "abc", 4) > 0);
    assert(env_strncasecmp("abc", "abcd", 4) < 0);

    // Completely different
    assert(env_strncasecmp("abc", "def", 3) < 0);
    assert(env_strncasecmp("def", "abc", 3) > 0);

    // Empty strings
    assert(env_strncasecmp("", "", 1) == 0);
    assert(env_strncasecmp("abc", "", 3) > 0);
    assert(env_strncasecmp("", "abc", 3) < 0);

    // Partial match, then difference
    assert(env_strncasecmp("abc", "abD", 3) < 0);
    assert(env_strncasecmp("abD", "abc", 3) > 0);

    // n == 0 should always return 0
    assert(env_strncasecmp("abc", "def", 0) == 0);

    // NULL arguments (your implementation returns 0)
    assert(env_strncasecmp(NULL, "abc", 3) == 0);
    assert(env_strncasecmp("abc", NULL, 3) == 0);
    assert(env_strncasecmp(NULL, NULL, 3) == 0);

    // n == 0 with NULLs
    assert(env_strncasecmp(NULL, NULL, 0) == 0);
    assert(env_strncasecmp("abc", NULL, 0) == 0);
    assert(env_strncasecmp(NULL, "abc", 0) == 0);

    return true;
}

bool minipal_test_strcpy_s(void)
{
    char buf[8];

    // Normal copy
    assert(env_strcpy_s(buf, sizeof(buf), "abc") == 0);
    assert(strcmp(buf, "abc") == 0);

    // Buffer too small
    assert(env_strcpy_s(buf, 4, "abcd") == ERANGE);
    assert(buf[0] == '\0');

    // Null src
    assert(env_strcpy_s(buf, sizeof(buf), NULL) == EINVAL);
    assert(buf[0] == '\0');

    // Null dest
    assert(env_strcpy_s(NULL, sizeof(buf), "abc") == EINVAL);

    // Zero size
    assert(env_strcpy_s(buf, 0, "abc") == EINVAL);

    return true;
}

bool minipal_test_strncpy_s(void)
{
    char buf[8];

    // Normal copy, count < src length
    assert(env_strncpy_s(buf, sizeof(buf), "abcdef", 3) == 0);
    assert(strcmp(buf, "abc") == 0);

    // Normal copy, count > src length
    assert(env_strncpy_s(buf, sizeof(buf), "abc", 6) == 0);
    assert(strcmp(buf, "abc") == 0);

    // Buffer too small
    assert(env_strncpy_s(buf, 4, "abcdef", 4) == ERANGE);
    assert(buf[0] == '\0');

    // Null src
    assert(env_strncpy_s(buf, sizeof(buf), NULL, 3) == EINVAL);
    assert(buf[0] == '\0');

    // Null dest
    assert(env_strncpy_s(NULL, sizeof(buf), "abc", 3) == EINVAL);

    // Zero size
    assert(env_strncpy_s(buf, 0, "abc", 3) == EINVAL);

    return true;
}

bool minipal_test_strcat_s(void)
{
    char buf[8];

    // Normal concat
    strcpy(buf, "abc");
    assert(env_strcat_s(buf, sizeof(buf), "de") == 0);
    assert(strcmp(buf, "abcde") == 0);

    // Buffer too small
    strcpy(buf, "abcdef");
    assert(env_strcat_s(buf, sizeof(buf), "gh") == ERANGE);
    assert(buf[0] == '\0');

    // Null src
    strcpy(buf, "abc");
    assert(env_strcat_s(buf, sizeof(buf), NULL) == EINVAL);
    assert(buf[0] == '\0');

    // Null dest
    assert(env_strcat_s(NULL, sizeof(buf), "abc") == EINVAL);

    // Zero size
    assert(env_strcat_s(buf, 0, "abc") == EINVAL);

    return true;
}

bool minipal_test_getenv_s(void)
{
    char buf[64];
    size_t required;

    // Set an environment variable for testing
    env_put_s("MINIPAL_TEST_ENV", "value");

    // Normal get
    assert(env_getenv_s(&required, buf, sizeof(buf), "MINIPAL_TEST_ENV") == 0);
    assert(strcmp(buf, "value") == 0);
    assert(required == strlen("value") + 1);

    // Buffer too small
    assert(env_getenv_s(&required, buf, 3, "MINIPAL_TEST_ENV") == ERANGE);
    assert(buf[0] == '\0');
    assert(required == strlen("value") + 1);

    // Not found
    assert(env_getenv_s(&required, buf, sizeof(buf), "MINIPAL_TEST_ENV_NOTFOUND") == 0);
    assert(buf[0] == '\0');
    assert(required == 0);

    // Null name
    assert(env_getenv_s(&required, buf, sizeof(buf), NULL) == EINVAL);

    // Null required_len
    assert(env_getenv_s(NULL, buf, sizeof(buf), "MINIPAL_TEST_ENV") == EINVAL);

    // Null buffer with nonzero size
    assert(env_getenv_s(&required, NULL, 1, "MINIPAL_TEST_ENV") == EINVAL);

    return true;
}

int main(int argc, char** argv, char** envp)
{
    minipal_tests_env_load_environ();
    minipal_tests_env_unload_environ();


#ifdef _CRTDBG_MAP_ALLOC
    _CrtMemCheckpoint (&eventpipe_memory_start_snapshot);
#endif

    minipal_tests_env_copy_free_environ();
    minipal_tests_env_get_environ();
    minipal_tests_env_exists();
    minipal_tests_env_get_copy();
    minipal_tests_env_get_s();
    minipal_tests_env_get_s_cache_consistency();
    minipal_tests_env_get_s_api_consistency();
    minipal_tests_env_set();
    minipal_tests_env_put();
    minipal_tests_env_unset();
    minipal_test_env_merge();
    minipal_test_env_cache();
    minipal_test_env_resize();
    minipal_test_strncasecmp();
    minipal_test_strcpy_s();
    minipal_test_strncpy_s();
    minipal_test_strcat_s();
    minipal_test_getenv_s();

#ifdef _CRTDBG_MAP_ALLOC
    _CrtMemCheckpoint (&eventpipe_memory_end_snapshot);
	if ( _CrtMemDifference(&eventpipe_memory_diff_snapshot, &eventpipe_memory_start_snapshot, &eventpipe_memory_end_snapshot) ) {
		_CrtMemDumpStatistics( &eventpipe_memory_diff_snapshot );
        assert(!"MEMORY LEAK DETECTED!!!!");
	}
#endif

    return 0;
}
