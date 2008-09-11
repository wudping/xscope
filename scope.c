/* ******************************************************
 *						      	*
 *  A spy program to reveal X11  traffic	    	*
 *						      	*
 *	James Peterson, 1988	       			*
 * Copyright (C) 1988 MCC
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of MCC not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  MCC makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * MCC DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL MCC BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *							*
 ***************************************************** */

#include "scope.h"

#include <sys/types.h>	       /* needed by sys/socket.h and netinet/in.h */
#include <sys/uio.h>	       /* for struct iovec, used by socket.h */
#include <sys/socket.h>	       /* for AF_INET, SOCK_STREAM, ... */
#include <sys/ioctl.h>	       /* for FIONCLEX, FIONBIO, ... */
#include <netinet/in.h>	       /* struct sockaddr_in */
#include <netdb.h>	       /* struct servent * and struct hostent * */
#include <errno.h>	       /* for EINTR, EADDRINUSE, ... */
extern int  errno;

/* ********************************************** */
/*                                                */
/* ********************************************** */

#define DefaultPort 6000

char    ServerHostName[255];
long    ServerBasePort = DefaultPort;
long    ServerInPort = 1;
long    ServerOutPort = 0;
long    ServerDisplay = 0;


/* ********************************************** */
/*                                                */
/*                                                */
/* ********************************************** */

short     GetServerport ()
{
  short     port;

  enterprocedure("GetServerport");

  port = ServerBasePort + ServerOutPort + ServerDisplay;
  debug(4,(stderr, "Server service is on port %d\n", port));
  return(port);
}

short     GetScopePort ()
{
  short     port;

  enterprocedure("GetScopePort");

  port = ServerBasePort + ServerInPort + ServerDisplay;
  debug(4,(stderr, "scope service is on port %d\n", port));
  return(port);
}

/* ********************************************** */
/*                                                */
/* ********************************************** */

Usage()
{
  fprintf(stderr, "Usage: xscope\n");
  fprintf(stderr, "              [-h<server-host>]\n");
  fprintf(stderr, "              [-i<in-port>]\n");
  fprintf(stderr, "              [-o<out-port>]\n");
  fprintf(stderr, "              [-d<display-number>]\n");
  fprintf(stderr, "              [-v<n>]  -- verbose output\n");
  fprintf(stderr, "              [-q]  -- quiet output\n");
  fprintf(stderr, "              [-D<debug-level>]\n");
  exit(1);
}


char *OfficialName();  /* forward type declaration */

ScanArgs(argc, argv)
     int     argc;
     char  **argv;
{
  NoExtensions = false /* default is to allow extensions */;
  Verbose = 1 /* default verbose-ness level */;
  RequestSync = false /* default is to just copy blocks */;

  /* Scan argument list */
  while (--argc > 0)
    {
      ++argv;
      if (**argv == '-')
	switch (*++*argv)
	  {
	    /*
	      debug levels:
	      	2 - trace each procedure entry
	      	4 - I/O, connections
	      	8 - Scope internals
	      	16 - Message protocol
		32 - 64 - malloc
		128 - 256 - really low level
	    */
	  case 'D':
	    debuglevel = atoi(++*argv);
	    if (debuglevel == 0)
	      debuglevel = 255;
	    debuglevel |= 1;
	    Verbose = 7;
	    debug(1,(stderr, "debuglevel = %d\n", debuglevel));
	    break;

	  case 'q': /* quiet mode */
	    Verbose = 0;
	    debug(1,(stderr, "Verbose = %d\n", Verbose));
	    break;

	  case 'v': /* verbose mode */
	    Verbose = atoi(++*argv);
	    debug(1,(stderr, "Verbose = %d\n", Verbose));
	    break;

	  case 's': /* synchronous mode */
	    RequestSync = true;
	    debug(1,(stderr, "RequestSync true\n"));
	    break;

	  case 'o':  /* output port */
	    ServerOutPort = atoi(++*argv);
	    if (ServerOutPort <= 0)
	      ServerOutPort = 0;
	    debug(1,(stderr, "ServerOutPort = %d\n", ServerOutPort));
	    break;

	  case 'd':
	    ServerDisplay = atoi(++*argv);
	    if (ServerDisplay <= 0)
	      ServerDisplay = 0;
	    debug(1,(stderr, "ServerDisplay=%d\n", ServerDisplay));
	    break;

	  case 'i':
	    ServerInPort = atoi(++*argv);
	    if (ServerInPort <= 0)
	      ServerInPort = 0;
	    debug(1,(stderr, "ServerInPort = %d\n", ServerInPort));
	    break;

	  case 'h':
	    if (++*argv != NULL && **argv != '\0')
	      (void)strcpy(ServerHostName, OfficialName(*argv));
	    debug(1,(stderr, "ServerHostName=%s\n", ServerHostName));
	    break;

	  case 'x':
	    NoExtensions = true;
	    debug(1,(stderr, "NoExtensions\n"));
	    break;

	  default:
	    fprintf(stderr, "Unknown option %c\n", **argv);
	    Usage();
	    break;

	  }
      else
	{
	  /* file argument to scope -- error */
	  Usage();
	}
    }

  /* check for different port numbers or different machines */
  if (ServerInPort == ServerOutPort)
    if (ServerHostName[0] == '\0')
      {
	fprintf(stderr, "Can't have xscope on same port as server (%d)\n",
		ServerInPort);
	Usage();
      }

}


