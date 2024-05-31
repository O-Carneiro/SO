cc := gcc
CFLAGS := -Wall -ansi


all: newsh ep1
clean:
	rm -rf *.out ep1 newsh ou*

newsh: newsh.c newsh.h
	${cc} ${CFLAGS} $@.c -lreadline -o $@
ep1: ep1.c ep1.h
	${cc} ${CFLAGS} $@.c -o $@




