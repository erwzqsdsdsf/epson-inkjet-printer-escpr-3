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
 * File Name:   escpr_cmd.c
 *
 ***********************************************************************/

#include "escpr_def.h"
#include "escpr_cmd.h"
#include "escpr_rle.h"
#include "escpr_osdep.h"

/*=======================================================================================*/
/* Debug                                                                                 */
/*=======================================================================================*/
/* #define ESCPR_IMAGE_LOG */

#include <stdio.h>
#include <stdlib.h>

#ifdef _ESCPR_DEBUG
#include <stdio.h>
#define dprintf(a) printf a
#else
#define dprintf(a)
#endif

/*=======================================================================================*/
/* Define Area                                                                           */
/*=======================================================================================*/
typedef struct tagESCPR_PAGE {
    ESCPR_BYTE4 Top;
    ESCPR_BYTE4 Bottom;
    ESCPR_BYTE4 Left;
    ESCPR_BYTE4 Right;
} ESCPR_PAGE;

typedef struct tagBASEPOINT {
    ESCPR_PAGE Border;
    ESCPR_PAGE Borderless;
#if defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR)
    ESCPR_PAGE Pad;
#endif
} ESCPR_BASEPT;

void SetGlobalBasePointData_300dpi(const int PaperWidth);
void SetGlobalBasePointData_360dpi(const int PaperWidth);

#define PAPERWIDTH_A3 4209

#if defined(_ZERO_MARGIN_MIRROR)
#define ESCPR_BLUR_FACTOR 4
#endif

/*=======================================================================================*/
/* Switch for Printing Method                                                            */
/*=======================================================================================*/

/*=======================================================================================*/
/* Definition of Macro                                                                   */
/*=======================================================================================*/
#define Max(a,b) ( ((a) > (b)) ? (a) : (b))
#define Min(a,b) ( ((a) < (b)) ? (a) : (b))

#define ESCPR_FreeLocalBuf(mem)     if(mem != NULL) ESCPR_Mem_Free(mem);

#define ESCPR_SendData(h,cmd,sz)    if(gESCPR_Func.gfpSendData(h, cmd, sz) < 0) \
                                        return ESCPR_ERR_SPOOL_IO;
#define Swap2Bytes(data) \
    ( (((data) >> 8) & 0x00FF) | (((data) << 8) & 0xFF00) )

#define Swap4Bytes(data) \
    ( (((data) >> 24) & 0x000000FF) | (((data) >>  8) & 0x0000FF00) | \
      (((data) <<  8) & 0x00FF0000) | (((data) << 24) & 0xFF000000) )

/*=======================================================================================*/
/* Global Area                                                                           */
/*=======================================================================================*/
extern ESCPR_GLOBALFUNC gESCPR_Func;
ESCPR_BYTE4 gESCPR_ImgBufSize;
ESCPR_BYTE4 gESCPR_CompBufSize;

#if defined(_ZERO_MARGIN_REPLICATE)  || defined(_ZERO_MARGIN_MIRROR)
BOOL gESCPR_Borderless;             /* borderless mode selected */
#endif

#if defined(_ZERO_MARGIN_MIRROR)
ESCPR_UBYTE1* gESCPR_DupBufferTop;  /* buffer to hold top region of image */
ESCPR_UBYTE1* gESCPR_DupBufferBot;  /* buffer to hold bottom region of image */
ESCPR_BYTE4 gESCPR_TopCnt;          /* counter for how many rasters we have saved */
ESCPR_BYTE4 gESCPR_BotCnt;          /* counter for how many rasters we have saved */
BOOL gESCPR_TopFlushed;             /* whether top needs to be flushed */
#endif

ESCPR_UBYTE4 gESCPR_PrintableAreaWidth;         /* users setting */
ESCPR_UBYTE4 gESCPR_PrintableAreaLength;        /* users setting */
ESCPR_BYTE1  gESCPR_bpp;                        /* 3 for RGB, 1 for 256 */
ESCPR_UBYTE2 gESCPR_offset_x, gESCPR_offset_y;  /* offset to be used. calculated when changing basepoint */

ESCPR_BYTE2 gESCPR_DPI_Multiplier;              /* 2 if resolution 720x720
                                                 * 1 if resolution 360x360 */
ESCPR_BASEPT gESCPR_BasePt;

ESCPR_BYTE2 gESCPR_WhiteColorValue;             /* Value of "true white" (either 255 or the 8-bit BMP index) */

#ifdef _ESCPR_DEBUG
ESCPR_UBYTE4 gESCPR_TotalDebugBandCount = 0;
#endif

/*=======================================================================================*/
/* Sub Routine                                                                           */
/*=======================================================================================*/
static ESCPR_UBYTE4 ESCPR_SendData_CheckResult(const ESCPR_UBYTE1* pBuf, ESCPR_UBYTE4 cbBuf)
{
	if(gESCPR_Func.gfpSendData(NULL, pBuf, cbBuf) < 0) {
		return ESCPR_ERR_SPOOL_IO;
	}
	return ESCPR_ERR_NOERROR;
}

ESCPR_ENDIAN ESCPR_CPU_Endian(void)
{
    static ESCPR_ENDIAN platform_endian = ESCPR_ENDIAN_NOT_TESTED;

    union {
    ESCPR_BYTE1 Array[2];
    ESCPR_BYTE2 Chars;
    } TestUnion;

    if(platform_endian == ESCPR_ENDIAN_NOT_TESTED) {
    /* Test platform Endianness */
    TestUnion.Array[0] = 'a';
    TestUnion.Array[1] = 'b';

    if (TestUnion.Chars == 0x6162) {
        platform_endian = ESCPR_ENDIAN_BIG;
    } else {
        platform_endian = ESCPR_ENDIAN_LITTLE;
    }
    }

    return platform_endian;

}

BOOL ESCPR_SetBigEndian_BYTE2(ESCPR_UBYTE2 value, ESCPR_UBYTE1 array[2])
{
    if(ESCPR_CPU_Endian() == ESCPR_ENDIAN_LITTLE) {
        value = Swap2Bytes(value);
    }

    ESCPR_Mem_Copy((ESCPR_UBYTE1*)array, (ESCPR_UBYTE1*)&value, 2);

    return TRUE;
}


BOOL ESCPR_SetBigEndian_BYTE4(ESCPR_UBYTE4 value, ESCPR_UBYTE1 array[4])
{
    if(ESCPR_CPU_Endian() == ESCPR_ENDIAN_LITTLE) {
        value = Swap4Bytes(value);
    }

    ESCPR_Mem_Copy((ESCPR_UBYTE1*)array, (ESCPR_UBYTE1*)&value, 4);

    return TRUE;
}

BOOL ESCPR_SetLittleEndian_BYTE4(ESCPR_UBYTE4 value, ESCPR_UBYTE1 array[4])
{
    if(ESCPR_CPU_Endian() == ESCPR_ENDIAN_BIG) {
        value = Swap4Bytes(value);
    }

    ESCPR_Mem_Copy((ESCPR_UBYTE1*)array, (ESCPR_UBYTE1*)&value, 4);

    return TRUE;
}