/* ********************************************** */
/*                                                */
/* ********************************************** */

main(argc, argv)
     int     argc;
     char  **argv;
{
  ScanArgs(argc, argv);
  InitializeFD();
  InitializeX11();
  SetUpStdin();
  SetUpConnectionSocket(GetScopePort());
  SetSignalHandling();

  MainLoop();
  exit(0);
}

TimerExpired()
{
  debug(16,(stderr, "Timer tick\n"));
}

/* ********************************************** */
/*                                                */
/* ********************************************** */

/*
  here is where we would add code to allow control from
  the keyboard.  We would want to read a command and
  interpret it.  Possibilties:

  (a) verbose level setting
  (b) reset time
  (c) save X requests to a file.
  (d) replay X requests from a file.
  (e) allow fake events, errors to be generated.
*/

ReadStdin(fd)
     FD fd;
{
  char    buf[2048];
  long    n;

  enterprocedure("ReadStdin");
  n = read(fd, buf, 2048);
  debug(4,(stderr, "read %d bytes from stdin\n", n));
}

SetUpStdin()
{
  enterprocedure("SetUpStdin");
  /* UsingFD(fileno(stdin), ReadStdin); */
}

/* ************************************************************ */
/*								*/
/*								*/
/* ************************************************************ */

/*
  xscope is really meant to look at one client at a time.  However,
  it can easily handle multiple clients and servers.  To do so,
  we need to have a pair of FDs: one for the client and one for the
  server for that client.  If either goes away, so does the other.
  We need to be able to identify the other FD of a pair, so that if
  we get input from one, we can write it to the other.
*/

struct fdinfo
{
  Boolean Server;
  long    ClientNumber;
  FD pair;
};

static long ClientNumber = 0;
struct fdinfo   FDinfo[StaticMaxFD];

SetUpPair(client, server)
     FD client;
     FD server;
{
  if (client >= 0)
    {
      ClientNumber += 1;
      FDinfo[client].Server = false;
      FDinfo[client].pair = server;
      FDinfo[client].ClientNumber = ClientNumber;
      if (server >= 0)
	{
	  FDinfo[server].Server = true;
	  FDinfo[server].pair = client;
	  FDinfo[server].ClientNumber = FDinfo[client].ClientNumber;
	}
    }
  else if (server >= 0)
      {
	(void)close(server);
	NotUsingFD(server);
      }
}


CloseConnection(fd)
     FD fd;
{
  debug(4,(stderr, "close %d and %d\n", fd, FDPair(fd)));
  StopClientConnection(ServerHalf(fd));
  StopServerConnection(ClientHalf(fd));

  (void)close(fd);
  NotUsingFD(fd);
  (void)close(FDPair(fd));
  NotUsingFD(FDPair(fd));
}

/* ************************************************************ */

FD FDPair(fd)
     FD fd;
{
  return(FDinfo[fd].pair);
}

FD ClientHalf(fd)
     FD fd;
{
  if (FDinfo[fd].Server)
    return(FDinfo[fd].pair);
  return(fd);
}

FD ServerHalf(fd)
     FD fd;
{
  if (FDinfo[fd].Server)
    return(fd);
  return(FDinfo[fd].pair);
}

char   *ClientName (fd)
     FD fd;
{
  static char name[20];

  (void)sprintf(name, " %s %d",
		(FDinfo[fd].Server ? "Server" : "Client"),
		FDinfo[fd].ClientNumber);
  return(name);
}


/* ********************************************** */
/*                                                */
/* ********************************************** */

/* when we get data from a client, we read it in, copy it to the
   server for this client, then dump it to the client. Note, we don't
   have to have a server, if there isn't one. */

