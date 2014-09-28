CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -s
LIBS = -lpthread

OBJS = src/rl78.o src/main.o src/srec.o src/wait_kbhit.o
OBJS_G10 = src/rl78g10.o src/main_g10.o src/srec.o src/crc16_ccit.o src/wait_kbhit.o
OBJS_LINUX = src/terminal.o src/serial.o
OBJS_WIN32 = src/terminal_win32.o src/serial_win32.o

all: rl78flash rl78g10flash

rl78flash: $(OBJS) $(OBJS_LINUX)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

rl78flash.exe: $(OBJS) $(OBJS_WIN32)
	$(CC) $(LDFLAGS) -o $@ $^

rl78g10flash: $(OBJS_G10) $(OBJS_LINUX)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

rl78g10flash.exe: $(OBJS_G10) $(OBJS_WIN32)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f rl78flash rl78flash.exe rl78g10flash rl78g10flash.exe src/*.o src/*~ *~

cygwin:
	make CC=i686-pc-cygwin-gcc

win32:
	make CC=i686-pc-mingw32-gcc rl78flash.exe rl78g10flash.exe

install: rl78flash rl78g10flash
	mkdir -p $(DESTDIR)/usr/bin
	install -m 755 -t $(DESTDIR)/usr/bin $<
