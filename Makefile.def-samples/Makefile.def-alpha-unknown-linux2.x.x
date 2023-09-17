# -------------------------------------------------------------------------
# choose your compiler (must be ANSI-compliant!) and linker command, plus
# any additionally needed flags

OBJDIR =
CC = gcc
CFLAGS = -O3 -fomit-frame-pointer -mtune=ev56 -mcpu=ev56 -Wall 
HOST_OBJEXTENSION = .o
LD = gcc
LDFLAGS =
HOST_EXEXTENSION = .o

# no cross build

TARG_OBJDIR = $(OBJDIR)
TARG_CC = $(CC)
TARG_CFLAGS = $(CFLAGS)
TARG_OBJEXTENSION = $(HOST_OBJEXTENSION)
TARG_LD = $(LD)
TARG_LDFLAGS = $(LDFLAGS)
TARG_EXEXTENSION = $(HOST_EXEXTENSION)

# -------------------------------------------------------------------------
# directories where binaries, includes, and manpages should go during
# installation

BINDIR = /usr/local/bin
INCDIR = /usr/local/include/asl
MANDIR = /usr/local/man
LIBDIR =
DOCDIR = /usr/local/doc/asl
