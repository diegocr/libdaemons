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

/** $Id: daemons_loop.c,v 0.2 2005/06/24 07:59:45 diegocr Exp $
 **/

/*:ts=4 */
#include "daemons.h"
#include <string.h> /* strdup */

extern ULONG NetsigMask, daemons_signals, forkmask;
extern short daemons_resolve, daemons_usefork;

void daemons_signal_dispatcher( ULONG sig );
void daemons_forkerror( struct service *bcfg );
BOOL fork(struct service **awaking);
void fork_poll( void );

void daemons_dispatcher( void )
{
	long mysock; ULONG emask;
	
	deb("socket event...");
	
	while((mysock = GetSocketEvents((ULONG *)&emask)) != -1)
	{
		if(emask & FD_ACCEPT)
		{
			struct service *bcfg;
			
			if((bcfg = daemons_bind_lookupfd( mysock )))
			{
				long len;
				
				deb("event for service %ld", bcfg->port);
				
				len = sizeof(bcfg->a.sa);
				if((bcfg->asock = accept( mysock, (struct sockaddr *) &bcfg->a.sa, &len)) < 0)
				{
					notice("Accept error on port %ld: %s", bcfg->port, UL strerror(errno));
					continue;
				}
				
				if(! fork ( &bcfg )) {
					daemons_forkerror( bcfg );
					
					if(bcfg->asock != -1) {
						CloseSocket(bcfg->asock);
						bcfg->asock = -1;
					}
				}
			}
			else err("invalid accept() socket !?");
		}
		if(emask & FD_ERROR)
		{
			err("NetError On FD %ld: %s", mysock, UL strerror(errno));
		}
	}
}

void daemons_loop( void )
{
	ULONG WaitSig = 0;
	
	daemons_signals |= NetsigMask | SIGBREAKF_CTRL_C;
	
	deb("Waiting for Signals %08lx", (long) daemons_signals );
	
	do {
		WaitSig = Wait( WaitSig | daemons_signals );
		
		if(WaitSig & SIGBREAKF_CTRL_C)
		{
			deb("Interrupt Signal Received");	break;
		}
		else if(WaitSig & NetsigMask)
		{
			daemons_dispatcher ( );
		}
		else if(WaitSig & forkmask)
		{
			fork_poll ( );
		}
		else {
			daemons_signal_dispatcher( WaitSig );
		}
	} while(1);
}

