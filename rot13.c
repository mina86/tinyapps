/*
 * Ecrypts message using ROT13
 * $Id: rot13.c,v 1.2 2005/12/29 18:50:12 mina86 Exp $
 * Released to Public Domain
 */

main(a){while(a=~getchar())putchar(~a-1/(~(a|32)/13*2-11)*13);}
