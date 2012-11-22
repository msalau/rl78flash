CFLAGS = -O2 -Wall -Wextra
LDFLAGS += -lpthread

rl78flash: src/rl78.o src/serial.o src/main.o src/srec.o src/terminal.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f rl78flash{,.exe} src/*.o src/*~ *~

cygwin:
	make CC=i686-pc-cygwin-gcc
