CC = gcc
CFLAGS = -Wall -Wno-unused-but-set-variable -Werror -O2 -s
CFLAGS += $(shell pkg-config --cflags fuse)
LIBS = $(shell pkg-config --libs fuse)
prefix = $(HOME)

PROG = romdirfs
OBJS += romdir.o
OBJS += romdirfs.o

all: $(PROG)

install: $(PROG)
	install $(PROG) $(prefix)/bin

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $? $(LIBS)

$(OBJS): romdir.h

clean:
	$(RM) $(PROG) $(OBJS)
