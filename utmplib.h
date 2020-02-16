#ifndef UTMPLIB_H
#define UTMPLIB_H
/* utmplib.h  - header file with decls of functions in utmplib.c */

#include <utmp.h>

int utmp_open(char *);
struct utmp *utmp_next();
int utmp_close();
int getTotalNumRecs(); 
off_t utmpSeek(off_t, int, int);
int getBufferSize();

#endif
