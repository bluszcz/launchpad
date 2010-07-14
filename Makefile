LDFLAGS=-lusb-1.0 -llo -lpthread -lasound

lpmidi: 
	gcc -lusb-1.0 -lpthread -lasound -o lpmidi lpmidi.c liblaunchpad.c

lposc:
	gcc -lusb-1.0 -lpthread -llo -o lposc lposc.c liblaunchpad.c

clean:
	rm -f *.o lpmidi lposc