DataFromClient(fd)
     FD fd;
{
  unsigned char    buf[2048];
  long    n;
  FD ServerFD;

  enterprocedure("DataFromClient");
  n = read(fd, (char *)buf, 2048);
  debug(4,(stderr, "read %d bytes from %s\n", n, ClientName(fd)));
  if (n < 0)
    {
      PrintTime();
      perror("Client --> read error:");
      CloseConnection(fd);
      return;
    }
  if (n == 0)
    {
      PrintTime();
      fprintf(stdout, "%s --> EOF\n", ClientName(fd));
      CloseConnection(fd);
      return;
    }
  if (debuglevel > 4)
    {
      fflush(stdout); fflush(stderr);
      DumpHexBuffer(buf, n);
      fprintf(stdout,"\n");
      fflush(stdout); fflush(stderr);
    }

  ServerFD = FDPair(fd);
  if (ServerFD < 0)
    {
      ServerFD = ConnectToServer(false);
      SetUpPair(fd, ServerFD);
    }

  /* write bytes from client to server, allow for server to fail */
  if (!RequestSync) WriteBytes(ServerFD, buf, n);

  /* also report the bytes to standard out */
  ReportFromClient(fd, buf, n);
}

/* ********************************************** */
/*                                                */
/* ********************************************** */

/* similar situation for the server, but note that if there is no client,
   we close the connection down -- don't need a server with no client. */

DataFromServer(fd)
     FD fd;
{
  unsigned char    buf[2048];
  long    n;
  FD ClientFD;

  enterprocedure("DataFromServer");
  n = read(fd, (char *)buf, 2048);
  debug(4,(stderr, "read %d bytes from %s\n", n, ClientName(fd)));
  if (n < 0)
    {
      PrintTime();
      perror("read error <- Server");
      CloseConnection(fd);
      return;
    }
  if (n == 0)
    {
      PrintTime();
      fprintf(stdout, "EOF <-- %s\n", ClientName(fd));
      CloseConnection(fd);
      return;
    }
  if (debuglevel > 4)
    {
      fflush(stdout); fflush(stderr);
      DumpHexBuffer(buf, n);
      fprintf(stdout,"\n");
      fflush(stdout); fflush(stderr);
    }

  ClientFD = FDPair(fd);
  if (ClientFD < 0)
    {
      CloseConnection(fd);
      return;
    }

  if (NoExtensions)
    {
      if (buf[0] == 1) /* reply code */
	{
	  short   SequenceNumber = IShort (&buf[2]);
	  short   Request = CheckReplyTable (fd, SequenceNumber);
	  if (Request == 98) /* Query Extension */
	    if (buf[8] == 1)
	      buf[8] = 0;
	}
    }

  /* write bytes from server to client, allow for client to fail */
  WriteBytes(ClientFD, buf, n);

  /* also report the bytes to standard out */
  ReportFromServer(fd, buf, n);
}



/* ************************************************************ */
/*								*/
/*								*/
/* ************************************************************ */

WriteBytes(fd, buf, n)
FD fd;
unsigned char   *buf;
long    n;
{
  long    BytesToWrite = n;
  unsigned char   *p = buf;

  if (fd < 0) return;
  if (!ValidFD(fd)) return;

  while (BytesToWrite > 0)
    {
      int     n1 = write (fd, (char *)p, (int)BytesToWrite);
      debug(4,(stderr, "write %d bytes to %s\n", n1, ClientName(fd)));
      if (n1 > 0)
	{
	  BytesToWrite -= n1;
	  p += n1;
	}
      else
	{
	  char message[255];
	  sprintf(message,"Error on write to %s", ClientName(fd));
	  perror(message);
	  CloseConnection(fd);
	  BytesToWrite = 0;
	}
    }
}


/* ************************************************************ */
/*								*/
/*     Create New Connection to a client program and to Server  */
/*								*/
/* ************************************************************ */

static int  ON = 1 /* used in ioctl */ ;

NewConnection(fd)
     FD fd;
{
  FD ServerFD = -1;
  FD ClientFD = -1;

  ClientFD = ConnectToClient(fd);
  ServerFD = ConnectToServer(true);
  SetUpPair(ClientFD, ServerFD);
}


/* ************************************************************ */

