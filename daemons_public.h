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
#ifndef DAEMONS_DAEMONS_H
#define DAEMONS_DAEMONS_H

/** $Id: daemons_public.h,v 0.1 2005/07/20 19:39:44 diegocr Exp $
 **/

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>

#include <sys/socket.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <netdb.h>



#define syslog( level, fmt, args...)					\
	({								\
		ULONG _tags[] = { args }; vsyslog( level, fmt, _tags);	\
	})
#define nfo(args...)	syslog(  LOG_INFO	, args)
#define warn(args...)	syslog(  LOG_WARN	, args)
#define err(args...)	syslog(  LOG_ERR	, args)
#define notice(args...)	syslog(  LOG_NOTICE	, args)
#define deb(args...)	syslog(  LOG_DEBUG	, args)


extern unsigned short __services[];

struct accept
{
	struct sockaddr_in   sa;
	struct hostent     * he;
	struct in_addr       ia;
};
struct client
{
	long sockfd;
	
	struct accept a;
	char *client_host, *client_ip;
	
	struct SignalSemaphore *sem;
	
	struct service *s; /* hmmm */
	struct Library *SocketBase;
	int h_errno, errno;
};
struct service
{
	long sockfd , asock;
	unsigned short port;
	
	BOOL (*dispatcher)(struct client *c);
};



BOOL daemons_bind_setup(BOOL (*global_dispatcher)(struct client *c));
BOOL daemons_bind_setdispatcher( unsigned short port, 
	BOOL (*dispatcher)(struct client *c));

void daemons_loop( void );
char *daemons_clock( void );


STRPTR daemons_recvln( STRPTR *dest, ULONG destlen, struct client *c );
LONG daemons_recv( STRPTR *dest, long destlen, struct client *c );
BOOL daemons_send( STRPTR string, long length, struct client *c );


char *strerror(int errnum);
extern int h_errno , errno;




#endif /* DAEMONS_DAEMONS_H */
