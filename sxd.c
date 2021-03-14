/* See LICENCE file for copyright and licence details */

#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>

#include "util.h"
#include "utf8.h"

/* sources */
#define _POSIX_C_SOURCE 200809L /* gcc */
#define _XOPEN_SOURCE 700

/* config.h related enumerations */
enum { FG, BG, BD, COLORNB };              /* colors */
enum { MAIN, FONTNB };                     /* fonts */
enum { DISABLE, ENABLE };                  /* option status */
enum { LEFT, TOP, CENTER, RIGHT, BOTTOM }; /* references */
enum { MOUSE1, MOUSE2, MOUSE3, BUTTONNB }; /* mouse buttons */

typedef XftColor Clr;

typedef struct Fnt {
	FcPattern *pattern;
	XftFont *xfont;
} Fnt;

typedef struct Line {
	unsigned int extw;
	struct Line *next;
	char *text;
} Line;

static GC gc;
static Window win, root, pwin;
static int screen;
static Clr *scheme;
static Fnt *fontset;
static Display *dpy;
static Line *input;
static const char *embed;
static XIC xic;
static unsigned int timer;

#include "config.h"

static Fnt 	*createfontset(const char *[], int);
static Clr	*createscheme(const char *[], int);
static void	 drawtextutf8(char *, unsigned int, Fnt *, Clr *, int, int);
static void	 drawcontent(void);
static void	 end(int);
static void	 getextw(char *, unsigned int, Fnt *, unsigned int *);
static int	 loadcolor(const char *, Clr *);
static int 	 loadfont(const char *, FcPattern *, Fnt *);
static void	 readstdin(Line **);
static void	 run(void);
static void	 usage(void);
static void	 winsetup(XWindowAttributes *);

int
main(int argc, char *argv[])
{
	XWindowAttributes wa;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {       /* version */
			fputs("sxd-"VERSION, stdout);
			exit(0);
		} else if (!strcmp(argv[i], "-aw")) /* adapte width */
			adaptw = ENABLE;
		else if (!strcmp(argv[i], "-ah"))   /* adapte height */
			adapth = ENABLE;
		else if (!strcmp(argv[i], "-hl"))   /* href LEFT */
			href = LEFT;
		else if (!strcmp(argv[i], "-hc"))   /* href CENTER */
			href = CENTER;
		else if (!strcmp(argv[i], "-hr"))   /* href RIGHT */
			href = RIGHT;
		else if (!strcmp(argv[i], "-vt"))   /* vref TOP */
			vref = TOP;
		else if (!strcmp(argv[i], "-vc"))   /* vref CENTER */
			vref = CENTER;
		else if (!strcmp(argv[i], "-vb"))   /* vref BOTTOM */
			vref = BOTTOM;
		else if (!strcmp(argv[i], "-t"))    /* keep on top */
			keepontop = ENABLE;
		else if (!strcmp(argv[i], "-s"))    /* static window */
			overrideredirect = ENABLE;
		else if (i + 1 == argc)
			usage();
		else if (!strcmp(argv[i], "-1"))    /* mouse 1 */
			outputs[MOUSE1] = argv[++i];
		else if (!strcmp(argv[i], "-2"))    /* mouse 2 */
			outputs[MOUSE2] = argv[++i];
		else if (!strcmp(argv[i], "-3"))    /* mouse 3 */
			outputs[MOUSE3] = argv[++i];
		else if (!strcmp(argv[i], "-bd"))   /* bd color */
			colors[BD] = argv[++i];
		else if (!strcmp(argv[i], "-bg"))   /* bg color */
			colors[BG] = argv[++i];
		else if (!strcmp(argv[i], "-fg"))   /* fg color */
			colors[FG] = argv[++i];
		else if (!strcmp(argv[i], "-fm"))   /* main font */
			fonts[MAIN] = argv[++i];
		else if (!strcmp(argv[i], "-g"))               /* gaps */
			wingappx = (unsigned int) estrtol(argv[++i], 10);
		else if (!strcmp(argv[i], "-t"))               /* timer */
			timer = (unsigned int) estrtol(argv[++i], 10);
		else if (!strcmp(argv[i], "-x"))               /* x */
			x = (int) estrtol(argv[++i], 10);
		else if (!strcmp(argv[i], "-y"))               /* y */
			y = (int) estrtol(argv[++i], 10);
		else if (!strcmp(argv[i], "-w"))               /* width */
			w = (unsigned int) estrtol(argv[++i], 10);
		else if (!strcmp(argv[i], "-h"))               /* hight */
			h = (unsigned int) estrtol(argv[++i], 10);
		else
			usage();
	}

	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		warning("no locale support");
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		error("connot open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	if (!embed || !(pwin = strtol(embed, NULL, 0)))
		pwin = root;
	if (!XGetWindowAttributes(dpy, pwin, &wa))
		error("could not get window attributes: 0x%lx", pwin);
	gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);

	if ((fontset = createfontset(fonts, FONTNB)) == NULL)
		error("could not create fontset");
	if ((scheme = createscheme(colors, COLORNB)) == NULL)
		error("could not create scheme");

