/*
 * EPSON ESC/P-R Printer Driver for Linux
 * Copyright (C) 2002-2008 AVASYS CORPORATION.
 * Copyright (C) Seiko Epson Corporation 2002-2008.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * As a special exception, AVASYS CORPORATION gives permission to
 * link the code of this program with libraries which are covered by
 * the AVASYS Public License and distribute their linked
 * combinations.  You must obey the GNU General Public License in all
 * respects for all of the code used other than the libraries which
 * are covered by AVASYS Public License.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "err.h"
#include "mem.h"
#include "str.h"
#include "csv.h"
#include "debug.h"
#include "libprtX.h"
#include "optBase.h"

#define PATH_MAX 256
#define NAME_MAX 41

#define WIDTH_BYTES(bits) (((bits) + 31) / 32 * 4)

#define PIPSLITE_FILTER_VERSION "* epson-escpr is a part of " PACKAGE_STRING

#define PIPSLITE_FILTER_USAGE "Usage: $ epson-escpr model width_pixel height_pixel Ink PageSize Quality"

typedef struct rtp_filter_option {
	char model[NAME_MAX];
	char model_low[NAME_MAX];
	char ink[NAME_MAX];
	char media[NAME_MAX];
	char quality[NAME_MAX];
} filter_option_t;


FILE* outfp = NULL;

/* static functions */
static int set_pips_parameter (filter_option_t *, ESCPR_OPT *, ESCPR_PRINT_QUALITY *);
static int get_page_size (ESCPR_PRINT_JOB *, const char *, const char *);
static int getMediaTypeID(char *);
static ESCPR_BYTE4 print_spool_fnc(void* , const ESCPR_UBYTE1*, ESCPR_UBYTE4);


