/*
 * Copyright 2010 Vincent Sanders <vince@kyllikki.org>
 *
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

/**
 * \file
 * Provides utility functions for finding readable files.
 *
 * These functions are intended to make finding resource files more
 *  straightforward.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utils/dirent.h" /** \todo why is this necessary for atari to get PATH_MAX and is there a better way */
#include "utils/utils.h"
#include "utils/config.h"
#include "utils/filepath.h"

/** maximum number of elements in the resource vector */
#define MAX_RESPATH 128

/* exported interface documented in filepath.h */
char *filepath_vsfindfile(char *str, const char *format, va_list ap)
{
	char *realpathname;
	char *pathname;
	int len;

	pathname = malloc(PATH_MAX);
	if (pathname == NULL)
		return NULL; /* unable to allocate memory */

	len = vsnprintf(pathname, PATH_MAX, format, ap);

	if ((len < 0) || (len >= PATH_MAX)) {
		/* error or output exceeded PATH_MAX length so
		 * operation is doomed to fail.
		 */
		free(pathname);
		return NULL;
	}

	realpathname = realpath(pathname, str);

	free(pathname);

	if (realpathname != NULL) {
		/* sucessfully expanded pathname */
		if (access(realpathname, R_OK) != 0) {
			/* realpath allocates the result when no buffer was supplied. */
			if (str == NULL)
				free(realpathname);
			return NULL;
		}
	}

	return realpathname;
}


/* exported interface documented in filepath.h */
char *filepath_sfindfile(char *str, const char *format, ...)
{
	va_list ap;
	char *ret;

	va_start(ap, format);
	ret = filepath_vsfindfile(str, format, ap);
	va_end(ap);

	return ret;
}


/* exported interface documented in filepath.h */
char *filepath_findfile(const char *format, ...)
{
	char *ret;
	va_list ap;

	va_start(ap, format);
	ret = filepath_vsfindfile(NULL, format, ap);
	va_end(ap);

	return ret;
}

/* exported interface documented in filepath.h */
char *filepath_sfind(char **respathv, char *filepath, const char *filename)
{
	int respathc = 0;

	if ((respathv == NULL) || (respathv[0] == NULL) || (filepath == NULL))
		return NULL;

	while (respathv[respathc] != NULL) {
		if (filepath_sfindfile(filepath, "%s/%s", respathv[respathc], filename) != NULL) {
			return filepath;
		}

		respathc++;
	}

	return NULL;
}


/* exported interface documented in filepath.h */
char *filepath_find(char **respathv, const char *filename)
{
	char *ret;
	char *filepath;

	if ((respathv == NULL) || (respathv[0] == NULL))
		return NULL;

	filepath = malloc(PATH_MAX);
	if (filepath == NULL)
		return NULL;

	ret = filepath_sfind(respathv, filepath, filename);

	if (ret == NULL)
		free(filepath);

	return ret;
}


/* exported interface documented in filepath.h */
char *
filepath_sfinddef(char **respathv,
		  char *filepath,
		  const char *filename,
		  const char *def)
{
	char t[PATH_MAX];
	char *ret;

	if ((respathv == NULL) || (respathv[0] == NULL) || (filepath == NULL))
		return NULL;

	ret = filepath_sfind(respathv, filepath, filename);

	if ((ret == NULL) && (def != NULL)) {
		/* search failed, return the path specified */
		ret = filepath;
		if (def[0] == '~') {
			snprintf(t, PATH_MAX, "%s/%s/%s", getenv("HOME"), def + 1, filename);
		} else {
			snprintf(t, PATH_MAX, "%s/%s", def, filename);
		}
		if (realpath(t, ret) == NULL) {
			strncpy(ret, t, PATH_MAX);
		}

	}
	return ret;
}


