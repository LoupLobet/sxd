# See LICENCE file for copyright and licence details

VERSION = 2.0
CC = cc
PROGNAME = sxd
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man
# OpenBSD uncomment
MANPREFIX = ${PREFIX}/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

FREETYPEINC = /usr/include/freetype2
# OpenBSD uncomment
FREETYPEINC = ${X11INC}/freetype2
FREETYPELIB = -lfontconfig -lXft

XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${FREETYPELIB} $(XINERAMALIBS)

CPPFLAGS = -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=2000809L ${XINERAMAFLAGS}
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS} ${INCS}
LDFLAGS = ${LIBS}

all: clean options sxd clean_object

options:
	@echo sxd build options:
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "CC      = ${CC}"

config.h:
	cp config.def.h $@

.c.o:
	${CC} -c ${CFLAGS} $<

sxd: sxd.o drw.o utf8.o util.o
	${CC} -o $@ sxd.o drw.o utf8.o util.o ${LDFLAGS}

clean_object:
	rm -f sxd.o drw.o utf8.o util.o
clean:
	rm -f sxd sxd.o drw.o utf8.o util.o *.core
	[ -e config.h ] || cp config.def.h config.h

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -r sxd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/sxd
	sed "s/VERSION/${VERSION}/g" <sxd.1 >${DESTDIR}${MANPREFIX}/man1/sxd.1


uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/sxd
	rm -f ${DESTDIR}${MANPREFIX}/man1/sxd.1

.PHONY: all options clean install uninstall