/* Useage: epson-escpr model width_pixel height_pixel Ink PageSize Quality */
int
main (int argc, char *argv[])
{
	filter_option_t fopt;
	char libfile[PATH_MAX + 1]; 

	long width_pixel, height_pixel;
	long HWResolution;
	long bytes_per_line;
	int byte_par_pixel;
	double x_mag, y_mag;
	double print_area_x, print_area_y;
	char *image_raw;
	unsigned char *band;

	long read_size = 0;
	int band_line;

	int err = 0;
	int i, j;

	/* 2004.04.15 added for 'cancel page' */
	int cancel = 0;
	
	/* 2005.11.28 added  */
	char *paper;
	char *bin_id;
	char *point;

	/* library options */
	ESCPR_OPT printOpt;
	ESCPR_PRINT_QUALITY printQuality;
	ESCPR_PRINT_JOB printJob;
	ESCPR_BANDBMP bandBmp;
	ESCPR_RECT bandRect;

/* attach point */
#ifdef USE_DEBUGGER
	int flag = 1;
	while (flag) sleep (3);
#endif /* USE_DEBUGGER */

	DEBUG_START;
	err_init (argv[0]);

	if (argc != 8)
	{
		for ( i = 1; i < argc; i++ ) {
			if ( (0 == strncmp(argv[i], "-v", (strlen("-v")+1)) )
				|| (0 == strncmp(argv[i], "--version", (strlen("--version")+1)) )
				) {
				fprintf(stderr, "%s\n", PIPSLITE_FILTER_VERSION);
				return 0;
			} else if ( (0 == strncmp(argv[i], "-u", (strlen("-u")+1)) )
				|| (0 == strncmp(argv[i], "--usage", (strlen("--usage")+1)) )
				) {
				fprintf(stderr, "%s\n", PIPSLITE_FILTER_USAGE);
				return 0;
			} else if ( (0 == strncmp(argv[i], "-h", (strlen("-h")+1)) )
				|| (0 == strncmp(argv[i], "--help", (strlen("--help")+1)) )
				) {
				fprintf(stderr, "%s\n%s\n", PIPSLITE_FILTER_VERSION, PIPSLITE_FILTER_USAGE);
				return 0;
			}
		}
		fprintf (stderr, "%s\n", PIPSLITE_FILTER_USAGE);
		return 1;
	}

	/* set filter options */
	memset (&fopt, 0, sizeof (filter_option_t));
	memset (&printOpt, 0, sizeof (ESCPR_OPT));
	memset (&printQuality, 0, sizeof(ESCPR_PRINT_QUALITY));
	memset (&printJob, 0, sizeof(ESCPR_PRINT_JOB));
	memset (&bandBmp, 0, sizeof(ESCPR_BANDBMP));
	memset (&bandRect, 0, sizeof(ESCPR_RECT));

	strncpy (fopt.model, argv[1], NAME_MAX);
	for (i = 0; fopt.model[i] != '\0' && i < NAME_MAX - 1; i ++)
		fopt.model_low[i] = tolower (fopt.model[i]);
	fopt.model_low[i] = '\0';

	width_pixel = atol (argv[2]);
	height_pixel = atol (argv[3]);
	HWResolution = atol (argv[4]);

	strncpy (fopt.ink, argv[5], NAME_MAX);
	strncpy (fopt.media, argv[6], NAME_MAX);
	strncpy (fopt.quality, argv[7], NAME_MAX);

	outfp = stdout;
	printOpt.fpspoolfunc = (ESCPR_FPSPOOLFUNC)print_spool_fnc;
	
	if (set_pips_parameter (&fopt, &printOpt, &printQuality))
		err_fatal ("Cannot get option of PIPS."); /* exit */

	point = fopt.media;
	if(point[0]=='T')
	{
		paper=str_clone(++point,strlen(fopt.media)-1);
		bin_id="BORDERLESS";
	}
	else
	{
		paper=str_clone(point,strlen(fopt.media));
		bin_id="BORDER";
	}

	switch(HWResolution){
	case 360:
		printJob.InResolution = ESCPR_IR_3636;
		break;
	case 720:
		printJob.InResolution = ESCPR_IR_7272;
		break;
	case 300:
		printJob.InResolution = ESCPR_IR_3030;
		break;
	case 600:
		printJob.InResolution = ESCPR_IR_6060;
		break;							
	}

	
	if (get_page_size (&printJob, paper, bin_id))
		err_fatal ("Cannot get page size of PIPS.");

	band_line = 2;

	if (strcmp (fopt.ink, "COLOR") == 0)
		byte_par_pixel = 3;
	else
		byte_par_pixel = 1;
	

	print_area_x = printJob.PrintableAreaWidth;
	print_area_y = printJob.PrintableAreaLength;

	/* setup band struct */
	bandBmp.WidthBytes = WIDTH_BYTES (print_area_x * 3 * 8 );

	band = (unsigned char *)mem_new (char, bandBmp.WidthBytes * band_line);
	memset (band, 0xff, bandBmp.WidthBytes * band_line);
	bandBmp.Bits = band;

	bandRect.left = 0;
	bandRect.right = printJob.PrintableAreaWidth;
	bandRect.top = 0;
	bandRect.bottom = 0;

	/* debug */
	DEBUG_JOB_STRUCT (printJob);
	DEBUG_QUALITY_STRUCT (printQuality);

	err = escprInitJob (&printOpt, &printQuality, &printJob);
	if (err)
		err_fatal ("Error occurred in \"INIT_FUNC\".");	/* exit */
	

	x_mag = (double)print_area_x / width_pixel;
	y_mag = (double)print_area_y / height_pixel;
	band_line = 2;
 	bytes_per_line = WIDTH_BYTES(width_pixel * byte_par_pixel * 8 );
	
	image_raw = mem_new0 (char, bytes_per_line);
	
	while ((read_size = read (STDIN_FILENO, image_raw, bytes_per_line)) > 0)
	{
		long x_count, y_count;
		int band_line_count;

		y_count = 0;
		band_line_count = 0;
		bandRect.top = 0;

		err = escprInitPage ();
		if (err)
			err_fatal ("Error occurred in \"PINIT_FUNC\"."); /* exit */
		
		for (i = 0; i < print_area_y; i ++)
		{
			char *line;

			line = (char *)(band + (bandBmp.WidthBytes * band_line_count));
			while ((0 == y_count) || ((i > (y_mag * y_count) - 1) && (y_count < height_pixel))) {
				while ((0 == err) && (read_size < bytes_per_line)) {
					size_t rsize;
					
					rsize = read(STDIN_FILENO, image_raw + read_size, bytes_per_line - read_size);
					/* 2009.03.17 epson-escpr-1.0.0 */
					/* if user cancels job from CUPS Web Admin, */
					/* epson-escpr-wrapper exit normally, */
					/* and read() return rsize = 0 */
					if (rsize <= 0) {
						if ((errno != EINTR) && (errno != EAGAIN) && (errno != EIO)) {
							err = -1;
							/* 2004.04.15		*/
							/* error then quit	*/ 
							/* don't care err = -1  */
							cancel = 1;
							goto quit;
							/* not reached		*/
							break;
						}
						usleep(50000);
					} else {
						read_size += rsize;
					}
				}

				/* 2004.04.01                                              */
				/* for "skip reading"                                      */
				/* modified y_count count up condition and clear read_size */
				if( read_size == bytes_per_line ){
					y_count++;
					/* need to clear read_size               */
					/* not clear, data still remains on pipe */
					read_size = 0;
				}
			}
			read_size = 0;
			
			/* convert input raster data to output byte data. */
			{
				int copybyte = 1;

				if ( 1 == byte_par_pixel )
					copybyte = 3;

				if (x_mag == 1)
				{

					if ( 1 == byte_par_pixel )
					{
						int k;

						for ( j = 0; j < print_area_x; j++ )
						{
							for ( k = 0; k < copybyte ; k++ )
								line[j * 3 + k] = image_raw[j];
						}
					}
					else
						memcpy (line, image_raw, bytes_per_line);
					
				}
				else
				{
					x_count = 0;
					
					for (j = 0; j < print_area_x; j ++)
					{
						int k;

						while ( x_count == 0 || j > x_mag * x_count)
							x_count ++;
						
						for ( k = 0; k < copybyte ; k++ )
							memcpy (line + (j * 3 ) + k,
							image_raw + ((x_count - 1) * byte_par_pixel),
							byte_par_pixel);
					}
				}
			}
			
			band_line_count ++;
			
			if (band_line_count >= band_line)
			{
				bandRect.bottom = bandRect.top + band_line_count;
				if (escprBandOut (&bandBmp, &bandRect))
					err_fatal ("Error occurred in \"OUT_FUNC\"."); /* exit */
				
				band_line_count = 0;
				bandRect.top = bandRect.bottom;
			}
			
		}
		
		if (band_line_count > 0)
		{
			bandRect.bottom = bandRect.top + band_line_count;
			
			if (escprBandOut (&bandBmp, &bandRect))  
				err_fatal ("Error occurred in \"OUT_FUNC\"."); /* exit */
		}
		
		if (escprTerminatePage (ESCPR_END_PAGE))
			err_fatal ("Error occurred in \"PEND_FUNC\".");	/* exit */
	}
	
	DEDBUG_END;

/* 2004.04.15 for 'error' */	
quit:;
        if( cancel ){
		escprTerminatePage (ESCPR_END_PAGE);
	}
     
	if (escprDestroyJob ())
		err_fatal ("Error occurred in \"END_FUNC\"."); /* exit */

	/* free alloced memory */
	mem_free(image_raw);
	mem_free(band);
	mem_free(paper);

	return 0;
}