#ifdef __OpenBSD__
	if (pledge("stdio rpath", NULL) == -1)
		error("pledge");
#endif

	readstdin(&input);
	winsetup(&wa);
	run();

	return 0;
}

static Fnt *
createfontset(const char *sfonts[], int fontnb)
{
	int i;
	Fnt *fontset = NULL;

	if (sfonts == NULL || fontnb < 0)
		return NULL;
	fontset = emalloc(fontnb * sizeof(Fnt));
	for (i = 0; i < fontnb; i++) {
		if (loadfont(sfonts[i], NULL, &fontset[i]))
			return NULL;
	}
	return fontset;
}

static Clr *
createscheme(const char *scolors[], int colornb)
{
	int i;
	Clr *clrs = NULL;

	if (scolors == NULL || colornb < 0)
		return NULL;
	if ((clrs = calloc(colornb, sizeof(Clr))) == NULL)
		error("could not allocate color scheme");
	for (i = 0; i < colornb; i++) {
		if (loadcolor(scolors[i], &clrs[i]))
			return NULL;
	}
	return clrs;
}

static void
drawtextutf8(char *text, unsigned int size, Fnt *font, Clr *color, int x, int y)
{
	char buf[1024];
	long utf8codepoint = 0;
	int charexists = 0;
	int len;
	int utf8strlen, utf8charlen;
	const char *utf8str = NULL;
	XftDraw *xftdrw = NULL;

	xftdrw = XftDrawCreate(dpy, win, DefaultVisual(dpy, screen),
	                       DefaultColormap(dpy, screen));
	utf8strlen = 0;
	utf8str = text;
	while(*text) {
		utf8charlen = utf8decode(text, &utf8codepoint, UTF_SIZ);
		charexists = charexists || XftCharExists(dpy, font->xfont, utf8codepoint);
		if (charexists) {
			utf8strlen += utf8charlen;
			text += utf8charlen;
		}
		if (!charexists)
			text++;
		else
			charexists = 0;
	}
	if (utf8strlen) {
		len = MIN(utf8strlen, sizeof(buf) - 1);
		if (len) {
			memcpy(buf, utf8str, len);
			buf[len] = '\0';
			XftDrawStringUtf8(xftdrw, color, font->xfont, x, y,
	                     (XftChar8 *) buf, len);
		}
	}
}

static void
drawcontent(void)
{
	/*
	 * draw window main content (i.e. stdin given text), with
	 * the [MAIN] font of fontset, according to configuration
	 * and command line arguments.
	 */
	int x, y;
	Line *line;

	x = wingappx;
	y = wingappx + fontset[MAIN].xfont->ascent;
	line = input;
	for (; line != NULL; line = line->next) {
		drawtextutf8(line->text, strlen(line->text),
		             &fontset[MAIN], &scheme[FG], x, y);
		y += fontset[MAIN].xfont->ascent + fontset[MAIN].xfont->descent;
	}
}

static void
getextw(char *str, unsigned int size, Fnt *font, unsigned int *extw)
{
	XGlyphInfo ext;

	if (font->xfont == NULL || str == NULL)
		return;
	XftTextExtentsUtf8(dpy, font->xfont, (XftChar8 *) str, size, &ext);
	if (extw != NULL)
		*extw = ext.xOff;
}

static int
loadcolor(const char *scolor, Clr *clr)
{
	if (scolor == NULL || clr == NULL)
		return 1;
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
			       DefaultColormap(dpy, screen), scolor, clr))
		error("could not allocate color: %s", scolor);
	return 0;
}

static int
loadfont(const char *ftname, FcPattern *ftpattern, Fnt *font)
{
	FcBool iscolorfont;
	FcPattern *pattern = NULL;
	XftFont *xfont = NULL;

	if (ftname) {
		if ((xfont = XftFontOpenName(dpy, screen, ftname)) == NULL) {
			warning("could not load font from name: %s\n", ftname);
			return 1;
		}
		if ((pattern = FcNameParse((FcChar8 *) ftname)) == NULL) {
			warning("could not load font from pattern: %s\n", ftname);
			return 1;
		}
	} else if (ftpattern) {
		if ((xfont = XftFontOpenPattern(dpy, ftpattern))) {
			warning("could not load font from pattern\n");
			return 1;
		}
	} else
		error("cannot load unspecified font");
	/* avoid color fonts to be used */
	if (FcPatternGetBool(xfont->pattern, FC_COLOR, 0, &iscolorfont) ==
	   FcResultMatch &&  iscolorfont) {
		XftFontClose(dpy, xfont);
		return 1;
	}
	font->xfont = xfont;
	font->pattern = pattern;
	return 0;
}

