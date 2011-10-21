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
 * File Name:   escpr_sp.c
 *
 ***********************************************************************/

#include "escpr_sp.h"

/*=======================================================================================*/
/* Debug                                                                                 */
/*=======================================================================================*/
/*#define _ESCPR_DEBUG_SP*/
/*#define _ESCPR_DEBUG_SP_VERBOSE*/

#ifdef _ESCPR_DEBUG_SP
#include <stdio.h>
#define dbprint(a) printf a
#else
#define dbprint(a)
#endif

#ifdef _ESCPR_DEBUG_SP
typedef enum _DUMP_TYPE {
    DUMP_HEX = 0,
    DUMP_ASCII,
    DUMP_S_TAG_ONLY,
} DUMP_TYPE;

char* str[] ={
                 "DUMP_HEX",
                 "DUMP_ASCII",
                 "DUMP_S_TAG_ONLY",
             };

static void print_PMREPLY(ESCPR_UBYTE1 * pm, DUMP_TYPE type, ESCPR_BYTE1* msg)
{
    ESCPR_UBYTE1 * p = pm;
    ESCPR_BYTE2 col = 0;

    if(*p != 'S') {
        if(type != DUMP_HEX) {
            return; /* do not anything */
        }

        /* Anyway if type is DUMP_HEX then dump it */
    }

    dbprint(("%s\n", msg));
    dbprint(("PM REPLY DUMP [TYPE:%s]\n", str[type]));

    if(type == DUMP_HEX) {
        while(!((*p == 0x0D) && (*(p+1) == 0x0A))) {
            dbprint(("0x%02X ",   *p++));

            if((++col % 10) == 0) {
                dbprint(("\n"));
            }
        }

    } else {
        while(*p == 'S') {
            dbprint(("%c ",   *p++));
            dbprint(("%02d\n", *p++));
            while(*p == 'T') {
                dbprint(("  %c",     *p++));
                dbprint(("  %02d",   *p++));
                dbprint(("  [0x%02X]", *p++));
                dbprint(("  %c\n",     *p++));
            }
            dbprint(("%c\n",     *p++));

            if(type == DUMP_S_TAG_ONLY) {
                break;
            }

            if ((*p == 0x0D) && (*(p+1) == 0x0A)) {
                break;
            }
        }

    }

    if(type != DUMP_S_TAG_ONLY) {
        dbprint(("0x%02X ",   *p++));
        dbprint(("0x%02X ",   *p));
    }

    dbprint(("\nEND\n"));

}
#endif

#ifdef _ESCPR_DEBUG_SP
#define DUMP_PMREPLY(a) print_PMREPLY a
#else
#define DUMP_PMREPLY(a)
#endif

#ifdef _ESCPR_DEBUG_SP_VERBOSE
#define VERBOSE_DUMP_PMREPLY(a) print_PMREPLY a
#define verbose_dbprint(a)      dbprint(a)
#else
#define VERBOSE_DUMP_PMREPLY(a)
#define verbose_dbprint(a)
#endif

/*=======================================================================================*/
/* Define Area                                                                           */
/*=======================================================================================*/

/* PM infomation to be retain in Service Pack module */
#define PM_HEADER_LEN         9
#define PM_TERMINATOR_LEN     2
#define PM_MAX_SIZE           512

typedef enum PM_STATE {
    PM_STATE_NOT_FILTERED = 0,
    PM_STATE_FILTERED
} PM_STATE;

typedef struct _tagESCPR_MODEL_INFO {
    PM_STATE        state;
    ESCPR_UBYTE1    data[PM_MAX_SIZE];
} ESCPR_MODEL_INFO;

/* Service Pack ChangeSpec function prototype */
typedef ESCPR_BYTE4 (* ESCPR_PFNSP)(void *, void *, void*, void*);

/* Each one do to fix 'Known Issue' about ESC/P-R printer */
static ESCPR_BYTE4 _SP_ChangeSpec_UpdatePMReply(void*, void*, void*, void*);
static ESCPR_BYTE4 _SP_ChangeSpec_600_720DPI(void* , void*, void*,void*);
static ESCPR_BYTE4 _SP_ChangeSpec_CompositeBlack(void*, void*, void*, void*);
static ESCPR_BYTE4 _SP_ChangeSpec_DraftOnly(void*, void*, void*, void*);

/* Functions should be ordered in the same index with SP_REQ_CODE */
ESCPR_PFNSP fpChangeSpec [] = {
                              _SP_ChangeSpec_UpdatePMReply,   /* SP_REQ_FIX_UPDATE_PMREPLY */
                              _SP_ChangeSpec_600_720DPI,      /* SP_REQ_FIX_600_720DPI */
                              _SP_ChangeSpec_CompositeBlack,  /* SP_REQ_FIX_COMPOSITE_BLACK */
                              _SP_ChangeSpec_DraftOnly        /* SP_REQ_FIX_DRAFTONLY */
                          };

/* Support media size id */
#define     PM_MSID_A4                  0x00
#define     PM_MSID_LETTER              0x01
#define     PM_MSID_LEGAL               0x02
#define     PM_MSID_A5                  0x03
#define     PM_MSID_A6                  0x04
#define     PM_MSID_B5                  0x05
#define     PM_MSID_EXECUTIVE           0x06
#define     PM_MSID_HALFLETTER          0x07
#define     PM_MSID_PANORAMIC           0x08
#define     PM_MSID_TRIM_4X6            0x09
#define     PM_MSID_4X6                 0x0A
#define     PM_MSID_5X8                 0x0B
#define     PM_MSID_8X10                0x0C
#define     PM_MSID_10X15               0x0D
#define     PM_MSID_200X300             0x0E
#define     PM_MSID_L                   0x0F
#define     PM_MSID_POSTCARD            0x10
#define     PM_MSID_DBLPOSTCARD         0x11
#define     PM_MSID_ENV_10_L            0x12
#define     PM_MSID_ENV_C6_L            0x13
#define     PM_MSID_ENV_DL_L            0x14
#define     PM_MSID_NEWEVN_L            0x15
#define     PM_MSID_CHOKEI_3            0x16
#define     PM_MSID_CHOKEI_4            0x17
#define     PM_MSID_YOKEI_1             0x18
#define     PM_MSID_YOKEI_2             0x19
#define     PM_MSID_YOKEI_3             0x1A
#define     PM_MSID_YOKEI_4             0x1B
#define     PM_MSID_2L                  0x1C
#define     PM_MSID_ENV_10_P            0x1D
#define     PM_MSID_ENV_C6_P            0x1E
#define     PM_MSID_ENV_DL_P            0x1F
#define     PM_MSID_NEWENV_P            0x20
#define     PM_MSID_MEISHI              0x21
#define     PM_MSID_BUZCARD_89X50       0x22
#define     PM_MSID_CARD_54X86          0x23
#define     PM_MSID_BUZCARD_55X91       0x24
#define     PM_MSID_ALBUM_L             0x25
#define     PM_MSID_ALBUM_A5            0x26
#define     PM_MSID_PALBUM_L_L          0x27
#define     PM_MSID_PALBUM_2L           0x28
#define     PM_MSID_PALBUM_A5_L         0x29
#define     PM_MSID_PALBUM_A4           0x2A
#define     PM_MSID_A3NOBI              0x3D
#define     PM_MSID_A3                  0x3E
#define     PM_MSID_B4                  0x3F
#define     PM_MSID_USB                 0x40
#define     PM_MSID_11X14               0x41
#define     PM_MSID_B3                  0x42
#define     PM_MSID_A2                  0x43
#define     PM_MSID_USC                 0x44
#define     PM_MSID_USER                0x63

