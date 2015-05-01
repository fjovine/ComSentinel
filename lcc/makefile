# Wedit Makefile for project comsentinel
SRCDIR="c:\users\fj\dropbox\jobs\job256-com sentinel"
CFLAGS=-I"c:\lcc\include" -I"C:\lcc\include"  -g2 
CC=$(LCCROOT)\bin\lcc.exe
LINKER=$(LCCROOT)\bin\lcclnk.exe
OBJS=\
	comsentinel.obj \
	comsentinel.res \
	set.obj \
	wintray.obj \

LIBS=SHELL32.lib SETUPAPI.lib ole32.lib
EXE="comsentinel.exe"

$(EXE):	$(OBJS) Makefile
	$(LINKER)  -subsystem windows -o $(SRCDIR)\"lcc\comsentinel.exe" $(OBJS) $(LIBS)

# Build comsentinel.c
COMSENTINEL_C=\
	$(SRCDIR)\comsentinel.h\
	$(SRCDIR)\set.h\
	$(SRCDIR)\wintray.h\

comsentinel.obj: $(COMSENTINEL_C) $(SRCDIR)\"comsentinel.c"
	$(CC) -c $(CFLAGS) $(SRCDIR)\"comsentinel.c"

# Build comsentinel.res
COMSENTINEL_RC=\
	$(SRCDIR)\comsentinel.h\

comsentinel.res:	$(COMSENTINEL_RC) $(SRCDIR)\comsentinel.rc
	$(LCCROOT)\bin\lrc.exe -I$(SRCDIR) -I"c:\lcc\include" -I"C:\lcc\include"  $(SRCDIR)\comsentinel.rc

# Build set.c
SET_C=\
	$(SRCDIR)\comsentinel.h\
	$(SRCDIR)\set.h\
	$(SRCDIR)\wintray.h\

set.obj: $(SET_C) $(SRCDIR)\"set.c"
	$(CC) -c $(CFLAGS) $(SRCDIR)\"set.c"

# Build wintray.c
WINTRAY_C=\
	$(SRCDIR)\comsentinel.h\
	$(SRCDIR)\set.h\
	$(SRCDIR)\wintray.h\

wintray.obj: $(WINTRAY_C) $(SRCDIR)\"wintray.c"
	$(CC) -c $(CFLAGS) $(SRCDIR)\"wintray.c"

link:
	$(LINKER)  -subsystem windows -o $(SRCDIR)\"lcc\comsentinel.exe" $(OBJS) $(LIBS)

clean:
	del $(OBJS) comsentinel.exe
