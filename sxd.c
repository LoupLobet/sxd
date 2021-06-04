/* See LICENCE file for copyright and licence details */

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include "drw.h"
#include "util.h"

/* sources */
#define _POSIX_C_SOURCE 200809L /* gcc */
#define _XOPEN_SOURCE 700

#define INTERSECT(x,y,w,h,r)  (MAX(0, MIN((x)+(w),(r).x_org+(r).width)  - MAX((x),(r).x_org)) \
                             * MAX(0, MIN((y)+(h),(r).y_org+(r).height) - MAX((y),(r).y_org)))

enum { FG, BG, BD, COLORNB };                   /* color scheme */
enum { LEFT = 1,   CENTER = 0,   RIGHT = -1 };  /* horizontal reference and text position */
enum { TOP = 1, /* CENTER = 0,*/ BOTTOM = -1 }; /* vertical reference */
enum { MANUAL, AUTO };                          /* geometry mode  */
enum { MOUSE1, MOUSE2, MOUSE3, BUTTONNB };      /* mouse buttons */

typedef union {
	float f;
	int i;
	unsigned int ui;
	char *s;
	const void *v;
} Arg;

typedef struct {
	unsigned int event;
	int oneshot;
	void (*func)(const Arg *);
	const Arg arg;
} Btn; /* mouse button */

static void	 cleanup(void);
static Fnt	 createfont(const char *, FcPattern *);
static Clr	*createscheme(const char *[], int);
static void	 getextw(char *, unsigned int, Fnt *, unsigned int *);
static int	 loadcolor(const char *, Clr *);
static void	 print(const Arg *);
static void	 readlines(FILE *);
static char	*replacetabs(char *, char *);
static void	 run(void);
static void	 spawn(const Arg *);
static void	 windraw(void);
static void	 winsetup(XWindowAttributes *);

static Fnt font;
static Window root, pwin, win;
static XIC xic;
static int linenb;
static int screen;
static Display *dpy;
static Drw *drw;
static char *embed;
static Line *lines;
static Clr *scheme;

#include "config.h"

int
main(int argc, char *argv[])
{
	XWindowAttributes wa;

	if (setlocale(LC_CTYPE, "") == NULL || !XSupportsLocale())
		error("no locale support");
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		error("cannot open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	if (embed == NULL || !(pwin = estrtol(embed, 0)))
		pwin = root;
	if (!XGetWindowAttributes(dpy, pwin, &wa))
		error("could not get window attributes: 0x%lx", pwin);
	drw = drw_create(dpy, pwin, screen, 0, 0, wa.width, wa.height);
	font = createfont(fontname, NULL);
	scheme = createscheme(colornames, COLORNB);
	readlines(stdin);

	winsetup(&wa);
	run();
}

static void
cleanup(void)
{
	free(scheme);
	drw_free(drw);
	XSync(dpy, False);
	XCloseDisplay(dpy);
}

static Fnt
createfont(const char *fname, FcPattern *fpattern)
{
	Fnt f;
	FcBool iscolorfont;
	FcPattern *pattern = NULL;
	XftFont *xfont = NULL;

	if (fname) {
		if ((xfont = XftFontOpenName(dpy, screen, fname)) == NULL)
			error("could not load font from name: %s", fname);
		if ((pattern = FcNameParse((FcChar8 *) fname)) == NULL)
			error("could not load font from pattern: %s", fname);
	} else if (fpattern) {
		if ((xfont = XftFontOpenPattern(dpy, fpattern)))
			error("could not load font from pattern");
	} else
		error("cannot load unspecified font");
	/* avoid color fonts to be used */
	if (FcPatternGetBool(xfont->pattern, FC_COLOR, 0, &iscolorfont) ==
	   FcResultMatch &&  iscolorfont) {
		XftFontClose(dpy, xfont);
		error("could not load color fonts");
	}
	f.xfont = xfont;
	f.pattern = pattern;
	return f;
}

static Clr *
createscheme(const char *cnames[], int cnb)
{
	int i;
	Clr *s = NULL;

	if (cnames == NULL || cnb < 0)
		error("color name: NULL pointer");
	if ((s = calloc(cnb, sizeof(Clr))) == NULL)
		error("could not allocate color scheme");
	for (i = 0; i < cnb; i++) {
		if (loadcolor(cnames[i], &s[i]))
			error("could not load color: %s\n", cnames[i]);
	}
	return s;
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
loadcolor(const char *cname, Clr *c)
{
	if (cname == NULL || c == NULL)
		return 1;
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
	    DefaultColormap(dpy, screen), cname, c))
		error("could not allocate color: %s", cname);
	return 0;
}

