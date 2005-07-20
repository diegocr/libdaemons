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

/** $Id: daemons_bind.c,v 0.2 2005/06/24 07:59:44 diegocr Exp $
 **/


#include "daemons.h"
#include <sys/ioctl.h>
#include <stdlib.h> /* free */

static struct service *__bcfg = NULL;
extern short daemons_bindretard;

INLINE long Bind( unsigned short port )
{
	long sockfd, len;
	struct sockaddr_in server;
	long sattr = FD_ACCEPT | FD_ERROR;

	if ((sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
		return sockfd;

	memset (&server, 0, sizeof (server));
	server.sin_family      = AF_INET;
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	server.sin_port        = htons (port);

	/* set something non-zero */
	len = sizeof (server);

	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &len, sizeof (len));
	
	len = 1;
	if((bind (sockfd, (struct sockaddr *)&server, sizeof (struct sockaddr)) < 0)
	|| (setsockopt( sockfd, SOL_SOCKET, SO_EVENTMASK, &sattr, sizeof(long)) < 0)
	|| (IoctlSocket ( sockfd, FIOASYNC, (char*)&len) < 0))
	{
		CloseSocket(sockfd);
		return -1;
	}

	listen (sockfd, 5);
	
	return sockfd;
}


BOOL daemons_bind_setup(BOOL (*global_dispatcher)(struct client *c))
{
	int i;
	
#if 0 /* TODO */
	if(daemons_bindretard) {
		daemons_bindretard = FALSE;
		return TRUE;
	}
#endif
	
	if(!__services[0]) {
		notice("no services specified");
		
		return FALSE;
	}
	
	for( i = 0; __services[i]; i++)
	{
		long sfd;
		struct service *bcfg = NULL;
		
		if((sfd = Bind(__services[i])) == -1)
		{
			notice("Can't bind port %ld: %s", 
				__services[i], UL strerror(errno));
		
			//return FALSE;
			continue;
		}
		
		if(!(bcfg = AllocMem(sizeof(struct service), MEMF_PUBLIC|MEMF_CLEAR)))
		{
			CloseSocket( sfd );
			notice("Can't bind port %ld: %s", 
				__services[i], UL "out of memory");
			
			return FALSE;
		}
		
		InitSemaphore(&bcfg->semaforo);
		
		bcfg->next	 = __bcfg;
		bcfg->port	 = __services[i];
		bcfg->sockfd	 = sfd;
		bcfg->dispatcher = global_dispatcher;
		__bcfg		 = bcfg;
		
		deb("Listening on port %ld, sockfd = %ld, data = 0x%08lX", 
			bcfg->port, bcfg->sockfd, (long)bcfg);
	}
	
	return TRUE;
}

void daemons_bind_cleanup( void )
{
	struct service *bcfg, *next = NULL;
	
	for( bcfg = __bcfg;   bcfg;   bcfg = next)
	{
		ObtainSemaphore(&bcfg->semaforo);
		
		if(bcfg->sockfd != -1) {
			shutdown(bcfg->sockfd, SHUT_RDWR);
			CloseSocket( bcfg->sockfd );
		}
		next = bcfg->next;
		FreeMem( bcfg, sizeof(struct service));
	}
}

BOOL daemons_bind_setdispatcher( unsigned short port, BOOL (*dispatcher)(struct client *c))
{
	struct service *bcfg;
	
	for( bcfg = __bcfg;   bcfg;   bcfg = bcfg->next)
	{
		if(port == bcfg->port)
		{
			deb("setting dispatcher 0x%08lX on port %ld", 
				UL dispatcher, port);
			
			bcfg->dispatcher = dispatcher;
			
			return TRUE;
		}
	}
	
//	err("PORT %ld DOENST EXISTS ON MY LIST, I CAN'T SET YOUR DISPATCHER", (long) port);
	
	// exit(20); ?
	return FALSE;
}

struct service *daemons_bind_lookupfd(long sockfd)
{
	struct service *bcfg;
	
	for( bcfg = __bcfg;   bcfg;   bcfg = bcfg->next)
	{
		if(sockfd == bcfg->sockfd)
		{
			return bcfg;
		}
	}
	
	return NULL;
}
