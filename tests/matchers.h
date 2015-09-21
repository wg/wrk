#ifndef WRK_TESTS_MATCHERS_H
#define WRK_TESTS_MATCHERS_H

#include <stdbool.h>
#include <string.h>

/**
 * Checks if two strings are equal.
 */
static bool
equal_strings(const char* str1, const char* str2)
{
	return strcmp(str1, str2) == 0;
}

#endif /* WRK_TESTS_MATCHERS_H */
