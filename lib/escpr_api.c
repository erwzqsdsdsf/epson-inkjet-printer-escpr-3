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
 * File Name:   escpr_api.c
 *
 ***********************************************************************/

#include "escpr_api.h"
#include "escpr_cmd.h"
#include "escpr_sp.h"

/*=======================================================================================*/
/* Debug                                                                                 */
/*=======================================================================================*/
#ifdef _ESCPR_DEBUG
#include <stdio.h>
#define dprintf(a) printf(a)
#else
#define dprintf(a)
#endif

/*=======================================================================================*/
/* Define Area                                                                           */
/*=======================================================================================*/


/*=======================================================================================*/
/* Global Area                                                                           */
/*=======================================================================================*/
ESCPR_GLOBALFUNC gESCPR_Func;

ESCPR_STATUS gESCPR_JobStatus  = ESCPR_STATUS_NOT_INITIALIZED;     /*  Job status */
ESCPR_STATUS gESCPR_PageStatus = ESCPR_STATUS_NOT_INITIALIZED;     /*  Page status */

ESCPR_ERR_CODE gESCPR_ErrorCode;

/*=======================================================================================*/
/* ESC/P-R Core Module API                                                               */
/*=======================================================================================*/
ESCPR_ERR_CODE escprInitJob(const ESCPR_OPT *pRGB_opt, 
        const ESCPR_PRINT_QUALITY *pPrintQuality, const ESCPR_PRINT_JOB *pPrintJob)
{
    ESCPR_PRINT_QUALITY spPrintQuality;
    ESCPR_PRINT_JOB     spPrintJob;

    dprintf(("ESCPRAPI : escprInitJob()\n"));
    
    gESCPR_ErrorCode = ESCPR_ERR_NOERROR;
    
    /*  ERROR CHECK! */
    if (gESCPR_JobStatus == ESCPR_STATUS_INITIALIZED)
    {
        gESCPR_ErrorCode = ESCPR_ERR_DON_T_PROCESS;
        dprintf("ESCPRAPI : escprInitJob() --- illegal call\n");
        return gESCPR_ErrorCode;
    }
    
    /* Entry Functions */
    gESCPR_Func.gfpSendData = pRGB_opt->fpspoolfunc;        /* Function of Send Data */

    /* NOTE : Using ChangeSpec function */
    /* Assigned prameters to this API would be used on the caller's side again */
    /* It must not be changed anything members of parameters                   */
    /* in order to avoid side-effect on the caller's side                      */
    /* Be sure that duplicated parameters must be used to call ChangeSpec      */
    /* because it would be changed someway internally                          */
    spPrintQuality = *pPrintQuality;
    spPrintJob     = *pPrintJob;

    /* ChangeSpec : Known issue (2005/10/14 #2) */
    ESCPR_RequestServicePack(SP_REQ_FIX_720DPI, (void*)&spPrintJob, 
        /* reserved */ NULL, NULL, NULL);

    /* ChangeSpec : Known issue (2005/10/14 #3) */
    ESCPR_RequestServicePack(SP_REQ_FIX_COMPOSITE_BLACK, (void*)&spPrintQuality, 
        /* reserved */ NULL, NULL, NULL);

    /* ChangeSpec : Known issue (2005/10/14 #4) */
    ESCPR_RequestServicePack(SP_REQ_FIX_DRAFTONLY, (void*)&spPrintQuality, (void*)&spPrintJob, 
        /* reserved */ NULL, NULL);
    
    /* Send Header Command (ExitPacketMode, InitPrinter, InitPrinter) */
    gESCPR_ErrorCode = ESCPR_MakeHeaderCmd();
    
    if( gESCPR_ErrorCode != ESCPR_ERR_NOERROR ){
        dprintf(("ESCPRAPI : ESCPR_MakeHeaderCmd Failed\n"));
        return gESCPR_ErrorCode;
    }
    
    /* Send Print Quality Command */
    gESCPR_ErrorCode = ESCPR_MakePrintQualityCmd(&spPrintQuality);
    
    if( gESCPR_ErrorCode != ESCPR_ERR_NOERROR ){
        dprintf(("ESCPRAPI : ESCPR_MakePrintQualityCmd Faild\n"));
        return gESCPR_ErrorCode;
    }
    
    /* Send Print Job Command */
    gESCPR_ErrorCode = ESCPR_MakePrintJobCmd(&spPrintJob);
    
    if (gESCPR_ErrorCode != ESCPR_ERR_NOERROR ){
        dprintf(("ESCPRAPI : ESCPR_MakePrintJobCmd Faild\n"));
        return gESCPR_ErrorCode;
    }

    /*  change status */
    gESCPR_JobStatus = ESCPR_STATUS_INITIALIZED;

    return gESCPR_ErrorCode;
}


