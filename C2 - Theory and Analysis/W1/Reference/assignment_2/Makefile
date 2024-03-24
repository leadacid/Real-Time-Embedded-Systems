INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -Wall -O3 -pthread -I. $(INCLUDE_DIRS) $(CDEFS)
LDFLAGS= -pthread -lrt -I.
LIBS= 
DEPS = realtime.h

HFILES= realtime.h
CFILES= realtime.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	sequencer

clean:
	-rm -f *.o *.d
	-rm -f sequencer

sequencer: sequencer.o realtime.o
	$(CC) -o $@ $^ $(LDFLAGS) 

depend:

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
