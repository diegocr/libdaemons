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

/** $Id: daemons_fork.c,v 0.3 2005/06/24 07:59:45 diegocr Exp $
 **/

#include "daemons.h"
#include <dos/dostags.h>
#include <stdlib.h> /* malloc */
#include <string.h> /* strdup */
#include <errno.h> /* EPROCLIM */
#include <stabs.h>

struct fork_msg {
	struct Task *parent_task, *child_task;
	struct MsgPort *parent_port, *child_port;
	
	struct SignalSemaphore sema;
	struct service **awaking_client;
	
	long skey;
	short working;
	
	struct fork_msg *next;
	struct fork_msg *prev;
};
struct child_msg {
	struct Message msg;
	short replyed;
};
static struct fork_msg *forks_list = NULL;

#ifndef UNIQUE_ID
# define UNIQUE_ID (-1)
#endif

void child_process( void );
void fork_poll( void );
long SocketTags(struct Library *SocketBase, int *h_errno, int *errno);
extern char *progname;
extern short daemons_resolve;
extern ULONG daemons_signals;
extern unsigned short daemons_forks;
static unsigned short forks = 0;
extern ULONG daemons_child_stacksize;
extern int daemons_child_priority;
extern struct TagItem *daemons_child_xtratags;

BOOL fork(struct service **awaking)
{
	BOOL done = FALSE;
	long skey;
	
	if(forks == daemons_forks)
	{
		errno = EPROCLIM;
		return FALSE;
	}
	
	if((skey = ReleaseSocket((*awaking)->asock, UNIQUE_ID)) != -1 )
	{
		struct fork_msg *newfork;
		
		if((newfork = malloc(sizeof(struct fork_msg))))
		{
			newfork->child_task = (struct Task *)
				CreateNewProcTags (
					NP_Entry,	(ULONG) &child_process,
					NP_Name,	(ULONG) progname,
					NP_Priority,	daemons_child_priority,
					NP_StackSize,	daemons_child_stacksize,
					TAG_MORE,	daemons_child_xtratags,
				TAG_DONE );
			
			if(newfork->child_task) {
				newfork->parent_task	= FindTask(NULL);
				newfork->awaking_client = awaking;
				
				newfork->skey		= skey;
				newfork->working	= FALSE;
				
				InitSemaphore(&newfork->sema);
				
				newfork->next = forks_list;
				newfork->prev = NULL;
				
				if(forks_list)
					forks_list->prev = newfork;
				
				forks_list = newfork;
				
				newfork->child_task->tc_UserData = newfork;
				Signal( newfork->child_task, SIGF_SINGLE );
				Wait(SIGF_SINGLE);
				
				done = TRUE;		forks++;
			}
			else {
				free( newfork );
			}
		}
	}
	
	return done;
}


