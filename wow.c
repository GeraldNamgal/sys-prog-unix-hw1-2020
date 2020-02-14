#include	<stdio.h>
#include	<string.h>
#include 	<stdbool.h>
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

char *wtmpFile = WTMP_FILE;			/* pointer to wtmp file (default is WTMP_FILE)	*/
struct tm dateInput;				/* for date input from command line 	*/
char *wowVersion;				/* version of wow ran (swow or bwow) */
time_t firstSecondOfDate;				/* 12:00 AM on input date in epoch seconds */
time_t lastSecondOfDate			;	/* 11:59 PM on input date in epoch seconds */

int handleArgs(int, char **);
void searchFile(int);	 

int main(int ac, char **av)
{
	if (handleArgs(ac, av) == -1)		/* handle command line arguments */
		exit(1);				
	int fdUtmp = utmp_open( wtmpFile );	/* open file */
	if ( fdUtmp == -1 ){
		fprintf(stderr,"%s: cannot open %s\n", *av, wtmpFile);
		exit(1);
	}	
	searchFile(fdUtmp);			/* search file's records for date input */	
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

/*
 * handleArgs()
 *  
 *	puts command line arguments into dateInput struct tm as midnight on the input date 
 * 	args: the number of command line arguments and the arguments themselves
 * 	rets: none
 */ 
int handleArgs(int ac, char **av)
{
	if (strcmp(av[0], "./bwow") == 0)
		wowVersion = "bwow";
	if (strcmp(av[0], "./swow") == 0)
		wowVersion = "swow";
	// code below referenced: https://www.epochconverter.com/programming/c)
	if (ac == 4) {
		dateInput.tm_year = atoi(av[1]) - 1900;
		dateInput.tm_mon = atoi(av[2]) - 1;
		dateInput.tm_mday = atoi(av[3]);
		dateInput.tm_hour = 0;
		dateInput.tm_min = 0;
		dateInput.tm_sec = 0;
		dateInput.tm_isdst = -1;
		return 0;
	}
	if (ac == 6) {
		if (strcmp(av[1], "-f") == 0) {
			wtmpFile = av[2];
			dateInput.tm_year = atoi(av[3]) - 1900;			
			dateInput.tm_mon = atoi(av[4]) - 1;
			dateInput.tm_mday = atoi(av[5]);
			dateInput.tm_hour = 0;
			dateInput.tm_min = 0;
			dateInput.tm_sec = 0;
			dateInput.tm_isdst = -1;
			return 0;
		}	
		else if (strcmp(av[4], "-f") == 0) {
			wtmpFile = av[5];
			dateInput.tm_year = atoi(av[1]) - 1900;
			dateInput.tm_mon = atoi(av[2]) - 1;
			dateInput.tm_mday = atoi(av[3]);
			dateInput.tm_hour = 0;
			dateInput.tm_min = 0;
			dateInput.tm_sec = 0;
			dateInput.tm_isdst = -1;
			return 0;
		}
	}						
	return -1;				/* return -1 if invalid arguments error */
}

void searchFile(int fdUtmp)
{
	struct utmp	*utbufp,		/* holds pointer to next rec	*/
			*utmp_next();		/* returns pointer to next	*/
	void		show_info( struct utmp * );	

	// paragraph below referenced https://gist.github.com/jakekara/c4ae2fc2ba4ec210252a184eece4c2d2
	int fileSize = lseek(fdUtmp, 0, SEEK_END),	// get file size
	numRecords = fileSize / sizeof(struct utmp);	// number of records in file			
	int secondsInADay = 86400;			// seconds in a day
	firstSecondOfDate = mktime(&dateInput);		// 12:00 AM on input date in epoch seconds
	lastSecondOfDate = firstSecondOfDate + secondsInADay - 1;	// 11:59 PM on input date in epoch seconds 
	bool foundStart = false;			// "found" flag for start of record(s) matching date input
	
	/* 
	 * linear search
	 *	
	 * 	Searches through record times one by one to find any that match the input date. A record's time
	 * 	matches the input date if it falls between 12:00 AM to 11:59 PM on the input date (using epoch
	 * 	seconds).
	 */		
	if (numRecords > 0 && strcmp(wowVersion, "swow") == 0) {
		lseek(fdUtmp, 0, SEEK_SET);		// move offset to beginning 
		while ( foundStart == false && ( utbufp = utmp_next() ) != NULL ) {					
			if (utbufp->ut_time >= firstSecondOfDate && utbufp->ut_time <= lastSecondOfDate) 
				foundStart = true;	// found start of records that match date input 								
		}
	}
	
	/*
	 * TODO: binary search (referenced https://www.programmingsimplified.com/c/source-code/c-program-binary-search)
	 * 
	 * 	Uses binary search to search through record times to find any that match the input but instead of
	 * 	a middle element that's typically used in binary search algorithms, my algorithm uses a middle
	 * 	buffer of elements (called "middleBuff" below). A record's time
	 * 	matches the input date if it falls between 12:00 AM to 11:59 PM on the input date (using epoch
	 * 	seconds).
	 */
	if (numRecords > 0 && strcmp(wowVersion, "bwow") == 0) {		
		int bufferSize = getNRECS(),		/* getNRECS() is from utmplib.c; bufferSize is used to
							   position "middleBuff" below to center of records */
		    firstRec = 0,			
		    lastRec = numRecords - 1,
		    middleBuff = ( (firstRec + lastRec) / 2 ) - (bufferSize / 2);
		    if (middleBuff < 0)			/* enforce lower bound of record indexes */
			middleBuff = 0;
		bool isFirst = false;			/* is a record that matches the input first in buffer? */

		lseek(fdUtmp, middleBuff * sizeof(struct utmp), SEEK_SET);	/* move offset to middle buffer */
		utbufp = utmp_next();			/* point to first record in buffer */
		
		/* TODO: debugging -- 
		printf("bufferSize = %d\nfirstRec = %d\nlastRect = %d\nnumRecords = %d\nmiddleBuff = %d\n", bufferSize, firstRec, lastRec, numRecords, middleBuff);
		show_info(utbufp);
		utbufp = getBuffElement(getNumRecs() - 1);
		show_info(utbufp);
		lseek(fdUtmp, (middleBuff + getNumRecs()) * sizeof(struct utmp), SEEK_SET);	// move offset to after buffer
		callReload(); 					// reload buffer
		utbufp = utmp_next();
		show_info(utbufp);
		utbufp = getBuffElement(getNumRecs() - 1);
		show_info(utbufp);*/
		
		while ( firstRec <= lastRec ) {		
			printf("Beginning of loop\n");
			printf("firstRec = %d\n", firstRec);
			printf("lastRec = %d\n", lastRec);
			printf("middleBuff = %d\n\n", middleBuff);	
			if (lastSecondOfDate < utbufp->ut_time) {	// is search date less than first rec in buffer?
				lastRec = middleBuff - 1;
			}
			else if (searchInput > getBuffElement(getNumRecs() - 1)->ut_time) {   // search input > last rec in buffer?			
				firstRec = middleBuff + getNumRecs();

				printf("middleBuff = %d\n\n", middleBuff);
				printf("searchInput = %ld\n", searchInput);
				printf("last elmt = %d\n", getBuffElement(getNumRecs() - 1)->ut_time);
				printf("Here3.\n");
			}
			else {					// search input in buffer?
				if (utbufp->ut_time >= searchInput && utbufp->ut_time < searchInput + secondsInADay) {
					isFirst = true;		// first rec in buffer matches input
					printf("Here1.\n");									
					break;
				}
				for (int i = 1; i < getNumRecs(); i++) {	// search rest of buffer
					utbufp = utmp_next();
					if (utbufp->ut_time >= searchInput && utbufp->ut_time < searchInput + secondsInADay) {
						printf("Here2.\n");
						foundStart = true;
						break;
					}
				}
				if (foundStart == true)
					break;				
			}
			middleBuff = ( (firstRec + lastRec) / 2 ) - (bufferSize / 2);
			if (middleBuff < 0)				// enforce lower bound of record indexes 
				middleBuff = 0;
			lseek(fdUtmp, middleBuff * sizeof(struct utmp), SEEK_SET);	// move offset to middle buffer 
			callReload();					// reload buffer 
			utbufp = utmp_next();				// point to first record in buffer 			
		}
	}	
	if (foundStart == true) {				// print any records that match date input
		while (utbufp != NULL) {
			if (utbufp->ut_time >= searchInput + secondsInADay)
				break;				// stop reading if past matching records
			else {
				show_info(utbufp);
				utbufp = utmp_next();
			}
		}
	}
}