ESCPR_ERR_CODE ESCPR_MakeHeaderCmd(void)
{
    static const ESCPR_UBYTE1 cmd_ExitPacketMode[] = {
        0x00, 0x00, 0x00, 0x1B, 0x01, 0x40, 0x45, 0x4A, 0x4C, 0x20,
        0x31, 0x32, 0x38, 0x34, 0x2E, 0x34, 0x0A, 0x40, 0x45, 0x4A,
        0x4C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0A
    };

    static const ESCPR_UBYTE1 cmd_InitPrinter[] = {
        0x1B, 0x40,
    };
    
    static const ESCPR_UBYTE1 cmd_EnterRemoteMode[] = {
        0x1B, 0x28, 0x52, 0x08, 0x00, 0x00, 'R', 'E', 'M', 'O', 'T', 'E', '1',
    };
    
    static const ESCPR_UBYTE1 cmd_RemoteJS[] = {
        'J', 'S', 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    
    static const ESCPR_UBYTE1 cmd_RemoteHD[] = {
        'H', 'D', 0x03, 0x00, 0x00, 0x03, 0x04,
    };
    
    static const ESCPR_UBYTE1 cmd_ExitRemoteMode[] = {
        0x1B, 0x00, 0x00, 0x00,
    };
    
    static const ESCPR_UBYTE1 cmd_RGBMode[] = {
        0x1B, 0x28, 0x52, 0x06, 0x00, 0x00, 0x45, 0x53, 0x43, 0x50, 0x52, 
    };
    
    /* Exit Packet Mode */
    ESCPR_SendData(NULL, cmd_ExitPacketMode, sizeof(cmd_ExitPacketMode));
    
    /* Initialize */
    ESCPR_SendData(NULL, cmd_InitPrinter, sizeof(cmd_InitPrinter));
    
    ESCPR_SendData(NULL, cmd_EnterRemoteMode, sizeof(cmd_EnterRemoteMode));

    /* Remote Command - TI */
    {
        ESCPR_LOCAL_TIME tm;
        ESCPR_UBYTE1 value_array_2[2];   /* Temporary Buffer for 2 byte Big Endian     */
        ESCPR_UBYTE1 *cmdBuff;           /* Temporary Buffer for Print Quality Command */
        ESCPR_UBYTE1 cmd_RemoteTI[] = {
            'T', 'I', 0x08, 0x00, 0x00,
            0x00, 0x00, /* YYYY */
            0x00,       /* MM */
            0x00,       /* DD */
            0x00,       /* hh */
            0x00,       /* mm */
            0x00,       /* ss */
        };

        ESCPR_GetLocalTime(&tm);

        cmdBuff = cmd_RemoteTI + REMOTE_HEADER_LENGTH;
        ESCPR_SetBigEndian_BYTE2(tm.year, value_array_2);
        ESCPR_Mem_Copy(cmdBuff, value_array_2, sizeof(value_array_2));
        cmdBuff += sizeof(value_array_2);
        *cmdBuff++ = (ESCPR_UBYTE1)tm.mon;
        *cmdBuff++ = (ESCPR_UBYTE1)tm.day;
        *cmdBuff++ = (ESCPR_UBYTE1)tm.hour;
        *cmdBuff++ = (ESCPR_UBYTE1)tm.min;
        *cmdBuff   = (ESCPR_UBYTE1)tm.sec;

        ESCPR_SendData(NULL, cmd_RemoteTI, sizeof(cmd_RemoteTI));
    }
    
    /* Remote Command - JS */
    ESCPR_SendData(NULL, cmd_RemoteJS, sizeof(cmd_RemoteJS));
    
    /* Remote Command - HD */
    ESCPR_SendData(NULL, cmd_RemoteHD, sizeof(cmd_RemoteHD));

    ESCPR_SendData(NULL, cmd_ExitRemoteMode, sizeof(cmd_ExitRemoteMode));

    /* Enter ESC/P-R mode */
    ESCPR_SendData(NULL, cmd_RGBMode, sizeof(cmd_RGBMode));

    return ESCPR_ERR_NOERROR;
}


ESCPR_ERR_CODE ESCPR_MakePrintQualityCmd(const ESCPR_PRINT_QUALITY *pPrintQuality)
{
    ESCPR_BYTE4 i;
    ESCPR_UBYTE1* j;
    BOOL whiteFound = FALSE;
    ESCPR_UBYTE4 currentIndex = 0;
    ESCPR_BYTE4 cmdSize;             /* All Size of Print Quality Command          */
    ESCPR_BYTE4 cpy_size;            /* Copy Size                                  */
    ESCPR_BYTE4 cpy_count = 0;       /* Counter for Set Command                    */
    ESCPR_UBYTE4 paramLength;        /* Parameter Length                           */
    ESCPR_UBYTE1 value_array_2[2];   /* Temporary Buffer for 2 byte Big Endian     */
    ESCPR_UBYTE1 value_array_4[4];   /* Temporary Buffer for 4 byte Big Endian     */
    ESCPR_UBYTE1 *cmdBuff;           /* Temporary Buffer for Print Quality Command */
    void *Temp;
    
    static const ESCPR_UBYTE1 Header[2] = {0x1B, 'q'};           /* ESC + Class    */
    static const ESCPR_UBYTE1 cmdName[4] = {'s', 'e', 't', 'q'}; /* Command Name   */

    if( pPrintQuality->ColorPlane == ESCPR_CP_FULLCOLOR ){ /* Full Color Printing */
        gESCPR_bpp = 3;
        
        /* In full color printing, set the global white value to 255 (for white raster skip) */
        gESCPR_WhiteColorValue = 255;
        
        /* Set All Command Size */
        paramLength = ESCPR_PRTQLTY_PARAMBLC_LENG;
        cmdSize = ESCPR_HEADER_LENG + paramLength;
        
        /* Allocate Temporary Buffer */
        Temp = ESCPR_Mem_Alloc(cmdSize);
        if(Temp == NULL){
            return ESCPR_ERR_CAN_T_ALLOC_MEMORY;
        } else{
            cmdBuff = (ESCPR_UBYTE1 *)Temp;
        }
        
        /* Set Parameter */
        
        /* Header */
        cpy_size = sizeof(Header);
        ESCPR_Mem_Copy((cmdBuff + cpy_count), Header, cpy_size);
        cpy_count += cpy_size;
        
        /* Parameter Length */
        cpy_size = sizeof(value_array_4);
        if(ESCPR_SetLittleEndian_BYTE4(paramLength, value_array_4)){
            ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_4, cpy_size);
            cpy_count += cpy_size;
        } else{
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_HAPPEN_PROBLEM;
        }
        
        /* Command Name */
        cpy_size = sizeof(cmdName);
        ESCPR_Mem_Copy((cmdBuff + cpy_count), cmdName, cpy_size);
        cpy_count += cpy_size;
        
        /* Media Type ID */
        *(cmdBuff + cpy_count) = pPrintQuality->MediaTypeID;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Print Quality */
        *(cmdBuff + cpy_count) = pPrintQuality->PrintQuality;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Color/Monochrome */
        *(cmdBuff + cpy_count) = pPrintQuality->ColorMono;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Brightness */
        *(cmdBuff + cpy_count) = pPrintQuality->Brightness;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Contrast */
        *(cmdBuff + cpy_count) = pPrintQuality->Contrast;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Saturation */
        *(cmdBuff + cpy_count) = pPrintQuality->Saturation;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Color Plane */
        *(cmdBuff + cpy_count) = pPrintQuality->ColorPlane;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Palette Size */
        cpy_size = sizeof(value_array_2);
        if(ESCPR_SetBigEndian_BYTE2(pPrintQuality->PaletteSize, value_array_2)){
            ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_2, cpy_size);
            cpy_count += cpy_size;
        } else{
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_HAPPEN_PROBLEM;
        }
        
        /* Debug Test >>> */
        if(cmdSize == cpy_count){
            dprintf(("ESCPRCMD : PrintQualityCmd [size = %d] : ", cpy_count));
            for(i = 0; i < cpy_count; i++){
                dprintf(("%.2X ", cmdBuff[i]));
            }
            dprintf(("\n"));
        } else{
            dprintf(("ESCPRCMD : ESCPR_Make PrintQualityCmd Faild!\n"));
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_HAPPEN_PROBLEM;
        }
        /* Debug Test <<< */
        
        /* Send Print Quality Command to Printer */
        if(ESCPR_SendData_CheckResult(cmdBuff, cmdSize) != ESCPR_ERR_NOERROR) {
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_SPOOL_IO;
		}
        
    } else{  /* 256 Color Printing */

        gESCPR_bpp = 1;
        
        /* Scan through palette for the index value of white and set the global white
         * value to this index for skipping white raster lines */
        for( j = pPrintQuality->PaletteData; 
                j < (pPrintQuality->PaletteData + pPrintQuality->PaletteSize);
                    j = j + 3){
            if(*j == 255 && *(j+1) == 255 && *(j+2) == 255){ /* If all entries in the 3-byte index are 255 (white) */
                whiteFound = TRUE;
                gESCPR_WhiteColorValue = currentIndex; /* set the global white value to the current index */
                break;  
            } else{
                currentIndex++; 
            }                       
        }
        
        if(whiteFound == FALSE){ /* White was never found in the palette! */
            gESCPR_WhiteColorValue = -1; /* Set the value of white to negative one */
        }
        
        /* Set All Command Size */
        paramLength = ESCPR_PRTQLTY_PARAMBLC_LENG + pPrintQuality->PaletteSize;
        cmdSize = ESCPR_HEADER_LENG + paramLength;
        
        /* Allocate Temporary Buffer */
        Temp = ESCPR_Mem_Alloc(cmdSize);
        if(Temp == NULL){
            return ESCPR_ERR_CAN_T_ALLOC_MEMORY;
        } else{
            cmdBuff = (ESCPR_UBYTE1 *)Temp;
        }
        
        /* Set Parameter */
        
        /* Header */
        cpy_size = sizeof(Header);
        ESCPR_Mem_Copy((cmdBuff + cpy_count), Header, cpy_size);
        cpy_count += cpy_size;
        
        /* Parameter Length */
        cpy_size = sizeof(value_array_4);
        if(ESCPR_SetLittleEndian_BYTE4(paramLength, value_array_4)){
            ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_4, cpy_size);
            cpy_count += cpy_size;
        } else{
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_HAPPEN_PROBLEM;
        }
        
        /* Command Name */
        cpy_size = sizeof(cmdName);
        ESCPR_Mem_Copy((cmdBuff + cpy_count), cmdName, cpy_size);
        cpy_count += cpy_size;
        
        /* Media Type ID */
        *(cmdBuff + cpy_count) = pPrintQuality->MediaTypeID;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Print Quality */
        *(cmdBuff + cpy_count) = pPrintQuality->PrintQuality;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Color/Monochrome */
        *(cmdBuff + cpy_count) = pPrintQuality->ColorMono;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Brightness */
        *(cmdBuff + cpy_count) = pPrintQuality->Brightness;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Contrast */
        *(cmdBuff + cpy_count) = pPrintQuality->Contrast;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Saturation */
        *(cmdBuff + cpy_count) = pPrintQuality->Saturation;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Color Plane */
        *(cmdBuff + cpy_count) = pPrintQuality->ColorPlane;
        cpy_count += sizeof(ESCPR_UBYTE1);
        
        /* Palette Size */
        cpy_size = sizeof(value_array_2);
        if(ESCPR_SetBigEndian_BYTE2(pPrintQuality->PaletteSize, value_array_2)){
            ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_2, cpy_size);
            cpy_count += cpy_size;
        } else{
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_HAPPEN_PROBLEM;
        }
        
        /* Palette Data */
        ESCPR_Mem_Copy((cmdBuff + cpy_count), 
                pPrintQuality->PaletteData, pPrintQuality->PaletteSize);

        cpy_count += pPrintQuality->PaletteSize;
        
        /* Debug Test >>> */
        if(cmdSize == cpy_count){
            dprintf(("ESCPRCMD : PrintQualityCmd [size = %d] : ", cpy_count));
            for(i = 0; i < cpy_count; i++){
                dprintf(("%.2X ", cmdBuff[i]));
            }
            dprintf(("\n"));
        } else{
            dprintf(("ESCPRCMD : ESCPR_Make PrintQualityCmd Faild!\n"));
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_HAPPEN_PROBLEM;
        }
        /* Debug Test <<< */
            
        /* Send Print Quality Command to Printer */
        if(ESCPR_SendData_CheckResult(cmdBuff, cmdSize) != ESCPR_ERR_NOERROR) {
			ESCPR_FreeLocalBuf(Temp);
            return ESCPR_ERR_SPOOL_IO;
		}        
    }
    
    /* Free Temporary Buffer */
    ESCPR_Mem_Free(Temp);
    
    return ESCPR_ERR_NOERROR;
}


