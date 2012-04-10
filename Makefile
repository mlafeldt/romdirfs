CC = gcc
INSTALL = install
CFLAGS = -Wall -Werror -O2 -s
CFLAGS += $(shell pkg-config --cflags fuse)
LIBS = $(shell pkg-config --libs fuse)
prefix = $(HOME)

PROG = romdirfs
OBJS += romdir.o
OBJS += romdirfs.o

all: $(PROG)

install: $(PROG)
	$(INSTALL) -d -m 755 '$(prefix)/bin/'
	$(INSTALL) $(PROG) '$(prefix)/bin/'

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $? $(LIBS)

$(OBJS): romdir.h

clean:
	$(RM) $(PROG) $(OBJS)
