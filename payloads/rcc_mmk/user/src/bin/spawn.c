/*******************************************************************************
 *  The BYTE UNIX Benchmarks - Release 3
 *          Module: spawn.c   SID: 3.3 5/15/91 19:30:20
 *
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Ben Smith, Rick Grehan or Tom Yagerat BYTE Magazine
 *	ben@bytepb.byte.com   rick_g@bytepb.byte.com   tyager@bytepb.byte.com
 *
 *******************************************************************************
 *  Modification Log:
 *  $Header: spawn.c,v 3.4 87/06/22 14:32:48 kjmcdonell Beta $
 *  August 29, 1990 - Modified timing routines (ty)
 *  October 22, 1997 - code cleanup to remove ANSI C compiler warnings
 *                     Andy Kahn <kahn@zk3.dec.com>
 *
 ******************************************************************************/
char SCCSid[] = "@(#) @(#)spawn.c:3.3 -- 5/15/91 19:30:20";
/*
 *  Process creation
 *
 */

#include <stdio.h>
#include <stdlib.h>

unsigned long iter;

void report()
{
	printf("COUNT|%lu|1|lps\n", iter);
	exit(0);
}

int main(argc, argv)
int	argc;
char	*argv[];
{
	int	slave, duration;
	int	status;

	duration = 10;

	iter = 0;
	// wake_me(duration, report);
    int start = get_time();
	while (1) {
		if ((slave = fork()) == 0) {
			/* slave .. boring */
			/* kill it right away */
			exit(0);
		} else if (slave < 0) {
			/* woops ... */
			printf("Fork failed at iteration %lu\n", iter);
			exit(2);
		} else
			/* master */
			wait(&status);
		if (status != 0) {
			printf("Bad wait status: 0x%x\n", status);
			exit(2);
		}
		iter++;
		int temp = get_time() - start;
    	if(temp >= 10000){
        	break;
    	}
	}

	printf("COUNT|%ld|1|lps\n", iter);
	exit(0);
}