void child_process( void )
{
	struct service *bcfg;
	struct fork_msg *fmsg;
	struct Library *SocketBase = NULL;
	struct Task *ThisChild = FindTask(NULL);
	
	Wait(SIGF_SINGLE);
	fmsg = (struct fork_msg *) ThisChild->tc_UserData;
	bcfg = (*fmsg->awaking_client);
	
	Forbid();
	Signal( fmsg->parent_task, SIGF_SINGLE );
	Permit();
	
	ObtainSemaphore(&fmsg->sema);
	if(!(fmsg->child_port = CreateMsgPort ( ))) {
		ReleaseSemaphore(&fmsg->sema);
		return;
	}
	fmsg->working = TRUE;
	ReleaseSemaphore(&fmsg->sema);
	
	if((SocketBase = OpenLibrary("bsdsocket.library", 4)))
	{
		long mysock;
		
		if((mysock = ObtainSocket( fmsg->skey, AF_INET, SOCK_STREAM, 0)) != -1)
		{
			struct client *client_data;
			
			if((client_data = malloc(sizeof(struct client))))
			{
				client_data->SocketBase = SocketBase;
				client_data->sockfd	= mysock;
				client_data->a		= bcfg->a;
				client_data->s		= bcfg;
				client_data->sem	= &bcfg->semaforo;
				
				SocketTags( SocketBase, &client_data->h_errno, &client_data->errno );
				
				client_data->client_ip	= strdup((char *) Inet_NtoA(client_data->a.sa.sin_addr.s_addr));
				
				if(daemons_resolve)
				{
					client_data->a.ia.s_addr = client_data->a.sa.sin_addr.s_addr; //inet_addr(client_data->a.client_ip);
					client_data->a.he = gethostbyaddr((const char *)&client_data->a.ia, sizeof(struct in_addr), AF_INET);
					
					client_data->client_host = client_data->a.he ? strdup((char *) client_data->a.he->h_name):NULL;
				}
				
				if(bcfg->dispatcher) {
					(*bcfg->dispatcher)( client_data );
					
					deb("dispatcher for service %ld ($%08lx) stopped.", bcfg->port, (long) SocketBase );
				}
				else {
					err("NO DISPATCHER FOR SERVICE %ld", bcfg->port);
				}
				
				if(client_data->client_host) {
					free(client_data->client_host); client_data->client_host = NULL;
				}
				if(client_data->client_ip) {
					free(client_data->client_ip); client_data->client_ip = NULL;
				}
				free( client_data );
			}
			
			if(mysock != -1) {
				shutdown( mysock, SHUT_RDWR);
				CloseSocket( mysock );
			}
		}
		
		CloseLibrary( SocketBase );
	}
	
	ObtainSemaphore(&fmsg->sema);
	fmsg->working = FALSE;
	
	do {
		struct child_msg *cmsg;
		
		cmsg = (struct child_msg *) GetMsg( fmsg->child_port );
		if(!cmsg)
			break;
		
		if(!cmsg->replyed)
			ReplyMsg((struct Message *) cmsg );
		
	} while(1);
	
	DeleteMsgPort(fmsg->child_port);
	fmsg->child_port = NULL;
	
	Forbid ( );
	ReleaseSemaphore(&fmsg->sema);
	Signal( fmsg->parent_task, SIGBREAKF_CTRL_E );
}



/**************************************************************************/

unsigned long forkmask = 0;
static struct MsgPort *parent = NULL;

void fork_setup( void )
{
	if(!(parent = CreateMsgPort ( )))
		exit( 36 );
	
	forkmask = (1L << parent->mp_SigBit) | SIGBREAKF_CTRL_E;
	
	daemons_signals |= forkmask;
}

void fork_cleanup( void )
{
	struct fork_msg *ptr;
	
	for( ptr = forks_list; ptr; ptr = ptr->next )
	{
		Signal( ptr->child_task, SIGBREAKF_CTRL_C );
	}
	
	do {
		fork_poll ( );
		if(! forks_list ) break;
		Wait( forkmask | SIGBREAKF_CTRL_C );
		
	} while(1);
	
	DeleteMsgPort( parent );
}

ADD2INIT(fork_setup,-60);
ADD2EXIT(fork_cleanup,-40);


/**************************************************************************/

void fork_poll( void )
{
	struct child_msg *cmsg;
	struct fork_msg *fmsg, *fmn;
	
	for( fmsg = forks_list; fmsg; fmsg = fmn)
	{
		ObtainSemaphore(&fmsg->sema);
		fmn = fmsg->next;
		
		if(!fmsg->working) {
			if(fmsg->prev)
				fmsg->prev->next = fmn;
			else
				forks_list = fmn;
			if(fmn)
				fmn->prev = fmsg->prev;
			
			free( fmsg );		forks--;
		}
		else {
			ReleaseSemaphore(&fmsg->sema);
		}
	}
	
	do {
		cmsg = (struct child_msg *) GetMsg( parent );
		if(!cmsg)
			break;
		
		if(cmsg->replyed) {
			free( cmsg );
		}
		else {
			cmsg->replyed = TRUE;
			ReplyMsg((struct Message *) cmsg );
		}
		
	} while(1);
}