ESCPR_ERR_CODE ESCPR_AdjustBasePoint(ESCPR_PRINT_JOB* pPrintJob) {
    /* temporary variables for max PAW/PAL */
    ESCPR_UBYTE4 maxPAWidthBorder, maxPAWidthBorderless;
    ESCPR_UBYTE4 maxPALengthBorder, maxPALengthBorderless;
    
    BOOL adjBorderless;
    
    ESCPR_UBYTE4 RightMargin, BottomMargin;
    
    adjBorderless = (pPrintJob->LeftMargin <  gESCPR_BasePt.Border.Left) 
        || (pPrintJob->TopMargin < gESCPR_BasePt.Border.Top);

    /* pPrintJob->ust the basepoint */
    if (adjBorderless) {

        gESCPR_offset_x = pPrintJob->LeftMargin - gESCPR_BasePt.Borderless.Left;
        gESCPR_offset_y = pPrintJob->TopMargin  - gESCPR_BasePt.Borderless.Top;

        pPrintJob->PrintableAreaWidth  += gESCPR_offset_x;
        pPrintJob->PrintableAreaLength += gESCPR_offset_y;

        pPrintJob->LeftMargin = (ESCPR_BYTE2)(gESCPR_BasePt.Borderless.Left);
        pPrintJob->TopMargin  = (ESCPR_BYTE2)(gESCPR_BasePt.Borderless.Top);

        /* max PAW/PAL */
        maxPALengthBorderless = pPrintJob->PaperLength*gESCPR_DPI_Multiplier 
            - pPrintJob->TopMargin - gESCPR_BasePt.Borderless.Bottom;
        maxPAWidthBorderless = pPrintJob->PaperWidth*gESCPR_DPI_Multiplier 
            - pPrintJob->LeftMargin - gESCPR_BasePt.Borderless.Right;

        if (pPrintJob->PrintableAreaLength > maxPALengthBorderless) {
            pPrintJob->PrintableAreaLength = maxPALengthBorderless;
        }

        if (pPrintJob->PrintableAreaWidth > maxPAWidthBorderless) {
            pPrintJob->PrintableAreaWidth = maxPAWidthBorderless;
        }

    } else {

        gESCPR_offset_x = pPrintJob->LeftMargin - gESCPR_BasePt.Border.Left;
        gESCPR_offset_y = pPrintJob->TopMargin  - gESCPR_BasePt.Border.Top;

        RightMargin  = (pPrintJob->PaperWidth * gESCPR_DPI_Multiplier)
                            - pPrintJob->LeftMargin - pPrintJob->PrintableAreaWidth;
        BottomMargin = (pPrintJob->PaperLength * gESCPR_DPI_Multiplier)
                            - pPrintJob->TopMargin  - pPrintJob->PrintableAreaLength;

        pPrintJob->PrintableAreaWidth  = (pPrintJob->PaperWidth * gESCPR_DPI_Multiplier)
                                            - gESCPR_BasePt.Border.Left - RightMargin;
        pPrintJob->PrintableAreaLength = (pPrintJob->PaperLength * gESCPR_DPI_Multiplier)
                                            - gESCPR_BasePt.Border.Top  - BottomMargin;

        pPrintJob->LeftMargin = (ESCPR_BYTE2)(gESCPR_BasePt.Border.Left);
        pPrintJob->TopMargin  = (ESCPR_BYTE2)(gESCPR_BasePt.Border.Top);

        /* max PAW/PAL */
        maxPALengthBorder = pPrintJob->PaperLength*gESCPR_DPI_Multiplier 
            - pPrintJob->TopMargin - gESCPR_BasePt.Border.Bottom;
        maxPAWidthBorder = pPrintJob->PaperWidth*gESCPR_DPI_Multiplier 
            - pPrintJob->LeftMargin - gESCPR_BasePt.Border.Right;

        if (pPrintJob->PrintableAreaLength > maxPALengthBorder) {
            pPrintJob->PrintableAreaLength = maxPALengthBorder;
        }

        if (pPrintJob->PrintableAreaWidth > maxPAWidthBorder) {
            pPrintJob->PrintableAreaWidth = maxPAWidthBorder;
        }

    }

    return ESCPR_ERR_NOERROR;
}

