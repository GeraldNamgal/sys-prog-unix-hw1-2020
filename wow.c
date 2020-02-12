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

int main(int ac, char **av)
{
	struct utmp	*utbufp,	/* holds pointer to next rec	*/
			*utmp_next();	/* returns pointer to next	*/
	void		show_info( struct utmp * );
	char *wtmpFile = WTMP_FILE;	/* pointer to file name 	*/
	struct tm dateInput;		/* tm struct for date input 	*/	
	
	// handle command line arguments (referenced: https://www.epochconverter.com/programming/c)
	if (ac == 4) {
		dateInput.tm_year = atoi(av[1]) - 1900;
		dateInput.tm_mon = atoi(av[2]) - 1;
		dateInput.tm_mday = atoi(av[3]);
		dateInput.tm_hour = 0;
		dateInput.tm_min = 0;
		dateInput.tm_sec = 0;
		dateInput.tm_isdst = -1;
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
		}
	} 

	int fdUtmp = utmp_open( wtmpFile );			/* open file */
	if ( fdUtmp == -1 ){
		fprintf(stderr,"%s: cannot open %s\n", *av, wtmpFile);
		exit(1);
	}
	// search for records matching date input
	time_t rawInput = mktime(&dateInput);			/* convert date input */
	int secsPerDay = 86400;
	bool foundStart = false;				/* "found" flag for records matching date input */
	// linear search		
	if (strcmp(av[0], "./swow") == 0) {
		while ( foundStart == false && ( utbufp = utmp_next() ) != NULL ) {		
			if (utbufp->ut_time >= rawInput && utbufp->ut_time < rawInput + secsPerDay) 
				foundStart = true;		/* flag start of records matching input */								
		}
	}
	// TODO: binary search (referenced https://www.programmingsimplified.com/c/source-code/c-program-binary-search)
	if (strcmp(av[0], "./bwow") == 0) {
		// get size of file (referenced https://gist.github.com/jakekara/c4ae2fc2ba4ec210252a184eece4c2d2)
		int fileSize = lseek(fdUtmp, 0, SEEK_END),	
		    numRecords = fileSize / sizeof(struct utmp),
		    first = 0,
		    last = numRecords - 1,
		    middle = ((first + last) / 2) * sizeof(struct utmp) - (NRECS / 2) * sizeof(struct utmp);
		    bool notFirst = false;			/* a record matching input isn't first in buffer */

		while(first <= last) {
			lseek(fdUtmp, middle, SEEK_SET);	/* move offset to middle */

		}			
	}	
	// print any records that match date input
	if (foundStart == true) {
		while (utbufp != NULL) {
			if (utbufp->ut_time >= rawInput + secsPerDay)
				break;				/* stop reading if past matching records */
			else {
				show_info(utbufp);
				utbufp = utmp_next();
			}
		}
	}	

	utmp_close( );
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

	if ( utbufp->ut_type != USER_PROCESS )
		return;
	
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
