/* See LICENCE file for copyright and licence details */

static char *tabstr  = "        "; /* define a printable string for '\t' characters */
static int overredir = 1;          /* override window redirection by the window manger */
static int keepontop = 0;          /* raise the window every time the visibility is obscured */
static int mon       = -1;         /* monitor, < 0 mean doesn't use particular monitor */

/* appearence */
static int gappx    = 10; /* window gap pixels */
static int borderpx = 1;  /* window border pixels */

/* font */
static char *fontname = "GoMono Nerd Font:size=10";

/* window placement */
static int usepointer = 0;    /* use the corsor as a top-left reference (0|1) */
static int href       = LEFT; /* horizontal reference (LEFT|CENTER|RIGH) */
static int vref       = TOP;  /* vertical reference (TOP|CENTER|BOTTOM) */
static int x          = 0;  /* x coordinates of the vref-href window corner */
static int y          = 0;  /* y coordinates of the vref-href window corner */

/* window geometry */
static int gmymode    = AUTO; /* geometry mode (MANUAL|AUTO) */
static unsigned int w = 500;  /* window width */
static unsigned int h = 250;  /* window height */

/* text */
static int textpos     = LEFT; /* text position in the window */
static int linepadding = 0;    /* padding pixels between lines */

/* color scheme */
static const char *colornames[COLORNB] = {
	[FG] = "#586e75",	/* foreground */
	[BG] = "#002b36",	/* background */
	[BD] = "#586e75",	/* border */
};

static char *menucmd[] = { "menu", NULL };

/* mouse profiles */
static char *mouseprofile = "clock";
static Profile profiles[] = {
	{ "default", {
	/*  Button       Event             Oneshot   Function      Argument */
		[MOUSE1] = { ButtonRelease,    0,        print,        { .s = "main1" } },
		[MOUSE2] = { ButtonPress,      1,        print,        { .s = "main2" } },
		[MOUSE3] = { ButtonRelease,    0,        print,        { .s = "main3" } }, } },
	{ "notification", {
		[MOUSE1] = { ButtonRelease,    0,        print,        { .s = "notification1" } },
		[MOUSE2] = { ButtonPress,      1,        print,        { .s = "notification2" } },
		[MOUSE3] = { ButtonRelease,    0,        print,        { .s = "notification3" } }, } },
	{ "clock", {
		[MOUSE1] = { ButtonRelease,    0,        print,        { .s = "clock1" } },
		[MOUSE2] = { ButtonPress,      1,        print,        { .s = "clock2" } },
		[MOUSE3] = { ButtonPress,      0,        spawn,        { .v = menucmd } }, } },
};
