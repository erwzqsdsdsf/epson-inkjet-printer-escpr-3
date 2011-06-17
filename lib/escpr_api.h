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
 * File Name:   escpr_api.h
 *
 ***********************************************************************/

#ifndef __EPSON_ESCPR_API_H__
#define __EPSON_ESCPR_API_H__

#include "escpr_def.h"

/*=======================================================================================*/
/* functions                                                                             */
/*=======================================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    ESCPR_ERR_CODE escprInitJob(const ESCPR_OPT *pRGB_opt,
                                const ESCPR_PRINT_QUALITY *pPrintQuality,
                                const ESCPR_PRINT_JOB *pPrintJob);
    ESCPR_ERR_CODE escprInitPage(void);
    ESCPR_ERR_CODE escprBandOut(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec);
    ESCPR_ERR_CODE escprTerminatePage(const ESCPR_UBYTE1 NextPage);
    ESCPR_ERR_CODE escprDestroyJob(void);
    ESCPR_ERR_CODE escprFilterPMReply(ESCPR_UBYTE1* pPMinfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