/* Support media type id */
#define     PM_MTID_PLAIN               0x00
#define     PM_MTID_360INKJET           0x01
#define     PM_MTID_IRON                0x02
#define     PM_MTID_PHOTOINKJET         0x03
#define     PM_MTID_PHOTOADSHEET        0x04
#define     PM_MTID_MATTE               0x05
#define     PM_MTID_PHOTO               0x06
#define     PM_MTID_PHOTOFILM           0x07
#define     PM_MTID_MINIPHOTO           0x08
#define     PM_MTID_OHP                 0x09
#define     PM_MTID_BACKLIGHT           0x0A
#define     PM_MTID_PGPHOTO             0x0B
#define     PM_MTID_PSPHOTO             0x0C
#define     PM_MTID_PLPHOTO             0x0D
#define     PM_MTID_MCGLOSSY            0x0E
#define     PM_MTID_ARCHMATTE           0x0F
#define     PM_MTID_WATERCOLOR          0x10
#define     PM_MTID_PROGLOSS            0x11
#define     PM_MTID_MATTEBOARD          0x12
#define     PM_MTID_PHOTOGLOSS          0x13
#define     PM_MTID_SEMIPROOF           0x14
#define     PM_MTID_SUPERFINE2          0x15
#define     PM_MTID_DSMATTE             0x16
#define     PM_MTID_CLPHOTO             0x17
#define     PM_MTID_ECOPHOTO            0x18
#define     PM_MTID_VELVETFINEART       0x19
#define     PM_MTID_PROOFSEMI           0x1A
#define     PM_MTID_HAGAKIRECL          0x1B
#define     PM_MTID_HAGAKIINKJET        0x1C
#define     PM_MTID_PHOTOINKJET2        0x1D
#define     PM_MTID_DURABRITE           0x1E
#define     PM_MTID_MATTEMEISHI         0x1F
#define     PM_MTID_HAGAKIATENA         0x20
#define     PM_MTID_PHOTOALBUM          0x21
#define     PM_MTID_PHOTOSTAND          0x22
#define     PM_MTID_RCB                 0x23
#define     PM_MTID_PGPHOTOEG           0x24
#define     PM_MTID_ENVELOPE            0x25
#define     PM_MTID_PLATINA             0x26
#define     PM_MTID_ULTRASMOOTH         0x27
/* add Wed Jan 28 2009 v */
#define     PM_MTID_SFHAGAKI            0x28  /* "Super Fine Postcard"            */
#define     PM_MTID_PHOTOSTD            0x29  /* "Premium Glossy Photo Paper"     */
#define     PM_MTID_GLOSSYHAGAKI        0x2A  /* "Glossy Postcard"                */
#define     PM_MTID_GLOSSYPHOTO         0x2B  /* "Glossy Photo Paper"             */
#define     PM_MTID_GLOSSYCAST          0x2C  /* "Epson Photo"                    */
#define     PM_MTID_BUSINESSCOAT        0x2D  /* "Business Ink Jet Coat Paper"    */
/* add Wed Jan 28 2009 ^ */
#define     PM_MTID_CDDVD               0x5B
#define     PM_MTID_CDDVDHIGH           0x5C
#define     PM_MTID_CLEANING            0x63

