CFLAGS= -ggdb -I$(NSYSTEM)/include
LFLAGS= -ggdb

all: testh2o nuevotesth2o
#	./testh2o
#	./nuevotesth2o

testh2o.o: testh2o.c h2o.h
	gcc $(CFLAGS) -c testh2o.c

nuevotesth2o.o: nuevotesth2o.c h2o.h
	gcc $(CFLAGS) -c nuevotesth2o.c

h2o.o: h2o.c h2o.h
	gcc $(CFLAGS) -c h2o.c

testh2o: testh2o.o h2o.o
	gcc $(LFLAGS) testh2o.o h2o.o $(NSYSTEM)/lib/libnSys.a -o testh2o

nuevotesth2o: nuevotesth2o.o h2o.o
	gcc $(LFLAGS) nuevotesth2o.o h2o.o $(NSYSTEM)/lib/libnSys.a -o nuevotesth2o

clean:
	rm -f *.o *~

cleanall:
	rm -f *.o *~ testh2o nuevotesth2o