void print(const Arg *arg)
{
	if (arg->s != NULL)
		printf("%s\n", arg->s);
}

static void
readlines(FILE *fp)
{
	char buf[BUFSIZ];
	int i;
	char *p = NULL;

	for (i = 0; fgets(buf, sizeof(buf), fp); i++) {
		lines = erealloc(lines, (i + 1) * sizeof(Line));
		if ((p = strchr(buf, '\n')) != NULL)
			*p = '\0';
		lines[i].text = replacetabs(buf, tabstr);
		getextw(lines[i].text, strlen(lines[i].text), &font, &lines[i].extw);
		linenb++;
	}
}

static char *
replacetabs(char *src, char *str)
{
	int i, j;
	char *cooked = NULL;

	for (i = 0, j = 0; i < strlen(src); i++) {
		if (src[i] == '\t')
			j++;
	}
	cooked = emalloc(strlen(src) + j * (strlen(str) - 1) + 1);
	for (i = 0, j = 0; i < strlen(src); i++) {
		if (src[i] == '\t') {
			(void)strcat(cooked, str);
			j += strlen(str);
		} else {
			cooked[j] = src[i];
			j++;
		}
	}
	return cooked;
}

static void
run(void)
{
	XEvent ev;
	unsigned int button;

	drw_setscheme(drw, scheme);
	drw_setfont(drw, font);
	/* waiting for window mapping */
	if (!overredir) {
		do {
			XNextEvent(dpy, &ev);
			if (XFilterEvent(&ev, win))
				continue;
			if (ev.type == ConfigureNotify)
				drw_resize(drw, ev.xconfigure.x, ev.xconfigure.y,
				           ev.xconfigure.width, ev.xconfigure.height);
		} while (ev.type != MapNotify);
	}

	while (!XNextEvent(dpy, &ev)) {
		if (XFilterEvent(&ev, win))
			continue;

		switch (ev.type) {
		case DestroyNotify:
			if (ev.xdestroywindow.window != win)
				break;
			cleanup();
			exit(1);
		case Expose:
			if (ev.xexpose.count == 0)
				windraw();
			break;
		case ButtonPress:
		case ButtonRelease:
			button = ev.xbutton.button - 1;
			if (ev.type == mouse[button].event)
				if (button <= BUTTONNB) {
					mouse[button].func(&mouse[button].arg);
					if (mouse[button].oneshot)
						exit(0);
				}
			break;
		case VisibilityNotify:
			if (keepontop && ev.xvisibility.state != VisibilityUnobscured)
				XRaiseWindow(dpy, win);
			break;
		}
	}

}

void spawn(const Arg *arg)
{
	if (fork() == 0) {
			if (dpy)
				close(ConnectionNumber(dpy));
			setsid();
			execvp(((char **)arg->v)[0], (char **)arg->v);
			error("execvp %s", ((char **)arg->v)[0]);
	}
}

static void
windraw(void)
{
	int i, x, y;

	/* draw background */
	drw_drawrect(drw, 0, 0, drw->w, drw->h, BG);
	/* draw text */
	for (i = 0; i < linenb; i++) {
		switch(textpos) {
		case LEFT:
			x = gappx;
			break;
		case CENTER:
			x = (drw->w - lines[i].extw) / 2;
			break;
		case RIGHT:
			x = drw->w - lines[i].extw - gappx;
			break;
		}
		y = gappx + font.xfont->ascent + i * (font.xfont->ascent +
		    font.xfont->descent + linepadding);
		drw_drawtext(drw, lines[i].text, strlen(lines[i].text), &font, &scheme[FG], x, y);
	}
	drw_map(drw, win, 0, 0, drw->w, drw->h);
}

