# Para Linux/amd64
SYSDEP= nStack-amd64.o
MAKELIB= ar-ranlib
DEFINES=

# Para Linux/i386
# SYSDEP= nStack-i386.o
# MAKELIB= ar-ranlib
# DEFINES=
#
# Para Sparc/Solaris
# SYSDEP= nStack-sparc.o
# MAKELIB= ar-lorder
# DEFINES= -DREOPENSTDIO -DSOLARIS
#
# Para Sparc/SunOS 4.X.X:
# SYSDEP= nStack-sparc.o
# MAKELIB= ar-ranlib
# DEFINES= -DREOPENSTDIO -DNO_UNDERSCORE
# OBS. IMPORTANTE: el directorio /usr/ccs/bin debe ir antes que /bin
# /usr/bin y /usr/local/bin en el path de busqueda de comandos

#------ fin parte parte dependiente -----

NSYSTEM= nProcess.o nTime.o nMsg.o nSem.o nMonitor.o nIO.o nDep.o \
         nMain.o nQueue.o nOther.o fifoqueues.o $(SYSDEP)
LIBNSYS= libnSys.a

CFLAGS= -ggdb -Wall -pedantic -I../include $(DEFINES)
LFLAGS= -ggdb 
#CFLAGS= -O2 -I../include $(DEFINES)
#LFLAGS=

# Para Linux x86 en maquinas de 64 bits (amd64)
#CFLAGS= -ggdb -m32 -I../include $(DEFINES)
#LFLAGS= -ggdb -m32

all: $(LIBNSYS)

.SUFFIXES:
.SUFFIXES: .o .c .s

.c.o .s.o:
	gcc -c $(CFLAGS) $<

$(LIBNSYS): $(NSYSTEM) $(MAKELIB)
	rm -rf $(LIBNSYS)
	sh $(MAKELIB) $@ $(NSYSTEM)

clean:
	rm -f *.o *~
