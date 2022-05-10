CFLAGS := -O2 -Wall -Wextra -Werror
LDFLAGS :=
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
	-rm -f rl78flash rl78flash.exe rl78g10flash rl78g10flash.exe src/*.o src/*~ *~ *.deb *.zip *.tar.gz ./rl78flash-* ./rl78flash_*

install: rl78flash rl78g10flash
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 -t $(DESTDIR)$(PREFIX)/bin $^

ifneq ($(MAKECMDGOALS),clean)

TAR ?= tar
TAG := $(shell git describe --tags)
VERSION := $(TAG:v%=%)
IS_x86_64 := $(shell $(CC) -dumpmachine | grep x86_64)
ARCH := $(if $(IS_x86_64),amd64,i386)

ifeq ($(SUFFIX),)
SUFFIX := $(ARCH)
endif

ZIP_NAME ?= rl78flash-$(VERSION)-$(SUFFIX)
TARGZ_NAME ?= rl78flash-$(VERSION)-$(SUFFIX)
DEB_NAME := rl78flash_$(VERSION)_$(ARCH)

zip: $(ZIP_NAME).zip

$(ZIP_NAME).zip: rl78flash.exe rl78g10flash.exe README.md
	rm -rf ./$(ZIP_NAME) $@
	mkdir -p ./$(ZIP_NAME)
	cp $^ ./$(ZIP_NAME)
	zip $@ ./$(ZIP_NAME)/*
	rm -rf ./$(ZIP_NAME)

tar.gz: $(TARGZ_NAME).tar.gz

$(TARGZ_NAME).tar.gz: rl78flash rl78g10flash README.md
	rm -rf ./$(TARGZ_NAME) $@
	mkdir -p ./$(TARGZ_NAME)
	cp $^ ./$(TARGZ_NAME)
	$(TAR) --owner=root --group=root -caf $@ ./$(TARGZ_NAME)/*
	rm -rf ./$(TARGZ_NAME)

deb: $(DEB_NAME).deb

$(DEB_NAME)/DEBIAN:
	mkdir -p $@

$(DEB_NAME)/DEBIAN/control: rl78flash.control.in | $(DEB_NAME)/DEBIAN
	cat $< | \
		sed -e "s|@VERSION@|$(VERSION)|" | \
		sed -e "s|@ARCH@|$(ARCH)|" > $@

$(DEB_NAME).deb: rl78flash rl78g10flash $(DEB_NAME)/DEBIAN/control
	mkdir -p $(DEB_NAME)/usr/bin
	cp rl78flash rl78g10flash $(DEB_NAME)/usr/bin
	fakeroot dpkg-deb --build $(DEB_NAME)
	rm -rf $(DEB_NAME)

endif
