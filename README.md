SXD(1)                      General Commands Manual                     SXD(1)



NAME
       sxd - Simple X Displayer

SYNOPSIS
       sxd [-kopuv] [-ga|-gm] [-hc|-hl|-hr] [-tc|-tl|-tr] [-vb|-vc|-vt] [-bd
       color] [-bg color] [-bx pixel] [-fg color] [-ft font] [-g geometry]
       [-gx pixel] [-h pixel] [-lp pixel] [-m profile] [-t string] [-w pixel]
       [-x pixel] [-y pixel]

DESCRIPTION
       sxd is a mouse-oriented, multi-purpose text displayer for X, which
       reads some text lines from standard input, and prints them in window.
       When a mouse button is pressed over the created window, a action is
       executed accordingly to the selected mouse profile (see -p option).
       Mouse profiles are defined by three Btn structures composed of the
       following fields:

              Event  can be ButtonPress or ButtonRelease ,and specifies the X
                     event which executes the action.

              Oneshot
                     can be 0 or 1, and specifies if the button event close
                     sxd. The associated action will be excuted before sxd
                     terminates.

              Func   can be one of the defined functions in sxd.c (e.g.  print
                     or spawn) , that takes a 'const Arg *' as argument. The
                     user can write custom functions to fit his needs.

              Arg    corresponds to one of the fileds of the Arg union. It can
                     be .f for a floating point number, .i for a integer, .u
                     for an unsigned integer, .s for a char pointer, .v for a
                     pointer of an undefined type.

       Profiles are defined in the config.h file, embeded in the "profiles"
       array. A profile is written the following way:

       { "profile name", {
            [MOUSE1] = { Event,    Oneshot,    Func,    { Arg = value } },
            [MOUSE2] = { Event,    Oneshot,    Func,    { Arg = value } },
            [MOUSE3] = { Event,    Oneshot,    Func,    { Arg = value } }, }
       },

       The sxd placement works with an horizontal and vertical "reference".
       Because the sxd window placement works with x,y coordinates,
       "references" are used to place 0,0 at the intersection the two
       references. Thoses references are:

              horizontal
                     possible values are CENTER, LEFT, and RIGHT, and
                     respectively represente: the horizontal center of the
                     screen, the left border of the screen, and the right
                     border of the screen.

              vertical
                     possible values are BOTTOM, CENTER, and TOP, and
                     respectively represente: the bottom border of the screen,
                     the vertical center of the screen, and the top border of
                     the screen.

       The x,y coordinates are the distance in pixel between 0,0 and the
       projection of 0,0 on the window (e.g. if the horizontal reference is
       LEFT and, the vertical reference is CENTER, the x,y coordinates are the
       distance between the center of the left border of the screen, and the
       center of the left border of the window).
       The pointer can also be used as the 0,0 cordinates, in this case 0,0 is
       computed the same way (i.e. with the horizontal and vertical
       references), and then moved to the pointer.


OPTIONS
       -k     Keeps the window on the top of other windows.

       -o     Overrides redirections from the window manager.

       -p     Sets the pointer as the 0,0 coordinates.

       -u     Shows the sxd usage.

       -v     Shows the sxd version.

       -ga|-gm
              Sets the window geometry to automatic|manual, in automatic mode,
              the window width and height are computed to fit the text.

       -hc|-hl|-hr
              Sets the horizontal reference to the center|the left|the right
              border of the screen.

       -tc|-tl|-tr
              Aligns the text with the center|the left|the right side of the
              window.

       -vb|-vc|-vt
              Sets the vertical reference to the bottom|the center|the top
              border of the screen.

       -bd  color
              Sets the window border color. Supported formats are #RGB,
              #RRGGBB.

       -bg  color
              Sets the window background color. Supported formats are #RGB,
              #RRGGBB.

       -bx  pixel
              Sets the window border width.

       -fg  color
              Sets the window background color. Supported formats are #RGB,
              #RRGGBB.

       -ft  font
              Sets the text font.

       -g  geometry
              Sets the window geometry. The form is
              [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]. See
              XParseGeometry(3)

       -gx  pixel
              Sets the gaps width around the text.

       -h  pixel
              Sets the window height.

       -lp  pixel
              Sets the padding between text lines.

       -m  profile
              Sets the mouse profile to use.

       -t  string
              Sets the string to replace tabs in the text.

       -w  pixel
              Sets the window width.

       -x  pixel
              Sets the window x coordinates.

       -y  pixel
              Sets the window y coordinates.

SEE ALSO
       91menu(1)



                                    SXD-2.0                             SXD(1)
