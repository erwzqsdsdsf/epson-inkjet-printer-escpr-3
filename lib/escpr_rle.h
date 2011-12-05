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
 * File Name:   escpr_rle.h
 *
 ***********************************************************************/

#ifndef __EPSON_ESCPR_RLE_H__
#define __EPSON_ESCPR_RLE_H__

#include "escpr_osdep.h"

ESCPR_BYTE4 ESCPR_RunLengthEncode(const ESCPR_UBYTE1 *pSrcAddr, ESCPR_UBYTE1 *pDstAddr, 
        ESCPR_BYTE2 pixel, ESCPR_BYTE1 bpp, ESCPR_UBYTE1 *pCompress);

#endif
