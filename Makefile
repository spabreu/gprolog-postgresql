# $Id$

# Makefile for gprolog/PostgreSQL interface

TARGET=gprolog-postgresql

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib/isco

VARIANT=-cx
GPLC=gplc$(VARIANT)

OPT=				# try -Ox when it works :-(
CFLAGS= $(OPT) \
	-fomit-frame-pointer \
	-Wall \
	-include /usr/lib/gprolog${VARIANT}/include/gprolog.h

LIBS=-lpq

OBJECTS=pl-pq-full.o
PLFILES=pl-pq-prolog.pl

all: $(TARGET)
	touch .timestamp

$(TARGET): $(PLFILES) $(OBJECTS)
	$(GPLC) -C "$(CFLAGS)" -o $(TARGET) $^ -L $(LIBS)

install: $(TARGET)
	install -c -m 555 $(TARGET) $(BINDIR)
	install -c -m 444 $(OBJECTS) $(PLFILES) $(LIBDIR)

clean::
	rm -f $(TARGET) *.o *~ \#* .timestamp

pl-pq-full.o: pl-pq.o timestamp.o
	ld -r -o $@ pl-pq.o timestamp.o -lpq -lpgtypes

timestamp.o: timestamp.c
	gcc -c \
	    $(CFLAGS) \
	    -I/usr/include/postgresql \
	    -I/usr/include/postgresql/internal \
	    -I/usr/include/postgresql/server timestamp.c

u%.o:: %.pl
	$(GPLC) -c $<

%.o:: %.c
	$(GPLC) -C "-I/usr/include/postgresql -I/usr/include/postgresql/server $(CFLAGS)" -c $<
