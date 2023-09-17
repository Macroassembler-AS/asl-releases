# -------------------------------------------------------------------------
# choose your compiler (must be ANSI-compliant!) and linker command, plus
# any additionally needed flags

OBJDIR =
CC = gcc
CFLAGS = -g -O3 -march=i586 -fomit-frame-pointer -Wall
#                    ^^^^^
#                    |||||
# adapt this to your target cpu (386/486 or higher)
# note that older gcc versions require -m[34]86 or -mcpu=i[34]86 instead of -march=i[34]86.
# @GNU: why does this have to change every two years ?!
HOST_OBJEXTENSION = .o
LD = gcc
LDFLAGS =
HOST_EXEXTENSION =

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
