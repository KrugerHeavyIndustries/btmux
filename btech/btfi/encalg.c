/*
 * Built-in encoding algorithms.
 *
 * TODO: Though we've tried to write this code as portably as possible, this
 * portion is going to be dependent on machine architecture.
 */

#include "autoconf.h"

#include <stddef.h>

#include "stream.h"
#include "values.h"

#include "encalg.h"


int
fi_init_encoding_algorithms(void)
{
	return 1;
}


/*
 * Non-working implementations of the encoding algorithms.
 */

static int
dummy_encoded_size(FI_EncodingContext *context,
                   const FI_Value *src)
{
	/* Always fails.  */
	return 0;
}

static int
dummy_encode(FI_EncodingContext *context, FI_Octet *dst,
             const FI_Value *src)
{
	/* Always fails.  */
	return 0;
}

static int
dummy_decoded_size(FI_EncodingContext *context,
                   FI_Length src_len, const FI_Octet *src)
{
	/* Always fails.  */
	return 0;
}

static int
dummy_decode(FI_EncodingContext *context, FI_Value *dst,
             FI_Length src_len, const FI_Octet *src)
{
	/* Always fails.  */
	return 0;
}


const FI_EncodingAlgorithm fi_ea_hexadecimal = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_hexadecimal */

const FI_EncodingAlgorithm fi_ea_base64 = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_base64 */

const FI_EncodingAlgorithm fi_ea_short = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_short */

const FI_EncodingAlgorithm fi_ea_int = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_int */

const FI_EncodingAlgorithm fi_ea_long = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_long */

const FI_EncodingAlgorithm fi_ea_boolean = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_boolean */

const FI_EncodingAlgorithm fi_ea_float = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_float */

const FI_EncodingAlgorithm fi_ea_double = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_double */

const FI_EncodingAlgorithm fi_ea_uuid = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_uuid */

const FI_EncodingAlgorithm fi_ea_cdata = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decoded_size,
	dummy_decode
}; /* fi_ea_cdata */
