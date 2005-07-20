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

/** $Id: daemons_recvsend.c,v 0.2 2005/06/24 07:59:45 diegocr Exp $
 **/

#include "daemons.h"
#include <string.h>
#include <errno.h>

#define SocketBase	c->SocketBase
#define h_errno		c->h_errno
#define sockfd		c->sockfd
#define errno		c->errno

extern int daemons_recv_timeout;
extern int daemons_recvln_timeout;

STRPTR daemons_recvln( STRPTR *dest, ULONG destlen, struct client *c)
{
	register STRPTR b = (*dest);
	
	UBYTE recv_char;
	
	fd_set readfds;
	
	struct timeval timeout = { daemons_recvln_timeout, 0 };
	
	long select_val, rErr, maxlen = destlen;
	
	FD_ZERO( (APTR)&readfds );
	FD_SET(sockfd, &readfds );
	
	/* we should pass 'daemons_signals' to WaitSelect() ?.. */
	if((select_val = WaitSelect( sockfd + 1, &readfds, NULL,NULL, &timeout, NULL)) < 0)
	{
		notice("SELECT ERROR: %s", (ULONG) strerror(errno));
		return NULL;
	}
	
	if(!select_val)
	{
		errno = ETIMEDOUT;
		return NULL;
	}
	
	do {
		rErr = recv( sockfd, &recv_char, 1, 0);
		
		if(rErr <= 0) break;
		
		if(recv_char == '\r') continue;
		if(recv_char == '\n') break;
		
		*b++ = recv_char;
		
	} while( --maxlen );	*b = '\0';
	
	if(rErr <= 0)
	{
	//	notice("ERROR RECEIVING (%s)", (ULONG)strerror(errno));
		return NULL;
	}
	
	if(!maxlen)
	{
		notice("****** RECVLN BUFFER OVERFLOW **************");
		
		errno = ENOBUFS;
		return NULL;
	}
	
	deb((*dest));
	
	return (*dest);
}


LONG daemons_recv( STRPTR *dest, long destlen, struct client *c)
{
	fd_set rdfs;
	long sel, rlen=0;
	
	struct timeval timeout = { daemons_recv_timeout, 0 };
	
	FD_ZERO(&rdfs);
	FD_SET(sockfd, &rdfs);
	
	if( WaitSelect( sockfd+1, &rdfs, NULL, NULL, &timeout, NULL))
	{
		rlen = recv( sockfd, (*dest), destlen, 0 );
	}
	
	return ((rlen > 0) ? rlen :(0));
}


BOOL daemons_send( STRPTR string, long length, struct client *c)
{
	long sent;
	
	if(length <= 0)
		length = (long) strlen( string );
	
	sent = send( sockfd, string, length, 0);
	
	if(sent != length) {
	//	notice("FAILED sending data to sockfd %ld: %s", sockfd, (ULONG)strerror(errno));
		
		return FALSE;
	}
	
	return TRUE;
}