ESCPR_ERR_CODE escprInitPage(void)
{
    dprintf(("ESCPRAPI : escprInitPage()\n"));
    
    /*  ERROR CHECK! */
    if (gESCPR_PageStatus == ESCPR_STATUS_INITIALIZED)
    {
        gESCPR_ErrorCode = ESCPR_ERR_DON_T_PROCESS;
        dprintf("ESCPRAPI : escprInitPage() --- illegal call\n");
        return gESCPR_ErrorCode;
    }

    /* Send Start Page Command */
    gESCPR_ErrorCode = ESCPR_MakeStartPageCmd();
    
    if( gESCPR_ErrorCode != ESCPR_ERR_NOERROR ){
        dprintf(("ESCPRAPI : ESCPR_MakeStartPageCmd Faild\n"));
        return gESCPR_ErrorCode;
    }

    /*  change status */
    gESCPR_PageStatus = ESCPR_STATUS_INITIALIZED;
    
    return gESCPR_ErrorCode;
}

ESCPR_ERR_CODE escprBandOut(const ESCPR_BANDBMP *pInBmp, ESCPR_RECT *pBandRec)
{
    dprintf(("ESCPRAPI : escprBandOut()\n"));

    /*  ERROR CHECK! */
    if( (gESCPR_JobStatus != ESCPR_STATUS_INITIALIZED) 
            || (gESCPR_PageStatus != ESCPR_STATUS_INITIALIZED) )
    {
        gESCPR_ErrorCode = ESCPR_ERR_DON_T_PROCESS;
        dprintf("ESCPRAPI : escprBandOut() --- illegal call\n");
        return gESCPR_ErrorCode;
    }
    
    /* Send Image Data */
    gESCPR_ErrorCode = ESCPR_MakeImageData(pInBmp, pBandRec);
    
    if( gESCPR_ErrorCode != ESCPR_ERR_NOERROR ){
        dprintf(("ESCPRAPI : ESCPR_MakeImageData Faild\n"));
        return gESCPR_ErrorCode;
    }
    
    return gESCPR_ErrorCode;
}

ESCPR_ERR_CODE escprTerminatePage(const ESCPR_UBYTE1 NextPage)
{
    dprintf(("ESCPRAPI : escprTerminatePage()\n"));

    /*  ERROR CHECK! */
    if (gESCPR_PageStatus == ESCPR_STATUS_NOT_INITIALIZED)
    {
        gESCPR_ErrorCode = ESCPR_ERR_DON_T_PROCESS;
        dprintf("ESCPRAPI : escprTerminatePage() --- illegal call\n");
        return gESCPR_ErrorCode;
    }

    /*  change status */
    gESCPR_PageStatus = ESCPR_STATUS_NOT_INITIALIZED;

    /* Send End Page Command */
    gESCPR_ErrorCode = ESCPR_MakeEndPageCmd(NextPage);
    
    if( gESCPR_ErrorCode != ESCPR_ERR_NOERROR ){
        dprintf(("ESCPRAPI : ESCPR_MakeEndPageCmd Faild\n"));
        return gESCPR_ErrorCode;
    }

    return gESCPR_ErrorCode;
}

ESCPR_ERR_CODE escprDestroyJob(void)
{
    dprintf(("ESCPRAPI : escprDestroyJob()\n"));

    /*  ERROR CHECK! */
    if (gESCPR_JobStatus == ESCPR_STATUS_NOT_INITIALIZED)
    {
        gESCPR_ErrorCode = ESCPR_ERR_DON_T_PROCESS;
        dprintf("ESCPRAPI : escprDestroyJob() --- illegal call\n");
        return gESCPR_ErrorCode;
    }

    /*  change status */
    gESCPR_JobStatus = ESCPR_STATUS_NOT_INITIALIZED;

    /* Send End Job Command */
    gESCPR_ErrorCode = ESCPR_MakeEndJobCmd();

    if( gESCPR_ErrorCode != ESCPR_ERR_NOERROR ){
        dprintf(("ESCPRAPI : ESCPR_MakeEndJobCmd Faild\n"));
        return gESCPR_ErrorCode;
    }

    return gESCPR_ErrorCode;
}

ESCPR_ERR_CODE escprFilterPMReply(ESCPR_UBYTE1* pPMinfo)
{
    ESCPR_ERR_CODE Ret;
    dprintf(("ESCPRAPI : escprFilterPMReply()\n"));

    /* ChangeSpec : Known issue (2005/10/14 #1) */
    Ret = ESCPR_RequestServicePack(SP_REQ_FIX_UPDATE_PMREPLY, pPMinfo,
       /* reserved */ NULL, NULL, NULL);

    if(Ret != ESCPR_SP_ERR_NONE) {
        dprintf(("ESCPRAPI : escprFilterPMReply() --- failed [%d]\n", Ret));
        gESCPR_ErrorCode = ESCPR_ERR_HAPPEN_PROBLEM;
        return gESCPR_ErrorCode;
    }

    return ESCPR_ERR_NOERROR;
}
