#ifndef UTMPLIB_H
#define UTMPLIB_H
/* utmplib.h  - header file with decls of functions in utmplib.c */

#include <utmp.h>

int utmp_open(char *);
struct utmp *utmp_next();
int utmp_close();
int utmpSeek(int, int, int);
struct utmp *getLastInBuff();
int getTotalNumRecs(); 

#endif
