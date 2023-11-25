all: client.exe server.exe

%.exe: %.o
	gcc $< -o $@

%.o: %.c
	gcc -Wall -c $< -o $@

clean:
	rm -f *.o *.exe