static void
winsetup(XWindowAttributes *pwa)
{
	XSetWindowAttributes swa;

	XClassHint xch = { "sxd", "sxd" };
	XIM xim;
	int i, px, py;
#ifdef XINERAMA
	XineramaScreenInfo *info = NULL;
	Window pw, xw, dw, *dws = NULL;
	unsigned int du;
	int a, area = 0, di, j, n;
#endif

	if (gmymode == AUTO) {
		w = 0;
		for (i = 0; i < linenb; i++) {
			if (lines[i].extw > w)
				w = lines[i].extw;
		}
		w += 2 * gappx;
		h = gappx * 2 + (font.xfont->ascent + font.xfont->descent) * linenb;
	}

	/* convert coordinates + references, into "X usable" coordinates */
#ifdef XINERAMA
	i = 0;
	if (pwin == root && (info = XineramaQueryScreens(dpy, &n))) {
		XGetInputFocus(dpy, &xw, &di);
		if (mon >= 0 && mon < n)
			i = mon;
		else if (xw != root && xw != PointerRoot && xw != None) {
			do {
				if (XQueryTree(dpy, (pw = xw), &dw, &xw, &dws, &du) && dws != NULL)
					XFree(dws);
			} while (xw != root && xw != pw);
			/* find the xinerama screen with which the sindow intersects most. */
			if (XGetWindowAttributes(dpy, pw, pwa)) {
				for (j = 0; j < n; j++)
					if ((a = INTERSECT(pwa->x, pwa->y, pwa->width,
					     pwa->height, info[j])) > area) {
						area = a;
						i = j;
					}
			}
		}
		if (mon < 0 && !area && drw_getpointer(drw, &px, &py))
			for (i = 0; i < n; i++)
				if (INTERSECT(px, py, 1, 1, info[i]))
					break;
		if (!usepointer) {
			if (href)
				/* expand x and h with *ref = +- 1 to understand */
				x = info[i].x_org + x + ((1 - href) / 2) *
				    (info[i].width - w - 2 * x);
			else
				x = info[i].x_org + (info[i].width - w) / 2;
			if (vref)
				y = info[i].y_org + y + ((1 - vref) / 2) *
				    (info[i].height - h - 2 * y);
			else
				y = info[i].y_org + (info[i].height - h) / 2;
		}
		XFree(info);
	} else
#endif
	{
		if (!XGetWindowAttributes(dpy, pwin, pwa))
			error("could not get window attributes: 0x%lx", pwin);
		if (!usepointer) {
			if (href)
				/* expand x and y with *ref = +- 1 to understand */
				x = x + ((1 - href) / 2) * (pwa->width - w - 2 * x);
			else
				x = (pwa->width - w) / 2;
			if (vref)
				y = y + ((1 - vref) / 2) * (pwa->height - h - 2 * y);
			else
				y = (pwa->height - h) / 2;
		}
	}
	if (usepointer) {
		printf("%d\t%d\n", x, y);
		if (drw_getpointer(drw, &px, &py))
			error("cannot query pointer");
		x = px + href * x - !href * w / 2;
		y = py + vref * y - !vref * h / 2;
	}
	drw_resize(drw, x + borderpx, y + borderpx, w, h);

	/* window setup */
	swa.override_redirect = overredir;
	swa.background_pixel = scheme[BG].pixel;
	swa.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
	                 ButtonReleaseMask | VisibilityChangeMask;
	win = XCreateWindow(dpy, pwin, x, y, w, h, borderpx, CopyFromParent,
	                    CopyFromParent, CopyFromParent, CWOverrideRedirect |
			    CWBackPixel | CWEventMask, &swa);
	XSetWindowBorder(dpy, win, scheme[BD].pixel);
	XSetClassHint(dpy, win, &xch);

	if ((xim = XOpenIM(dpy, NULL, NULL, NULL)) == NULL)
		error("cannot open input device");
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	                XNClientWindow, win, XNFocusWindow, win, NULL);
	XMapWindow(dpy, win);
	XSync(dpy, False);
}
