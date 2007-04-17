#include "autoconf.h"

#include "stream.h"

#include "encalg.h"


/*
 * Non-working implementations of the encoding algorithms.
 */

static int
dummy_encode(FI_OctetStream *dst, const void *src)
{
	/* Always fails.  */
	return 0;
}

static int
dummy_decode(void *dst, FI_OctetStream *src)
{
	/* Always fails.  */
	return 0;
}

const FI_EncodingAlgorithm fi_ea_hexadecimal = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_hexadecimal */

const FI_EncodingAlgorithm fi_ea_base64 = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_base64 */

const FI_EncodingAlgorithm fi_ea_short = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_short */

const FI_EncodingAlgorithm fi_ea_int = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_int */

const FI_EncodingAlgorithm fi_ea_long = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_long */

const FI_EncodingAlgorithm fi_ea_boolean = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_boolean */

const FI_EncodingAlgorithm fi_ea_float = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_float */

const FI_EncodingAlgorithm fi_ea_double = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_double */

const FI_EncodingAlgorithm fi_ea_uuid = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_uuid */

const FI_EncodingAlgorithm fi_ea_cdata = {
	dummy_encode,
	dummy_decode
}; /* fi_ea_cdata */