static void
readstdin(Line **input)
{
	char buf[BUFSIZ];
	int i;
	Line *line = NULL;
	char *p = NULL;

	if (*input != NULL)
		return;
	line = *input = emalloc(sizeof(Line));
	for (i = 0; fgets(buf, sizeof(buf), stdin); i++) {
		if ((p = strchr(buf, '\n')) != NULL)
			*p = '\0';
		if (i) {
			line->next = emalloc(sizeof(Line));
			line = line->next;
		}
		line->text = emalloc(strlen(buf) + 1);
		strcpy(line->text, buf);
		getextw(line->text, strlen(line->text), &fontset[MAIN], &line->extw);
	}
	line->next = NULL;
}

static void
run(void)
{
	XEvent ev;
	unsigned int button;

	/* set timer */
	if (timer > 0) {
		signal(SIGALRM, end);
		alarm(timer);
	}

	for (;;) {
		XNextEvent(dpy, &ev);
		switch(ev.type) {
		case ButtonPress:
			/* return string according to config.h */
			button = ev.xbutton.button;
			if (button <= BUTTONNB && !(ev.xbutton.state & WMMASK)) {
				fprintf(stdout, "%s\n", outputs[button - 1]);
				end(0);
			}
			break;
		case Expose:
			drawcontent();
			break;
		case VisibilityNotify:
			/* keep window on top */
			if (keepontop && ev.xvisibility.state != VisibilityUnobscured)
				XRaiseWindow(dpy, win);
			break;
		}
	}
}

static void
end(int sig)
{
	/*
	 * clean exit function, used when a timer was set,
	 * or when the user press mouse button on window.
	 */
	int i;

	exit(sig);
	XCloseDisplay(dpy);
	XFreeGC(dpy, gc);
	for (i = 0; i < FONTNB; i++) {
		if (fontset[i].pattern)
			FcPatternDestroy(fontset[i].pattern);
		XftFontClose(dpy, fontset[i].xfont);
	}
}

static void
usage(void)
{
	(void)fputs("usage: sxd [-aw] [-ah] [-hl|-hc|-hr] [-s] [-t] [-v] [-vt|-vc|-vb] [-1 retstr]\n"
		    "           [-2 retstr] [-3 retstr] [-bd color] [-bg color] [-fg color] [-fm font]\n"
		    "           [-g pixel] [-t sec] [-x pixel] [-y pixel] [-w pixel] [-h pixel]\n", stderr);
	exit(1);
}

static void
winsetup(XWindowAttributes *wa)
{
	XSetWindowAttributes swa;
	XClassHint xch = {progname, progname};	/* cf config.h */
	XIM xim;
	unsigned long valuemask = CWBackPixel | CWEventMask;
	unsigned int texth = 0;
	unsigned int textw = 0;
	unsigned int ww = 0, wh = 0;
	int i;
	int wx = 0, wy = 0;
	Line *line;

	line = input;
	for (i = 0; line != NULL; i++) {
		if (textw < line->extw)
			textw = line->extw;
		line = line->next;
	}
	texth = (fontset[MAIN].xfont->ascent + fontset[MAIN].xfont->descent) * i;

	/* compute window dimensions */
	if (adaptw && textw + 2 * wingappx != w)
		ww = textw + 2 * wingappx;
	else
		ww = w;
	if (adapth && texth + 2 * wingappx != h)
		wh = texth + 2 * wingappx;
	else
		wh = h;

	if (href == LEFT)
		wx = x;
	else if (href == RIGHT)
		wx = wa->width - ww - x;
	else if (href == CENTER)
		wx = (int) (0.5 * (wa->width - ww));
	else
		error("unknow horizontal reference: %s", href);
	if (vref == TOP)
		wy = y;
	else if (vref == BOTTOM)
		wy = wa->height - wh - y;
	else if (href == CENTER)
		wy = (int) (0.5 * (wa->height - wh));
	else
		error("unknow vertical reference: %s", vref);

	swa.background_pixel = scheme[BG].pixel;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | VisibilityChangeMask;
	swa.override_redirect = True;
	if (overrideredirect)
		valuemask = valuemask | CWOverrideRedirect;
	win = XCreateWindow(dpy, pwin, wx, wy, ww, wh, borderpx,
	                    CopyFromParent, CopyFromParent, CopyFromParent,
	                    valuemask, &swa);
	XSetWindowBorder(dpy, win, scheme[BD].pixel);
	XSetClassHint(dpy, win, &xch);

	if ((xim = XOpenIM(dpy, NULL, NULL, NULL)) == NULL)
		error("XOpenIM: could not open input device");
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	                XNClientWindow, win, XNFocusWindow, win, NULL);
	XMapRaised(dpy, win);
}
