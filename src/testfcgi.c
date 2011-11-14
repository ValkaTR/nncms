// gcc -I/usr/include/fastcgi -lfcgi testfcgi.c -o testfcgi
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <alloca.h>
#include <fcgiapp.h>
#include <stdio.h>
#define LISTENSOCK_FILENO 0
#define LISTENSOCK_FLAGS 0
int main(int argc, char** argv) {
  openlog ("testfastcgi", LOG_CONS|LOG_NDELAY, LOG_USER);
  int err = FCGX_Init (); // call before Accept in multithreaded apps
  if (err) { syslog (LOG_INFO, "FCGX_Init failed: %d", err); return 1; }
  FCGX_Request cgi;
  err = FCGX_InitRequest(&cgi, LISTENSOCK_FILENO, LISTENSOCK_FLAGS);
  if (err) { syslog (LOG_INFO, "FCGX_InitRequest failed: %d", err); return 2; }
  while (1) {
    err = FCGX_Accept_r(&cgi);
    if (err) { syslog (LOG_INFO, "FCGX_Accept_r stopped: %d", err); break; }
    char** envp;
    int size = 1024 * 32;
    for (envp = cgi.envp; *envp; ++envp) size += strlen(*envp);
    char*  result = (char*) alloca(size);
    strncat (result, "Status: 200 OK\r\nContent-Type: text/html\r\n\r\n", size);
	FILE *pFile = fopen( "/var/www/logs/req->lpszOutput.txt", "r" );
	if( pFile == 0 )
	{
	    strncat (result, "<html><head><title>testcgi</title></head><body><ul>\r\n", size);
	    for (envp = cgi.envp; *envp; ++envp) {
    		strncat(result, "<li>", size); 
		    strncat(result, *envp, size); 
		    strncat(result, "</li>\r\n", size);
		}
	    strncat(result, "</ul></body></html>\r\n", size);
	}
	else
	{
		char szBuffer[1024 * 32];
		fread( szBuffer, sizeof( szBuffer ), sizeof( char ), pFile );
		fclose( pFile );
		strncat( result, szBuffer, size );
	}
    FCGX_PutStr(result, strlen(result), cgi.out);
    FCGX_Finish_r(&cgi);
  }
  FCGX_ShutdownPending();
  return 0;
}

