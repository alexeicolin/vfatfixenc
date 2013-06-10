all: lsdir fixenc

lsdir: lsdir.c
	gcc -o lsdir lsdir.c

fixenc: fixenc.c
	gcc -o fixenc fixenc.c -liconv

clean:
	rm -f *.o fixenc lsdir
