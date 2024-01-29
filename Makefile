
PREFIX=/usr

blacklist: blacklist.c flag.h
	$(CC) -o $@ $<

install: blacklist
	mkdir -p $(PREFIX)/bin
	cp blacklist $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/blacklist

uninstall:
	rm $(PREFIX)/bin/blacklist

