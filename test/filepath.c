/*
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include <check.h>

#include "utils/filepath.h"

START_TEST(filepath_expand_test)
{
	char **paths;

	ck_assert_int_eq(setenv("NETSURF_TEST_SHORT", "x", 1), 0);
	ck_assert_int_eq(setenv("NETSURF_TEST_LONG", "a-much-longer-directory-name", 1), 0);
	ck_assert_int_eq(setenv("NETSURF_TEST_EMPTY", "", 1), 0);
	ck_assert_int_eq(unsetenv("NETSURF_TEST_MISSING"), 0);
	paths = filepath_path_to_strvec(
		".:${NETSURF_TEST_MISSING}:${NETSURF_TEST_EMPTY}:"
		"prefix/${NETSURF_TEST_SHORT}${NETSURF_TEST_LONG}/suffix:"
		"${NETSURF_TEST_SHORT}:${unclosed");

	ck_assert_ptr_nonnull(paths);
	ck_assert_str_eq(paths[0], ".");
	ck_assert_str_eq(paths[1], "prefix/xa-much-longer-directory-name/suffix");
	ck_assert_str_eq(paths[2], "x");
	ck_assert_str_eq(paths[3], "${unclosed");
	ck_assert_ptr_null(paths[4]);
	filepath_free_strvec(paths);
}
END_TEST

START_TEST(filepath_vector_limit_test)
{
	char base[] = ".";
	char *directories[] = {base, NULL};
	const char *locales[257] = {0};
	char **paths;
	size_t i;

	/* Every localized path exists, so the vector must enforce its limit. */
	for (i = 0; i < 256; i++)
		locales[i] = ".";

	paths = filepath_generate(directories, locales);
	ck_assert_ptr_nonnull(paths);
	for (i = 0; paths[i] != NULL; i++) {
		ck_assert_uint_lt(i, 256);
	}
	ck_assert_str_eq(paths[i - 1], base);
	filepath_free_strvec(paths);
}
END_TEST

int main(void)
{
	Suite *suite = suite_create("Resource paths");
	TCase *tests = tcase_create("Resource search");
	SRunner *runner;
	int failed;

	tcase_add_test(tests, filepath_expand_test);
	tcase_add_test(tests, filepath_vector_limit_test);
	suite_add_tcase(suite, tests);
	runner = srunner_create(suite);
	srunner_run_all(runner, CK_NORMAL);
	failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
