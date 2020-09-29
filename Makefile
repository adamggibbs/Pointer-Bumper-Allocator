CC            = gcc
SPECIAL_FLAGS = -ggdb -Wall -DDEBUG_ALLOC
#SPECIAL_FLAGS = -ggdb -Wall
CFLAGS        = -std=gnu99 $(SPECIAL_FLAGS)

all: libpb libbf memtest

libpb: pb-alloc.o safeio.o
	$(CC) $(CFLAGS) -fPIC -shared -o libpb.so pb-alloc.o safeio.o

pb-alloc.o: pb-alloc.c safeio.h
	$(CC) $(CFLAGS) -c pb-alloc.c

libbf: bf-alloc.o safeio.o
	$(CC) $(CFLAGS) -fPIC -shared -o libbf.so bf-alloc.o safeio.o

bf-alloc.o: bf-alloc.c safeio.h
	$(CC) $(CFLAGS) -c bf-alloc.c

libsf: sf-alloc.o safeio.o
	$(CC) $(CFLAGS) -fPIC -shared -o libsf.so sf-alloc.o safeio.o

memtest: memtest.c
	$(CC) $(CFLAGS) -o memtest memtest.c

safeio.o: safeio.c safeio.h
	$(CC) $(CFLAGS) -c safeio.c

docs:
	doxygen

clean:
	rm -rf *.o *.so memtest
