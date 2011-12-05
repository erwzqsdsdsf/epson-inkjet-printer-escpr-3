/*
 * Epson Inkjet Printer Driver (ESC/P-R) for Linux
 * Copyright (C) 2002-2005 AVASYS CORPORATION.
 * Copyright (C) Seiko Epson Corporation 2002-2009.
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
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/* added 22-04-2004 */
#include <signal.h>

#define NAME_MAX 41

#define WIDTH_BYTES(bits) (((bits) + 31) / 32 * 4)

#define PIPSLITE_WRAPPER_VERSION "* epson-escpr-wrapepr is a part of " PACKAGE_STRING


typedef struct rtp_filter_option {
	char model[NAME_MAX + 1];
	char model_low[NAME_MAX + 1];
	char ink[NAME_MAX + 1];
	char media[NAME_MAX + 1];
	char quality[NAME_MAX + 1];
} filter_option_t;

/* Static functions */
static int get_option_for_ppd (const char *, filter_option_t *);
static int get_option_for_arg (const char *, filter_option_t *);
/* Get value for PPD. */
static char * get_default_choice (ppd_file_t *, const char *);

/* added 22-04-2004 */
static void sig_set (void);
static void sigterm_handler (int sig);

int cancel_flg;

/*
 * $$$ CUPS Filter Options $$$
 *
 * printer  - The name of the printer queue
 *            (normally this is the name of the program being run)
 * job      - The numeric job ID for the job being printed
 * user     - The string from the originating-user-name attribute
 * title    - The string from the job-name attribute
 * copies   - The numeric value from the number-copies attribute
 * options  - String representations of the job template attributes, separated
 *            by spaces. Boolean attributes are provided as "name" for true
 *            values and "noname" for false values. All other attributes are
 *            provided as "name=value" for single-valued attributes and
 *            "name=value1,value2,...,valueN" for set attributes
 * filename - The request file
 */
int
main (int argc, char *argv[])
{
	int fd;			/* file descriptor */
	FILE *pfp;
	int i;			/* loop */
	cups_raster_t *ras;	/* raster stream for printing */
	cups_page_header_t header; /* page device dictionary header */
	filter_option_t fopt;

/* attach point */
#ifdef USE_DEBUGGER
	int flag = 1;
	while (flag) sleep (3);
#endif /* USE_DEBUGGER */


	cancel_flg = 0;
	memset (&fopt, 0, sizeof (filter_option_t));
	/* added 22-04-2004 */
	sig_set();

	if (argc < 6 || argc > 7)
	{
		for ( i = 1; i < argc; i++ ) {
			if ( (0 == strncmp(argv[i], "-v", (strlen("-v")+1)) )
				|| (0 == strncmp(argv[i], "--version", (strlen("--version")+1)) )
				) {
				fprintf(stderr, "%s\n", PIPSLITE_WRAPPER_VERSION);
				return 0;
			} else if ( (0 == strncmp(argv[i], "-h", (strlen("-h")+1)) )
				|| (0 == strncmp(argv[i], "--help", (strlen("--help")+1)) )
				) {
				fprintf(stderr, "%s\n", PIPSLITE_WRAPPER_VERSION);
				return 0;
			}
		}
		fprintf (stderr, "Insufficient options.\n");
		return 1;
	}

	if (argc == 7)
	{
		fd = open (argv[6], O_RDONLY);
		if (fd < 0)
		{
			perror ("open");
			return 1;
		}
	}
	else
	{
		fd = 0;
	}
	 
	if (get_option_for_arg (argv[5], &fopt))
	{
		fprintf (stderr, "Cannot read filter option. Cannot get option of PIPS.");
		return 1;
	}

	if (get_option_for_ppd (argv[0], &fopt))
	{
		fprintf (stderr, "PPD file not found, or PPD file is broken. Cannot get option of PIPS.");
		return 1;
	}

	/* Print start */
	ras = cupsRasterOpen (fd, CUPS_RASTER_READ);
	if (ras == NULL)
	{
		fprintf (stderr, "Can't open CUPS raster file.");
		return 1;
	}

	pfp = NULL;
	while (cupsRasterReadHeader (ras, &header) && !cancel_flg)
	{
		int image_bytes;
		char *image_raw;
		int write_size = 0;

		if (pfp == NULL)
		{
			char tmpbuf[256];

			sprintf (tmpbuf, "%s/%s \"%s\" %d %d %d %s %s %s",
				 CUPS_FILTER_PATH,
				 CUPS_FILTER_NAME,
				 fopt.model,
				 header.cupsWidth,
				 header.cupsHeight,
				 header.HWResolution[0],
				 fopt.ink,
				 fopt.media,
				 fopt.quality);

			pfp = popen (tmpbuf, "w");

			if (pfp == NULL)
			{
				perror ("popen");
				return 1;
			}
		}

		image_bytes = WIDTH_BYTES(header.cupsBytesPerLine * 8);
		image_raw = (char *)calloc (sizeof (char), image_bytes);

		for (i = 0; i < header.cupsHeight && !cancel_flg; i ++)
		{
			if (!cupsRasterReadPixels (ras, (unsigned char*)image_raw, header.cupsBytesPerLine))
			{
				fprintf (stderr, "cupsRasterReadPixels");
				return 1;
			}
			
			write_size = fwrite (image_raw, image_bytes, 1, pfp);
			if (write_size != 1)
			{
				perror ("fwrite");
				return 8;
			}
		}
		
		free (image_raw);
	}
	
	pclose (pfp);
	cupsRasterClose (ras);
	return 0;
}


