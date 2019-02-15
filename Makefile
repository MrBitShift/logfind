CFLAGS=-Wall -g

clean:
	rm -f logfind

all:
	rm -f logfind
	make logfind
	./logfind
