cc := gcc
CFLAGS := -O3 -Wall


all: clean ep3
clean:
	rm -rf *.out ep3 

ep3: ep3.c ep3.h
	${cc} ${CFLAGS} $@.c -lreadline -o $@.out