/* Support media size id table */
static struct _tag_PM_PAPER_SIZE {
    ESCPR_UBYTE1   id;      /* PM_MTID_xx */
    ESCPR_BYTE4    width360;   /* pixels for 360 DPI */
    ESCPR_BYTE4    length360;  /* pixels for 360 DPI */
    ESCPR_BYTE4    width300;   /* pixels for 300 DPI */
    ESCPR_BYTE4    length300;  /* pixels for 300 DPI */
} PM_PAPER_SIZE [] = {
    /* ID                  Width  Length   Width  Length*/
    {PM_MSID_A4,            2976,   4209,  2480,  3507},
    {PM_MSID_LETTER,        3060,   3960,  2550,  3300},
    {PM_MSID_LEGAL,         3060,   5040,  2550,  4200},
    {PM_MSID_A5,            2098,   2976,  1748,  2480},
    {PM_MSID_A6,            1488,   2098,  1240,  1748},
    {PM_MSID_B5,            2580,   3643,  2149,  3035},
    {PM_MSID_EXECUTIVE,     2610,   3780,  2175,  3150},
    {PM_MSID_HALFLETTER,    1980,   3060,  1650,  2550},
    {PM_MSID_PANORAMIC,     2976,   8419,  2480,  7016},
    {PM_MSID_TRIM_4X6,      1610,   2330,  1342,  1942},
    {PM_MSID_4X6,           1440,   2160,  1200,  1800},
    {PM_MSID_5X8,           1800,   2880,  1500,  2400},
    {PM_MSID_8X10,          2880,   3600,  2400,  3000},
    {PM_MSID_10X15,         1417,   2125,  1181,  1771},
    {PM_MSID_200X300,       3061,   4790,  2551,  3992},
    {PM_MSID_L,             1260,   1800,  1050,  1500},
    {PM_MSID_POSTCARD,      1417,   2098,  1181,  1748},
    {PM_MSID_DBLPOSTCARD,   2835,   2098,  2363,  1748},
    {PM_MSID_ENV_10_L,      3420,   1485,  2850,  1238},
    {PM_MSID_ENV_C6_L,      2296,   1616,  1913,  1347},
    {PM_MSID_ENV_DL_L,      3118,   1559,  2598,  1299},
    {PM_MSID_NEWEVN_L,      3118,   1871,  2598,  1559},
    {PM_MSID_CHOKEI_3,      1701,   3685,  1418,  3071},
    {PM_MSID_CHOKEI_4,      1276,   3161,  1063,  2634},
    {PM_MSID_YOKEI_1,       1701,   2494,  1418,  2078},
    {PM_MSID_YOKEI_2,       1616,   2296,  1347,  1913},
    {PM_MSID_YOKEI_3,       1389,   2098,  1158,  1748},
    {PM_MSID_YOKEI_4,       1488,   3331,  1240,  2776},
    {PM_MSID_2L,            1800,   2522,  1500,  2100},
    {PM_MSID_ENV_10_P,      1485,   3420,  1238,  2850},
    {PM_MSID_ENV_C6_P,      1616,   2296,  1347,  1913},
    {PM_MSID_ENV_DL_P,      1559,   3118,  1299,  2598},
    {PM_MSID_NEWENV_P,      1871,   3118,  1559,  2598},
    {PM_MSID_MEISHI,        1261,    779,  1051,   649},
    {PM_MSID_BUZCARD_89X50, 1261,    709,  1051,   591},
    {PM_MSID_CARD_54X86,     765,   1219,   638,  1016},
    {PM_MSID_BUZCARD_55X91,  780,   1290,   650,  1075},
    {PM_MSID_ALBUM_L,       1800,   2607,  1200,  2133},
    {PM_MSID_ALBUM_A5,      2976,   4294,  2480,  3578},
    {PM_MSID_PALBUM_L_L,    1800,   1260,  1500,  1050},
    {PM_MSID_PALBUM_2L,     1800,   2521,  1500,  2101},
    {PM_MSID_PALBUM_A5_L,   2976,   2101,  2480,  1751},
    {PM_MSID_PALBUM_A4,     2976,   4203,  2480,  3503},
    {PM_MSID_A3NOBI,        4663,   6846,  3886,  5705},
    {PM_MSID_A3,            4209,   5953,  3507,  4960},
    {PM_MSID_B4,            3643,   5159,  3036,  4299},
    {PM_MSID_USB,           3960,   6120,  3300,  5100},
    {PM_MSID_11X14,         3960,   5040,  3300,  4200},
    {PM_MSID_B3,            5159,   7285,  4299,  6071},
    {PM_MSID_A2,            5953,   8419,  4961,  7016},
    {PM_MSID_USC,           6120,   7920,  5100,  6600},
    {PM_MSID_USER,             0,      0},
};

ESCPR_BYTE2 NUM_PAPER_SIZE = sizeof(PM_PAPER_SIZE) / sizeof(PM_PAPER_SIZE[0]);

/* Support media type id table */
ESCPR_UBYTE1 PM_TYPE_IDS [] = {
    PM_MTID_PLAIN,
    PM_MTID_360INKJET,
    PM_MTID_IRON,
    PM_MTID_PHOTOINKJET,
    PM_MTID_PHOTOADSHEET,
    PM_MTID_MATTE,
    PM_MTID_PHOTO,
    PM_MTID_PHOTOFILM,
    PM_MTID_MINIPHOTO,
    PM_MTID_OHP,
    PM_MTID_BACKLIGHT,
    PM_MTID_PGPHOTO,
    PM_MTID_PSPHOTO,
    PM_MTID_PLPHOTO,
    PM_MTID_MCGLOSSY,
    PM_MTID_ARCHMATTE,
    PM_MTID_WATERCOLOR,
    PM_MTID_PROGLOSS,
    PM_MTID_MATTEBOARD,
    PM_MTID_PHOTOGLOSS,
    PM_MTID_SEMIPROOF,
    PM_MTID_SUPERFINE2,
    PM_MTID_DSMATTE,
    PM_MTID_CLPHOTO,
    PM_MTID_ECOPHOTO,
    PM_MTID_VELVETFINEART,
    PM_MTID_PROOFSEMI,
    PM_MTID_HAGAKIRECL,
    PM_MTID_HAGAKIINKJET,
    PM_MTID_PHOTOINKJET2,
    PM_MTID_DURABRITE,
    PM_MTID_MATTEMEISHI,
    PM_MTID_HAGAKIATENA,
    PM_MTID_PHOTOALBUM,
    PM_MTID_PHOTOSTAND,
    PM_MTID_RCB,
    PM_MTID_PGPHOTOEG,
    PM_MTID_ENVELOPE,
    PM_MTID_PLATINA,
    PM_MTID_ULTRASMOOTH,
/* add Wed Jan 28 2009 v */
    PM_MTID_SFHAGAKI,       /* "Super Fine Postcard"            */
    PM_MTID_PHOTOSTD,       /* "Premium Glossy Photo Paper"     */
    PM_MTID_GLOSSYHAGAKI,   /* "Glossy Postcard"                */
    PM_MTID_GLOSSYPHOTO,    /* "Glossy Photo Paper"             */
    PM_MTID_GLOSSYCAST,     /* "Epson Photo"                    */
    PM_MTID_BUSINESSCOAT,   /* "Business Ink Jet Coat Paper"    */
/* add Wed Jan 28 2009 ^ */
    PM_MTID_CDDVD,
    PM_MTID_CDDVDHIGH,
    PM_MTID_CLEANING,
};

ESCPR_BYTE2 NUM_TYPE_IDS = sizeof(PM_TYPE_IDS) / sizeof(PM_TYPE_IDS[0]);

/*=======================================================================================*/
/* Global Area                                                                           */
/*=======================================================================================*/
static ESCPR_MODEL_INFO g_PMinfo; /* Filtered pm info to be retained within ServicePack */

/*=======================================================================================*/
/* Local function definition Area                                                        */
/*=======================================================================================*/
/* ---------------------------------*/
/* Local function : _pmFindSfield() */
/* ---------------------------------*/
/* Find a 'S' field to match the id in pSrc and save its */
/* starting('S') and ending pointer('/') to each parameters */
/* pSrc should be a complete PM REPLY format */
/* that start with 'S' and terminate at '0x0D 0x0A' */
/* function return length of found 'S' fields  */
/* or (-1) to indicate no matching field exist */
static ESCPR_BYTE2  _pmFindSfield(ESCPR_UBYTE1 id, ESCPR_UBYTE1* pSrc,
                                  ESCPR_UBYTE1** pStart, ESCPR_UBYTE1** pEnd)
{

    while (*pSrc != 0xD || *(pSrc+1) != 0xA) {

        *pStart = NULL;
        *pEnd   = NULL;

        /* find 'S' */
        while(*pSrc == 'S') {
            if(id == *(pSrc+1)) {
                *pStart = pSrc;
            }

            pSrc += 2;

            while(*pSrc == 'T') {
                pSrc += 4;
            }

            /* Found id */
            if(*pStart != NULL) {
                *pEnd = pSrc;
                return (*pEnd - *pStart)+1;
            }

            /* next 'S' */
            pSrc++;
        }
    }

    return (-1);

}


