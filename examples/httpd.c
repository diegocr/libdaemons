/*
 * Daemons IFace (http://daemons.sourceforge.net/) Example.
 * 
 * This file contains a little HTTP Daemon to serve just one page.
 */

/** $Id: httpd.c,v 0.1 2005/07/20 19:58:36 diegocr Exp $
 **/

#include <daemons.h>
#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/dos.h>

char *progname = "HTTPd";		/* used on syslog messages and childs process name */
ushort __services[] = { 80, 0 };	/* services to listen on */
short syslog_debug = 1;			/* enables syslog debug */

#define HTML_PAGE	"index.htm"	/* public html page */

#ifndef MIN
# define MIN( a, b )	(((a)<(b)) ? (a):(b))
#endif

#define SocketBase      c->SocketBase
#define h_errno         c->h_errno
#define errno           c->errno

BOOL http_dispatcher( struct client *c )
{
	BPTR fh;
	UBYTE buffer[4096], *ptr=buffer, htmlpage[8192];
	
	if(!daemons_recv( &ptr, sizeof(buffer)-1, c )) {
		
		err("recv error: %s", strerror(errno));
		return FALSE;
	}
	
	nfo("connection from %s (%s)", c->client_ip, c->client_host );
	deb( buffer );
	
	if(strncmp( ptr, "GET ", 4)) {
		/* we accept GET methods only */
		
		err("rejecting no-GET request from %s", c->client_host );
		
		daemons_send("HTTP/1.0 403 Forbidden\r\n\r\n", -1, c );
		return FALSE;
	}
	
	if(strncmp( &ptr[3], " / ", 3)) {
		/* this example just sends the main page */
		
		daemons_send("HTTP/1.0 404 Not Found\r\n\r\n", -1, c );
		return FALSE;
	}
	
	ObtainSemaphore(c->sem);
	
	*htmlpage = 0;
	if((fh = Open( HTML_PAGE, MODE_OLDFILE)))
	{
		struct FileInfoBlock fib;
		
		if(ExamineFH( fh, &fib)) {
			
			Read( fh, htmlpage, MIN( sizeof(htmlpage), fib.fib_Size));
		}
		
		Close(fh);
	}
	
	ReleaseSemaphore(c->sem);
	
	if(!*htmlpage) {
		
		deb("failed reading " HTML_PAGE );
		
		sprintf( htmlpage, "Hello %s!...", c->client_host );
	}
	
	daemons_send( 
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html; charset=ISO-8859-1\r\n"
		"Connection: Closed\r\n\r\n", -1, c );
	
	daemons_send( htmlpage, -1, c );
	
	nfo("connection with %s closed", c->client_host );
	
	return TRUE;
}


int main()
{
	daemons_bind_setdispatcher( 80, http_dispatcher); /* declare dispatcher for service 80 */
	
	daemons_loop(); /* loop waiting for signals events */
	
	/* all done, terminating program */
	return 0;
}
