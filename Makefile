VERSION = 1.0
CC = cc
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man
PROGNAME = sxd

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib
FREETYPELIB = -lfontconfig -lXft
FREETYPEINC = ${X11INC}/freetype2
INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${FREETYPELIB}

CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS} ${INCS}
LDFLAGS = ${LIBS}

all: clean options sxd clean_object

options:
	@echo sxd build options:
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "CC      = ${CC}"

.c.o :
	${CC} -c ${CFLAGS} $<

sxd: sxd.o util.o utf8.o
	${CC} -o $@ sxd.o util.o utf8.o ${LDFLAGS}

clean_object:
	rm -f sxd.o util.o utf8.o
clean:
	rm -f sxd sxd.o util.o utf8.o *.core

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -r sxd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/sxd
	sed "s/VERSION/${VERSION}/g" <sxd.1 >${DESTDIR}${MANPREFIX}/man1/sxd.1


uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/sxd
	rm -f ${DESTDIR}${MANPREFIX}/man1/sxd.1

.PHONY: all options clean install uninstall