ESCPR_ERR_CODE ESCPR_MakePrintJobCmd(const ESCPR_PRINT_JOB *pPrintJob)
{
    ESCPR_BYTE4 i;
    ESCPR_BYTE4 cmdSize;             /* All Size of Print Job Command              */
    ESCPR_BYTE4 cpy_size;            /* Copy Size                                  */
    ESCPR_BYTE4 cpy_count = 0;       /* Counter for Set Command                    */
    ESCPR_UBYTE4 paramLength;        /* Parameter Length                           */
    ESCPR_UBYTE1 value_array_2[2];   /* Temporary Buffer for 2 byte Big Endian     */
    ESCPR_UBYTE1 value_array_4[4];   /* Temporary Buffer for 4 byte Big Endian     */

    ESCPR_UBYTE1 cmdBuff[ESCPR_HEADER_LENG + ESCPR_PRTJOB_PARAMBLC_LENG];    
                                     /* Temporary Buffer for Print Quality Command */
    
    static const ESCPR_UBYTE1 Header[2] = {0x1B, 'j'};           /* ESC + Class    */
    static const ESCPR_UBYTE1 cmdName[4] = {'s', 'e', 't', 'j'}; /* Command Name   */

    ESCPR_PRINT_JOB cpyPrintJob;

    /* fill out globals */
    if (pPrintJob->InResolution == ESCPR_IR_7272 || pPrintJob->InResolution == ESCPR_IR_6060) {
        gESCPR_DPI_Multiplier = 2;
    } else {
        gESCPR_DPI_Multiplier = 1;
    }

    if (pPrintJob->InResolution == ESCPR_IR_3030 || pPrintJob->InResolution == ESCPR_IR_6060) {
        SetGlobalBasePointData_300dpi(pPrintJob->PaperWidth);
    } else {
        SetGlobalBasePointData_360dpi(pPrintJob->PaperWidth);
    }

    if (pPrintJob->TopMargin < gESCPR_BasePt.Borderless.Top ||
            pPrintJob->LeftMargin < gESCPR_BasePt.Borderless.Left) {
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }

    /* create a copy of pPrintJob */
    cpyPrintJob.PaperWidth = pPrintJob->PaperWidth;
    cpyPrintJob.PaperLength = pPrintJob->PaperLength;
    cpyPrintJob.TopMargin = pPrintJob->TopMargin;
    cpyPrintJob.LeftMargin = pPrintJob->LeftMargin;
    cpyPrintJob.PrintableAreaWidth = pPrintJob->PrintableAreaWidth;
    cpyPrintJob.PrintableAreaLength = pPrintJob->PrintableAreaLength;
    cpyPrintJob.InResolution = pPrintJob->InResolution;
    cpyPrintJob.PrintDirection = pPrintJob->PrintDirection;



{
	FILE *fp;
	fp=fopen("/tmp/test.txt", "a+");
	fprintf(fp, "pPrintJob->PaperWidth=%d\n",pPrintJob->PaperWidth);
	fprintf(fp, "pPrintJob->PaperLength=%d\n",pPrintJob->PaperLength);
	fprintf(fp, "pPrintJob->TopMargin=%d\n",pPrintJob->TopMargin);
	fprintf(fp, "pPrintJob->LeftMargin=%d\n",pPrintJob->LeftMargin);
	fprintf(fp, "pPrintJob->PrintableAreaWidth=%d\n",pPrintJob->PrintableAreaWidth);
	fprintf(fp, "pPrintJob->PrintableAreaLength=%d\n",pPrintJob->PrintableAreaLength);
	fprintf(fp, "pPrintJob->InResolution=%d\n",pPrintJob->InResolution);
	fclose(fp);
}	


#if defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR)

    if (cpyPrintJob.LeftMargin < 0 || cpyPrintJob.TopMargin < 0) return ESCPR_ERR_HAPPEN_PROBLEM;
    /* use borderless technique or not */
    gESCPR_Borderless = (cpyPrintJob.LeftMargin == 0 
        && cpyPrintJob.TopMargin == 0
        && cpyPrintJob.PaperWidth*gESCPR_DPI_Multiplier == cpyPrintJob.PrintableAreaWidth
        && cpyPrintJob.PaperLength*gESCPR_DPI_Multiplier 
            == cpyPrintJob.PrintableAreaLength);

#if defined(_ZERO_MARGIN_MIRROR)
    gESCPR_DupBufferTop = NULL;
    gESCPR_DupBufferBot = NULL;
#endif

    if (gESCPR_Borderless) {

        /* change to borderless margins */
        cpyPrintJob.TopMargin = -1*gESCPR_BasePt.Pad.Top;
        cpyPrintJob.LeftMargin = -1*gESCPR_BasePt.Pad.Left;
        cpyPrintJob.PrintableAreaLength += gESCPR_BasePt.Pad.Top + gESCPR_BasePt.Pad.Bottom;
        cpyPrintJob.PrintableAreaWidth += gESCPR_BasePt.Pad.Left + gESCPR_BasePt.Pad.Right;

        dprintf(("cpyPrintJob.TopMargin: %d\n",cpyPrintJob.TopMargin));
        dprintf(("cpyPrintJob.LeftMargin: %d\n",cpyPrintJob.LeftMargin));
        dprintf(("cpyPrintJob.PrintableAreaWidth: %d\n",cpyPrintJob.PrintableAreaWidth));
        dprintf(("cpyPrintJob.PrintableAreaLength: %d\n",cpyPrintJob.PrintableAreaLength));

#if defined(_ZERO_MARGIN_MIRROR)
        /* allocate memory for duplicated region and save buffer */
        gESCPR_DupBufferTop = ESCPR_Mem_Alloc(
                cpyPrintJob.PrintableAreaWidth*gESCPR_bpp*gESCPR_BasePt.Pad.Top*2);

        if (gESCPR_DupBufferTop == NULL) return ESCPR_ERR_CAN_T_ALLOC_MEMORY;
        ESCPR_Mem_Set(gESCPR_DupBufferTop,255,
                cpyPrintJob.PrintableAreaWidth*gESCPR_bpp*gESCPR_BasePt.Pad.Top*2);

        gESCPR_DupBufferBot = ESCPR_Mem_Alloc(
                cpyPrintJob.PrintableAreaWidth*gESCPR_bpp*gESCPR_BasePt.Pad.Bottom*2);

        if (gESCPR_DupBufferBot == NULL) {
			ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
			return ESCPR_ERR_CAN_T_ALLOC_MEMORY;
		}
        ESCPR_Mem_Set(gESCPR_DupBufferBot,255,
                cpyPrintJob.PrintableAreaWidth*gESCPR_bpp*gESCPR_BasePt.Pad.Bottom*2);

#endif /* defined(_ZERO_MARGIN_MIRROR) */
    } else {
        ESCPR_AdjustBasePoint(&cpyPrintJob);
    }
#else  /* defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR) */
    ESCPR_AdjustBasePoint(&cpyPrintJob);
#endif /* defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR) */

    gESCPR_PrintableAreaLength = cpyPrintJob.PrintableAreaLength;
    gESCPR_PrintableAreaWidth = cpyPrintJob.PrintableAreaWidth;

    paramLength = ESCPR_PRTJOB_PARAMBLC_LENG;
    cmdSize = ESCPR_HEADER_LENG + paramLength;
    
    /* Debug Test >>> */
    dprintf(("---------------\n"));
    dprintf(("ESCPR_PRINT_JOB - Job Command is made from this parameter\n"));
    dprintf(("---------------\n"));
    dprintf(("\tPaperWidth: %d\n",            cpyPrintJob.PaperWidth));
    dprintf(("\tPaperLength: %d\n",           cpyPrintJob.PaperLength));
    dprintf(("\tTopMargin: %d\n",             cpyPrintJob.TopMargin));
    dprintf(("\tLeftMargin: %d\n",            cpyPrintJob.LeftMargin));
    dprintf(("\tPrintableAreaWidth: %d\n",    cpyPrintJob.PrintableAreaWidth));
    dprintf(("\tPrintableAreaLength: %d\n",   cpyPrintJob.PrintableAreaLength));
    dprintf(("\tInResolution: %d\n",          cpyPrintJob.InResolution));
    dprintf(("\tPrintDirection: %d\n",        cpyPrintJob.PrintDirection));
    dprintf(("\n"));
    /* Debug Test <<< */
    
    
    /* Set Parameter */

    /* Header */
    cpy_size = sizeof(Header);
    ESCPR_Mem_Copy((cmdBuff + cpy_count), Header, cpy_size);
    cpy_count += cpy_size;

    /* Parameter Length */
    cpy_size = sizeof(value_array_4);
    if(ESCPR_SetLittleEndian_BYTE4(paramLength, value_array_4)){
        ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_4, cpy_size);
        cpy_count += cpy_size;
    } else{
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Command Name */
    cpy_size = sizeof(cmdName);
    ESCPR_Mem_Copy((cmdBuff + cpy_count), cmdName, cpy_size);
    cpy_count += cpy_size;
    
    /* Paper Width */
    cpy_size = sizeof(value_array_4);
    if(ESCPR_SetBigEndian_BYTE4(cpyPrintJob.PaperWidth, value_array_4)){
        ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_4, cpy_size);
        cpy_count += cpy_size;
    } else{
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Paper Length */
    cpy_size = sizeof(value_array_4);
    if(ESCPR_SetBigEndian_BYTE4(cpyPrintJob.PaperLength, value_array_4)){
        ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_4, cpy_size);
        cpy_count += cpy_size;
    } else{
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Top Margin */
    cpy_size = sizeof(value_array_2);
    if(ESCPR_SetBigEndian_BYTE2(cpyPrintJob.TopMargin, value_array_2)){
        ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_2, cpy_size);
        cpy_count += cpy_size;
    } else{
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Left Margin */
    cpy_size = sizeof(value_array_2);
    if(ESCPR_SetBigEndian_BYTE2(cpyPrintJob.LeftMargin, value_array_2)){
        ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_2, cpy_size);
        cpy_count += cpy_size;
    } else{
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Printable Area - Width */
    cpy_size = sizeof(value_array_4);
    if(ESCPR_SetBigEndian_BYTE4(cpyPrintJob.PrintableAreaWidth, value_array_4)){
        ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_4, cpy_size);
        cpy_count += cpy_size;
    } else{
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Printable Area - Length */
    cpy_size = sizeof(value_array_4);
    if(ESCPR_SetBigEndian_BYTE4(cpyPrintJob.PrintableAreaLength, value_array_4)){
        ESCPR_Mem_Copy((cmdBuff + cpy_count), value_array_4, cpy_size);
        cpy_count += cpy_size;
    } else{
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Input Resolution */
    *(cmdBuff + cpy_count) = cpyPrintJob.InResolution;
    cpy_count += sizeof(ESCPR_UBYTE1);
    
    /* Print Direction */
    *(cmdBuff + cpy_count) = cpyPrintJob.PrintDirection;
    cpy_count += sizeof(ESCPR_UBYTE1);
    
    /* Debug Test >>> */
    if(cmdSize == cpy_count){
        dprintf(("ESCPRCMD : PrintJob Cmd [size = %d] : ", cpy_count));
        for(i = 0; i < cpy_count; i++){
            dprintf(("%.2X ", cmdBuff[i]));
        }
        dprintf(("\n"));
    } else{
        dprintf(("ESCPRCMD : ESCPR_Make PrintQualityCmd Faild!\n"));
#if defined(_ZERO_MARGIN_MIRROR)
		ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
		ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
#endif
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    /* Debug Test <<< */
    
    /* Send Print Job Command to Printer */
#if defined(_ZERO_MARGIN_MIRROR)
        if(ESCPR_SendData_CheckResult(cmdBuff, cmdSize) != ESCPR_ERR_NOERROR) {
			ESCPR_FreeLocalBuf(gESCPR_DupBufferTop);
			ESCPR_FreeLocalBuf(gESCPR_DupBufferBot);
            return ESCPR_ERR_SPOOL_IO;
		}       		
#else
		ESCPR_SendData(NULL, cmdBuff, cmdSize);
#endif

    paramLength = ESCPR_SENDDATA_PARAMBLC_LENG 
        + (cpyPrintJob.PrintableAreaWidth * gESCPR_bpp);
    gESCPR_ImgBufSize = ESCPR_HEADER_LENG + paramLength;

    gESCPR_CompBufSize = (ESCPR_BYTE4)(cpyPrintJob.PrintableAreaWidth * gESCPR_bpp) + 256;  /* 256 is temp buffer */

    return ESCPR_ERR_NOERROR;
}


ESCPR_ERR_CODE ESCPR_MakeStartPageCmd(void)
{
    static const ESCPR_UBYTE1 cmd_StartPage[] = {
        0x1B,  'p', 0x00, 0x00, 0x00, 0x00,  's',  't',  't',  'p',
    };
    
    ESCPR_SendData(NULL, cmd_StartPage, sizeof(cmd_StartPage));

#if defined(_ZERO_MARGIN_MIRROR)
    if (gESCPR_Borderless) {
        gESCPR_TopCnt = 0;
        gESCPR_BotCnt = 0;
        gESCPR_TopFlushed = FALSE;

        if (gESCPR_DupBufferTop != NULL)
        ESCPR_Mem_Set(gESCPR_DupBufferTop,255,
                gESCPR_PrintableAreaWidth*gESCPR_bpp*gESCPR_BasePt.Pad.Top*2);

        if (gESCPR_DupBufferBot != NULL)
        ESCPR_Mem_Set(gESCPR_DupBufferBot,255,
                gESCPR_PrintableAreaWidth*gESCPR_bpp*gESCPR_BasePt.Pad.Bottom*2);

    }
#endif /* defined(_ZERO_MARGIN_MIRROR) */

    return ESCPR_ERR_NOERROR;
}


ESCPR_ERR_CODE ESCPR_MakeImageData(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec)
{
    ESCPR_BYTE4 i;
    ESCPR_UBYTE1* j;
    ESCPR_ERR_CODE Error;                                       /* Error Code    */
    ESCPR_BYTE4 BandHeight = pBandRec->bottom - pBandRec->top;  /* Band Length   */
    const ESCPR_BYTE4 MaxBandHeight = (ESCPR_BYTE4)gESCPR_PrintableAreaLength - pBandRec->top;

    ESCPR_RECT NewBandRec;   /* New Rectangle */
    ESCPR_BANDBMP NewInBmp;  /* New Band Data */
    
    ESCPR_BYTE1 skip = FALSE;

#ifdef _ESCPR_DEBUG
    ESCPR_UBYTE4 DebugBandCount = 0;
#endif
    
    /* Initialize */
    NewInBmp.WidthBytes = pInBmp->WidthBytes;

    /* error check */
    if (pInBmp->Bits == NULL || BandHeight <= 0 || pBandRec->right - pBandRec->left <= 0) {
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }

    if (BandHeight > MaxBandHeight) { 
        BandHeight = MaxBandHeight;
    }
    
    /**************************************************************
     * Core Module 1.0.9
     * The following section of code forces the core
     * module to skip raster lines that are completely white
     * in order to minimize data size.
     * This function is compatible with 8-bit palettized and 
     * 24-bit RGB bitmaps.
     * ************************************************************/
    
    for(i=0; i < BandHeight; i++){
       
        /* calculate position of raster */
        NewInBmp.Bits = ( ((ESCPR_UBYTE1 *)pInBmp->Bits) + (i * pInBmp->WidthBytes) );
        
        /* Make new BandRect */
        NewBandRec.top      = pBandRec->top + i;
        NewBandRec.left     = pBandRec->left;
        NewBandRec.bottom   = NewBandRec.top + 1;
        NewBandRec.right    = pBandRec->right;
        
        skip = FALSE;
        
        for( j = NewInBmp.Bits; j < (NewInBmp.Bits + NewInBmp.WidthBytes); j++){
            if(*j != gESCPR_WhiteColorValue){ /* if any byte in the line is ever non-white */
                skip = FALSE; /* don't skip the line, and break out of the test loop */
                break;
            } else {
                skip = TRUE; /* otherwise, (tentatively) skip the band */
            }
        }

        if(skip == TRUE){ /* If the line happens to be completely white, */
#ifdef _ESCPR_DEBUG
            DebugBandCount++;
#endif      
            continue;   /* Skip the band (don't print) and iterate again */
        }
        
        /* Make 1 line of raster data */
        Error = ESCPR_RasterFilter(&NewInBmp, &NewBandRec);
        if( Error != ESCPR_ERR_NOERROR ) return Error;
    }
    
    /* Some useful debug information - check and see how many lines were actually skipped. */
#ifdef _ESCPR_DEBUG
    gESCPR_TotalDebugBandCount = gESCPR_TotalDebugBandCount + DebugBandCount;
    dprintf(("ESCPRCMD: Skipped %d white raster lines.\n", gESCPR_TotalDebugBandCount));
#endif
    
    return ESCPR_ERR_NOERROR;
}


ESCPR_ERR_CODE ESCPR_RasterFilter(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec) {

    ESCPR_RECT AdjBandRec;      /* Rectangle after BasePointAdjustment */

#if defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR)
    ESCPR_BYTE4 i;
    ESCPR_BANDBMP OutBmp;       /* New band data */
    ESCPR_BYTE4 RepCnt = 0;     /* Repeat count for band data */
    ESCPR_ERR_CODE Error;       /* Error Code */
    ESCPR_UBYTE1* BandBuffer;   /* Temp Buffer for band data */

#endif /* defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR) */

#if defined(_ZERO_MARGIN_MIRROR)
    ESCPR_BYTE4 j;
    ESCPR_RECT NewBandRec;      /* New Rectangle used to divide bands */
#endif /* defined(_ZERO_MARGIN_MIRROR) */

    /* change rectangle due to base point adjustment */
    AdjBandRec.top    = pBandRec->top    + gESCPR_offset_y;
    AdjBandRec.left   = pBandRec->left   + gESCPR_offset_x;
    AdjBandRec.bottom = pBandRec->bottom + gESCPR_offset_y;
    AdjBandRec.right  = pBandRec->right  + gESCPR_offset_x;

    /* band is not visible */
    if ((ESCPR_UBYTE4)AdjBandRec.bottom > gESCPR_PrintableAreaLength){
        return ESCPR_ERR_NOERROR;
    }

    if ((ESCPR_UBYTE4)AdjBandRec.right > gESCPR_PrintableAreaWidth){
        AdjBandRec.right = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth;
    }

#if defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR)
    if (gESCPR_Borderless) {
        if ((ESCPR_UBYTE4)AdjBandRec.top > gESCPR_PrintableAreaLength -
                gESCPR_BasePt.Pad.Top - gESCPR_BasePt.Pad.Bottom ||
            (ESCPR_UBYTE4)AdjBandRec.left > gESCPR_PrintableAreaWidth - 
            gESCPR_BasePt.Pad.Left - gESCPR_BasePt.Pad.Right) {

            return ESCPR_ERR_NOERROR;
        }
    }
#endif

#if defined(_ZERO_MARGIN_REPLICATE)
    if (gESCPR_Borderless) {
        BandBuffer = ESCPR_Mem_Alloc(gESCPR_PrintableAreaWidth*gESCPR_bpp);
        if (BandBuffer == NULL) return ESCPR_ERR_CAN_T_ALLOC_MEMORY;

        Error = ESCPR_RasterFilterReplicate(pInBmp,&AdjBandRec,&RepCnt,&OutBmp,BandBuffer);

        if (Error != ESCPR_ERR_NOERROR) {
            ESCPR_Mem_Free(BandBuffer);
            return Error;
        }

        for (i=0; i < RepCnt; i++) {
            Error =  ESCPR_MakeOneRasterData(&OutBmp,&AdjBandRec);
            if (Error != ESCPR_ERR_NOERROR) {
                ESCPR_Mem_Free(BandBuffer);
                return Error;
            }

            AdjBandRec.top += 1;
            AdjBandRec.bottom += 1;
        }

        ESCPR_Mem_Free(BandBuffer);

        return ESCPR_ERR_NOERROR;
    }

#endif /* defined(_ZERO_MARGIN_REPLICATE) */

#if defined(_ZERO_MARGIN_MIRROR)
    if (gESCPR_Borderless) {
        ESCPR_UBYTE1* pBits;    /* temporary pointer used to save band data ptr */

        BandBuffer = ESCPR_Mem_Alloc(gESCPR_PrintableAreaWidth*gESCPR_bpp);
        if (BandBuffer == NULL) return ESCPR_ERR_CAN_T_ALLOC_MEMORY;
        ESCPR_Mem_Set(BandBuffer,255,gESCPR_PrintableAreaWidth*gESCPR_bpp);

        Error = ESCPR_RasterFilterMirror(pInBmp,&AdjBandRec,&RepCnt,&OutBmp,BandBuffer);
        if (Error != ESCPR_ERR_NOERROR) { 
            ESCPR_Mem_Free(BandBuffer);
            return Error;
        }

        dprintf(("RFZeroMirror: T,B,L,R [%d,%d,%d,%d] Rep: %d\n",
                AdjBandRec.top,
                AdjBandRec.bottom,
                AdjBandRec.left,
                AdjBandRec.right,
                RepCnt));

        pBits = OutBmp.Bits;

        for (i=0; i < RepCnt; i++) {

            NewBandRec.left  = AdjBandRec.left;
            NewBandRec.right = AdjBandRec.right;

            for (j=0; j < (AdjBandRec.bottom - AdjBandRec.top); j++) {

                /* change Bits to point to next band */
                OutBmp.Bits = pBits + j*gESCPR_PrintableAreaWidth*gESCPR_bpp;

                NewBandRec.top    = AdjBandRec.top+j;
                NewBandRec.bottom = NewBandRec.top+1;

                Error =  ESCPR_MakeOneRasterData(&OutBmp,&NewBandRec);
                if (Error != ESCPR_ERR_NOERROR) {
                    ESCPR_Mem_Free(BandBuffer);
                    return Error;
                }
            }

            AdjBandRec.top += 1;
            AdjBandRec.bottom += 1;
        }

        ESCPR_Mem_Free(BandBuffer);

        return ESCPR_ERR_NOERROR;
    }
#endif /* defined(_ZERO_MARGIN_MIRROR) */

    return ESCPR_MakeOneRasterData(pInBmp,&AdjBandRec);
}


#if defined(_ZERO_MARGIN_MIRROR) || (_ZERO_MARGIN_REPLICATE)
ESCPR_ERR_CODE ESCPR_CalculatePadding(ESCPR_RECT *pBandRec,ESCPR_BYTE2* NumPadLeft, 
        ESCPR_BYTE2* NumPadRight, ESCPR_UBYTE2* RasterPixelSize) {

    ESCPR_BYTE4 MaxRasterSize;
    *NumPadRight = 0;
    *NumPadLeft = 0;

    if (gESCPR_Borderless) {
        /* we always make the left side pad because we need to align the user's 
         * image correctly. the right pad can be made with the user's excess data */
        MaxRasterSize = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth - gESCPR_BasePt.Pad.Left - pBandRec->left;
    } else {
        MaxRasterSize = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth;
    }

    if( (ESCPR_UBYTE4)(pBandRec->right - pBandRec->left) <= MaxRasterSize){
        *RasterPixelSize = (ESCPR_UBYTE2)(pBandRec->right - pBandRec->left);
    } else{
        *RasterPixelSize = (ESCPR_UBYTE2) MaxRasterSize;
    }

    /* calculate the padding we need */
    if (gESCPR_Borderless) {
        /* only need left pad if there is no offset */
        if (pBandRec->left == 0) {
            *NumPadLeft = gESCPR_BasePt.Pad.Left;
        } else {
            /* add white space */
            pBandRec->left += gESCPR_BasePt.Pad.Left;
            *NumPadLeft = 0;
        }

        /* calculate padding */
        *NumPadRight = (ESCPR_BYTE2)gESCPR_PrintableAreaWidth - 
            *NumPadLeft - pBandRec->left - *RasterPixelSize;

        /* only need right pad if user sent full raster */
        if (*NumPadRight > gESCPR_BasePt.Pad.Right) *NumPadRight = 0;
        if (*NumPadRight < 0) *NumPadRight = 0;

        pBandRec->right = pBandRec->left + *RasterPixelSize + *NumPadLeft + *NumPadRight;

        dprintf(("PrintableAreaWidth: %d\n",gESCPR_PrintableAreaWidth));
        dprintf(("*RasterPixelSize: %d\n",*RasterPixelSize));
        dprintf(("Pad [%d,%d]\n",*NumPadLeft,*NumPadRight));
        dprintf(("Rect L.R [%d,%d]\n",pBandRec->left,pBandRec->right));
    }
    return ESCPR_ERR_NOERROR;
}

#endif

#if defined(_ZERO_MARGIN_REPLICATE)
ESCPR_ERR_CODE ESCPR_RasterFilterReplicate(const ESCPR_BANDBMP* pInBmp, 
        ESCPR_RECT *pBandRec, ESCPR_BYTE4* RepCnt, ESCPR_BANDBMP* pOutBmp, 
        ESCPR_UBYTE1* TempBuffer) {

    ESCPR_BYTE4    i;
    ESCPR_BYTE2    NumPadLeft  = 0;    /* Padding on the left */
    ESCPR_BYTE2    NumPadRight = 0;    /* Padding on the right */
    ESCPR_UBYTE2   RasterPixelSize;    /* Raster Pixel Size */
    ESCPR_ERR_CODE Error;              /* Error Code */

    pOutBmp->Bits = TempBuffer;
    pOutBmp->WidthBytes = gESCPR_bpp*gESCPR_PrintableAreaWidth;

    Error = ESCPR_CalculatePadding(pBandRec, &NumPadLeft, &NumPadRight, &RasterPixelSize);
    if (Error != ESCPR_ERR_NOERROR) return Error;

    /* fill in left and right pads */
    for(i=0; i < NumPadLeft; i++) {
        ESCPR_Mem_Copy(pOutBmp->Bits+i*gESCPR_bpp,pInBmp->Bits,gESCPR_bpp);
    }

    ESCPR_Mem_Copy(pOutBmp->Bits+NumPadLeft*gESCPR_bpp,
            pInBmp->Bits,RasterPixelSize*gESCPR_bpp);

    for (i=0; i < NumPadRight; i++) {
        ESCPR_Mem_Copy(pOutBmp->Bits
                +(NumPadLeft+RasterPixelSize+i)*gESCPR_bpp,
                &pInBmp->Bits[(RasterPixelSize-1)*gESCPR_bpp],gESCPR_bpp);
    }

    /* adjust the rectangle coordinates */
    if (pBandRec->top == 0) {
        *RepCnt = gESCPR_BasePt.Pad.Top;
    } else if (pBandRec->bottom == 
            (gESCPR_PrintableAreaLength-gESCPR_BasePt.Pad.Top-gESCPR_BasePt.Pad.Bottom)) {

        pBandRec->top    += gESCPR_BasePt.Pad.Top;
        pBandRec->bottom += gESCPR_BasePt.Pad.Top;

        *RepCnt = gESCPR_BasePt.Pad.Bottom;

    } else {
        pBandRec->top    += gESCPR_BasePt.Pad.Top;
        pBandRec->bottom += gESCPR_BasePt.Pad.Top;
        *RepCnt = 1;
    }

    return ESCPR_ERR_NOERROR;
} 
#endif


#if defined(_ZERO_MARGIN_MIRROR)
ESCPR_ERR_CODE ESCPR_RasterFilterMirror(const ESCPR_BANDBMP* pInBmp, ESCPR_RECT *pBandRec, 
        ESCPR_BYTE4* RepCnt, ESCPR_BANDBMP* pOutBmp, ESCPR_UBYTE1* TempBuffer) {

    ESCPR_BYTE4       i;
    ESCPR_BYTE2       NumPadLeft  = 0;    /* Number of padding on the left */
    ESCPR_BYTE2       NumPadRight = 0;    /* Number of padding on the right */
    ESCPR_UBYTE2      RasterPixelSize;    /* Raster Pixel Size */
    const ESCPR_BYTE4 RasterWidth = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth*gESCPR_bpp;
    ESCPR_BYTE4       Src,Dst;            /* Src/Dst for copying */
    ESCPR_BYTE4       TotalRasterSize;    /* Raster pixel size + padding */
    ESCPR_ERR_CODE    Error;

    pOutBmp->Bits = TempBuffer;
    pOutBmp->WidthBytes = RasterWidth;

    Error = ESCPR_CalculatePadding(pBandRec, &NumPadLeft, &NumPadRight, &RasterPixelSize);
    if (Error != ESCPR_ERR_NOERROR) return Error;

    /* fill in left and right pads */
    for(i=0; i < NumPadLeft; i++) {
        Dst = i;
        Src = NumPadLeft-i;

        Src += (rand() % (ESCPR_BLUR_FACTOR)*2) - ESCPR_BLUR_FACTOR;
        if (Src < 0) Src = 0;
        if (Src > RasterPixelSize-1) Src = RasterPixelSize-1;

        ESCPR_Mem_Copy(
                pOutBmp->Bits + Dst*gESCPR_bpp,
                pInBmp->Bits  + Src*gESCPR_bpp,
                gESCPR_bpp);
    }

    ESCPR_Mem_Copy(pOutBmp->Bits+(NumPadLeft)*gESCPR_bpp,
            pInBmp->Bits,
            RasterPixelSize*gESCPR_bpp);

    for (i=0; i < NumPadRight; i++) {
        Dst = i + NumPadLeft + RasterPixelSize;
        Src = RasterPixelSize-1-i;

        Src += (rand() % (ESCPR_BLUR_FACTOR)*2) - ESCPR_BLUR_FACTOR;
        if (Src < 0) Src = 0;
        if (Src > RasterPixelSize-1) Src = RasterPixelSize-1;

        ESCPR_Mem_Copy(
                pOutBmp->Bits + Dst*gESCPR_bpp,
                pInBmp->Bits  + Src*gESCPR_bpp,
                gESCPR_bpp);
    }

    /* adjust the rectangle coordinates */
    pBandRec->top += gESCPR_BasePt.Pad.Top;
    pBandRec->bottom += gESCPR_BasePt.Pad.Top;
    TotalRasterSize = RasterPixelSize+NumPadLeft+NumPadRight;
    *RepCnt = 1;

    /*** save rasters to create duplicate region later ***/

    /* top */
    if (pBandRec->top < 2*gESCPR_BasePt.Pad.Top) {
        Dst = pBandRec->top;
        Src = 0;

        ESCPR_Mem_Copy(
                gESCPR_DupBufferTop+Dst*RasterWidth+pBandRec->left*gESCPR_bpp, 
                pOutBmp->Bits, 
                TotalRasterSize*gESCPR_bpp);

        *RepCnt = 0;
        gESCPR_TopCnt++;
    }

    if (pBandRec->top >= 2*gESCPR_BasePt.Pad.Top && gESCPR_TopFlushed == FALSE) {
        /* if we passed the flushing point and top has not been flushed, it
         * means user skipped the top rasters (by setting offset). there is nothing
         * to flush.
         */
        gESCPR_TopFlushed = TRUE;
    }

    /* bottom */
    if (pBandRec->top > Max(0,gESCPR_PrintableAreaLength - 2*gESCPR_BasePt.Pad.Bottom -1)) {
        Src = 0;
        Dst = gESCPR_BasePt.Pad.Bottom - 
            (gESCPR_PrintableAreaLength - gESCPR_BasePt.Pad.Bottom - pBandRec->top);
            /* distance from bottom of image */

        ESCPR_Mem_Copy(
                gESCPR_DupBufferBot+Dst*RasterWidth +pBandRec->left*gESCPR_bpp, 
                pOutBmp->Bits, 
                TotalRasterSize*gESCPR_bpp);

        /* only output this line if the top has already been printed */
        /* ( if the top has not been flushed, this line will be flushed with the top) */
        if (gESCPR_TopFlushed) {
            *RepCnt = 1;
        } else {
            *RepCnt = 0;
        }
        gESCPR_BotCnt++;
    }

    /*** Create duplicated region ***/
    if (pBandRec->top == gESCPR_PrintableAreaLength - gESCPR_BasePt.Pad.Bottom - 1) {
        /* Create Bottom Duplication Region */

        /* this is last raster , if top has not been flushed, we must flush */
        if (gESCPR_TopFlushed == FALSE) {
            Error = ESCPR_MakeImageFlushTop();
            if (Error != ESCPR_ERR_NOERROR) return Error;

            /* since we flushed top (which includes this raster), 
             * do not print the first raster in MakeMirrorImageBottom */

            Error = ESCPR_MakeMirrorImageBottom(pBandRec,pOutBmp,FALSE);
            if (Error != ESCPR_ERR_NOERROR) return Error;

        } else {
            Error = ESCPR_MakeMirrorImageBottom(pBandRec,pOutBmp,TRUE);
            if (Error != ESCPR_ERR_NOERROR) return Error;
        }
        *RepCnt = 1;

    } else if (pBandRec->top == 2*gESCPR_BasePt.Pad.Top - 1) {

        /* Create Top Duplication Region */
        Error = ESCPR_MakeMirrorImageTop(pBandRec,pOutBmp);
        if (Error != ESCPR_ERR_NOERROR) return Error;

        *RepCnt = 1;
    }

    return ESCPR_ERR_NOERROR;
} 

ESCPR_ERR_CODE ESCPR_MakeMirrorImageBottom(ESCPR_RECT* pBandRec, ESCPR_BANDBMP* pOutBmp,
        const BOOL PrintLastRaster) {

    const ESCPR_BYTE4 RasterWidth = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth*gESCPR_bpp;
    const ESCPR_BYTE4 LowerBoundY = Max(0,(gESCPR_BasePt.Pad.Bottom-gESCPR_BotCnt));
    const ESCPR_BYTE1 Offset = (PrintLastRaster) ? -1 : 0;

    ESCPR_BYTE4 Dst,Src;        /* dst,src row   */
    ESCPR_BYTE4 Src_x,Src_y;    /* dst,src pixel */
    ESCPR_BYTE4 i,j;

    dprintf(("MakeMirrorImageBot: gESCPR_BotCnt: %d LowerBoundY: %d\n",
            gESCPR_BotCnt,LowerBoundY));

    for (i=0; i < gESCPR_BasePt.Pad.Bottom; i++) {

        Dst = (gESCPR_BasePt.Pad.Bottom+i);
        Src = (gESCPR_BasePt.Pad.Bottom-i-1);

        for (j=0; j < gESCPR_PrintableAreaWidth; j++) {
            const ESCPR_BYTE4 Dst_x = j;

            Src_x = j   + (rand() % (ESCPR_BLUR_FACTOR)*2) - ESCPR_BLUR_FACTOR;
            Src_y = Src + (rand() % (ESCPR_BLUR_FACTOR)*2) - ESCPR_BLUR_FACTOR;

            if (Src_x < 0) Src_x = 0;
            if (Src_x > (ESCPR_BYTE4)gESCPR_PrintableAreaWidth-1) Src_x = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth-1;

            if (Src_y < LowerBoundY) Src_y = LowerBoundY;
            if (Src_y > gESCPR_BasePt.Pad.Bottom-1) Src_y = gESCPR_BasePt.Pad.Bottom-1;

            ESCPR_Mem_Copy(
                    gESCPR_DupBufferBot + Dst*RasterWidth   + Dst_x*gESCPR_bpp, 
                    gESCPR_DupBufferBot + Src_y*RasterWidth + Src_x*gESCPR_bpp,
                    gESCPR_bpp);
        }
    }

    /* only print out new section */
    pOutBmp->Bits = gESCPR_DupBufferBot + RasterWidth*(gESCPR_BasePt.Pad.Bottom+Offset);
    pOutBmp->WidthBytes = RasterWidth;

    pBandRec->top = pBandRec->bottom+Offset;
    pBandRec->bottom += gESCPR_BasePt.Pad.Bottom;
    pBandRec->left = 0;
    pBandRec->right = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth;

    return ESCPR_ERR_NOERROR;
}

ESCPR_ERR_CODE ESCPR_MakeMirrorImageTop(ESCPR_RECT* pBandRec, ESCPR_BANDBMP* pOutBmp) {

    const ESCPR_BYTE4 RasterWidth = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth*gESCPR_bpp;
    const ESCPR_BYTE4 UpperBoundY = Min(gESCPR_BasePt.Pad.Top,gESCPR_TopCnt);
    ESCPR_BYTE4 i,j;
    ESCPR_BYTE4 Dst,Src;        /* dst,src row   */
    ESCPR_BYTE4 Src_x,Src_y;    /* dst,src pixel */

    gESCPR_TopFlushed = TRUE;

    dprintf(("MakeMirrorImageTop: gESCPR_TopCnt: %d UpperBoundY: %d\n",
                gESCPR_TopCnt,UpperBoundY));

    for (i=0; i < gESCPR_BasePt.Pad.Top; i++) {
        Dst = i;
        Src = 2*gESCPR_BasePt.Pad.Top-i-1;

        for (j=0; j < gESCPR_PrintableAreaWidth; j++) {
            const ESCPR_BYTE4 Dst_x = j;

            Src_x = j   + (rand() % (ESCPR_BLUR_FACTOR)*2) - ESCPR_BLUR_FACTOR;
            Src_y = Src + (rand() % (ESCPR_BLUR_FACTOR)*2) - ESCPR_BLUR_FACTOR;

            if (Src_x < 0) Src_x = 0;
            if (Src_x > (ESCPR_BYTE4)gESCPR_PrintableAreaWidth-1) Src_x = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth-1;

            if (Src_y < 0) Src_y = 0;
            if (Src_y > UpperBoundY-1) Src_y = UpperBoundY-1;

            /* for now , straight copy of the rows */
            ESCPR_Mem_Copy(
                    gESCPR_DupBufferTop + Dst*RasterWidth   + Dst_x*gESCPR_bpp, 
                    gESCPR_DupBufferTop + Src_y*RasterWidth + Src_x*gESCPR_bpp, 
                    gESCPR_bpp);

        }
    }

    pOutBmp->Bits = gESCPR_DupBufferTop;
    pOutBmp->WidthBytes = RasterWidth;

    pBandRec->top = 0;
    pBandRec->bottom = gESCPR_BasePt.Pad.Top*2;
    pBandRec->left = 0;
    pBandRec->right = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth;

    return ESCPR_ERR_NOERROR;
}

ESCPR_ERR_CODE ESCPR_MakeImageFlushTop(void) {
    ESCPR_RECT BandRec;
    ESCPR_BANDBMP BandBmp;
    ESCPR_ERR_CODE Error;

    dprintf(("ESCPR_MakeImageFlushTop: TopFlush\n"));

    /* there is nothing to flush */
    if (gESCPR_TopCnt == 0) {
        gESCPR_TopFlushed = TRUE;
        return ESCPR_ERR_NOERROR;
    }

    BandRec.right = (ESCPR_BYTE4)gESCPR_PrintableAreaWidth;
    BandRec.left  = 0;

    /* send remaining data */
    Error = ESCPR_MakeMirrorImageTop(&BandRec,&BandBmp);
    if (Error != ESCPR_ERR_NOERROR) return Error;

    /* update rectangle */
    BandRec.top = 0;
    BandRec.bottom = gESCPR_TopCnt+gESCPR_BasePt.Pad.Top;

    /* turn off borderless so mirrored data isnt made again */
    gESCPR_Borderless = FALSE;

    /* create the data */
    Error = ESCPR_MakeImageData(&BandBmp,&BandRec);

    gESCPR_Borderless = TRUE;
    return ESCPR_ERR_NOERROR;
}
#endif

ESCPR_ERR_CODE ESCPR_MakeOneRasterData(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec)
{
    ESCPR_BYTE4 cmdSize;             /* All Size of Print Job Command            */
    ESCPR_BYTE4 cpy_size;            /* Copy Size                                */
    ESCPR_BYTE4 cpy_count = 0;       /* Counter for Set Command                  */
    ESCPR_UBYTE2 RasterDataSize;     /* Raster Data Size                         */
    ESCPR_UBYTE2 RasterPixelSize;    /* Raster Pixel Size                        */
    ESCPR_UBYTE1 compres_flg;        /* Compression flg (1:done compression      */
                                     /*  0:undone compression)                   */

    ESCPR_UBYTE4 paramLength;        /* Parameter Length                         */
    ESCPR_UBYTE1 value_array_2[2];   /* Temporary Buffer for 2 byte Big Endian   */
    ESCPR_UBYTE1 value_array_4[4];   /* Temporary Buffer for 4 byte Big Endian   */
    ESCPR_UBYTE1 *compression_data, *ImageBuff;

#ifdef ESCPR_PACKET_4KB
    ESCPR_BYTE4 rest_size;
    ESCPR_BYTE4 i;                          
#endif
    void *pCompData, *pImageBuff;
    
    static const ESCPR_UBYTE1 Header[2] = {0x1B, 'd'};           /* ESC + Class  */
    static const ESCPR_UBYTE1 cmdName[4] = {'d', 's', 'n', 'd'}; /* Command Name */

    dprintf(("MakeOneRasterData: T,B,L,R [%d,%d,%d,%d]\n", 
            pBandRec->top, pBandRec->bottom, pBandRec->left, pBandRec->right));

    if( (ESCPR_UBYTE4)(pBandRec->right - pBandRec->left) <= gESCPR_PrintableAreaWidth){
        RasterPixelSize = (ESCPR_UBYTE2)(pBandRec->right - pBandRec->left);
    } else{
        RasterPixelSize = (ESCPR_UBYTE2) gESCPR_PrintableAreaWidth;
    }

#ifdef ESCPR_IMAGE_LOG
    dprintf(("ESCPRCMD : ImageData [size = %d] : ", (pBandRec->right - pBandRec->left)* 3 ));
    for(i = 0; i < (pBandRec->right - pBandRec->left) * 3; i++){
        dprintf(("%.2X ", pInBmp->Bits[i]));
    }
    dprintf(("\n"));
#endif
    
    pImageBuff = ESCPR_Mem_Alloc(gESCPR_ImgBufSize);
    if(pImageBuff == NULL){
        return ESCPR_ERR_CAN_T_ALLOC_MEMORY;
    }else{
        ImageBuff = (ESCPR_UBYTE1 *)pImageBuff;
        ESCPR_Mem_Set(ImageBuff, 0xFF, gESCPR_ImgBufSize);
    }
    
    pCompData = ESCPR_Mem_Alloc(gESCPR_CompBufSize);  /* 256 is temp buffer */

    if(pCompData == NULL){
		ESCPR_FreeLocalBuf(pImageBuff);
        return ESCPR_ERR_CAN_T_ALLOC_MEMORY;
    }else{
        compression_data = (ESCPR_UBYTE1 *)pCompData;
        ESCPR_Mem_Set(compression_data, 0xFF, gESCPR_CompBufSize);
    }
    
    RasterDataSize = (ESCPR_UBYTE2) ESCPR_RunLengthEncode((ESCPR_UBYTE1 *)pInBmp->Bits, 
                                                          compression_data,
                                                          RasterPixelSize,
                                                          gESCPR_bpp,
                                                          &compres_flg);
    
    if(compres_flg == 0){
        RasterDataSize = (ESCPR_UBYTE2)(RasterPixelSize * gESCPR_bpp);
        compres_flg = 0;
        compression_data = (ESCPR_UBYTE1 *)pInBmp->Bits;
    }
    
    /* Set Parameter Length */
    paramLength = ESCPR_SENDDATA_PARAMBLC_LENG + RasterDataSize;
    cmdSize = ESCPR_HEADER_LENG + paramLength;
    
    /* Set Parameter */
    
    /* Header */
    cpy_size = sizeof(Header);
    ESCPR_Mem_Copy((ImageBuff + cpy_count), Header, cpy_size);
    cpy_count += cpy_size;
    
    /* Parameter Length */
    cpy_size = sizeof(value_array_4);
    if(ESCPR_SetLittleEndian_BYTE4(paramLength, value_array_4)){
        ESCPR_Mem_Copy((ImageBuff + cpy_count), value_array_4, cpy_size);
        cpy_count += cpy_size;
    } else{
		ESCPR_FreeLocalBuf(pCompData);
		ESCPR_FreeLocalBuf(pImageBuff);
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Command Name */
    cpy_size = sizeof(cmdName);
    ESCPR_Mem_Copy((ImageBuff + cpy_count), cmdName, cpy_size);
    cpy_count += cpy_size;
    
    /* lXoffset */
    cpy_size = sizeof(value_array_2);
    if(ESCPR_SetBigEndian_BYTE2((ESCPR_UBYTE2)pBandRec->left, value_array_2)){
        ESCPR_Mem_Copy((ImageBuff + cpy_count), value_array_2, cpy_size);
        cpy_count += cpy_size;
    } else{
		ESCPR_FreeLocalBuf(pCompData);
		ESCPR_FreeLocalBuf(pImageBuff);
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* lYoffset */
    cpy_size = sizeof(value_array_2);
    if(ESCPR_SetBigEndian_BYTE2((ESCPR_UBYTE2)pBandRec->top, value_array_2)){
        ESCPR_Mem_Copy((ImageBuff + cpy_count), value_array_2, cpy_size);
        cpy_count += cpy_size;
    } else{
		ESCPR_FreeLocalBuf(pCompData);
		ESCPR_FreeLocalBuf(pImageBuff);
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* Compression Mode */
    if(compres_flg == 1){
        *(ImageBuff + cpy_count) = ESCPR_COMP_DONE;
    } else{
        *(ImageBuff + cpy_count) = ESCPR_COMP_NO;
    }
    cpy_count += sizeof(ESCPR_UBYTE1);   
    
    /* Raster Data Size */
    cpy_size = sizeof(value_array_2);
    if(ESCPR_SetBigEndian_BYTE2(RasterDataSize, value_array_2)){
        ESCPR_Mem_Copy((ImageBuff + cpy_count), value_array_2, cpy_size);
        cpy_count += cpy_size;
    } else{
		ESCPR_FreeLocalBuf(pCompData);
		ESCPR_FreeLocalBuf(pImageBuff);
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
    
    /* RGB Raster Data */
    ESCPR_Mem_Copy((ImageBuff + cpy_count), compression_data, RasterDataSize);
    cpy_count += RasterDataSize;
    
    /* Send Print Quality Command to Printer */
#ifdef ESCPR_PACKET_4KB  /*  Set 1 packett size in 4096byte */
    if(cmdSize > ESCPR_PACKET_SIZE_4KB) {
        for(i = 0; i < (ESCPR_BYTE4)(cmdSize / ESCPR_PACKET_SIZE_4KB); i++){

#ifdef ESCPR_IMAGE_LOG
            dprintf(("ESCPRCMD : SendData [size = %d] : ", ESCPR_PACKET_SIZE_4KB));
            for(i = 0; i < ESCPR_PACKET_SIZE_4KB; i++){
                dprintf(("%.2X ", ImageBuff[i]));
            }
            dprintf(("\n"));
#endif /*  ESCPR_IMAGE_LOG */
			if(ESCPR_SendData_CheckResult(ImageBuff, ESCPR_PACKET_SIZE_4KB) != ESCPR_ERR_NOERROR) {
				ESCPR_FreeLocalBuf(pCompData);
				ESCPR_FreeLocalBuf(pImageBuff);
				return ESCPR_ERR_SPOOL_IO;
			}
            ImageBuff += ESCPR_PACKET_SIZE_4KB;
        }
        rest_size = cmdSize - 
            ((ESCPR_BYTE4)(cmdSize / ESCPR_PACKET_SIZE_4KB)) * ESCPR_PACKET_SIZE_4KB;

		if(ESCPR_SendData_CheckResult(ImageBuff, rest_size) != ESCPR_ERR_NOERROR) {
			ESCPR_FreeLocalBuf(pCompData);
			ESCPR_FreeLocalBuf(pImageBuff);
			return ESCPR_ERR_SPOOL_IO;
		}

#ifdef ESCPR_IMAGE_LOG
        dprintf(("ESCPRCMD : SendData [size = %d] : ", rest_size));
        for(i = 0; i < rest_size; i++){
            dprintf(("%.2X ", ImageBuff[i]));
        }
        dprintf(("\n"));
#endif /*  ESCPR_IMAGE_LOG */
    } else{

#ifdef ESCPR_IMAGE_LOG
        if(cmdSize == cpy_count){
            dprintf(("ESCPRCMD : SendData [size = %d] : ", cpy_count));
            for(i = 0; i < cpy_count; i++){
                dprintf(("%.2X ", ImageBuff[i]));
            }
            dprintf(("\n"));
        } else{
            dprintf(("ESCPRCMD : ESCPR_Make PrintQualityCmd Faild!\n"));
			ESCPR_FreeLocalBuf(pCompData);
			ESCPR_FreeLocalBuf(pImageBuff);
            return ESCPR_ERR_HAPPEN_PROBLEM;
        }
#endif /*  ESCPR_IMAGE_LOG */
        if(ESCPR_SendData_CheckResult(ImageBuff, cmdSize) != ESCPR_ERR_NOERROR) {
			ESCPR_FreeLocalBuf(pCompData);
			ESCPR_FreeLocalBuf(pImageBuff);
            return ESCPR_ERR_SPOOL_IO;
		}
    }
#else /*  ESCPR_PACKET_4KB */

#ifdef ESCPR_IMAGE_LOG
    if(cmdSize == cpy_count){
        dprintf(("ESCPRCMD : SendData [size = %d] : ", cpy_count));
        for(i = 0; i < cpy_count; i++){
            dprintf(("%.2X ", ImageBuff[i]));
        }
        dprintf(("\n"));
    } else{
        dprintf(("ESCPRCMD : ESCPR_Make PrintQualityCmd Faild!\n"));
		ESCPR_FreeLocalBuf(pCompData);
		ESCPR_FreeLocalBuf(pImageBuff);
        return ESCPR_ERR_HAPPEN_PROBLEM;
    }
#endif /*  ESCPR_IMAGE_LOG */
    if(ESCPR_SendData_CheckResult(ImageBuff, cmdSize) != ESCPR_ERR_NOERROR) {
		ESCPR_FreeLocalBuf(pCompData);
		ESCPR_FreeLocalBuf(pImageBuff);
        return ESCPR_ERR_SPOOL_IO;
	}

#endif /* ESCPR_PACKET_4KB */

    /* Free RunLength Compression Buffer */
    ESCPR_Mem_Free(pCompData);

    /* Free Image Data Buffer */
    ESCPR_Mem_Free(pImageBuff);

    return ESCPR_ERR_NOERROR;
}

ESCPR_ERR_CODE ESCPR_MakeEndPageCmd(const ESCPR_UBYTE1 NextPage){
    
    static const ESCPR_UBYTE1 cmd_EndPage[] = {
        0x1B,  'p', 0x01, 0x00, 0x00, 0x00,  'e',  'n',  'd',  'p'};
    
#if defined(_ZERO_MARGIN_MIRROR)
    if (!gESCPR_TopFlushed && gESCPR_Borderless) {
        ESCPR_MakeImageFlushTop();
    }
#endif

    ESCPR_SendData(NULL, cmd_EndPage, sizeof(cmd_EndPage));
    ESCPR_SendData(NULL, &NextPage, sizeof(NextPage));
    
    return ESCPR_ERR_NOERROR;
}


ESCPR_ERR_CODE ESCPR_MakeEndJobCmd(void)
{
    static const ESCPR_UBYTE1 cmd_EndJob[] = {
        0x1B,  'j', 0x00, 0x00, 0x00, 0x00,  'e',  'n',  'd',  'j',
    };
    
    static const ESCPR_UBYTE1 cmd_InitPrinter[] = {
        0x1B, 0x40,
    };
    
    static const ESCPR_UBYTE1 cmd_EnterRemoteMode[] = {
        0x1B, 0x28, 0x52, 0x08, 0x00, 0x00, 'R', 'E', 'M', 'O', 'T', 'E', '1',
    };
    
    static const ESCPR_UBYTE1 cmd_RemoteJE[] = {
        'J', 'E', 0x01, 0x00, 0x00,
    };
    
    static const ESCPR_UBYTE1 cmd_ExitRemoteMode[] = {
        0x1B, 0x00, 0x00, 0x00,
    };
    
    /* End Job */
    ESCPR_SendData(NULL, cmd_EndJob, sizeof(cmd_EndJob));
    
    /* Initialize */
    ESCPR_SendData(NULL, cmd_InitPrinter, sizeof(cmd_InitPrinter));
    
    /* Remote Command - JE */
    ESCPR_SendData(NULL, cmd_EnterRemoteMode, sizeof(cmd_EnterRemoteMode));
    ESCPR_SendData(NULL, cmd_RemoteJE, sizeof(cmd_RemoteJE));
    ESCPR_SendData(NULL, cmd_ExitRemoteMode, sizeof(cmd_ExitRemoteMode));

#if defined(_ZERO_MARGIN_MIRROR)
    if (gESCPR_DupBufferTop != NULL) ESCPR_Mem_Free(gESCPR_DupBufferTop);
    if (gESCPR_DupBufferBot != NULL) ESCPR_Mem_Free(gESCPR_DupBufferBot);
#endif /* defined(_ZERO_MARGIN_MIRROR) */
    
    return ESCPR_ERR_NOERROR;
}

void SetGlobalBasePointData_360dpi(const int PaperWidth) {

    /* BORDER */
    gESCPR_BasePt.Border.Top = 
        (42)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Border.Bottom =
        (42)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Border.Left =
        (42)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Border.Right =
        (42)*gESCPR_DPI_Multiplier;

    /* BORDERLESS */
    gESCPR_BasePt.Borderless.Top = 
        (-42)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Borderless.Bottom = 
        (-70)*gESCPR_DPI_Multiplier;

    if (PaperWidth < PAPERWIDTH_A3) {
        gESCPR_BasePt.Borderless.Left =
            (-36)*gESCPR_DPI_Multiplier;
        gESCPR_BasePt.Borderless.Right =
            (-36)*gESCPR_DPI_Multiplier;

    } else {
        gESCPR_BasePt.Borderless.Left =
            (-48)*gESCPR_DPI_Multiplier;
        gESCPR_BasePt.Borderless.Right =
            (-48)*gESCPR_DPI_Multiplier;
    }
#if defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR)
    /* PAD */
    gESCPR_BasePt.Pad.Top = 
        (-1)*gESCPR_BasePt.Borderless.Top;
    gESCPR_BasePt.Pad.Bottom =
        (-1)*gESCPR_BasePt.Borderless.Bottom;
    gESCPR_BasePt.Pad.Left =
        (-1)*gESCPR_BasePt.Borderless.Left;
    gESCPR_BasePt.Pad.Right =
        (-1)*gESCPR_BasePt.Borderless.Right;
#endif
}

void SetGlobalBasePointData_300dpi(const int PaperWidth) {

    /* BORDER */
    gESCPR_BasePt.Border.Top = 
        (35)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Border.Bottom =
        (35)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Border.Left =
        (35)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Border.Right =
        (35)*gESCPR_DPI_Multiplier;

    /* BORDERLESS */
    gESCPR_BasePt.Borderless.Top = 
        (-35)*gESCPR_DPI_Multiplier;
    gESCPR_BasePt.Borderless.Bottom = 
        (-58)*gESCPR_DPI_Multiplier;

    if (PaperWidth < PAPERWIDTH_A3) {
        gESCPR_BasePt.Borderless.Left =
            (-30)*gESCPR_DPI_Multiplier;
        gESCPR_BasePt.Borderless.Right =
            (-30)*gESCPR_DPI_Multiplier;

    } else {
        gESCPR_BasePt.Borderless.Left =
            (-35)*gESCPR_DPI_Multiplier;
        gESCPR_BasePt.Borderless.Right =
            (-35)*gESCPR_DPI_Multiplier;
    }
#if defined(_ZERO_MARGIN_REPLICATE) || defined(_ZERO_MARGIN_MIRROR)
    /* PAD */
    gESCPR_BasePt.Pad.Top = 
        (-1)*gESCPR_BasePt.Borderless.Top;
    gESCPR_BasePt.Pad.Bottom =
        (-1)*gESCPR_BasePt.Borderless.Bottom;
    gESCPR_BasePt.Pad.Left =
        (-1)*gESCPR_BasePt.Borderless.Left;
    gESCPR_BasePt.Pad.Right =
        (-1)*gESCPR_BasePt.Borderless.Right;
#endif
}
