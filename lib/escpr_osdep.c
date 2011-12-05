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
 * File Name:   escpr_osdep.c
 *
 ***********************************************************************/

#include <malloc.h>
#include <string.h>
#include <time.h>

#include "escpr_osdep.h"

void ESCPR_Mem_Set(void *Dest, ESCPR_BYTE4 c, size_t Size)
{
    memset(Dest, c, Size);
}

void ESCPR_Mem_Copy(void *Dest, const void *Src, size_t Size)
{
    memcpy(Dest, Src, Size);
}

void* ESCPR_Mem_Alloc(size_t MemSize)
{
    void *pdummy;
    
    if(MemSize==0)
        return NULL;
    else {
        pdummy = malloc( MemSize );
        if( pdummy == NULL )
            return pdummy;
        ESCPR_Mem_Set( pdummy, 0, MemSize);
        return pdummy;
    }
}

void ESCPR_Mem_Free(void *MemPtr)
{
    free(MemPtr);
}

BOOL ESCPR_Mem_Compare(const void *s1, const void *s2, size_t n) {
    return memcmp(s1,s2,n);
}

void ESCPR_GetLocalTime(ESCPR_LOCAL_TIME *epsTime)
{
    time_t now;
    struct tm *t;

    now = time(NULL);
    t = localtime(&now);

    epsTime->year = (ESCPR_UBYTE2)t->tm_year;
    epsTime->mon = (ESCPR_UBYTE1)t->tm_mon;
    epsTime->day = (ESCPR_UBYTE1)t->tm_mday;
    epsTime->hour = (ESCPR_UBYTE1)t->tm_hour;
    epsTime->min = (ESCPR_UBYTE1)t->tm_min;
    epsTime->sec = (ESCPR_UBYTE1)t->tm_sec;
}