FD ConnectToClient(ConnectionSocket)
     FD ConnectionSocket;
{
  FD ClientFD;
  struct sockaddr_in  from;
  size_t   len = sizeof (from);

  enterprocedure("ConnectToClient");

  ClientFD = accept(ConnectionSocket, (struct sockaddr *)&from, &len);
  debug(4,(stderr, "Connect To Client: FD %d\n", ClientFD));
  if (ClientFD < 0 && errno == EWOULDBLOCK)
    {
      debug(4,(stderr, "Almost blocked accepting FD %d\n", ClientFD));
      panic("Can't connect to Client");
    }
  if (ClientFD < 0)
    {
      debug(4,(stderr, "NewConnection: error %d\n", errno));
      panic("Can't connect to Client");
    }

  UsingFD(ClientFD, DataFromClient);
#ifdef FIOCLEX
  (void)ioctl(ClientFD, FIOCLEX, 0);
#endif /* #ifdef FIOCLEX */
  (void)ioctl(ClientFD, FIONBIO, &ON);
  StartClientConnection(ClientFD);
  return(ClientFD);
}



/* ************************************************************ */
/*								*/
/*								*/
/* ************************************************************ */



FD ConnectToServer(report)
     Boolean report;
{
  FD ServerFD;
  struct sockaddr_in  sin;
  struct hostent *hp;
#ifndef SO_DONTLINGER
  struct linger linger;
#endif /* #ifndef SO_DONTLINGER */

  enterprocedure("ConnectToServer");

  /* establish a socket to the name server for this host */
  bzero((char *)&sin, sizeof(sin));
  ServerFD = socket(AF_INET, SOCK_STREAM, 0);
  if (ServerFD < 0)
    {
      perror("socket() to Server failed");
      debug(1,(stderr, "socket failed\n"));
      panic("Can't open connection to Server");
    }
  (void) setsockopt(ServerFD, SOL_SOCKET, SO_REUSEADDR,  (char *) NULL, 0);
#ifdef SO_USELOOPBACK
  (void) setsockopt(ServerFD, SOL_SOCKET, SO_USELOOPBACK,(char *) NULL, 0);
#endif /* #ifdef SO_USELOOPBACK */
#ifdef SO_DONTLINGER
  (void) setsockopt(ServerFD, SOL_SOCKET, SO_DONTLINGER, (char *) NULL, 0);
#else /* #ifdef SO_DONTLINGER */
  linger.l_onoff = 0;
  linger.l_linger = 0;
  (void) setsockopt(ServerFD, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof linger);
#endif /* #ifdef SO_DONTLINGER */

  /* determine the host machine for this process */
  if (ServerHostName[0] == '\0')
    (void) gethostname(ServerHostName, sizeof (ServerHostName));
  debug(4,(stderr, "try to connect on %s\n", ServerHostName));

  hp = gethostbyname(ServerHostName);
  if (hp == 0)
    {
      perror("gethostbyname failed");
      debug(1,(stderr, "gethostbyname failed for %s\n", ServerHostName));
      panic("Can't open connection to Server");
    }

  sin.sin_family = AF_INET;
  bcopy((char *)hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
  sin.sin_port = htons(GetServerport());

  if ((sin.sin_port == ScopePort)
      && strcmp(ServerHostName, ScopeHost) == 0)
    {
      char error_message[100];
      (void)sprintf(error_message, "Trying to attach to myself: %s,%d\n",
	      ServerHostName, sin.sin_port);
      panic(error_message);
    }

  /* ******************************************************** */
  /* try to connect to Server */

  if (connect(ServerFD, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
      debug(4,(stderr, "connect returns errno of %d\n", errno));
      if (errno != 0)
	if (report)
	  perror("connect");
      switch (errno)
	{
	case ECONNREFUSED:
	  /* experience says this is because there is no Server
	     to connect to */
	  (void)close(ServerFD);
	  debug(1,(stderr, "No Server\n"));
	  if (report)
	    warn("Can't open connection to Server");
	  return(-1);

	default:
	  (void)close(ServerFD);
	  panic("Can't open connection to Server");
	}
    }

  debug(4,(stderr, "Connect To Server: FD %d\n", ServerFD));
  if (ServerFD >= 0)
    {
      UsingFD(ServerFD, DataFromServer);
      StartServerConnection(ServerFD);
    }
  return(ServerFD);
}


/* ********************************************** */
/*                                                */
/* ********************************************** */

char *OfficialName(name)
char *name;
{
  struct hostent *HostEntry;

  HostEntry = gethostbyname(name);
  if (HostEntry == NULL)
    {
      perror("gethostbyname");
      exit(-1);
    }
  debug(4,(stderr, "Official name of %s is %s\n", name, HostEntry->h_name));
  return(HostEntry->h_name);
}