/* ---------------------------------- */
/* Local function : _pmAppendTfield() */
/* ---------------------------------- */
/* Append 'T' field to pDes if same field dose not exsit */
/* but same one aleady exsits just combine mode properdy */
/* pDes should have a complete 'S' field consist of 'S' and '/' */
/* and pT should have a 'T' field of 4 bytes starts with 'T' */
/* function return increased bytes so that caller change the last pos */
/* or (-1) to indicate nothing changed */
static ESCPR_BYTE2 _pmAppendTfield(ESCPR_UBYTE1* pT, ESCPR_UBYTE1* pDes)
{
    ESCPR_UBYTE1  t_id = *(pT+1);
    ESCPR_BYTE2 t_exist = 0;

    if(*pDes == 'S') {

        pDes += 2; /* move to first 'T' */

        while(*pDes == 'T') {

            /* same id exist */
            if(t_id == *(pDes+1)) {
                /* Just combine mode property */
                *(pDes+2) |= *(pT+2);

                t_exist = 1;
                break;
            }

            /* next 'T' */
            pDes += 4;
        }

        /* samd id field dose not exist */
        /* Append new 'T' fields */
        if(t_exist == 0) {
            ESCPR_Mem_Copy(pDes, pT, 4);
            pDes += 4;
            *pDes = '/';

            return 4; /* size of 'T' field */
        }

        /* type id aleady exist then do not anything */
    }

    return (-1);

}


/* -------------------------------- */
/* Local function : _pmScanTfield() */
/* -------------------------------- */
/* find a 'T' field to match the id */
/* function return the first pos of matched 'T' at the pSfield or NULL */
static ESCPR_UBYTE1 * _pmScanTfield(ESCPR_UBYTE1 id, ESCPR_UBYTE1* pSfield)
{
    ESCPR_UBYTE1* pScan = pSfield;
    ESCPR_UBYTE1* pT = NULL;

    if(*pScan == 'S') {
        pScan += 2;

        while(*pScan == 'T') {
            if(id == *(pScan+1)) {
                pT = pScan;
                break;
            }

            pScan += 4;
        }
    }

    return pT;

}


/* ------------------------------------------------ */
/* Lcoal function : _pmValidateRemoveUnknownSfield() */
/* ------------------------------------------------ */
/* Copy valid fields to reply buffer only. */
/* Remove unknown 'S' field */
/* Minimum conditons for valid PM REPLY */
/* - it must have a complete 'S' field more than one ( 'S' ~ '/') */
/* - it must end with 0xD and 0xA */
/* function returns the number of valid fields */
static ESCPR_BYTE2 _pmValidateRemoveUnknownSfield(ESCPR_UBYTE1* pDes, ESCPR_UBYTE1* pSrc)
{

    ESCPR_UBYTE1* pPrev = NULL; /* save previous pointer */
    ESCPR_UBYTE1* pS    = NULL; /* valid field's starting position */
    ESCPR_UBYTE1* pE    = NULL; /* valid field's ending postion */

    ESCPR_BYTE2 valid = 0; /* flag for indicating 'S' field's validation */
    ESCPR_BYTE2 t_cnt = 0; /* count valid 'T' fields */
    ESCPR_BYTE2 s_idx = 0; /* index of PM_PAPER_SIZE */

    ESCPR_BYTE2 num_valid_fields = 0; /* value for returning */

#ifdef _TEST_PM_STEP_1 /* Change first 'S' field's id to unknown id such as 0xFF */
    *(pSrc+1) = 0xFF;
#endif

    while (*pSrc != 0xD || *(pSrc+1) != 0xA) {
        pPrev = pSrc;

        pS = NULL;
        pE = NULL;
        valid = 0;
        t_cnt = 0;
        s_idx = 0;


        if(*pSrc == 'S') {
            pS = pSrc;
            pSrc += 2;

            while(*pSrc == 'T') {
                pSrc += 3;

                if(*pSrc == '/') {
                    pSrc++;
                    t_cnt++;
                }
            }

            if(t_cnt && *pSrc == '/') {
                pE = pSrc;
            }

        }

        /* Copy valid and support 'S' fields only */
        /* Valid means size id should be find in its table */
        /* and 'T' field exist at least more than one */
        /* Unknown 'S' field should be removed */
        if(pS && pE) {
            for(s_idx = 0; s_idx < NUM_PAPER_SIZE; s_idx++) {
                if(PM_PAPER_SIZE[s_idx].id == *(pS+1)) {
                    ESCPR_Mem_Copy(pDes, pS, (pE-pS)+1);
                    pDes += (pE-pS)+1;
                    valid = 1;

                    /* now increase num of valid fields */
                    num_valid_fields++;

                    break;
                }
            }
        }

        /* Restore work buffer pos to the previous */
        /* cause fail to get a valid fields */
        if(valid == 0) {
            pSrc = pPrev;
        }

        pSrc++;
    }

    *pDes++ = *pSrc++;   /* 0xD */
    *pDes++ = *pSrc;     /* 0xA */

    return num_valid_fields;

}


