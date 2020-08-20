CCX = g++
CFLAGS = -Wall --std=c++11
LIBS = -lncursesw
PREFIX = /usr/local

gorp: gorp.o Display.o Message.o Process.o
	$(CCX) -o $@ $^ $(LIBS)

gorp.o: gorp.cpp Display.h Message.h Process.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Display.o: Display.cpp Display.h Message.h Process.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Message.o: Message.cpp Message.h
	$(CCX) -c $(CFLAGS) -o $@ $<

Process.o: Process.cpp Process.h
	$(CCX) -c $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f gorp *.o

.PHONY: install
install: gorp
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/man/man1
	gzip -c gorp.1 > $(DESTDIR)$(PREFIX)/man/man1/gorp.1.gz
