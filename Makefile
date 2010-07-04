CC=gcc
LDFLAGS="-lusb-1.0 -llo"
SOURCES=liblaunchpad.c test.c lposc.c
OBJECTS=liblaunchpad.o test.o lposc.o
EXECUTABLE = lposc

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

liblaunchpad.o:
	$(CC) -c liblaunchpad.c

test.o:
	$(CC) -c test.c

lposc.o:
	$(CC) -c lposc.c

clean:
	rm -f *.o test lposc