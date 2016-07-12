CFLAGS = -O2 -Wall -Wextra
LDFLAGS = -s
LIBS = -lpthread

PREFIX ?= /usr/local

OBJS = src/rl78.o src/main.o src/srec.o src/wait_kbhit.o
OBJS_G10 = src/rl78g10.o src/main_g10.o src/srec.o src/crc16_ccit.o src/wait_kbhit.o
OBJS_LINUX = src/terminal.o src/serial.o
OBJS_WIN32 = src/terminal_win32.o src/serial_win32.o

.PHONY: all win32 clean install zip

all: rl78flash rl78g10flash

win32: rl78flash.exe rl78g10flash.exe

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

install: rl78flash rl78g10flash
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 -t $(DESTDIR)$(PREFIX)/bin $^

ifeq ($(MAKECMDGOALS),zip)

NAME := rl78flash-$(shell git describe --tags)

zip: $(NAME).zip

$(NAME).zip: rl78flash.exe rl78g10flash.exe README.md
	mkdir -p ./$(NAME)
	cp $^ ./$(NAME)
	zip $@ ./$(NAME)/*
	rm -rf ./$(NAME)

endif