static int
get_option_for_ppd (const char *printer, filter_option_t *filter_opt_p)
{
	char *ppd_path;		/* Path of PPD */
	ppd_file_t *ppd_p;	/* Struct of PPD */
	char *opt;		/* Temporary buffer (PPD option) */
	int i;			/* loop */

	/* Get option from PPD. */
	ppd_path = (char *) cupsGetPPD (printer);
	ppd_p = ppdOpenFile (ppd_path);
	if(NULL == ppd_p){
		return 1;
	}

	/* Make library file name */
	strcpy (filter_opt_p->model, ppd_p->modelname);
	for (i = 0; filter_opt_p->model[i] != '\0' && i < NAME_MAX; i ++)
		filter_opt_p->model_low[i] = tolower (filter_opt_p->model[i]);
	filter_opt_p->model_low[i] = '\0';

	/* media */
	if (filter_opt_p->media[0] == '\0')
	{
		opt = get_default_choice (ppd_p, "PageSize");
		if (!opt)
			return 1;

		strcpy (filter_opt_p->media, opt);
	}

	/* ink */
	if (filter_opt_p->ink[0] == '\0')
	{
		opt = get_default_choice (ppd_p, "Ink");
		if (!opt)
			return 1;

		strcpy (filter_opt_p->ink, opt);
	}

	/* quality */
	if (filter_opt_p->quality[0] == '\0')
	{
		opt = get_default_choice (ppd_p, "Quality");
		if (!opt)
			return 1;

		strcpy (filter_opt_p->quality, opt);
	}

#ifdef INK_CHANGE_SYSTEM
	/* inkset */
	if (filter_opt_p->inkset[0] == '\0')
	{
		opt = get_default_choice (ppd_p, "InkSet");
		if (!opt)
			return 1;

		strcpy (filter_opt_p->inkset, opt);
	}
#endif

	ppdClose (ppd_p);
	return 0;
}


static int
get_option_for_arg (const char *opt_str, filter_option_t *filter_opt_p)
{
	int opt_num;
	cups_option_t *option_p;
	const char *opt;

	const char *media_names[] = { "PageSize", "PageRegion",  "media", "" };
	int i;

	if (strlen (opt_str) == 0)
		return 0;

	opt_num = 0;
	option_p = NULL;
	opt_num = cupsParseOptions (opt_str, opt_num, &option_p);

	for (i = 0; *media_names[i] != '\0'; i ++)  /* chenged Wed Jan 28 2009 */
	{
		opt = cupsGetOption (media_names[i], opt_num, option_p);
		if (opt)
		{
			int num;

			num = strcspn (opt, ",");
			if (num >= PPD_MAX_NAME)
				return 1;

			strncpy (filter_opt_p->media, opt, num);
			filter_opt_p->media[num] = '\0';

			break;
		}
	}

	opt = cupsGetOption ("Ink", opt_num, option_p);
	if (opt)
		strcpy (filter_opt_p->ink, opt);

	opt = cupsGetOption ("Quality", opt_num, option_p);
	if (opt)
		strcpy (filter_opt_p->quality, opt);


	return 0;
}


/* Get value for PPD */
static char *
get_default_choice (ppd_file_t *ppd_p, const char *key)
{
	ppd_option_t *option;
	ppd_choice_t *choice;

	option = ppdFindOption (ppd_p, key);
	if (!option)
		return NULL;
	
	choice = ppdFindChoice (option, option->defchoice);
	if (!choice)
		return NULL;

	return choice->choice;
}

static void
sig_set (void)
{

#ifdef HAVE_SIGSET /* Use System V signals over POSIX to avoid bugs */
	sigset (SIGTERM, sigterm_handler);
#elif defined(HAVE_SIGACTION)
        {
          struct sigaction action;

          memset (&action, 0, sizeof(action));

          sigemptyset (&action.sa_mask);
          action.sa_handler = sigterm_handler;
          sigaction (SIGTERM, &action, NULL);
        }
#else
	signal (SIGTERM, sigterm_handler);
#endif /* HAVE_SIGSET */

  return;
}


static void
sigterm_handler (int sig)
{
  cancel_flg = 1;

  return;
}
