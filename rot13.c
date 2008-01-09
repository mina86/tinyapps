/*
 * Encrypts message using ROT13
 * $Id: rot13.c,v 1.4 2008/01/09 18:46:41 mina86 Exp $
 * Released to Public Domain
 */

#include <stdio.h>
main(a){while(a=~getchar())putchar(~a-1/(~(a|32)/13*2-11)*13);}
