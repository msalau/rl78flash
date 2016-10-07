CFLAGS := -O2 -Wall -Wextra -Werror
LDFLAGS := -s
LIBS := -lpthread

PREFIX ?= /usr/local

OBJS := src/rl78.o src/main.o src/srec.o src/wait_kbhit.o
OBJS_G10 := src/rl78g10.o src/main_g10.o src/srec.o src/crc16_ccit.o src/wait_kbhit.o
OBJS_LINUX := src/terminal.o src/serial.o
OBJS_WIN32 := src/terminal_win32.o src/serial_win32.o

.PHONY: all win32 clean install zip deb

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

IS_x86_64 := $(shell $(CC) -dumpmachine | grep x86_64)
ARCH := $(if $(IS_x86_64),win64,win32)
VERSION := $(shell git describe --tags)
NAME := rl78flash-$(VERSION:v%=%)-$(ARCH)

zip: $(NAME).zip

$(NAME).zip: rl78flash.exe rl78g10flash.exe README.md
	mkdir -p ./$(NAME)
	cp $^ ./$(NAME)
	zip $@ ./$(NAME)/*
	rm -rf ./$(NAME)

endif

ifeq ($(MAKECMDGOALS),deb)

IS_x86_64 := $(shell $(CC) -dumpmachine | grep x86_64)
ARCH := $(if $(IS_x86_64),amd64,i386)
TAG := $(shell git describe --tags)
VERSION := $(TAG:v%=%)
NAME := rl78flash_$(VERSION)_$(ARCH)

deb: $(NAME).deb

$(NAME)/DEBIAN:
	mkdir -p $@

$(NAME)/DEBIAN/control: rl78flash.control.in | $(NAME)/DEBIAN
	cat $< | \
		sed -e "s|@VERSION@|$(VERSION)|" | \
		sed -e "s|@ARCH@|$(ARCH)|" > $@

$(NAME).deb: rl78flash rl78g10flash $(NAME)/DEBIAN/control
	mkdir -p $(NAME)/usr/bin
	cp rl78flash rl78g10flash $(NAME)/usr/bin
	fakeroot dpkg-deb --build $(NAME)
	rm -rf $(NAME)

endif
