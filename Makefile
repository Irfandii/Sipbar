PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

CC      ?= cc
CFLAGS  += -std=c99 -Wall -Wextra -Os
LDFLAGS += -s

all: sipbar

sipbar: sipbar.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install: sipbar
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 sipbar $(DESTDIR)$(BINDIR)/sipbar

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/sipbar

clean:
	rm -f sipbar

.PHONY: all install uninstall clean
