# Generated automatically from Makefile.in by configure.
# --------------------------------------------------------------------------
# Makefile for Hugs
#
# The Hugs 98 system is Copyright (c) Mark P Jones, Alastair Reid, the
# Yale Haskell Group, and the OGI School of Science & Engineering at OHSU,
# 1994-2002, All rights reserved.  It is distributed as
# free software under the license in the file "License", which is
# included in the distribution.
# --------------------------------------------------------------------------

# Targets:
#
# <default>:   	  make hugs.exe and runhugs.exe
# install:	  make and install programs/libraries
# hugs.exe:    	  make minimal working system
# runhugs.exe:	  make batch-mode version of Hugs
# hugsscript.dll: make DLL version of Hugs
# clean:       	  delete files not required in running system
# distclean:      delete files that can be regenerated using C compiler
# veryclean:      delete all machine generated files 
#                   (you need perl, bison/yacc, etc to rebuild these files)
# check:	  run regression tests
# TAGS:           build emacs TAGS table

# This rule goes first to make it the default choice
default		:: hugs.exe runhugs.exe hugsscript.dll

CC		= cl /nologo
CFLAGS		= -clr -DDOTNET -DMONTH_YEAR="\"Nov 2002\""
#CFLAGS		= -ML -Zi -DMONTH_YEAR="\"Nov 2002\""
OPTFLAGS	= -O2

LD		= $(CC) -clr
LDFLAGS    	= 
# When debugging:
#LDFLAGS	= -Zi
DLL_FLAGS	= /LD
PIC_FLAGS	= 
LIBS		=  kernel32.lib advapi32.lib
LINK_FLAGS      = /link /assemblymodule:dotnet\HugsWrapper.netmodule

YACC		= bison
RM		= -del
CP		= copy



.SUFFIXES	:
.SUFFIXES	: .c .h .obj
HFILES		= prelude.h config.h options.h storage.h connect.h \
		  errors.h command.h server.h HsFFI.h module.h machdep.h
DOTNET_FILES    = dotnet\Invoker.cpp dotnet\prim.cpp dotnet\HugsServ.cpp
CFILES		= hugs.c storage.c input.c static.c type.c subst.c \
		  output.c compiler.c machine.c interp.c builtin.c \
		  server.c ffi.c module.c machdep.c $(DOTNET_FILES)
INCFILES	= parser.c preds.c bignums.c scc.c timer.c \
		  printer.c iomonad.c interns.c stmonad.c array.c
YFILES		= parser.y
SOURCES		= $(HFILES) $(CFILES) $(INCFILES) $(YFILES) 
OBJECTS		= storage.obj input.obj static.obj type.obj subst.obj \
	          compiler.obj plugin.obj builtin.obj machine.obj output.obj ffi.obj \
		  version.obj module.obj machdep.obj

DOBJECTS        = dotnet\Invoker.obj dotnet\prim.obj
		  
IOBJECTS	= hugs.obj hugs.res $(OBJECTS) $(DOBJECTS)
PRELUDE		= config.h options.h prelude.h

################################################################
# Hugs interpreter and standalone evaluator
################################################################

hugs.exe	: $(IOBJECTS) dotnet\HugsWrapper.netmodule
		  $(LD) $(LDFLAGS) $(IOBJECTS)  $(LIBS) -o hugs.exe  $(LINK_FLAGS)
		  

SERVER_OBJECTS	= server.obj runhugs.res $(OBJECTS) $(DOBJECTS)
runhugs.obj	: $(PRELUDE) hugs.c machdep.h timer.c runhugs.c
runhugs.exe	: runhugs.obj $(SERVER_OBJECTS)
		  $(LD) $(LDFLAGS) runhugs.obj $(SERVER_OBJECTS) $(LIBS) -o runhugs.exe $(LINK_FLAGS)

ffihugs.obj	: $(PRELUDE) hugs.c machdep.h timer.c runhugs.c
ffihugs.exe	: ffihugs.obj $(SERVER_OBJECTS)
		  $(CC) $(LDFLAGS) ffihugs.obj $(SERVER_OBJECTS) $(LIBS) -o ffihugs.exe $(LINK_FLAGS)

ffihugs.obj	: runhugs.c
		  $(CC) -c $(CFLAGS) $(OPTFLAGS) -DFFI_COMPILER runhugs.c /Foffihugs.obj

hugsscript.obj  : hugsscript.c
hugsscript.dll  : $(SERVER_OBJECTS) hugsscript.obj
		  $(CC) $(DLL_FLAGS) $(LDFLAGS) $(SERVER_OBJECTS) hugsscript.obj $(LIBS) -o hugsscript.dll

#
# Create all the FFI extension DLLs.
#
FFIHUGS=..\ffihugs
FFIHUGS_OPTS=+G -W -98 -P

.PHONY: ffi-dlls

