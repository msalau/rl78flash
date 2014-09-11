CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -s
LIBS = -lpthread

OBJS = src/rl78.o src/main.o src/srec.o src/wait_kbhit.o
OBJS_LINUX = src/terminal.o src/serial.o
OBJS_WIN32 = src/terminal_win32.o src/serial_win32.o

rl78flash: $(OBJS) $(OBJS_LINUX)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

rl78flash.exe: $(OBJS) $(OBJS_WIN32)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f rl78flash{,.exe} src/*.o src/*~ *~

cygwin:
	make CC=i686-pc-cygwin-gcc

win32:
	make CC=i686-pc-mingw32-gcc rl78flash.exe

install: rl78flash
	mkdir -p $(DESTDIR)/usr/bin
	install -m 755 -t $(DESTDIR)/usr/bin $<
