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

/** $Id: daemons_utils.c,v 0.2 2005/06/24 07:59:45 diegocr Exp $
 **/

#include "daemons.h"
#include <stdarg.h>
#include <stdlib.h>

static UWORD fmtfunc[] = { 0x16c0, 0x4e75 };

void inform(const char *fmt, ...)
{
	static char msg[1024];
	va_list args;
	
	va_start( args, fmt );
	
	RawDoFmt( fmt, args, (APTR)fmtfunc, msg);
	va_end( args );
	
	PutStr( msg );
}

#if 0
void inform_connection(struct service *s)
{
	inform("%ld: connection from %s (%s)\n", 
		s->accept.client_ip, 
		s->accept.client_host ? s->accept.client_host:"unknown host");
}

void inform_error(struct service *s, char *fmt, ...)
{
	inform("**** error on sockfd %ld(%ld), service %ld: %s\n", 
		s->accept.sockfd, s->sockfd, s->port, strerror(errno));
}
#endif

/***************************************************************************/

