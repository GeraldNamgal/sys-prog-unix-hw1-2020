#include	<stdio.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
/*
 *	who version 2		- read /var/run/utmp and list info therein
 * 				- surpresses empty records
 *				- formats time nicely
 */

/* UTMP_FILE is a symbol defined in utmp.h */
/* note: compile with -DTEXTDATE for dates format: Feb  5, 1978 	*/
/*       otherwise, program defaults to NUMDATE (1978-02-05)		*/

#define TEXTDATE
#ifndef	DATE_FMT
#ifdef	TEXTDATE
#define	DATE_FMT	"%b %e %H:%M"		/* text format	*/
#else
#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/
#endif
#endif

int main(int ac, char *av[])
{
	struct utmp	utbuf;		/* read info into here */
	int		utmpfd;		/* read from this descriptor */
	void		show_info(struct utmp *);
	(void)		ac;

	if ( (utmpfd = open( UTMP_FILE, O_RDONLY )) == -1 ){
		fprintf(stderr,"%s: cannot open %s\n", *av, UTMP_FILE);
		exit(1);
	}

	while ( read( utmpfd, &utbuf, sizeof(utbuf) ) == sizeof(utbuf) )
		show_info( &utbuf );
	close( utmpfd );
	return 0;
}
/*
 *	show info()
 *			displays the contents of the utmp struct
 *			in human readable form
 *			* displays nothing if record has no user name
 */
void show_info( struct utmp *utbufp )
{
	void showtime(time_t, char *);

	if ( utbufp->ut_type != USER_PROCESS )
		return;

	printf("%-8s", utbufp->ut_name);		/* the logname	*/
	printf(" ");					/* a space	*/
 	printf("%-12.12s", utbufp->ut_line);		/* the tty	*/
	printf(" ");					/* a space	*/
	showtime( utbufp->ut_time , DATE_FMT);		/* display time	*/
	if ( utbufp->ut_host[0] != '\0' )
		printf(" (%s)", utbufp->ut_host);	/* the host	*/
	printf("\n");					/* newline	*/
}

#define	MAXDATELEN	100

void showtime( time_t timeval , char *fmt )
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
