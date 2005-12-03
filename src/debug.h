#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef assert
#define assert(x) if(!(x)) do { perror(__FUNCTION__); abort(); } while(0)
#endif
#ifndef assert_valid
#define assert_valid(x) if((x) == NULL) do { perror(__FUNCTION__); abort(); } while(0)
#endif

/* DEBUG only error messages */
#ifdef DEBUG
	/* debug test: 
 	* if the first argument `test' evaluates as true,
 	* the rest of the arguments are passed to fprintf */ 
	#define dtest(test, args...)	\
	do { if (test) {	\
		fprintf(stderr, "%5d %s (%s:%d)] ", getpid(), __FUNCTION__, __FILE__, __LINE__);	\
		fprintf(stderr, args);	\
		fprintf(stderr, "\n");	\
	}} while(0)

	/* debug print 
	 * prints arguments */
	#define dprintk(args...)	\
	do {	\
        struct timeval my__tv = { 0, 0 }; \
        gettimeofday(&my__tv, NULL); \
        fprintf(stderr, "%02d%02d%02d.%08d:%05d %s (%s:%d)] ",  \
                (((int)my__tv.tv_sec) % 86400)/3600, ((int)my__tv.tv_sec % 3600)/60, ((int)my__tv.tv_sec % 60), \
                (int)my__tv.tv_usec, getpid(), __FUNCTION__, __FILE__, __LINE__);	\
        fprintf(stderr, args);	\
        fprintf(stderr, "\n");	\
	} while(0)
#else
	#define dtest(args...)
	#define dprintk(args...)
#endif /* DEBUG */

#define printk(args...)	\
do {	\
    fprintf(stderr, "%s (%s:%d)] ", __FILE__, __FUNCTION__, __LINE__);	\
    fprintf(stderr, args);	\
    fprintf(stderr, "\n");	\
} while(0)

#define dassert(condition, args...)	\
	do {	\
	if (!(condition)) {	\
		fprintf(stderr, "%s (%s:%d)] ", __FUNCTION__, __FILE__, __LINE__);	\
		fprintf(stderr, args);	\
		fprintf(stderr, "\n");	\
	    abort(); \
    }} while (0)

#define IF_FAIL_ERRNO(condition, args...)	\
	do {	\
	if (!(condition)) {	\
		fprintf(stderr, "%s (%s:%d)] ", __FUNCTION__, __FILE__, __LINE__);	\
		fprintf(stderr, args);	\
		fprintf(stderr, ": ");	\
		perror(NULL);	\
		abort();	\
    }} while (0)

#define handle_errno(x) if((x)<0) do { fprintf(stderr, "%s (%s:%d)] %s\n", __FUNCTION__, __FILE__,  __LINE__, strerror(errno)); abort(); } while(0)

#endif /* !_DEBUG_H_ */
