#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <errno.h>

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
	#define DEBUG_TEST(test, args...)	\
	do { if (test) {	\
		fprintf(stderr, "[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);	\
		fprintf(stderr, args);	\
		fprintf(stderr, "\n");	\
	}} while(0)

	/* debug print 
	 * prints arguments */
	#define DEBUG_PRINT(args...)	\
	do {	\
	fprintf(stderr, "[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);	\
	fprintf(stderr, args);	\
	fprintf(stderr, "\n");	\
	} while(0)
#else
	#define DEBUG_TEST(args...)
	#define DEBUG_PRINT(args...)
#endif /* DEBUG */


#define IF_FAIL(condition, args...)	\
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

#define EMIT_STDERR(format, args...) fprintf(stderr, "%s (%s:%d)] " format "\n", ##args)

#define handle_errno(x) if((x)<0) do { fprintf(stderr, "%s (%s:%d)] %s\n", __FUNCTION__, __FILE__,  __LINE__, strerror(errno)); abort(); } while(0)

#endif /* !_DEBUG_H_ */
