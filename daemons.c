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

/** $Id: daemons.c,v 0.2 2005/06/24 07:59:44 diegocr Exp $
 **/

#include "daemons.h"
#include <stabs.h>
#include <amitcp/socketbasetags.h>

struct Library *SocketBase = NULL;

int h_errno = 0, errno = 0;
static struct MsgPort *myPort = NULL;

static long netsig = -1;
   ULONG NetsigMask = 0;

extern char *progname;
extern short syslog_debug;

void __request(const char *msg); /* @ libnix */

long SocketTags(struct Library *SocketBase, int *h_errno, int *errno)
{
	return SocketBaseTags(
		SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(*errno))),(ULONG) errno,
		SBTM_SETVAL(SBTC_HERRNOLONGPTR),           (ULONG) h_errno,
		SBTM_SETVAL(SBTC_LOGTAGPTR),               (ULONG) progname,
		SBTM_SETVAL(SBTC_LOGSTAT),                 /*LOG_PID |*/ LOG_CONS,
		SBTM_SETVAL(SBTC_LOGFACILITY),             LOG_DAEMON,
		SBTM_SETVAL(SBTC_LOGMASK),                 (syslog_debug ? LOG_UPTO(LOG_DEBUG):LOG_UPTO(LOG_INFO)),
	TAG_DONE);
}

INLINE long __SocketTags(struct Library *SocketBase)
{
	int res;
	
	res = SocketBaseTags(
		SBTM_SETVAL(SBTC_SIGEVENTMASK),	NetsigMask,
	//	SBTM_SETVAL(SBTC_SIGIOMASK),	NetsigMask,
	TAG_DONE);
	
	if(res)
		return res;
	
	return SocketTags( SocketBase, &h_errno, &errno );
}

INLINE BOOL daemons_setup( void )
{
	Forbid();
	myPort = FindPort( progname );
	Permit();
	
	if(myPort)
		return FALSE;
	
	if(!(myPort = CreatePort( progname, 0)))
		return FALSE;
	
	if((netsig = AllocSignal(-1)) == -1)
	{
//		err("Failed Allocating Signals");
		return FALSE;
	}
	
	NetsigMask = (1L << netsig);
	
	if(__SocketTags ( SocketBase ))
	{
		__request("BaseTags error");
		return FALSE;
	}
	
	return TRUE;
}




void daemons_startup( void )
{
	if(!(SocketBase = OpenLibrary("bsdsocket.library", 4))) {
		__request("No TCP/IP Running.");
		exit(20);
	}
	
	if(!daemons_setup()) {
		__request("Couldn't setup daemons interface");
		exit(20);
	}
	
	if(!daemons_bind_setup( NULL )) {
		__request("bind setup error");
		exit(20);
	}
	
	nfo("started.");
	
	inform("\e[1m%s successfully installed\e[0m\n", progname);
	
#if 0
	inform( "> This program uses \e[3m ´'Daemons IFace'´ \e[0m library.\n"
		"> (c) 2005 Diego Casorran <diegocr@users.sf.net>\n");
#endif
}

void daemons_cleanup( void )
{
	if(SocketBase) {
		nfo("exiting.");
		
		daemons_bind_cleanup();
		
		CloseLibrary(SocketBase);
	}
	
	if(netsig != -1)
		FreeSignal( netsig );
	
	if(myPort)
		DeletePort(myPort);
}

ADD2INIT(daemons_startup,-50);
ADD2EXIT(daemons_cleanup,-50);

