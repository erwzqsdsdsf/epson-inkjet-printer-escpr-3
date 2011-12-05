/*
 * EPSON ESC/P-R Printer Driver for Linux
 * Copyright (C) 2002-2005 AVASYS CORPORATION.
 * Copyright (C) Seiko Epson Corporation 2002-2005.
 *
 *  This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA.
 *
 * As a special exception, AVASYS CORPORATION gives permission to
 * link the code of this program with libraries which are covered by
 * the AVASYS Public License and distribute their linked
 * combinations.  You must obey the GNU General Public License in all
 * respects for all of the code used other than the libraries which
 * are covered by AVASYS Public License.
 */
#ifndef CSV_H
#define CSV_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "def.h"

typedef struct csvlist_str
{
	char *path;
	char *top_p;
	long size;

	char **list;
	int lnum;

} csvlist_t;

BEGIN_C

csvlist_t *csvlist_open (const char *);
void csvlist_close (csvlist_t *);
int csvlist_search_keyword (csvlist_t *, int, const char *);
char * csvlist_get_word (csvlist_t *, int);
int csvlist_get_max (csvlist_t *);

END_C

#endif /* CSV_H */
