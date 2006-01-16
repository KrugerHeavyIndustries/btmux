#ifndef _FUMIGATE_
#define _FUMIGATE_

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define dassert(x) do { if(!(x)) { \
    struct timeval tv; struct tm tm; time_t now; \
    time(&now); \
    localtime_r(&now, &tm); \
    gettimeofday(&tv, NULL); \
    fprintf(stderr, "%02d%02d%02d.%08d:%5d %s (%s:%d] failed assertion '%s'\n", \
            tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec, getpid(), __FUNCTION__, \
            __FILE__, __LINE__, #x); abort(); } } while(0)

#define dperror(x) do { if(x) { \
    struct timeval tv; struct tm tm; time_t now; \
    time(&now); \
    localtime_r(&now, &tm); \
    gettimeofday(&tv, NULL); \
    fprintf(stderr, "%02d%02d%02d.%08d:%5d %s (%s:%d] '%s' failed with '%s'\n", \
            tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec, getpid(), __FUNCTION__, \
            __FILE__, __LINE__, #x, strerror(errno)); abort(); } } while(0)


/* DEBUG only error messages */
#ifdef DEBUG
	/* debug test: 
 	* if the first argument `test' evaluates as false,
 	* the rest of the arguments are passed to fprintf */ 
	#define dtest(test, args...)	\
	do { if (!test) {	\
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

    #define hexdump(buffer, size) \
        do { \
            int my__count; \
            unsigned char *my__buffer = (unsigned char *)buffer; \
            struct timeval my__tv = { 0, 0 }; \
            gettimeofday(&my__tv, NULL); \
            fprintf(stderr, "%02d%02d%02d.%08d:%05d %s (%s:%d)] buffer %s len %d\n\t",  \
                (((int)my__tv.tv_sec) % 86400)/3600, ((int)my__tv.tv_sec % 3600)/60, ((int)my__tv.tv_sec % 60), \
                (int)my__tv.tv_usec, getpid(), __FUNCTION__, __FILE__, __LINE__, #buffer, size);	\
            for(my__count = 0; my__count < size; my__count++) { \
                fprintf(stderr, "%02x ", my__buffer[my__count]); \
                if(my__count && my__count % 16 == 15) fprintf(stderr, "\n\t"); \
            } \
            fprintf(stderr, "\n"); \
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

#endif /* !_FUMIGATE_ */
