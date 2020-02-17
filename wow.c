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
 *         				- surpresses empty records
 *         				- formats time nicely
 *         				- buffers input (using utmplib)
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

static char *wtmpFile = WTMP_FILE;       /* wtmp file (default is WTMP_FILE) */
static struct tm dateInput;             /* for date input from command line  */
static char *wowVersion;		        /* version of wow ran (swow or bwow) */
static time_t firstSecOfDate;     /* 12:00 AM on input date in epoch seconds */
static time_t lastSecOfDate;	  /* 11:59 PM on input date in epoch seconds */
static struct utmp *utbufp;		                /* holds pointer to next rec */

static int handleArgs(int, char **);
static bool searchFile();
static bool linearSearch();
static bool binarySearch();
static void printMatches();

// TODO: Javadoc comments including args, etc.
 
/* main(int ac, char **av)
 * purpose: 
 * args: 
 * rets: 
 */
int main(int ac, char **av)
{
    if ( handleArgs(ac, av) == -1 )		    /* handle command line arguments */
        exit(1);			                                /* exit on error */    	
    if ( utmp_open( wtmpFile ) == -1 ){                         /* open file */
        fprintf(stderr,"%s: cannot open %s\n", *av, wtmpFile);
        exit(1);
    }	
    if ( searchFile() )              /* search file's records for date input */	
        printMatches();	         /* print records if searching found matches */   			         
    utmp_close();
    return 0;
}

/*
 *	show info()
 *			displays the contents of the utmp struct
 *			in human readable form
 *			* displays nothing if record has no user name	
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

/* handleArgs(int ac, char **av)  
 * purpose: Puts user input into dateInput struct as midnight on the input date
 * and determines which search method and wtmp file to use.
 * args: the number of command line arguments and the arguments themselves
 * rets: an integer for completion status
 * note: (code referenced: https://www.epochconverter.com/programming/c)
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
    return -1;				        /* invalid arguments error */
}

/*  searchFile()
 *
 */ 
static bool searchFile()
{   
    int totalNumRecords = getTotalNumRecs();         // function from utmplib.c   		
    firstSecOfDate = mktime(&dateInput);		  // 12:00AM (in epoch seconds)
    lastSecOfDate = firstSecOfDate + secondsInADay - 1;	// 11:59PM (epoch secs)
    bool foundStartOfList = false;			       // start of matching records
    void	show_info( struct utmp * );

   	/* call linear search fxn if command line argument was for swow	*/
    if (totalNumRecords > 0 && strcmp(wowVersion, "swow") == 0) {
        foundStartOfList = linearSearch();
    }	    
    /* call binarySearch fxn if command line argument was for bwow */
    if (totalNumRecords > 0 && strcmp(wowVersion, "bwow") == 0) {
        foundStartOfList = binarySearch();
    }
    
    return foundStartOfList;
}

/* printMatches()
 *
 */
static void printMatches()
{
    while (utbufp != NULL) {
            if (utbufp->ut_time > lastSecOfDate)
                break;		   // stop reading records if past matching records
            else {
                show_info(utbufp);
                utbufp = utmp_next();
            }
    }
} 

/*  linear search(int)
 *	
 *  Searches through record times one by one to find any that match the input date. A record's time
 *  matches the input date if it falls between 12:00 AM to 11:59 PM on the input date (those times
 *  correspond to the variables "firstSecOfDate" and "lastSecOfDate", respectively, which are
 *  in epoch seconds).
 */
static bool linearSearch()
{    
    utmpSeek(0, firstSecOfDate, lastSecOfDate);              // move offset to 0    
    
    while ( ( utbufp = utmp_next() ) != NULL ) {  // search for matching records					
        if (utbufp->ut_time >= firstSecOfDate && utbufp->ut_time <= lastSecOfDate) 
            return true;        	          // found matching block of records								
    }

    return false;                       // didn't find matching block of records
}

/* TODO: binary search (referenced
 * https://www.programmingsimplified.com/c/source-code/c-program-binary-search)
 * 
 * 	Uses binary search to search through record times to find any that match the input. A record's time
 * 	matches the input date if it falls between 12:00 AM to 11:59 PM on the input date (those times correspond
 * 	to the variables "firstSecOfDate" and "lastSecOfDate", respectively, which are in epoch seconds).
 *   args: 
 *   rets: true if found start of block of matching records and false otherwise  
 */
static bool binarySearch()
{  
    int  low = 0,  high = getTotalNumRecs() - 1,  middle = (low + high) / 2;		
    bool foundMatch = false;		             // if record matches input date   
    utmpSeek(middle * sizeof(struct utmp), firstSecOfDate, lastSecOfDate);    
    utbufp = utmp_next();			                   // point to middle record    
    while (low <= high && utbufp != NULL) {        
        if (lastSecOfDate < utbufp->ut_time)   // is input date < record's time?            
            high = middle - 1;      
        else if (firstSecOfDate > utbufp->ut_time)     // is input date greater?     
            low = middle + 1;              
        else { 					              // else input date equals record's            
            foundMatch = true;            
            break;
        }                 
        middle = (low + high) / 2;
        off_t returnValue = utmpSeek(middle * sizeof(struct utmp),
                                      firstSecOfDate, lastSecOfDate);
        /* if didn't return error or index requested, it found start of block */
        if ( returnValue != -1
              && returnValue != (off_t) ( middle * sizeof( struct utmp ) ) ) {
            utbufp = utmp_next();            
            return true;
        }
        utbufp = utmp_next();                          // point to next record			
    }
    if (foundMatch == true)        
        return backtrack(middle, firstSecOfDate, &utbufp);    
    return false;    	
}