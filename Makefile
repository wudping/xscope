InstallRoot = /home/peterson
SHELL=/bin/sh
INSTALL = install
DFLAG = -g

##################################################################
#
#   xscope -- spy on characters to and from an X11 server
#
#	Author: JLPeterson, MCC
#
# Installs bin/xscope
#
##################################################################

CFLAGS = ${DFLAG} ${IFLAGS}

all:	xscope

SRCS = scope.c common.c fd.c server.c decode11.c table11.c print11.c prtype.c
OBJS = scope.o common.o fd.o server.o decode11.o table11.o print11.o prtype.o

xscope:	 ${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o xscope


##################################################################

install: all
	${INSTALL} xscope    /home/peterson/bin/xscope
	${INSTALL} xscope.1  /home/peterson/man/xscope.1


##################################################################
#
#   configuration
#
config:
	sed 's|$$<\InstallRoot>|'"${InstallRoot}|g" < Makefile.meta > Makefile

	
##################################################################
#
#   lint
#
lint:
	lint -bux ${IFLAGS} ${SRCS}

##################################################################
#
#   clean
#
clean:
	rm -f *.o xscope
	rm -f core load.map *~ \#*

##################################################################
#
#   dependencies
#
scope.o:  scope.c scope.h
common.o: common.c scope.h
fd.o: fd.c fd.h scope.h
server.o: server.c scope.h x11.h
decode11.o: decode11.c scope.h x11.h
table11.o: table11.c scope.h x11.h
print11.o: print11.c scope.h x11.h
prtype.o: prtype.c scope.h x11.h
scope.h:  fd.h
