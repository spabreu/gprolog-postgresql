GPLC=gplc-cx
LIBDIR=/usr/lib/gprolog-cx/ext/postgresql
LIBS=-L -lpq

all: test bench

test: test.pl
	$(GPLC) -o $@ test.pl $(LIBDIR)/interface.pl $(LIBDIR)/module.o $(LIBS)

bench: test.pl bench.pl
	$(GPLC) -o $@ test.pl bench.pl $(LIBDIR)/interface.pl $(LIBDIR)/module.o $(LIBS)

clean:
	rm -f *~ \#*
	rm -f *.o
	rm -f bench test
