/*
 * Error handling.
 */

#ifndef BTECH_FI_ERRORS_H
#define BTECH_FI_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
	FI_ERROR_NONE,			/* No error */
	FI_ERROR_UNKNOWN,		/* Unknown error */
	FI_ERROR_UNSUPPORTED,		/* Unsupported operation */
	FI_ERROR_OOM,			/* Out of memory */
	FI_ERROR_EOS,			/* End of stream */
	FI_ERROR_NOFILE,		/* File not found */
	FI_ERROR_INVAL,			/* Invalid argument */
	FI_ERROR_ILLEGAL,		/* Illegal state */
	FI_ERROR_ERRNO,			/* Check errno */
	FI_ERROR_EXCEPTION		/* Caught Exception */
} FI_ErrorCode;

typedef struct {
	FI_ErrorCode error_code;	/* error code */
	const char *error_string;	/* descriptive string; read-only */
} FI_ErrorInfo;

#define FI_CLEAR_ERROR(ei) \
	do { \
		(ei).error_code = FI_ERROR_NONE; \
		(ei).error_string = NULL; \
	} while (0)

#define FI_SET_ERROR(ei,ec) \
	do { \
		(ei).error_code = (ec); \
		(ei).error_string = fi_error_strings[(ec)]; \
	} while (0)

#define FI_COPY_ERROR(lhs,rhs) \
	do { \
		(lhs).error_code = (rhs).error_code; \
		(lhs).error_string = (rhs).error_string; \
	} while (0)

extern const char *const fi_error_strings[];

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !BTECH_FI_ERRORS_H */
