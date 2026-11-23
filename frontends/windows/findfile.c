/*
 * Copyright 2008 Daniel Silverstone <dsilvers@netsurf-browser.org>
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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

#include <winsock2.h>
#include <windows.h>

#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/log.h"
#include "utils/url.h"
#include "utils/utils.h"
#include "utils/filepath.h"

#include "windows/findfile.h"

/** Create an array of valid paths to search for resources.
 *
 * The idea is that all the complex path computation to find resources
 * is performed here, once, rather than every time a resource is
 * searched for.
 */
char **
nsws_init_resource(const char *resource_path)
{
	char **pathv; /* resource path string vector */
	char **respath; /* resource paths vector */
	const char *lang = NULL;
	char winpath[MAX_PATH];
	const char suffix[] = "windows\\res";
	DWORD length;
	size_t pathi;
	char *slsh;

	pathv = filepath_path_to_strvec(resource_path);
	if (pathv == NULL)
		return NULL;
 
	/* Skip the optional path if Windows cannot return a complete name. */
	length = GetModuleFileNameA(NULL, winpath, sizeof(winpath));
	if (length != 0 && length < sizeof(winpath)) {
		slsh = strrchr(winpath, '\\');
		if (slsh != NULL) {
			char *leaf = slsh + 1;

			/* Installed and portable builds keep resources by the executable.
			 * Keep the legacy windows\res layout for development builds.
			 */
			*leaf = '\0';
			if ((size_t)(leaf - winpath) <= sizeof(winpath) - sizeof(suffix)) {
				DWORD attrs;

				memcpy(leaf, suffix, sizeof(suffix));
				attrs = GetFileAttributesA(winpath);
				if (attrs == INVALID_FILE_ATTRIBUTES ||
				    (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0)
					*leaf = '\0';
			}
			pathi = 0;
			while (pathv[pathi] != NULL)
				pathi++;
			pathv[pathi] = strdup(winpath);
		}
	}

	respath = filepath_generate(pathv, &lang);

	filepath_free_strvec(pathv);

	return respath;
}

static char *realpath(const char *path, char *resolved_path)
{
	/* useless, but there we go */
	return strncpy(resolved_path, path, PATH_MAX);
}


/**
 * Locate a shared resource file by searching known places in order.
 *
 * Search order is: ~/.netsurf/, $NETSURFRES/ (where NETSURFRES is an
 * environment variable), then the path specified in
 * NETSURF_WINDOWS_RESPATH in the Makefile then .\\res\\ [windows paths]
 *
 * \param  buf      buffer to write to.  must be at least PATH_MAX chars
 * \param  filename file to look for
 * \param  def      default to return if file not found
 * \return The passed in buffer
 */

char *nsws_find_resource(char *buf, const char *filename, const char *def)
{
	char *cdir = getenv("HOME");
	char t[PATH_MAX];

	if (cdir != NULL) {
		NSLOG(netsurf, INFO, "Found Home %s", cdir);
		strcpy(t, cdir);
		strcat(t, "/.netsurf/");
		strcat(t, filename);
		if ((realpath(t, buf) != NULL)  && (access(buf, R_OK) == 0))
			return buf;
	}

	cdir = getenv("NETSURFRES");

	if (cdir != NULL) {
		if (realpath(cdir , buf) != NULL) {
			strcat(buf, "/");
			strcat(buf, filename);
			if (access(buf, R_OK) == 0)
				return buf;
		}
	}

	strcpy(t, NETSURF_WINDOWS_RESPATH);
	strcat(t, filename);
	if ((realpath(t, buf) != NULL) && (access(buf, R_OK) == 0))
		return buf;
	
	getcwd(t, PATH_MAX - SLEN("\\res\\") - strlen(filename));
	strcat(t, "\\res\\");
	strcat(t, filename);
	NSLOG(netsurf, INFO, "looking in %s", t);
	if ((realpath(t, buf) != NULL) && (access(buf, R_OK) == 0))
		return buf;

	if (def[0] == '~') {
		snprintf(t, PATH_MAX, "%s%s", getenv("HOME"), def + 1);
		if (realpath(t, buf) == NULL) {
			strcpy(buf, t);
		}
	} else {
		if (realpath(def, buf) == NULL) {
			strcpy(buf, def);
		}
	}

	return buf;
}


/*
 * Local Variables:
 * c-basic-offset: 8
 * End:
 */

