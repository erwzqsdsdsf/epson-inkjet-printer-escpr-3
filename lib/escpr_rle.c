/***********************************************************************
 *
 * Copyright (c) 2005 Seiko Epson Corporation.
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
 * File Name:   escpr_rle.c
 *
 ***********************************************************************/

/************************************************************************/
/* Function     : The library of runlength compression                  */
/************************************************************************/
#include "escpr_rle.h"
#include "escpr_osdep.h"

#include <stdio.h>
/************************************************************************/
/* Function                                                             */
/*      runlength compression for RGB                                   */
/* Name                                                                 */
/*      RunLengthEncode                                                 */
/* Options                                                              */
/*      ESCPR_UBYTE1 *pSrcStartAddr : An address of original raster     */
/*      ESCPR_UBYTE1 *pDstStartAddr : An address of compressed raster   */
/*      ESCPR_BYTE4  pixel          : original raster size              */
/*      ESCPR_BYTE4  pCompress      : compression flg                   */ 
/*      ESCPR_BYTE4  bpp            : bits per pixel                    */
/*                                      (3) for full color              */
/*                                      (1) for 256                     */
/*                                       done compression   : 1         */ 
/*                                       undone compressing : 0         */
/* Return value                                                         */
/*      ESCPR_BYTE4                 : compressed data size (byte)       */
/************************************************************************/
ESCPR_BYTE4 ESCPR_RunLengthEncode(const ESCPR_UBYTE1 *pSrcAddr, 
        ESCPR_UBYTE1 *pDstAddr, ESCPR_BYTE2 pixel, ESCPR_BYTE1 bpp, ESCPR_UBYTE1 *pCompress)
{
    ESCPR_BYTE4     srcCnt  = 0;                    /* counter              */
    ESCPR_BYTE4     repCnt  = 0;                    /* replay conter        */
    ESCPR_BYTE4     retCnt  = 0;                    /* conpressed data size */

    *pCompress = 1;

    while (srcCnt < pixel) {
        /* In case of replay data */
        if (((srcCnt + 1 < pixel) 
                    && (!ESCPR_Mem_Compare(pSrcAddr, 
                            pSrcAddr + bpp, bpp)))) {
            repCnt = 2;
            while ((srcCnt + repCnt < pixel) && (repCnt < 0x81) 
                    && (!ESCPR_Mem_Compare(pSrcAddr + (repCnt - 1) * bpp, 
                            pSrcAddr + repCnt * bpp, bpp))) {
                repCnt++;
            }

            /* Set replay count and data */
            /* Set data counter                     */
            *pDstAddr++ = (ESCPR_UBYTE1)(0xFF - repCnt + 2 );        

            /* Set data                             */
            ESCPR_Mem_Copy(pDstAddr, pSrcAddr, bpp);    

            /* Renewal data size counter            */
            srcCnt += repCnt;                                       

            /* Renewal compressed data size counter */
            retCnt += 1 + bpp;                          

            /* Renewal original data address        */
            pSrcAddr += bpp * repCnt;                   

            /* Renewal compressed data address      */
            pDstAddr += bpp;                            
        }

        /* In case of non replay data */
        else {
            ESCPR_BYTE4 copySize;
            repCnt = 1;

            /* compare next data with next and next data */
            while ((srcCnt + repCnt + 1< pixel) 
                    && (repCnt < 0x80) 
                    && (ESCPR_Mem_Compare(pSrcAddr + repCnt * bpp, 
                            pSrcAddr + (repCnt + 1) * bpp, bpp)) ){
                repCnt++;
            }
            
            /* Renewal compressed data size counter */
            retCnt += 1 + repCnt * bpp;       

            /* Set data counter                     */
            *pDstAddr++ = (ESCPR_UBYTE1)(repCnt - 1);                

            /* Renewal data size counter            */

            /* Size of non replay data (byte)       */
            srcCnt += repCnt;                                       
            copySize = repCnt * bpp;                    

            /* Set data                             */
            ESCPR_Mem_Copy(pDstAddr, pSrcAddr, copySize);           

            /* Renewal original data address        */
            pSrcAddr += copySize;                                   

            /* Renewal compressed data address      */
            pDstAddr += copySize;                                   
        }

        /* If compressed data size is bigger than original data size */
        /* stop compression process. */
        if(retCnt > pixel*bpp){
            *pCompress = 0;
            break;
        }
    }
    return(retCnt);
}

