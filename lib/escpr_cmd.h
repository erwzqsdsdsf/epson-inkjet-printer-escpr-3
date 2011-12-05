/***********************************************************************
 *
 * Copyright (c) 2005-2008 Seiko Epson Corporation.
 * 
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * File Name:   escpr_cmd.h
 *
 ***********************************************************************/

#ifndef __EPSON_ESCPR_CMD_H__
#define __EPSON_ESCPR_CMD_H__

#include "escpr_def.h"

#define ESCPR_HEADER_LENG                   10
#define ESCPR_PRTQLTY_PARAMBLC_LENG         9
#define ESCPR_PRTJOB_PARAMBLC_LENG          22
#define ESCPR_SENDDATA_PARAMBLC_LENG        7
#define REMOTE_HEADER_LENGTH                5


/*#define ESCPR_PACKET_4KB*/                    /*  Set packet size to 4096byte */

#ifdef ESCPR_PACKET_4KB
#define ESCPR_PACKET_SIZE_4KB               4090
#endif

ESCPR_ERR_CODE ESCPR_MakeHeaderCmd(void);
ESCPR_ERR_CODE ESCPR_MakePrintQualityCmd(const ESCPR_PRINT_QUALITY *pPrintQuality);
ESCPR_ERR_CODE ESCPR_MakePrintJobCmd(const ESCPR_PRINT_JOB *pPrintJob);
ESCPR_ERR_CODE ESCPR_MakeStartPageCmd(void);
ESCPR_ERR_CODE ESCPR_MakeImageData(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec);
ESCPR_ERR_CODE ESCPR_MakeEndPageCmd(const ESCPR_UBYTE1 NextPage);
ESCPR_ERR_CODE ESCPR_MakeEndJobCmd(void);
ESCPR_ERR_CODE ESCPR_MakeOneRasterData(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec);
ESCPR_ERR_CODE ESCPR_RasterFilter(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec);

BOOL ESCPR_SetBigEndian_BYTE2(ESCPR_UBYTE2 value, ESCPR_UBYTE1 array[2]);
BOOL ESCPR_SetBigEndian_BYTE4(ESCPR_UBYTE4 value, ESCPR_UBYTE1 array[4]);
BOOL ESCPR_SetLittleEndian_BYTE4(ESCPR_UBYTE4 value, ESCPR_UBYTE1 array[4]);

ESCPR_ERR_CODE ESCPR_AdjustBasePoint(ESCPR_PRINT_JOB* pPrintJob);

ESCPR_ENDIAN ESCPR_CPU_Endian(void);

#if defined(_ZERO_MARGIN_MIRROR) || defined (_ZERO_MARGIN_REPLICATE)

/**
 * Calculate the left & right padding, and the size of the user's raster.
 *
 * pBandRec         I/O     Rectangle for the input. Will be changed to include Padding.
 * NumPadRight      Output  The amount of padding needed on the right.
 * NumPadLeft       Output  The amount of padding needed on the left.
 * RasterPixelSize  Output  The size of the user's data (not including padding).
 */
ESCPR_ERR_CODE ESCPR_CalculatePadding(ESCPR_RECT *pBandRec,ESCPR_BYTE2* NumPadLeft, 
        ESCPR_BYTE2* NumPadRight, ESCPR_UBYTE2* RasterPixelSize);
#endif /* defined(_ZERO_MARGIN_MIRROR) || defined (_ZERO_MARGIN_REPLICATE) */

#if defined(_ZERO_MARGIN_REPLICATE)
/**
 * Transform the band data into the replicated band data.
 *
 * pInBmp       Input   Band data.
 * pBandRec     I/O     Rectangle for pInBmp on input. Rectangle for pOutBmp on output.
 *                      Band should be 1 raster only.
 * repCount     Output  Number of times the band should be repeated. pBandRec has
 *                      cordinates for the first band. The other (repeated) bands
 *                      should adjust the rectangle accordingly.
 * pOutBmp      Output  (Do not allocate memory). Pointer to band data.
 * TempBuffer   Input   Temporary buffer that should be 
 *                      gESCPR_bpp*gESCPR_PrintableAreaWidth wide. pOutBmp may (but not
 *                      necessarily) point to this.
 */
ESCPR_ERR_CODE ESCPR_RasterFilterReplicate(const ESCPR_BANDBMP* pInBmp, 
        ESCPR_RECT *pBandRec, ESCPR_BYTE4* repCount, ESCPR_BANDBMP* pOutBmp,
        ESCPR_UBYTE1* TempBuffer);
#endif

#if defined(_ZERO_MARGIN_MIRROR)
/**
 * Transform the band data into the mirrored band data.
 *
 * pInBmp       Input   Band data.
 * pBandRec     I/O     Rectangle for pInBmp on input. Rectangle for pOutBmp on output.
 *                      Band should be 1 raster only.
 * repCount     Output  Will be 1 or 0. 
 *                      0 means discard output of this function (band was saved in memory). 
 *                      1 means send pOutBmp to printer.
 * pOutBmp      Output  (Do not allocate memory). Pointer to band data.
 * TempBuffer   Input   Temporary buffer that should be 
 *                      gESCPR_bpp*gESCPR_PrintableAreaWidth wide. pOutBmp may (but not
 *                      necessarily) point to this.
 */
ESCPR_ERR_CODE ESCPR_RasterFilterMirror(const ESCPR_BANDBMP* pInBmp, ESCPR_RECT *pBandRec, 
        ESCPR_BYTE4* repCount, ESCPR_BANDBMP* pOutBmp,ESCPR_UBYTE1* TempBuffer);

/**
 * Create a rectangle for the top mirrored area of the image, as well as the top
 * area where the mirrored area was created from. i.e. Create a rectangle for
 * the region 0 to 2*ESCPR_PAD_TOP.
 *
 * pBandRec     Output   Filled with dimensions for rectangle.    
 * pOutBmp      Output   Point to band data.
 */
ESCPR_ERR_CODE ESCPR_MakeMirrorImageTop(ESCPR_RECT* pBandRec,ESCPR_BANDBMP* pOutBmp);

/**
 * Create a rectangle for the bottom mirrored area of the image. If PrintLastRaster is
 * TRUE, the last raster of the original data is also included in the rectangle. If
 * PrintLastRaster is FALSE, only the mirrored area is included.
 *
 * pBandRec         Output   Filled with dimensions for rectangle.    
 * pOutBmp          Output   Point to band data.
 * PrintLastRaster  Input    Whether to include the last raster of the original data.
 */
ESCPR_ERR_CODE ESCPR_MakeMirrorImageBottom(ESCPR_RECT* pBandRec,ESCPR_BANDBMP* pOutBmp,
        const BOOL PrintLastRaster);

/**
 * Send the top mirrored area. Uses the data stored in gESCPR_DupBufferTop. Sets
 * the global flag gESCPR_TopFlushed to TRUE.
 */
ESCPR_ERR_CODE ESCPR_MakeImageFlushTop(void);
#endif

#endif
