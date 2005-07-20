/*
 * Daemons IFace (http://daemons.sourceforge.net/) Example.
 */

/** $Id: echod.c,v 0.1 2005/07/20 19:49:13 diegocr Exp $
 **/

#include <daemons.h>

char *progname = "echod";	/* program name, to use on syslog messages and childs process name */
ushort __services[] = { 7, 0 };	/* services to listen on */
short syslog_debug = 1;		/* enables syslog debug */

short echo_dispatcher(struct client *c)
{
	u_char buffer[1024], *ptr=buffer;
	
	while(daemons_recv( &ptr, sizeof(buffer) -1, c ))
	{
		daemons_send( buffer, -1, c );
	}
	
	return 1;
}

int main()
{
	daemons_bind_setdispatcher( 7, echo_dispatcher); /* declare dispatcher for service 7 */
	
	daemons_loop(); /* loop waiting for signals events */
	
	/* all done, terminating program */
	return 0;
}

