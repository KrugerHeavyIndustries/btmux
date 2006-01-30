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
    time(&now); localtime_r(&now, &tm); gettimeofday(&tv, NULL); \
    fprintf(stderr, "%02d%02d%02d.%08d:%5d %s (%s:%d] failed assertion '%s'\n", \
            tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec, getpid(), __FUNCTION__, \
            __FILE__, __LINE__, #x); abort(); } } while(0)

#define dperror(x) do { if(x) { \
    struct timeval tv; struct tm tm; time_t now; \
    time(&now); localtime_r(&now, &tm); gettimeofday(&tv, NULL); \
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
        struct timeval __tv; struct tm __tm; time_t __now; \
        time(&__now); localtime_r(&__now, &__tm); gettimeofday(&__tv, NULL); \
        fprintf(stderr, "%02d%02d%02d.%08d:%5d %s (%s:%d)] ", \
            __tm.tm_hour, __tm.tm_min, __tm.tm_sec, (int)__tv.tv_usec, getpid(), __FUNCTION__, \
            __FILE__, __LINE__); \
        fprintf(stderr, args);	\
        fprintf(stderr, "\n");	\
	} while(0)

    #define bhexdump(buffer, size) \
        do { \
            struct timeval tv; struct tm tm; time_t now; \
            int my__count; unsigned char *my__buffer = (unsigned char *)buffer; \
            time(&now); localtime_r(&now, &tm); gettimeofday(&tv, NULL); \
            fprintf(stderr, "%02d%02d%02d.%08d:%5d %s (%s:%d)] buffer %s at %p len %d ", \
                tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec, getpid(), __FUNCTION__, \
                __FILE__, __LINE__, #buffer, buffer, size); \
            for(my__count = 0; my__count < size; my__count++) { \
                fprintf(stderr, "%02x ", my__buffer[my__count]); \
                if(my__count && my__count % 16 == 15) fprintf(stderr, "\n\t"); \
            } \
            fprintf(stderr, "\n"); \
        } while(0) 
    
#else
	#define dtest(args...)
	#define dprintk(args...)
    #define dhexdump(args...)
#endif /* DEBUG */

#define printk(args...)	\
	do {	\
        struct timeval tv; struct tm tm; time_t now; \
        time(&now); localtime_r(&now, &tm); gettimeofday(&tv, NULL); \
        fprintf(stderr, "%02d%02d%02d.%08d:%5d %s (%s:%d)] ", \
            tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec, getpid(), __FUNCTION__, \
            __FILE__, __LINE__); \
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