static int
set_pips_parameter (filter_option_t *filter_opt_p, ESCPR_OPT *printOpt, ESCPR_PRINT_QUALITY *printQuality)
{
	char *mediaType;
	char *quality;
	char *ink;

	if (strlen (filter_opt_p->media) == 0
	    || strlen (filter_opt_p->ink) == 0
	    || strlen (filter_opt_p->quality) == 0)
		return 1;

	/* pickup MediaType & Quality from input */
	quality = strrchr(filter_opt_p->quality, '_');

	if(strlen(quality) == 0)
		return 1;

	mediaType = str_clone (filter_opt_p->quality, strlen(filter_opt_p->quality) - strlen(quality));

	/* Media Type ID */
	printQuality->MediaTypeID = getMediaTypeID(mediaType); 

	/* Print Quality */
	if(strcmp(quality, "_DRAFT") == 0)
		printQuality->PrintQuality = ESCPR_PQ_DRAFT;
	else if(strcmp(quality, "_NORMAL") == 0)
		printQuality->PrintQuality = ESCPR_PQ_NORMAL;
	else
		printQuality->PrintQuality = ESCPR_PQ_HIGH;		  

	/* Ink */
	ink = str_clone (filter_opt_p->ink, strlen (filter_opt_p->ink));
 	if (strcmp (ink, "COLOR") == 0) 
 		printQuality->ColorMono = ESCPR_CM_COLOR; 
 	else 
 		printQuality->ColorMono = ESCPR_CM_MONOCHROME; 

	/* Others */
	printQuality->Brightness = 0;
	printQuality->Contrast = 0;
	printQuality->Saturation = 0;
	printQuality->ColorPlane = ESCPR_CP_FULLCOLOR; /* default for epson-escpr */
	printQuality->PaletteSize = 0;                 /* cause ColorPlane is FULLCOLOR */
	printQuality->PaletteData = NULL;

	/* free alloced memory */
	mem_free(mediaType);
	mem_free(ink);

	return 0;
}


