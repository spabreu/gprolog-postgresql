GPLC=gplc-cx
LIBDIR=/usr/lib/gprolog-cx/ext/postgresql
LIBS=-L -lpq

test: test.pl
	$(GPLC) -o $@ $< $(LIBDIR)/interface.pl $(LIBDIR)/module.o $(LIBS)
