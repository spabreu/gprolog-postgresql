GPLC=gplc-cx
LIBDIR=/usr/lib/gprolog-cx/ext/postgresql
LIBS=-L -lpq

all: test bench test2 bench2

test: test.pl
	$(GPLC) -o $@ test.pl $(LIBDIR)/interface.pl $(LIBDIR)/module.o $(LIBS)

bench: test.pl bench.pl
	$(GPLC) -o $@ test.pl bench.pl $(LIBDIR)/interface.pl $(LIBDIR)/module.o $(LIBS)

test2: test2.pl
	$(GPLC) -o $@ test2.pl $(LIBDIR)/interface.pl $(LIBDIR)/module.o $(LIBS)

bench2: test2.pl bench.pl
	$(GPLC) -o $@ test2.pl bench.pl $(LIBDIR)/interface.pl $(LIBDIR)/module.o $(LIBS)

clean:
	rm -f *~ \#*
	rm -f *.o
	rm -f bench test

times-all.txt:: bench2
	for X in date number int codes atom;		\
	do						\
	    for N in 1 10 20 50;			\
	    do						\
		for T in x y;				\
		do					\
		    for B in 0 1;			\
		    do					\
			for K in 1 2 3;			\
			do				\
			    ./bench2 $$N $$B $$T'_'$$X;	\
			done;				\
		    done;				\
		done;					\
	    done;					\
	done 1>&times-all.txt
