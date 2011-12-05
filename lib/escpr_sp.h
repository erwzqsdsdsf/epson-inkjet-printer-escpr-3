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
 * File Name:   escpr_sp.h
 *
 ***********************************************************************/

#ifndef __EPSON_ESCPR_SP_H__
#define __EPSON_ESCPR_SP_H__

#include "escpr_def.h"

/* Function retun code */
#define ESCPR_SP_ERR_NONE                      0
#define ESCPR_SP_ERR_INVALID_REQUEST           (-1)
#define ESCPR_SP_ERR_PM_INVALID_POINTER        (-2)
#define ESCPR_SP_ERR_PM_INVALID_HEADER         (-3)
#define ESCPR_SP_ERR_PM_INVALID_TERMINATOR     (-4)
#define ESCPR_SP_ERR_PM_NO_VALID_FIELD         (-5)

/* Service Pack request code */
typedef enum _SP_REQ {
    SP_REQ_FIX_UPDATE_PMREPLY= 0,
    SP_REQ_FIX_600_720DPI,
    SP_REQ_FIX_COMPOSITE_BLACK,
    SP_REQ_FIX_DRAFTONLY
} SP_REQ_CODE;

/* note about parameters */
/* request : Request code */
/* pData1st : Required data type for specific request */
/* pData2nd : Required data type for specific request */
/* pData3th : Required data type for specific request (currently just reserved) */
/* pData4th : Required data type for specific request (currently just reserved) */
ESCPR_ERR_CODE ESCPR_RequestServicePack(SP_REQ_CODE request, 
    void* pData1st, void* pData2nd, void* pData3th, void* pData4th);

#endif

