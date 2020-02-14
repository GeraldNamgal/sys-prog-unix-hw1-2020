/* utmplib.c  - functions to buffer reads from utmp file 
 *
 *      functions are
 *              int utmp_open( filename )   - open file
 *                      returns -1 on error
 *              struct utmp *utmp_next( )   - return pointer to next struct
 *                      returns NULL on eof or read error
 *              int utmp_close()            - close file
 *
 *      reads NRECS per read and then doles them out from the buffer
 *      hist: 2012-02-14 slightly modified error handling (thanks mk)
 */
#include        <stdio.h>
#include        <fcntl.h>
#include        <sys/types.h>
#include        <utmp.h>
#include	<unistd.h>

#define NRECS   1
#define UTSIZE  (sizeof(struct utmp))

static	struct utmp utmpbuf[NRECS];			/* storage	*/
static  int     num_recs;                               /* num stored   */
static  int     cur_rec;                                /* next to go   */
static  int     fd_utmp = -1;                           /* read from    */

static  int  utmp_reload();

/*
 * utmp_open -- connect to specified file
 *  args: name of a utmp file
 *  rets: -1 if error, fd for file otherwise
 */
int utmp_open( char *filename )
{
        fd_utmp = open( filename, O_RDONLY );           /* open it      */
        cur_rec = num_recs = 0;                         /* no recs yet  */
        return fd_utmp;                                 /* report       */
}

/*
 * utmp_next -- return address of next record in file
 *  args: none
 *  rets: address of record in buffer
 *  note: this location will be reused on next buffer reload
 */
struct utmp *utmp_next()
{
        struct utmp *recp;

        if ( fd_utmp == -1 )                            /* error ?      */
                return NULL;
        if ( cur_rec==num_recs && utmp_reload() <= 0 )  /* any more ?   */
                return NULL;

	recp = &(utmpbuf[cur_rec]);	/* get address of next record   */
        cur_rec++;
        return recp;
}

static int utmp_reload()
/*
 *      read next bunch of records into buffer
 *      rets: 0=>EOF, -1=>error, else number of records read
 */
{
        int     amt_read;
                                                
	amt_read = read(fd_utmp, utmpbuf, NRECS*UTSIZE);   /* read data	*/
	if ( amt_read < 0 )			/* mark errors as EOF   */
		return -1;
                                                
        num_recs = amt_read/UTSIZE;		/* how many did we get?	*/
        cur_rec  = 0;				/* reset pointer	*/
        return num_recs;			/* report results	*/
}

/*
 * utmp_close -- disconnenect
 *  args: none
 *  rets: ret value from close(2)  -- see man 2 close
 */
int utmp_close()
{
	int rv = 0;
        if ( fd_utmp != -1 ){                   /* don't close if not   */
                rv = close( fd_utmp );          /* open                 */
		fd_utmp = -1;			/* record as closed	*/
	}
	return rv;
}

int getNRECS()
{
        return NRECS;
}

int getNumRecs()
{
        return num_recs;
}

struct utmp *getBuffElement(int index)
{
        struct utmp *recp;

        recp = &(utmpbuf[index]);	/* get address of requested record   */
        return recp;
}

void callReload()
{
        utmp_reload();
}