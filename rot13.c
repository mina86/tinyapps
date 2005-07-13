/*
 * Shows CPU load and some other information.
 * $Id: rot13.c,v 1.1 2005/07/13 11:02:15 mina86 Exp $
 * Released to Public Domain
 */

main(a){while(a=~getchar())putchar(~a-1/(~(a|32)/13*2-11)*13);}
