# if you type 'make' without arguments, this is the default
PROG = extract
all: $(PROG)

# other source files and the associated object files (this can be blank) SRC is unused here
SRC     = extract.c subs.c misc.c
HEAD1   = misc.h
HEAD2   = subs.h
OBJ     = extract.o subs.o misc.o

# Testing
TDIR	= ./tests

# special libraries
LIB	=

# select the compiler and flags yopu can over-ride on command line e.g. make DEBUG= 
CC	= gcc
DEBUG	= -ggdb
CSTD	=
WARN	= -Wall -Wextra -Werror
CDEFS	=
CFLAGS	= -I. $(DEBUG) $(WARN) $(CSTD) $(CDEFS)

#list of .h dependencies
extract.o:	$(HEAD1) $(HEAD2)
misc.o:     $(HEAD1)
subs.o:     $(HEAD1) $(HEAD2)

# specify how to compile the target
$(PROG):	$(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIB) -o $@

# test the program
test:	$(PROG)
	(cd $(TDIR); make $@)

# remove binaries
.PHONY: clean clobber
clean:
	rm -f $(OBJ) $(PROG)
	(cd $(TDIR); make $@)

# remove binaries and other junk
clobber: clean
	rm -f core