/* ----------------------------------------- */
/* Local function : _pmCorrectUnknownTfield() */
/* ----------------------------------------- */
/* Change a unknown 'T' field to PGPP's in case of */
/* PGPP dose not exist its 'S' field */
/* if aleady PGPP exist delete it */
static void _pmCorrectUnknownTfield(ESCPR_UBYTE1* pDes, ESCPR_UBYTE1* pSrc)
{

    static const ESCPR_UBYTE1 PGPP_FIELD [ ] = { 0x54, 0x0B, 0x87, 0x2F };

    ESCPR_BYTE2 PGPP   = 0; /* Premium Glossy Photo Paper (type id : 0x0b) */
    ESCPR_BYTE2 t_idx  = 0; /* Index of table defined Support 'T' id table */
    ESCPR_UBYTE1 *  pScan = NULL; /* word pointer for scanning id */

#ifdef _TEST_PM_STEP_2 /* Change 'T' field's id to unknown id such as 0xFF */
    *(pSrc+3) = 0xFF;
#endif

    while (*pSrc != 0xD || *(pSrc+1) != 0xA) {
        /* reset PGPP flag each new 'S' field */
        PGPP = 0;

        if(*pSrc == 'S') {
            /* Scan PGPP in current 'S' field */
            pScan = pSrc;

            if(_pmScanTfield(PM_MTID_PGPHOTO, pScan) != NULL) {
                PGPP = 1;
            }

            *pDes++ = *pSrc++;
            *pDes++ = *pSrc++;

            while(*pSrc == 'T') {
                /* Copy support 'T' field */
                for(t_idx = 0; t_idx < NUM_TYPE_IDS; t_idx++) {
                    if(PM_TYPE_IDS[t_idx] == *(pSrc+1)) {
                        ESCPR_Mem_Copy(pDes, pSrc, 4);
                        pDes += 4;
                        break;
                    }
                }

                /* Unknown type id encountered */
                /* if PGPP did not exist in 'S' field */
                /* then append PGPP fields to pDes */
                if(t_idx == NUM_TYPE_IDS && PGPP == 0) {
                    ESCPR_Mem_Copy(pDes, PGPP_FIELD, 4);
                    pDes += 4;

                    PGPP = 1;
                }

                /* move to next 'T' */
                pSrc += 4;
            }

            /* copy '/' and move next 'S' */
            *pDes++ = *pSrc++;
        }
    }

    *pDes++ = *pSrc++;   /* 0xD */
    *pDes++ = *pSrc;     /* 0xA */

}


/* --------------------------------------------- */
/* Local function : _pmCorrectDupulicatedFields() */
/* --------------------------------------------- */
/* Merge duplicated fields */
static void _pmCorrectDupulicatedFields(ESCPR_UBYTE1* pDes, ESCPR_UBYTE1* pSrc)
{
/* MERGED_FIELD only meaning in this function */
#define MERGED_FIELD    0xFF
/* memcpy macro for readability */
#define COPY_BYTES(des,src,size)    ESCPR_Mem_Copy(des,src,size); des+=size;

    ESCPR_UBYTE1 merged_buf[PM_MAX_SIZE];

    ESCPR_UBYTE1* pFieldS = NULL; /* current 'S' in merged buffer */
    ESCPR_UBYTE1* pFieldT = NULL; /* work pontter to merge a 'T' */
    ESCPR_UBYTE1* pS      = NULL; /* duplicated field's starting position */
    ESCPR_UBYTE1* pE      = NULL; /* duplicated field's ending postion */
    ESCPR_UBYTE1* pM      = NULL; /* pos of merged buffer */
    ESCPR_UBYTE1* pScan   = NULL; /* work pointer to find a field */
    ESCPR_UBYTE1  s_id    = (-1); /* current 'S' id */
    ESCPR_BYTE2 bytes;

#ifdef _TEST_PM_STEP_3
    *(pSrc+8) = 0x0F; /* make duplicate 'S' */
#endif

    pM = &merged_buf[0];

    /* Aleady merged fields no need to copy again */
    while (*pSrc != 0xD || *(pSrc+1) != 0xA) {
        pFieldS = NULL;


        if(*pSrc == 'S') {
            VERBOSE_DUMP_PMREPLY((pSrc, DUMP_S_TAG_ONLY, "< STEP 3 : SOURCE 'S' ... >"));

            /* save current 'S' id */
            s_id = *(pSrc+1);

            if(s_id != MERGED_FIELD) {
                /* Current 'S' field's starting pos */
                /* it is used to merge fields later */
                pFieldS = pM;

                COPY_BYTES(pM, pSrc, 2);
            }

            pSrc += 2; /* move to first 'T' */

            /* Merge 'T' fields */
            while(*pSrc == 'T') {

		if(pFieldS && s_id != MERGED_FIELD) {
		    /* if 'T' aleady exist just combine its property by BIT OR operation */
		    if((pFieldT = _pmScanTfield(*(pSrc+1), pFieldS)) != NULL) {
			*(pFieldT+2) |= *(pSrc+2);
		    }

		    /* Copy only new 'T' field */
		    if(pFieldT == NULL) {
			COPY_BYTES(pM, pSrc, 4);
		    }
		}

                pSrc += 4; /* next 'T' */
            }
        }

        if(s_id != MERGED_FIELD) {
            COPY_BYTES(pM, pSrc, 1);
        }
        pSrc++;

        /* aleady merged field just go on next */
        if(s_id == MERGED_FIELD)  {
            continue;
        }

        /*----------------------------------------------------*/
        /* Find dupulicated 'S' being followed and merge them */

        pScan = pSrc; /* do not change pSrc in following loop */

        while(_pmFindSfield(s_id, pScan, &pS, &pE) > 0) {

            /* Change source's 'S' id to MERGED_FIELD */
            *(pS+1) = MERGED_FIELD;
            pS += 2;

            /* merge dupulicated 'T' */
            while(*pS == 'T') {

                /* Append NEW 'T' field to the current 'S' field */
                /* aleady same 'T' exist only its mode property will be combined */
                /* after called function */
		if(pFieldS) {
		    if((bytes = _pmAppendTfield(pS, pFieldS)) > 0) {

			/* update merged_buf's the last pos that pM point it */
			pM += bytes; /* MUST 4 BYTES(size of 'T' field) ! */
		    }
		}

                pS += 4; /* next 'T' */
            }

            /* find next 'S' */
            pScan = (pE+1);

            VERBOSE_DUMP_PMREPLY((pFieldS, DUMP_S_TAG_ONLY, "< STEP 3 : MERGE PROCESSING ... >"));
        }
    }

    /* 0x0D & 0x0A */
    COPY_BYTES(pM, pSrc, 2);

    /*----------------------------------*/
    /* Copy the merged PM REPLY to pDes */

    pM = &merged_buf[0];

    while (*pM != 0xD || *(pM+1) != 0xA) {
        *pDes++ = *pM++;
    }

    *pDes++ = *pM++; /* 0xD */
    *pDes++ = *pM;   /* 0xA */

}


