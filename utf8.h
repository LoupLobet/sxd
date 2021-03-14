/* See LICENCE file for copyright and licence details */

#define UTF_SIZ     4

long	 utf8decodebyte(const char, int *);
int	 utf8validate(long *, int);
int	 utf8decode(const char *, long *, int);
