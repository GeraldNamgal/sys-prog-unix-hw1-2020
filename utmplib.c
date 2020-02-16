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
#include	    <unistd.h>
#include 	    <stdbool.h>

#define NRECS   10
#define UTSIZE  (sizeof(struct utmp))

static	struct utmp utmpbuf[NRECS];			            /* storage	*/
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
            rv = close( fd_utmp );              /* open                 */
		fd_utmp = -1;			                /* record as closed	*/
	}
	return rv;
}

/* getTotalNumRecs
 *   args: none
 *   rets: the number of records in the file
 * referenced https://gist.github.com/jakekara/c4ae2fc2ba4ec210252a184eece4c2d2
 */
int getTotalNumRecs()
{
    int fileSize = lseek(fd_utmp, 0, SEEK_END);	               // get file size
    int totalNumRecords = fileSize / sizeof(struct utmp);	
    return totalNumRecords;
}

/* utmpSeek
 *   args: index of record position 
 *   rets: index of record position; -1 on error
 */ 
off_t utmpSeek(off_t position, int firstSecOfDate, int lastSecOfDate)
{
    off_t returnValue = -1;                      // default return value (error)    
    if (num_recs > 0) {                      // are there records in the buffer?
        printf("HERE\n");
        struct utmp minElement = utmpbuf[0];
        struct utmp maxElement = utmpbuf[num_recs - 1];
        if ( lastSecOfDate >= minElement.ut_time     // is date input in buffer?
              && firstSecOfDate <= maxElement.ut_time ) {
            printf("HERE2\n\n");            
            for (int i = 1; i < num_recs; i++) {             // find first match                
                if (utmpbuf[i].ut_time >= firstSecOfDate          // is a match?
                     && utmpbuf[i].ut_time <= lastSecOfDate) {                
                    cur_rec = i;                    // move cur_rec to the match
                    return lseek(fd_utmp, 0, SEEK_CUR);    // same offset                    
                }
            }            
        }        
    }    
    
    printf("HERE3\n\n");
    returnValue = lseek(fd_utmp, position, SEEK_SET);    
    utmp_reload();
    
    return returnValue;					                         
}

/* backtrack()
 * purpose: Utility function that helps binarySearch() find start of block of
 *          records that match date input by reading values backward in file.
 */ 
bool backtrack(int fromIndex, int firstSecOfDate, struct utmp **utbufp) {     
    int      bufferSize = NRECS;
    int      startPoint = (fromIndex / bufferSize) * bufferSize;
    time_t   newFirstSec = 0,   newLastSec = firstSecOfDate - 1;
    bool     foundTransition = false;         // transitioned to a previous date
    utmpSeek(startPoint * sizeof(struct utmp), newFirstSec, newLastSec);
    *utbufp = utmp_next();                    // point to first record in buffer
    while(1) {           
        printf("startPoint = %d\n", startPoint);
        printf("(*utbufp)->ut_time = %d\n", (*utbufp)->ut_time);
        printf("newLastSec = %ld\n", newLastSec);          
        if ( (*utbufp)->ut_time <= newLastSec ) {          // did we transition?
            printf("LALA\n");
            printf("(*utbufp)->ut_time = %d\n", (*utbufp)->ut_time);
            foundTransition = true;
            break;
        }
        if (startPoint - 1 < 0) {                 // it's the first date in file 
            printf("(*utbufp)->ut_time = %d\n", (*utbufp)->ut_time);
            printf("newLastSec = %ld\n", newLastSec);
            foundTransition = true;
            cur_rec--;                   // move cur_rec back to start of buffer
            break;
        }
        else {                                         // else keep backtracking
            startPoint = ( (startPoint - 1) / bufferSize ) * bufferSize;
            utmpSeek(startPoint * sizeof(struct utmp), newFirstSec, newLastSec);
            *utbufp = utmp_next();            // point to first record in buffer
        }
    }
    /* if found transition, search forward for original date */
    if (foundTransition == true) {  
        while ( ( *utbufp = utmp_next() ) != NULL ) {       
            if ( (*utbufp)->ut_time >= firstSecOfDate ) {
                return true;
            }                          
        }
    }   
    return foundTransition;
}

