#include	<string.h>
#include	<stdio.h>
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
	void		show_info( struct utmp *, struct tm );
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

	if ( utmp_open( wtmpFile ) == -1 ){
		fprintf(stderr,"%s: cannot open %s\n", *av, wtmpFile);
		exit(1);
	}
	while ( ( utbufp = utmp_next() ) != NULL )
		show_info( utbufp, dateInput );
	utmp_close( );
	return 0;
}
/*
 *	show info()
 *			displays the contents of the utmp struct
 *			in human readable form
 *			* displays nothing if record has no user name
 */
void
show_info( struct utmp *utbufp, struct tm dateInput )
{
	void	showtime( time_t , char *);

	if ( utbufp->ut_type != USER_PROCESS )
		return;

	/* check that time matches user input */
	
	//printf("%d\n", utbufp->ut_time);
	
	//printf("%ld\n", mktime(&dateInput));	

	//time_t timeValue = utbufp->ut_time;			/* get time */
	//struct tm *tp = localtime(&timeValue);			/* convert time */
	//if (tp->tm_year == dateInput.tm_year && tp->tm_mon == dateInput.tm_mon
	//    && tp->tm_mday == dateInput.tm_mday)
	time_t rawInput = mktime(&dateInput);
	int secsPerDay = 86400;
	if (utbufp->ut_time >= rawInput && utbufp->ut_time < rawInput + secsPerDay)
	{
		printf("%-8s", utbufp->ut_name);		/* the logname	*/
		printf(" ");					/* a space	*/
		printf("%-12.12s", utbufp->ut_line);		/* the tty	*/
		printf(" ");					/* a space	*/
		showtime( utbufp->ut_time, DATE_FMT );		/* display time	*/
		if ( utbufp->ut_host[0] != '\0' )
			printf(" (%s)", utbufp->ut_host);	/* the host	*/
		printf("\n");					/* newline	*/
	}
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
