
#include <daemons.h>
#include <stdlib.h>
#include <stdio.h>

char *progname = "TCPPortMon2";
unsigned int __services[] = { 0 };

short syslog_debug = 1;
short daemons_bindretard = 1;

short log_dispatcher(struct client *c)
{
	UBYTE *buf, fname[128];
	long len;
	BPTR fh;
	
	if(!(buf=malloc(262144))) {
		Printf("out of meory !?\n");
		return 0;
	}
	
	ObtainSemaphore(c->sem);
	
	sprintf( fname, "ram:%s-%ld.tcplog", progname, (long) c->s->port);
	
	if(!(fh = Open( fname, MODE_READWRITE))) {
		Printf(" **** CAN'T OPEN %s\n", (long) fname );
		free(buf);
		ReleaseSemaphore(c->sem);
		return 0;
	}
	
	Seek( fh, 0, OFFSET_END );
	
	FPrintf( fh, "client_host = %s, client_ip = %s, request:\n", (long) c->client_host, (long) c->client_ip);
	
	if((len = daemons_recv( c->sockfd, &buf, 262144, c->SocketBase )) >0 )
	{
		Seek( fh, 0, OFFSET_END );
		Write( fh, buf, len );
	}
	else Printf("ERROR RECEIVING\n");
	
	free(buf);
	Close( fh );
	ReleaseSemaphore(c->sem);
	
	return 1;
}

int main()
{
	LONG arg[1] = {0};
	struct RDArgs *args;
	
	if((args = ReadArgs( "SERVICES/A/M", (long*)arg, NULL)))
	{
		int i = 0;
		STRPTR ** arry = (STRPTR **) arg[0];
		
		Printf("User-Services: ");
		
		do {
			__services[i++] = atoi((char *)(*arry));
			__services[ i ] = 0;
			
			Printf("%ld, ", (long)__services[ i-1 ] ); Flush(Output());
			
		} while(*++arry);
		
		Printf("\n");
		
		if(daemons_bind_setup( log_dispatcher ))
			daemons_loop();
		
		FreeArgs(args);
	}
	else PrintFault(IoErr(),0L);
	
	return 0;
}

size_t strlen(const char *string);
size_t strlen(const char *string)
{ const char *s=string;
  
  if(!(string && *string))
  	return 0;
  
  do;while(*s++); return ~(string-s);
}