/* Get PageSize for PIPS */
static int
get_page_size (ESCPR_PRINT_JOB *printJob, const char *paper, const char *bin_id)
{
	csvlist_t *csv_p;
	const char *path = PAPER_PATH;
	int pos;

	long l_margin, r_margin, t_margin, b_margin;
	
	csv_p = csvlist_open (path);
	if (!csv_p)
		err_fatal ("%s : List file is broken.", path);

	pos = csvlist_search_keyword (csv_p, 0, paper);
	if (pos < 0)
		err_fatal ("%s : Unknown PageSize.", paper);


	if(printJob->InResolution == ESCPR_IR_3636 || printJob->InResolution == ESCPR_IR_7272){
		printJob->PaperWidth = atol (csvlist_get_word (csv_p, pos + 2));
		printJob->PaperLength = atol (csvlist_get_word (csv_p, pos + 3));

		l_margin = atol (csvlist_get_word (csv_p, pos + 4));
		r_margin = atol (csvlist_get_word (csv_p, pos + 5));
		t_margin = atol (csvlist_get_word (csv_p, pos + 6));
		b_margin = atol (csvlist_get_word (csv_p, pos + 7));
	}else{
		printJob->PaperWidth = atol (csvlist_get_word (csv_p, pos + 8));
		printJob->PaperLength = atol (csvlist_get_word (csv_p, pos + 9));

		l_margin = atol (csvlist_get_word (csv_p, pos + 10));
		r_margin = atol (csvlist_get_word (csv_p, pos + 11));
		t_margin = atol (csvlist_get_word (csv_p, pos + 12));
		b_margin = atol (csvlist_get_word (csv_p, pos + 13));

	}


	if (!strcmp (bin_id, "BORDER"))
	{
 		printJob->PrintableAreaWidth = printJob->PaperWidth - l_margin - r_margin;
		printJob->PrintableAreaLength = printJob->PaperLength - t_margin - b_margin;
		printJob->TopMargin = t_margin;
		printJob->LeftMargin = l_margin;
	}

	else if (!strcmp (bin_id, "BORDERLESS"))
	{
		if(printJob->InResolution == ESCPR_IR_3636 || printJob->InResolution == ESCPR_IR_7272){
	 		printJob->PrintableAreaWidth = printJob->PaperWidth + 72;
			printJob->PrintableAreaLength = printJob->PaperLength + 112;
			printJob->TopMargin = -42;
			if ( printJob->PaperWidth < 4209 ) 
				printJob->LeftMargin = -36;
			else
				printJob->LeftMargin = -48;
		}else{
	 		printJob->PrintableAreaWidth = printJob->PaperWidth + 60;
			printJob->PrintableAreaLength = printJob->PaperLength + 93;
			printJob->TopMargin = -35;
			if ( printJob->PaperWidth < 3507 ) 
				printJob->LeftMargin = -30;
			else
				printJob->LeftMargin = -40;

		}

	}

	else
	{
		err_fatal ("%s : This sheet feeder is not supported.");
	}

	printJob->PrintDirection =  0; 

	/* free alloced memory */
	csvlist_close(csv_p);

	return 0;
}

static int  getMediaTypeID(char *rsc_name)
{
  int j;

  for(j = 0; mediaTypeData[j].value != END_ARRAY; j++)
    if(strcmp(mediaTypeData[j].rsc_name,rsc_name) == 0)
      return mediaTypeData[j].value;
  return -1;
}

ESCPR_BYTE4 print_spool_fnc(void* hParam, const ESCPR_UBYTE1* pBuf, ESCPR_UBYTE4 cbBuf) 
{
	long int i;
	for (i = 0; i < cbBuf; i++)
		putc(*(pBuf + i), outfp);
	return 1;
}
