/* TODO: javadoc
 *
 */

#include	<stdio.h>
#include	<string.h>
#include 	<stdbool.h>
#include	<math.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include	"utmplib.h"	/* include interface def for utmplib.c	*/

/*
 *	who version 3.0		- read /var/run/utmp and list info therein
 *				- surpresses empty records
 *				- formats time nicely
 *				- buffers input (using utmplib)
 */

#define TEXTDATE
#ifndef	DATE_FMT
#ifdef	TEXTDATE    
#define	DATE_FMT	"%b %e %H:%M"		/* text format	*/
#else
#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/
#endif
#endif
#define secondsInADay 86400

static char *wtmpFile = WTMP_FILE;    /* wtmp file (default is WTMP_FILE) */
static struct tm dateInput;       /* for date input from command line 	*/
static char *wowVersion;		  /* version of wow ran (swow or bwow) */
static time_t firstSecOfDate;  /* 12:00 AM on input date in epoch seconds */
static time_t lastSecOfDate;	  /* 11:59 PM on input date in epoch seconds */

static int handleArgs(int, char **);
static void searchFile(int);
static bool binarySearch(int);	 

int main(int ac, char **av)
{
    if ( handleArgs(ac, av) == -1 )		/* handle command line arguments */
        exit(1);			            /* exit on error */    
    int fdUtmp = utmp_open( wtmpFile );	/* open file */
    if ( fdUtmp == -1 ){
        fprintf(stderr,"%s: cannot open %s\n", *av, wtmpFile);
        exit(1);
    }	
    searchFile(fdUtmp);			    /* search file's records for date input */	
    utmp_close();
    return 0;
}

/*
 *	show info()
 *			displays the contents of the utmp struct
 *			in human readable form
 *			* displays nothing if record has no user name
 *	TODO: Javadoc comments including args, etc.
 */
void
show_info( struct utmp *utbufp )
{
        void	showtime( time_t , char *);

        /* TODO: uncomment this after debugging --
        if ( utbufp->ut_type != USER_PROCESS )
                return;*/
        
        printf("%-8s", utbufp->ut_name);		/* the logname	*/
        printf(" ");					/* a space	*/
        printf("%-12.12s", utbufp->ut_line);		/* the tty	*/
        printf(" ");					/* a space	*/
        showtime( utbufp->ut_time, DATE_FMT );		/* display time	*/
        if ( utbufp->ut_host[0] != '\0' )
                printf(" (%s)", utbufp->ut_host);	/* the host	*/
        printf("\n");					/* newline	*/	
}

#define	MAXDATELEN	100

void
showtime( time_t timeval , char *fmt )
/*
 * displays time in a format fit for human consumption.
 * Uses localtime to convert the timeval into a struct of elements
 * (see localtime(3)) and uses strftime to format the data
 */
{
        char	result[MAXDATELEN];

        struct tm *tp = localtime(&timeval);		/* convert time	*/
        strftime(result, MAXDATELEN, fmt, tp);		/* format it	*/
        fputs(result, stdout);
}

/* handleArgs()
 *  
 *	Puts user input into dateInput struct as midnight on the input date, and
 *  determines which search method and wtmp file to use.
 * 	args: the number of command line arguments and the arguments themselves
 * 	rets: an integer for completion status
 *  (code referenced: https://www.epochconverter.com/programming/c)
 */ 
static int handleArgs(int ac, char **av)
{
    if ( strcmp(av[0], "./bwow") == 0 ) {wowVersion = "bwow";}
    if ( strcmp(av[0], "./swow") == 0 ) {wowVersion = "swow";}    
    if (ac == 4) {
        dateInput.tm_year = atoi(av[1]) - 1900; dateInput.tm_isdst = -1;
        dateInput.tm_mon = atoi(av[2]) - 1; dateInput.tm_mday = atoi(av[3]);
        dateInput.tm_hour = 0; dateInput.tm_min = 0; dateInput.tm_sec = 0;        
        return 0;
    }
    if (ac == 6) {
        if (strcmp(av[1], "-f") == 0) {
            wtmpFile = av[2]; dateInput.tm_year = atoi(av[3]) - 1900;			
            dateInput.tm_mon = atoi(av[4]) - 1;
            dateInput.tm_mday = atoi(av[5]); dateInput.tm_hour = 0;
            dateInput.tm_min = 0; dateInput.tm_sec = 0;
            dateInput.tm_isdst = -1;
            return 0;
        }	
        else if (strcmp(av[4], "-f") == 0) {
            wtmpFile = av[5]; dateInput.tm_year = atoi(av[1]) - 1900;
            dateInput.tm_mon = atoi(av[2]) - 1;
            dateInput.tm_mday = atoi(av[3]); dateInput.tm_hour = 0;
            dateInput.tm_min = 0; dateInput.tm_sec = 0;
            dateInput.tm_isdst = -1;
            return 0;
        }
    }						
    return -1;				/* return -1 if invalid arguments error */
}

