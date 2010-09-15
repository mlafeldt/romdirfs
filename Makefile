CC = gcc
CFLAGS = -Wall -O2 -s
CFLAGS += $(shell pkg-config --cflags fuse)
LIBS = $(shell pkg-config --libs fuse)
PROG = romdirfs

all: $(PROG)

install: $(PROG)
	install $(PROG) /usr/local/bin

$(PROG): romdir.o romdirfs.o
	$(CC) $(CFLAGS) -o $@ $? $(LIBS)

romdir.o romdirfs.o: romdir.h

clean:
	rm -f $(PROG) *.o
