all: ping.exe pinger.exe

%.exe: %.o
	gcc $< -o $@

%.o: %.c
	gcc -c $< -o $@

clean:
	rm -f *.o *.exe
