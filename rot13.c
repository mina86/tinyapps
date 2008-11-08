/*
 * Encrypts message using ROT13
 * $Id: rot13.c,v 1.5 2008/11/08 23:45:57 mina86 Exp $
 * Released to Public Domain
 *
 * This is part of Tiny Applications Collection
 *   -> http://tinyapps.sourceforge.net/
 */

#include <stdio.h>
main(a){while(a=~getchar())putchar(~a-1/(~(a|32)/13*2-11)*13);}
