# if you type 'make' without arguments, this is the default
PROG = test
all: $(PROG)

# Testing
SCRIPT	= ./runtests
OUT     = ./out/*

.PHONY: test clean clobber
# test the program
test:
	echo Running Public Tests
	$(SCRIPT)

# remove output files
clobber: clean

clean:
	rm -f $(OUT)
