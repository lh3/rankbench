CC=			gcc
CFLAGS=		-g -Wall -Wc++-compat -O3
CPPFLAGS=
INCLUDES=
OBJS=		rld0.o b2b.o rle.o rope.o mrope.o
PROG=		fmd2b2b rankbench
LIBS=		-lpthread -lz -lm

ifneq ($(asan),)
	CFLAGS+=-fsanitize=address
	LIBS+=-fsanitize=address -ldl
endif

.SUFFIXES:.c .o
.PHONY:all clean depend

.c.o:
		$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@

all:$(PROG)

fmd2b2b:$(OBJS) fmd2b2b.o
		$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

rankbench:$(OBJS) bench.o
		$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
		rm -fr *.o a.out $(PROG) *~ *.a *.dSYM

depend:
		(LC_ALL=C; export LC_ALL; makedepend -Y -- $(CFLAGS) $(CPPFLAGS) -- *.c)

# DO NOT DELETE

b2b.o: b2b.h
bench.o: mrope.h rope.h rld0.h b2b.h
fmd2b2b.o: rld0.h b2b.h
mrope.o: mrope.h rope.h rle.h
rld0.o: rld0.h
rle.o: rle.h
rope.o: rle.h rope.h