/* --------------------------------------------- */
/* Local function : _pmAdjustQuality()            */
/* --------------------------------------------- */
/* Adjust quality properties to the formal */
/* example : quality has only draft mode -> turn on normal mode */
static void _pmAdjustQuality(ESCPR_UBYTE1* pData)
{
    ESCPR_UBYTE1* p = pData;

    /* skip pm heder */
    p += PM_HEADER_LEN;

    verbose_dbprint(("< STEP 4 :  Adjust quality >\n"));

    /* adjuct each quality properties */
    while(!(*p == 0x0D && *(p+1) == 0x0A)) {
        while(*p == 'S') {

            verbose_dbprint(("%c %02d\n", *p, *(p+1)));

            p += 2; /* move to the first T field */

            while(*p == 'T') {

                verbose_dbprint(("\t%c %02d 0x%02x %c -> ", *p, *(p+1), *(p+2), *(p+3)));

                p += 2; /* move to quality pos */

                /* Quality property */
                switch(*p & 0x07) {
                    /* Should be handled following case 1 bit of Draft turned on only */
                    case 0x01: /* 0 0 1 -> 0 1 1 */
                        *p |= (1<<1); /* turn normal on */
                        break;
                    default:
                        break;
                }

                verbose_dbprint(("%c %02d 0x%02x %c\n", *(p-2), *(p-1), *(p), *(p+1)));

                p += 2; /* move to the next T field */
            }

            p += 1; /* move to the next S field */
        }
    }

}


/*=======================================================================================*/
/* ESC/P-R Service Pack ChangeSpec Known Issues (only used by ESC/P-R Core module)           */
/*=======================================================================================*/

/* Known Issue : 2005/10/14 #1 */
/*=======================================================================================
 SYMPTOMS
 CAUSE
 RESOLUTION
  - Invalid formats       : Delete
  - Unknown 'S' field     : Delete
  - Unknown 'T' field     : Replace to PGPP-Premium Glossy Photo Paper(id:0x0b) - field
                            If PGPP aleady exist its 'S' field
                            then just combine the mode property
  - Duplicated 'S' fields : Merge together
  - Duplicated 'T' fields : Merge together and combine each mode properties
  - Only DRAFT mode exist : Add NORMAL mode to its print quality property

  NOTE  : Be sure that the pData is a pointers that a starting address of 512 bytes buffer
          should be assigned or memory acces violation should be occured.

 Last Modification : 2005/10/14
 Revision : 1.0
=======================================================================================*/
static ESCPR_BYTE4 _SP_ChangeSpec_UpdatePMReply(void* pData, void* pReserved1, void* pReserved2, void* pReserved3)
{
    ESCPR_UBYTE1* pBefore = (ESCPR_UBYTE1*)pData;
    ESCPR_UBYTE1* pAfter  = NULL;
    ESCPR_UBYTE1* pSrc    = NULL;
    ESCPR_UBYTE1* pDes    = NULL;

    static const ESCPR_UBYTE1 PM_REPLY_HEADER[PM_HEADER_LEN] = {
      /*  @     B     D     C   <SP>    P     M   <CR>  <LF> */
        0x40, 0x42, 0x44, 0x43, 0x20, 0x50, 0x4D, 0x0D, 0x0A
    };


    ESCPR_BYTE2 i;

    dbprint(("[[ ESC/P-R CHANGE SPEC : Known issue (2005/10/14 #1) ]]\n"));

    /* Check parameters */
    if(pBefore == NULL) {
        dbprint(("_SP_ChangeSpec_UpdatePMReply > ESCPR_SP_ERR_PM_INVALID_POINTER\n"));
        return (ESCPR_SP_ERR_PM_INVALID_POINTER);
    }

    if(ESCPR_Mem_Compare(pBefore, PM_REPLY_HEADER, PM_HEADER_LEN) != 0) {
        dbprint(("_SP_ChangeSpec_UpdatePMReply > ESCPR_SP_ERR_PM_INVALID_HEADER\n"));
        return (ESCPR_SP_ERR_PM_INVALID_HEADER);
    }

    for(i = PM_HEADER_LEN; i <= (PM_MAX_SIZE-PM_TERMINATOR_LEN); i++) {
        if(pBefore[i]== 0x0D && pBefore[i+1] == 0x0A) {
            break;
        }
    }

    if(i > (PM_MAX_SIZE-PM_TERMINATOR_LEN)) {
        dbprint(("_SP_ChangeSpec_UpdatePMReply > ESCPR_SP_ERR_PM_INVALID_TERMINATOR\n"));
        return (ESCPR_SP_ERR_PM_INVALID_TERMINATOR);
    }

    /* Initialize g_PMinfo */
    ESCPR_Mem_Set(&g_PMinfo.data[0], 0x00, PM_MAX_SIZE);
    ESCPR_Mem_Copy(&g_PMinfo.data[0], PM_REPLY_HEADER, PM_HEADER_LEN);
    g_PMinfo.state = PM_STATE_NOT_FILTERED;

    /* Use work pointers to call each filter functions */
    pBefore = (ESCPR_UBYTE1*)pData;
    pAfter  = &g_PMinfo.data[PM_HEADER_LEN];

    pSrc = pBefore;
    pDes = pAfter;

    /* Correct PM REPLY following 4 steps */

    /* STEP 1 : Copy only valid fields to reply buffer and remove unknown 'S' from reply */
    pSrc += PM_HEADER_LEN; /* the position of first 'S' field */

    DUMP_PMREPLY((pSrc, DUMP_HEX, "< ORIGINAL >"));

    if(_pmValidateRemoveUnknownSfield(pDes, pSrc) == 0) {
        dbprint(("_SP_ChangeSpec_UpdatePMReply > ESCPR_SP_ERR_PM_NO_VALID_FIELD\n"));
        return (ESCPR_SP_ERR_PM_NO_VALID_FIELD);
    }

    VERBOSE_DUMP_PMREPLY((pDes, DUMP_ASCII, "< STEP 1 PASSED >"));

    /* STEP 2 : Correct unknown 'T' fields */
    ESCPR_Mem_Set(pBefore, 0x00, PM_MAX_SIZE);
    ESCPR_Mem_Copy(pBefore, pDes, PM_MAX_SIZE);

    pSrc = pBefore;
    pDes = pAfter;

    _pmCorrectUnknownTfield(pDes, pSrc);

    VERBOSE_DUMP_PMREPLY((pDes, DUMP_ASCII, "< STEP 2 PASSED >"));

    /* STEP 3 : Merge duplicated fields */
    ESCPR_Mem_Set(pBefore, 0x00, PM_MAX_SIZE);
    ESCPR_Mem_Copy(pBefore, pDes, PM_MAX_SIZE);

    pSrc = pBefore;
    pDes = pAfter;

    _pmCorrectDupulicatedFields(pDes, pSrc);

    VERBOSE_DUMP_PMREPLY((pDes, DUMP_ASCII, "< STEP 3 PASSED >"));

    /* Now, Service Pack retains filtered data its original quality properties */
    /* within the inner buffer g_PMinfo.data */
    /* This data would be referenced whenever it is required to compare its originality */
    DUMP_PMREPLY((&g_PMinfo.data[PM_HEADER_LEN], DUMP_ASCII, \
                  "< FILTERED (Retained within SP-same printer's caps) >"));
    g_PMinfo.state = PM_STATE_FILTERED;

    /* STEP 4 : Adjust quality properties to the formal in order to return to the driver */
    /* it dose not change the filtered data through previous steps retained within Service Pack */
    /* but just change the buffer asigned as parameter (in this case pData) */
    /* after duplicating the filtered data to it */
    /* See releated Known Issue : 2005/10/14 #4 */
    ESCPR_Mem_Copy(pData, &g_PMinfo.data[0], PM_MAX_SIZE);
    _pmAdjustQuality((ESCPR_UBYTE1*)pData);
    DUMP_PMREPLY(((ESCPR_UBYTE1*)pData+PM_HEADER_LEN, DUMP_ASCII, \
                  "< FILTERED (Returned data to the driver-adjusted quality properties) >"));

    return ESCPR_SP_ERR_NONE;
}