/* exported interface documented in filepath.h */
char **
filepath_generate(char * const *pathv, const char * const *langv)
{
	char **respath; /* resource paths vector */
	int pathc = 0;
	int langc = 0;
	int respathc = 0;
	struct stat dstat;
	char *tmppath;
	int tmppathlen;

	if (pathv == NULL || langv == NULL)
		return NULL;

	respath = calloc(MAX_RESPATH, sizeof(char *));

	while ((respath != NULL) &&
	       (pathv[pathc] != NULL) &&
	       (respathc < MAX_RESPATH - 1)) {
		if ((stat(pathv[pathc], &dstat) == 0) &&
		    S_ISDIR(dstat.st_mode)) {
			/* path element exists and is a directory */
			langc = 0;
			/* Reserve slots for the base directory and NULL terminator. */
			while ((langv[langc] != NULL) &&
			       (respathc < MAX_RESPATH - 2)) {
				tmppathlen = snprintf(NULL,
						      0,
						      "%s/%s",
						      pathv[pathc],
						      langv[langc]);
				if (tmppathlen < 0)
					break;
				tmppath = malloc((size_t)tmppathlen + 1);
				if (tmppath == NULL) {
					break;
				}
				snprintf(tmppath,
					 (size_t)tmppathlen + 1,
					 "%s/%s",
					 pathv[pathc],
					 langv[langc]);

				if ((stat(tmppath, &dstat) == 0) &&
				    S_ISDIR(dstat.st_mode)) {
					/* path element exists and is a directory */
					respath[respathc++] = tmppath;
				} else {
					free(tmppath);
				}

				langc++;
			}
			/* Keep the vector contiguous if allocating the base path fails. */
			tmppath = strdup(pathv[pathc]);
			if (tmppath == NULL) {
				filepath_free_strvec(respath);
				return NULL;
			}
			respath[respathc++] = tmppath;
		}
		pathc++;
	}
	return respath;
}


/**
 * expand ${} in a string into environment variables.
 *
 * @param path The pathname to expand.
 * @param pathlen The length of the path element.
 * @return A string with the expanded path or NULL on empty expansion or error.
 */
static char *
expand_path(const char *path, size_t pathlen)
{
	char *exp = malloc(pathlen + 1);
	size_t explen = pathlen;
	size_t cursor = 0;

	if (exp == NULL)
		return NULL;

	memcpy(exp, path, pathlen);
	exp[pathlen] = 0;

	while (cursor < explen) {
		char *closing;
		const char *value;
		size_t end;
		size_t tokenlen;
		size_t valuelen;

		if (exp[cursor] != '$' || exp[cursor + 1] != '{') {
			cursor++;
			continue;
		}
		closing = strchr(exp + cursor + 2, '}');
		if (closing == NULL)
			break;

		end = closing - exp + 1;
		tokenlen = end - cursor;
		*closing = 0;
		value = getenv(exp + cursor + 2);
		valuelen = value == NULL ? 0 : strlen(value);

		/* Grow before copying. Keep the unread suffix intact on shrink. */
		if (valuelen > tokenlen) {
			char *larger;

			if (valuelen > SIZE_MAX - (explen - tokenlen) - 1) {
				free(exp);
				return NULL;
			}
			larger = realloc(exp, explen - tokenlen + valuelen + 1);
			if (larger == NULL) {
				free(exp);
				return NULL;
			}
			exp = larger;
		}
		memmove(exp + cursor + valuelen, exp + end, explen - end + 1);
		if (valuelen != 0)
			memcpy(exp + cursor, value, valuelen);
		explen = explen - tokenlen + valuelen;
		/* Continue with the original suffix, including adjacent tokens. */
		cursor += valuelen;
	}

	if (explen == 0) {
		free(exp);
		return NULL;
	}

	return exp;
}


/* exported interface documented in filepath.h */
char **
filepath_path_to_strvec(const char *path)
{
	char **strvec;
	int strc = 0;
	const char *estart; /* path element start */
	const char *eend; /* path element end */
	size_t elen;

	strvec = calloc(MAX_RESPATH, sizeof(char *));
	if (strvec == NULL)
		return NULL;

	/* Treat an omitted search path as an empty list. */
	if (path == NULL)
		return strvec;

	estart = eend = path;

	while (strc < (MAX_RESPATH - 2)) {
		while ( (*eend != 0) && (*eend != ':') )
			eend++;
		elen = eend - estart;

		if (elen > 0) {
			/* A one-character directory such as "." is valid. */
			strvec[strc] = expand_path(estart, elen);
			if (strvec[strc] != NULL) {
				/* successfully expanded an element */
				strc++;
			}
		}

		/* skip colons */
		while (*eend == ':')
			eend++;

		/* check for termination */
		if (*eend == 0)
			break;

		estart = eend;
	}
	return strvec;
}


/* exported interface documented in filepath.h */
void filepath_free_strvec(char **pathv)
{
	int p = 0;

	if (pathv == NULL)
		return;

	while (pathv[p] != NULL) {
		free(pathv[p++]);
	}
	free(pathv);
}
