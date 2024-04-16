/*******************************************************************************
 *  The BYTE UNIX Benchmarks - Release 3
 *          Module: syscall.c   SID: 3.3 5/15/91 19:30:21
 *
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Ben Smith, Rick Grehan or Tom Yager at BYTE Magazine
 *	ben@bytepb.byte.com   rick_g@bytepb.byte.com   tyager@bytepb.byte.com
 *
 *******************************************************************************
 *  Modification Log:
 *  $Header: syscall.c,v 3.4 87/06/22 14:32:54 kjmcdonell Beta $
 *  August 29, 1990 - Modified timing routines
 *  October 22, 1997 - code cleanup to remove ANSI C compiler warnings
 *                     Andy Kahn <kahn@zk3.dec.com>
 *
 ******************************************************************************/
/*
 *  syscall  -- sit in a loop calling the system
 *
 */
char SCCSid[] = "@(#) @(#)syscall.c:3.3 -- 5/15/91 19:30:21";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned long iter;

void report()
{
	printf("COUNT|%ld|1|lps\n", iter);
	exit(0);
}

int create_fd()
{
	int fd[2];
    int pipe_ans = pipe(fd);
    // printf("pipe ans is %d\n", pipe_ans);
    // printf("fd 0 is %d, fd 1 is %d\n", fd[0], fd[1]); 
    int close_ans = close(fd[1]); 

    // printf("close ans is %d\n", close_ans);

	if (pipe_ans != 0 || close_ans != 0)
	    exit(1);

    // printf("fd create over\n");

	return fd[0];
}

int main(argc, argv)
int	argc;
char	*argv[];
{
    char   *test;
	int	duration;
	int	fd;
    int start_time;
    int temp;
    // printf("exec syscall test\n");
    test = "close";

	duration = 10;

	iter = 0;
	// wake_me(duration, report);
    switch (test[0]) {
        case 'm':
            start_time = get_time();
	        fd = create_fd();
	        while (1) {
		        close(dup(fd));
                getpid();
		        getuid();
		        umask(022);
		        iter++;
                temp = get_time() - start_time;
                if(temp >= 10000){
                    break;
                }
	        }
            break;
	    /* NOTREACHED */
        case 'c':
            start_time = get_time();
            fd = create_fd();
            while (1) {
                close(dup(fd));
                iter++;
                temp = get_time() - start_time;
                if(temp >= 10000){
                    break;
                }
            }
            break;
           /* NOTREACHED */
        case 'g':
            while (1) {
                getpid();
                iter++;
            }
            break;
           /* NOTREACHED */
        case 'e':
            while (1) {
                int pid = fork();
                if (pid < 0) {
                    printf("%s: fork failed\n", argv[0]);
                    exit(1);
                } else if (pid == 0) {
                    printf("%s: exec /bin/true failed\n", argv[0]);
                    exit(1);
                } else {
                    if (waitpid(pid, 0) < 0) {
                        printf("%s: waitpid failed\n", argv[0]);
                        exit(1);
                    }
                }
                iter++;
            }
            break;
           /* NOTREACHED */
        }
    printf("COUNT|%ld|1|lps\n", iter);
    
    exit(0);

}