static void searchFile(int fdUtmp)
{
    struct utmp	*utbufp,		    /* holds pointer to next rec */
                *utmp_next();		/* returns pointer to next	*/
    void		show_info( struct utmp * );
    
    int totalNumRecords = getTotalNumRecs();    // function from utmplib.c   		
    firstSecOfDate = mktime(&dateInput);		// 12:00 AM (in epoch seconds)
    lastSecOfDate = firstSecOfDate + secondsInADay - 1;	// 11:59 PM 
    bool foundStartOfList = false;			    // start of matching records
    
    /* 
        * linear search (if command line argument was for swow)
        *	
        * 	Searches through record times one by one to find any that match the input date. A record's time
        * 	matches the input date if it falls between 12:00 AM to 11:59 PM on the input date (those times
        * 	correspond to the variables "firstSecOfDate" and "lastSecOfDate", respectively, which are
        * 	in epoch seconds).
        */		
    if (totalNumRecords > 0 && strcmp(wowVersion, "swow") == 0) {
        lseek(fdUtmp, 0, SEEK_SET);			// move offset to beginning 
        while ( foundStartOfList == false && ( utbufp = utmp_next() ) != NULL ) {					
            if (utbufp->ut_time >= firstSecOfDate && utbufp->ut_time <= lastSecOfDate) 
                foundStartOfList = true;	// found start of records that match date input 								
        }
    }	
    
    // call binarySearch fxn if command line argument was for bwow
    if (totalNumRecords > 0 && strcmp(wowVersion, "bwow") == 0) {
        foundStartOfList = binarySearch(fdUtmp);
    }

    // if searching found records that match date input, then print them
    if (foundStartOfList == true) {			// print records that match date input
        while (utbufp != NULL) {
            if (utbufp->ut_time > lastSecOfDate)
                break;			// stop reading records if past matching records
            else {
                show_info(utbufp);
                utbufp = utmp_next();
            }
        }
    }
}

/*
 * TODO: binary search (referenced https://www.programmingsimplified.com/c/source-code/c-program-binary-search)
 * 
 * 	Uses binary search to search through record times to find any that match the input. A record's time
 * 	matches the input date if it falls between 12:00 AM to 11:59 PM on the input date (those times correspond
 * 	to the variables "firstSecOfDate" and "lastSecOfDate", respectively, which are in epoch seconds).
 */
static bool binarySearch(int fdUtmp)
{
    struct utmp	*utbufp,		    /* holds pointer to next rec */
                *utmp_next();		/* returns pointer to next	*/    
    int  low = 0,  high = getTotalNumRecs() - 1,  middle = (low + high) / 2;		
    bool foundMatch = false;		// a record matches input date
    bool foundStartOfList = false;	// flag for start of block matching input    
    utmpSeek(middle, firstSecOfDate, lastSecOfDate); // find next record    
    utbufp = utmp_next();			// point to next record    
    while (low <= high && utbufp != NULL) {		
        if (lastSecOfDate < utbufp->ut_time) {  // input date < record time?
            high = middle - 1;
        }
        else if (firstSecOfDate > utbufp->ut_time) {    // is input greater?			
            low = middle + 1;		
        }
        else {					             // else input date equals record's
            foundMatch = true;
            break;													
        }

        middle = (low + high) / 2;
        utbufp = utmpSeek(middle, firstSecOfDate, lastSecOfDate);
        utbufp = utmp_next();        // point to next record			
    }
    // TODO: backtrack to first record that matches date input...
    if (foundMatch == true) {	
        /* used ceiling the quotient formula (dividend + divisor - 1) / divisor
          (referenced https://stackoverflow.com/questions/2745074/fast-ceiling-
           of-an-integer-division-in-c-c) */	
        int backtrackAmt = (getTotalNumRecs() + 30 - 1) / 30;	 // TODO: 30 because it's avg days in a month
        int startPoint = middle - backtrackAmt;		
        // TODO: some loop for code below...
            lseek(fdUtmp, startPoint, SEEK_CUR);
            // TODO: callReload();		   // reload buffer; callReload() is from utmplib.c
            // TODO: utbufp = utmp_next();  // point to next record
            // TODO: ...
            // TODO: startPoint = startPoint - backtrackAmt
    }
    if (foundStartOfList == true)					 
        return true;			   // return true if found block matching input
    return false;	
}