ffi-dlls:
	$(FFIHUGS) $(FFIHUGS_OPTS) +L../libraries/Hugs/Storable_aux.c Hugs.Storable
	$(FFIHUGS) $(FFIHUGS_OPTS) Foreign.Marshal.Alloc
	$(FFIHUGS) $(FFIHUGS_OPTS) Foreign.Marshal.Utils
	$(FFIHUGS) $(FFIHUGS_OPTS) +L../libraries/Foreign/C/errno.c Foreign.C.Error
	$(FFIHUGS) $(FFIHUGS_OPTS) +L../libraries/Network/initWinSock.c +Lwsock32.lib Network.Socket
	$(FFIHUGS) $(FFIHUGS_OPTS) +Lwsock32.lib Network.BSD

################################################################
# Clean, distclean, veryclean, TAGS
################################################################

clean		::
		  $(RM) *.o
		  $(RM) *.O
		  $(RM) *.obj
		  $(RM) *.OBJ
		  $(RM) *.LIB
		  $(RM) *.DEF
		  $(RM) *.RES
		  $(RM) *.EXP
		  $(RM) *.ILK
		  $(RM) *.PDB
		  $(RM) *.TD2
		  $(RM) *.MAP
		  $(RM) *.CSM
		  $(RM) *.TR2
		  $(RM) *.DSW
		  $(RM) *.RES
		  $(RM) *.aux
		  $(RM) *.hp
		  $(RM) hugs.exe
		  $(RM) runhugs.exe
		  $(RM) ffihugs.exe

distclean	:: clean
distclean	::
		  $(RM) hugs.exe
		  $(RM) runhugs.exe
		  $(RM) *.pdf
		  $(RM) TAGS
		  $(RM) *~
veryclean	:: distclean
veryclean	::
		  $(RM) options.h
		  $(RM) config.h
TAGS		::
		  etags *.[ych]

################################################################
# Dependencies
################################################################

.c.obj	:
		  $(CC) -c $(CFLAGS) $(OPTFLAGS) $<

dotnet\prim.obj	: dotnet\prim.cpp
		  $(CC) -c $(CFLAGS) $(OPTFLAGS) -I. dotnet\prim.cpp /Fodotnet\prim.obj

dotnet\Invoker.obj : dotnet\Invoker.cpp
		  $(CC) -c $(CFLAGS) $(OPTFLAGS) -I. dotnet\Invoker.cpp /Fodotnet\Invoker.obj

dotnet\HugsServ.obj : dotnet\HugsServ.cpp
		  $(CC) -c $(CFLAGS) $(OPTFLAGS) -I. dotnet\HugsServ.cpp /Fodotnet\HugsServ.obj

dotnet\HugsServ.cpp : dotnet\HugsServ.h

dotnet\HugsWrapper.netmodule : dotnet\HugsWrapper.cs
		  csc /t:module /unsafe /out:dotnet\HugsWrapper.netmodule dotnet\HugsWrapper.cs  /addmodule:dotnet\HugsServ.obj /addmodule:dotnet\Invoker.obj

# These are compiled with less optimisation to avoid optimisation bugs in
# certain compilers.  This may be overly conservative on some compilers.
compiler.obj	: compiler.c
		  $(CC) -c $(CFLAGS) compiler.c
static.obj	: static.c
		  $(CC) -c $(CFLAGS) static.c

parser.c	: parser.y
		  -$(YACC) parser.y
		  mv parser.tab.c parser.c

# veryclean	::
# 		  $(RM) parser.c

hugs.res	: msc\hugs.rc ..\icons\hugsicon.ico
		rc /imsc /i..\icons /r /fo hugs.res msc\hugs.rc 

runhugs.res	: msc\runhugs.rc ..\icons\hsxicon.ico
		rc /imsc /I..\icons /r /fo runhugs.res msc\runhugs.rc 

################################################################
# Dependencies
################################################################

server.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h \
		  command.h machdep.h timer.c hugs.c server.c server.h
hugs.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h \
		  command.h machdep.h timer.c
storage.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h
input.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h \
		  parser.c command.h
subst.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h subst.h
static.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h subst.h \
		  scc.c module.h
type.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h subst.h \
		  preds.c scc.c
output.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h
compiler.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h 
machine.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h
plugin.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h
builtin.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h builtin.h \
		  bignums.c printer.c iomonad.c interns.c stmonad.c array.c dirprim.c timeprim.c
ffi.obj		: $(PRELUDE) storage.h HsFFI.h connect.h errors.h
module.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h module.h
machdep.obj	: $(PRELUDE) storage.h HsFFI.h connect.h errors.h machdep.h


dotnet\HugsWrapper.netmodule: $(PRELUDE) dotnet\HugsServ.obj dotnet\Invoker.obj
dotnet\prim.obj : $(PRELUDE) dotnet\prim.cpp dotnet\Invoker.h dotnet\Invoker.cpp

################################################################
# Regression tests (none supplied)
################################################################

check		:


################################################################
# End of Makefile
################################################################