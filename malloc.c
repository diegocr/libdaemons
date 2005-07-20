/*
 * This file is part of the Daemons IFace Library.
 * 
 * Copyright (c) 2005, Diego Casorran <diegocr at users dot sf dot net>
 * All rights reserved.
 * 
 * 
 * Redistribution  and  use  in  source  and  binary  forms,  with  or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * -  Redistributions  of  source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this  list  of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * -  Neither  the name of the author(s) nor the names of its contributors may
 * be  used  to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND  ANY  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE  DISCLAIMED.   IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
 * CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO,  PROCUREMENT OF
 * SUBSTITUTE  GOODS  OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION)  HOWEVER  CAUSED  AND  ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT,  STRICT  LIABILITY,  OR  TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/** $Id: malloc.c,v 0.1 2005/05/22 12:47:22 diegocr Exp $
 **/


#include <proto/exec.h>
#include <stabs.h>

STATIC struct SignalSemaphore
	__malloc_semaphore_stack, 
	*__malloc_semaphore=NULL;

STATIC APTR __malloc_mempool =  NULL;

extern ULONG 
	__malloc_memflags, 
	__malloc_minsize,
	__malloc_mempool_puddleSize, 
	__malloc_mempool_threshSize;


APTR malloc( ULONG length )
{
	LONG size = length;
	ULONG *mem;
	
	if(__malloc_mempool == NULL)
	{
		if(__malloc_mempool_threshSize > __malloc_mempool_puddleSize) {
			// Alert ?
			return NULL;
		}
		if(!(__malloc_mempool = 
			CreatePool( __malloc_memflags, 
				__malloc_mempool_puddleSize, 
					__malloc_mempool_threshSize )))
		return NULL;
		
		InitSemaphore(&__malloc_semaphore_stack);
		__malloc_semaphore = &__malloc_semaphore_stack;
	}
	
	if( size <= 0)
		return NULL;
	
	if(size < (LONG)__malloc_minsize)
		size = __malloc_minsize;
	
	size += sizeof(ULONG) + MEM_BLOCKMASK;
	size &= ~MEM_BLOCKMASK;
	
	ObtainSemaphore(__malloc_semaphore);
	if((mem = AllocPooled( __malloc_mempool, size)))
		*mem++=size;
	
	ReleaseSemaphore(__malloc_semaphore);
	
	return mem;
}

APTR calloc( ULONG num, ULONG size)
{
	if(!num || !size)
		return NULL;
	
	size *= num;
	
	return malloc( size );
}

VOID free( APTR mem )
{
	ULONG *omem=mem;
	
	if(omem)
	{
		ULONG size = *(--omem);
		
		
		ObtainSemaphore(__malloc_semaphore);
		
		if(size) {
			FreePooled( __malloc_mempool, omem, size);
	//		*(((ULONG *)mem)-1) = 0;
		}
		
		ReleaseSemaphore(__malloc_semaphore);
	}
}
APTR realloc( APTR old, ULONG size )
{
	LONG nsize = size, osize, *o=old;
	APTR nmem;
	
	if(!old)
		return malloc( nsize );
	
	osize = (*(o-1)) - sizeof(ULONG);
	if (nsize <= osize)
		return old;
	
	if((nmem = malloc( nsize )))
	{
		ULONG *n = nmem;
		
		osize >>= 2;
		while(osize--)
			*n++ = *o++;
		
		free( old );
	}
	
	return nmem;
}



void __free_malloc_mempool( void )
{
	if(__malloc_mempool)
	{
		ObtainSemaphore(__malloc_semaphore);
		
		DeletePool( __malloc_mempool );
		__malloc_mempool = NULL;
		
		ReleaseSemaphore(__malloc_semaphore);
	}
}
ADD2EXIT(__free_malloc_mempool, -50);