/* Known Issue : 2005/10/14 #2 */
/*=======================================================================================
 SYMPTOMS
 CAUSE
 RESOLUTION
 Last Modification : 2005/10/14
 Revision : 1.0
=======================================================================================*/
static ESCPR_BYTE4 _SP_ChangeSpec_600_720DPI(void* pData, void* pReserved1, void* pReserved2, void* pReserved3)
{
    ESCPR_PRINT_JOB* pPrintJob = (ESCPR_PRINT_JOB*)pData;

    dbprint(("[[ ESC/P-R CHANGE SPEC : Known issue (2005/10/14 #2) ]]\n"));

    verbose_dbprint(("pPrintJob->PaperWidth           = %d\n", pPrintJob->PaperWidth));
    verbose_dbprint(("pPrintJob->PaperLength          = %d\n", pPrintJob->PaperLength));
    verbose_dbprint(("pPrintJob->TopMargin            = %d\n", pPrintJob->TopMargin));
    verbose_dbprint(("pPrintJob->LeftMargin           = %d\n", pPrintJob->LeftMargin));
    verbose_dbprint(("pPrintJob->PrintableAreaWidth   = %d\n", pPrintJob->PrintableAreaWidth));
    verbose_dbprint(("pPrintJob->PrintableAreaLength  = %d\n", pPrintJob->PrintableAreaLength));
    verbose_dbprint(("pPrintJob->InResolution         = %d\n", pPrintJob->InResolution));
    verbose_dbprint(("pPrintJob->PrintDirection       = %d\n", pPrintJob->PrintDirection));

    /* Support Input Resolution 360x360dpi/300x300dpi only */
    if(pPrintJob->InResolution == ESCPR_IR_7272 || pPrintJob->InResolution == ESCPR_IR_6060){
    pPrintJob->TopMargin           /= 2;
    pPrintJob->LeftMargin          /= 2;
    pPrintJob->PrintableAreaWidth  /= 2;
    pPrintJob->PrintableAreaLength /= 2;
    if(pPrintJob->InResolution == ESCPR_IR_7272){
	pPrintJob->InResolution         = ESCPR_IR_3636;
    }else{
       pPrintJob->InResolution         = ESCPR_IR_3030;
     }

    verbose_dbprint((">> ESC/P-R input resolution has been changed to 360x360 from 720x720 dpi\n"));
    verbose_dbprint(("pPrintJob->PaperWidth           = %d\n", pPrintJob->PaperWidth));
    verbose_dbprint(("pPrintJob->PaperLength          = %d\n", pPrintJob->PaperLength));
    verbose_dbprint(("pPrintJob->TopMargin            = %d\n", pPrintJob->TopMargin));
    verbose_dbprint(("pPrintJob->LeftMargin           = %d\n", pPrintJob->LeftMargin));
    verbose_dbprint(("pPrintJob->PrintableAreaWidth   = %d\n", pPrintJob->PrintableAreaWidth));
    verbose_dbprint(("pPrintJob->PrintableAreaLength  = %d\n", pPrintJob->PrintableAreaLength));
    verbose_dbprint(("pPrintJob->InResolution         = %d\n", pPrintJob->InResolution));
    verbose_dbprint(("pPrintJob->PrintDirection       = %d\n", pPrintJob->PrintDirection));
    }

    return ESCPR_SP_ERR_NONE;
}


/* Known Issue : 2005/10/14 #3 */
/*=======================================================================================
 SYMPTOMS
 CAUSE
 RESOLUTION
 Last Modification : 2005/10/14
 Revision : 1.0
=======================================================================================*/
static ESCPR_BYTE4 _SP_ChangeSpec_CompositeBlack(void* pData, void* pReserved1, void* pReserved2, void* pReserved3)
{
    ESCPR_PRINT_QUALITY* pPrintQuality = (ESCPR_PRINT_QUALITY*)pData;

    dbprint(("[[ ESC/P-R CHANGE SPEC : Known issue (2005/10/14 #3) ]]\n"));

    verbose_dbprint((">> pPrintQuality->ColorMono : %d -> ", pPrintQuality->ColorMono));

    /* Use the composite black for the monochrome printing */
    if(pPrintQuality->ColorMono == ESCPR_CM_MONOCHROME){
        pPrintQuality->ColorMono = ESCPR_CM_COLOR;
    }

    verbose_dbprint(("%d\n", pPrintQuality->ColorMono));

    return ESCPR_SP_ERR_NONE;
}


