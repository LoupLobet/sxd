/* See LICENCE file for copyright and licence details */

#define WMMASK Mod4Mask		           /* window manager key */

static char *progname = "sxd";         /* program name */

/* location */
static int x = 50;                     /* default x coordinate */
static int y = 50;                     /* default y coordinate */
static int href = CENTER;              /* horizontal reference { LEFT, CENTER, RIGHT } */
static int vref = CENTER;              /* vertical reference   { TOP, CENTER, BOTTOM } */

/* options { DISABLE, ENABLE } */
static int adaptw = ENABLE;            /* adapt width to text */
static int adapth = ENABLE;            /* adapt height to text */
static int keepontop = ENABLE;         /* keep window on top */
static int overrideredirect = ENABLE;  /* move/resize window */

/* default geometry */

static unsigned int w = 1000;          /* default width */
static unsigned int h = 100;           /* default height */
static unsigned int borderpx = 3;      /* border width */
static unsigned int wingappx = 10;     /* window gaps */

/* color scheme */
static const char *colors[COLORNB] = {
	[FG] = "#000000",
	[BG] = "#ffffff",
	[BD] = "#57a8a8",
};

/* fontset */
static const char *fonts[FONTNB] = {
	[MAIN] = "GoMono Nerd Font:pixelsize=12:antialias=true:autohint=true",
};

/* mouse buttons */
static char *outputs[BUTTONNB] = {
	[MOUSE1] = "1",
	[MOUSE2] = "2",
	[MOUSE3] = "3",
};
