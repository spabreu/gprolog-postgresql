# $Id$

# Makefile for gprolog/PostgreSQL interface

TARGET=gprolog-postgresql

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib/isco

VARIANT=-cx
GPLC=gplc$(VARIANT)

CFLAGS=-C "\
	-fomit-frame-pointer\
	-Wall\
	-include /usr/lib/gprolog${VARIANT}/include/gprolog.h"
LIBS=-lpq

OBJECTS=pl-pq.o
PLFILES=pl-pq-prolog.pl

all: $(TARGET)
	touch .timestamp

$(TARGET): $(PLFILES) $(OBJECTS)
	$(GPLC) $(CFLAGS) -o $(TARGET) $^ -L $(LIBS)

install: $(TARGET)
	install -c -m 555 $(TARGET) $(BINDIR)
	install -c -m 444 $(OBJECTS) $(PLFILES) $(LIBDIR)

clean::
	rm -f $(TARGET) *.o *~ \#* .timestamp

%.o:: %.pl
	$(GPLC) -c $<

%.o:: %.c
	$(GPLC) $(CFLAGS) -c $<