/* Known Issue : 2005/10/14 #4 */
/*=======================================================================================
 SYMPTOMS
 CAUSE
 RESOLUTION
 Last Modification : 2005/10/14
 Revision : 1.0
=======================================================================================*/
static ESCPR_BYTE4 _SP_ChangeSpec_DraftOnly(void* pPrintQuality, void* pPrintJob,
                                        void* pReserved1, void* pReserved2)
{

#define Q_DRAFT  0
#define Q_NORMAL 1
#define Q_HIGH   2

    ESCPR_PRINT_QUALITY* pQuality = (ESCPR_PRINT_QUALITY*)pPrintQuality;
    ESCPR_PRINT_JOB*     pJob     = (ESCPR_PRINT_JOB*)pPrintJob;

    ESCPR_BYTE4   width, length;
    ESCPR_UBYTE1  s_id     = -1;
    ESCPR_UBYTE1  t_id     = -1;
    ESCPR_UBYTE1* pPMinfo  = NULL;
    ESCPR_UBYTE1* pS_begin = NULL;
    ESCPR_UBYTE1* pS_end   = NULL;
    ESCPR_UBYTE1* pTfield  = NULL;

    ESCPR_BYTE2   i;
    ESCPR_BYTE1   quality[3]; /* Q_DRAFT / Q_NORMAL / Q_HIGH */

    dbprint(("[[ ESC/P-R CHANGE SPEC : Known issue (2005/10/14 #4) ]]\n"));

    if(g_PMinfo.state != PM_STATE_FILTERED) {
        dbprint(("ChangeSpec_DraftOnly : PM info not initialized\n"));
        /* it is not able to hadle this situation so do nothing */
        return ESCPR_SP_ERR_NONE;
    }

    if(!(pQuality->PrintQuality == ESCPR_PQ_DRAFT ||
        pQuality->PrintQuality == ESCPR_PQ_NORMAL ||
        pQuality->PrintQuality == ESCPR_PQ_HIGH)) {
        dbprint(("ChangeSpec_DraftOnly : invalid value for PrintQuality (%d)\n", pQuality->PrintQuality));
        /* it is not able to hadle this situation so do nothing */
        return ESCPR_SP_ERR_NONE;
    }

    /* find S id with paper size */
    width  = pJob->PaperWidth;
    length = pJob->PaperLength;

    /* use 360/300 dpi unit */
    if(pJob->InResolution == ESCPR_IR_7272 || pJob->InResolution == ESCPR_IR_6060) {
        width  /= 2;
        length /= 2;
    }

    for(i = 0; i < NUM_PAPER_SIZE; i++) {
        if((PM_PAPER_SIZE[i].width360 == width || PM_PAPER_SIZE[i].width300 == width)
        && (PM_PAPER_SIZE[i].length360 == length || PM_PAPER_SIZE[i].length300 == length)) {
            s_id = PM_PAPER_SIZE[i].id;
        }
    }
    verbose_dbprint(("ChangeSpec_DrfatOnly : size id(%d), width(%d), length(%d)\n", s_id, width, length));

    /* Refer the data retained within Service Pack */
    pPMinfo = &g_PMinfo.data[PM_HEADER_LEN];

    /* S field start postion */
    if(_pmFindSfield(s_id, pPMinfo, &pS_begin, &pS_end) < 0) {
        dbprint(("ChangeSpec_DraftOnly : cannot find S_id(%d)\n", s_id));
        /* it is not able to hadle this situation so do nothing */
        return ESCPR_SP_ERR_NONE;
    };

    VERBOSE_DUMP_PMREPLY((pS_begin, DUMP_S_TAG_ONLY,
            "< ChangeSpec_DraftOnly : retained S field info >"));

    /* Fetch the T field */
    t_id = pQuality->MediaTypeID;

    if((pTfield = _pmScanTfield(t_id, pS_begin)) == NULL) {
        dbprint(("ChangeSpec_DraftOnly : cannot find T_id(%d)\n", t_id));
        /* it is not able to hadle this situation so do nothing */
        return ESCPR_SP_ERR_NONE;
    }

    /* Quality should be assigned to the only supported mode */
    verbose_dbprint((" >> adjusted PrintQuality : %d -> ", pQuality->PrintQuality));

    if(!((*(pTfield+2) & 0x07) &   /* Printer's support mode actually */
        (1<<pQuality->PrintQuality))) { /* Upper layer(driver) assigned mode */

        /* The quality mode which is not supported by printer is assigned */
        /* Replace it to printer's support mode */
        switch(*(pTfield+2) & 0x07) {
            case 0x01: /* 0 0 1 : Draft  only */
                quality[Q_DRAFT]  = ESCPR_PQ_DRAFT;
                quality[Q_NORMAL] = ESCPR_PQ_DRAFT;
                quality[Q_HIGH]   = ESCPR_PQ_DRAFT;
                break;
            case 0x02: /* 0 1 0 : Normal only */
                quality[Q_DRAFT]  = ESCPR_PQ_NORMAL;
                quality[Q_NORMAL] = ESCPR_PQ_NORMAL;
                quality[Q_HIGH]   = ESCPR_PQ_NORMAL;
                break;
            case 0x04: /* 1 0 0 : High   only */
                quality[Q_DRAFT]  = ESCPR_PQ_HIGH;
                quality[Q_NORMAL] = ESCPR_PQ_HIGH;
                quality[Q_HIGH]   = ESCPR_PQ_HIGH;
                break;
            case 0x03: /* 0 1 1 : Normal and Draft   */
                quality[Q_DRAFT]  = ESCPR_PQ_DRAFT;
                quality[Q_NORMAL] = ESCPR_PQ_NORMAL;
                quality[Q_HIGH]   = ESCPR_PQ_NORMAL;
                break;
            case 0x05: /* 1 0 1 : High   and Draft   */
                quality[Q_DRAFT]  = ESCPR_PQ_DRAFT;
                quality[Q_NORMAL] = ESCPR_PQ_HIGH;
                quality[Q_HIGH]   = ESCPR_PQ_HIGH;
                break;
            case 0x06: /* 1 1 0 : High   and Normal  */
                quality[Q_DRAFT]  = ESCPR_PQ_NORMAL;
                quality[Q_NORMAL] = ESCPR_PQ_NORMAL;
                quality[Q_HIGH]   = ESCPR_PQ_HIGH;
                break;
            case 0x07: /* 1 1 1 : Anything possible  */
                break;
            default:
                break;
        }

        /* Now, the value of quality array of index which is same as PrintQuality is valid */
        pQuality->PrintQuality = quality[pQuality->PrintQuality];
    }

    verbose_dbprint(("%d\n", pQuality->PrintQuality));

    return ESCPR_SP_ERR_NONE;
}


/*=======================================================================================*/
/* ESC/P-R Service Pack Module API (only used by ESC/P-R Core module)                    */
/*=======================================================================================*/
ESCPR_ERR_CODE ESCPR_RequestServicePack(SP_REQ_CODE request,
                                        void* pData1st, void* pData2nd, void* pData3th, void* pData4th)
{
    ESCPR_BYTE4 Ret = ESCPR_SP_ERR_NONE;

    /* Validation request code */
    switch(request) {
        case SP_REQ_FIX_UPDATE_PMREPLY:
        case SP_REQ_FIX_600_720DPI:
        case SP_REQ_FIX_COMPOSITE_BLACK:
        case SP_REQ_FIX_DRAFTONLY:
            break;
        default:
            return ESCPR_SP_ERR_INVALID_REQUEST;
    }

    /* Call ChangeSpec function */
    Ret = (*fpChangeSpec[request])(pData1st, pData2nd, pData3th, pData4th);

    return Ret;
}


/* Enf of File */
