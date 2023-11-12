target1 = pinger
target2 = ping

all: $(target1).exe $(target2).exe

$(target1).exe: $(target1).o
	gcc $(target1).o -o $(target1).exe

$(target1).o: src/$(target1).c
	gcc -g -c src/$(target1).c

$(target2).exe: $(target2).o
	gcc $(target2).o -o $(target2).exe

$(target2).o: src/$(target2).c
	gcc -g -c src/$(target2).c

clean:
	rm -f *.o *.exe

