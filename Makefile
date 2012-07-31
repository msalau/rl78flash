CFLAGS = -O2

rl78flash: src/rl78.o src/serial.o src/main.o src/srec.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f rl78flash src/*.o src/*~ *~

