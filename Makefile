#%ifswitch --diet diet DIET
#%setifdef DIET -nostdinc DIETINC
#%setifdef DIET -L/opt/diet/lib DIETLIB
#%prepifdef DIET /opt/diet/lib:/opt/diet/include
#%dir jelio.h JELIOINC -I
#%dir libjelio.a JELIOLIB -L
#%seteval X86 uname -m|grep i.86
#%setifdef X86 -march=i486 ARCH
#?V=`cat version.txt|cut -d ' ' -f 2`
#%switch --prefix PREFIX
#%switch --mandir MANDIR
#%switch --sysconfdir SYSCONFDIR
#%ifswitch --syslog -DUSE_SYSLOG SYSLOG
#%ifnswitch --prefix /usr PREFIX
#%ifnswitch --sysconfdir /etc SYSCONFDIR
#%ifnswitch --mandir $(PREFIX)/share/man MANDIR
#?CFLAGS = -DVERSION=\"$(V)\" -DPREFIX=\"$(PREFIX)\" -DSYSCONFDIR=\"$(SYSCONFDIR)\"  -DMANDIR=\"$(MANDIR)\" -Wall -Os -g $(SYSLOG)
#?CC=$(DIET) gcc $(ARCH)
#?LDFLAGS=$(DIETLIB)
#?all:	arp-monitor
#?arp-monitor:	arp-monitor.o jelopt.o jelist.o
#?	$(CC) $(LDFLAGS) -o arp-monitor arp-monitor.o jelopt.o jelist.o -ljelio
#?install:	arp-monitor
#?	mkdir -p $(DESTDIR)/$(PREFIX)/bin $(DESTDIR)/$(MANDIR)/man1
#?	strip arp-monitor
#?	cp -f arp-monitor $(DESTDIR)/$(PREFIX)/bin
#?	cp -f man/*.1 $(DESTDIR)/$(MANDIR)/man1
#?clean:
#?	rm -f *.o arp-monitor
#?tar:	clean
#?	make-tarball.sh
#?manpage:	arp-monitor.mn
#?	./mk-cli-manpage.sh monitoring 1 "Jens Låås"
JELIOINC=-I/usr/include
JELIOLIB=-L/usr/lib
PREFIX= /usr
SYSCONFDIR= /etc
MANDIR= $(PREFIX)/share/man
V=`cat version.txt|cut -d ' ' -f 2`
CFLAGS = -DVERSION=\"$(V)\" -DPREFIX=\"$(PREFIX)\" -DSYSCONFDIR=\"$(SYSCONFDIR)\"  -DMANDIR=\"$(MANDIR)\" -Wall -Os -g $(SYSLOG)
CC=$(DIET) gcc $(ARCH)
LDFLAGS=$(DIETLIB)
all:	arp-monitor
arp-monitor:	arp-monitor.o jelopt.o jelist.o
	$(CC) $(LDFLAGS) -o arp-monitor arp-monitor.o jelopt.o jelist.o -ljelio
install:	arp-monitor
	mkdir -p $(DESTDIR)/$(PREFIX)/bin $(DESTDIR)/$(MANDIR)/man1
	strip arp-monitor
	cp -f arp-monitor $(DESTDIR)/$(PREFIX)/bin
	cp -f man/*.1 $(DESTDIR)/$(MANDIR)/man1
clean:
	rm -f *.o arp-monitor
tar:	clean
	make-tarball.sh
manpage:	arp-monitor.mn
	./mk-cli-manpage.sh monitoring 1 "Jens Låås"
