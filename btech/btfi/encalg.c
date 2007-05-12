/*
 * Built-in encoding algorithms.
 *
 * TODO: Though we've tried to write the Fast Infoset code as portably as
 * possible, this portion is going to be dependent on machine architecture.  It
 * should work on 99% of sane architectures, but there are always the bizarre
 * ones...
 *
 * For simplicity of implementation, we've made a few assumptions which are
 * true of the vast majority of modern machines, but which certainly aren't
 * universal, or guaranteed by the C standard.
 *
 * 1) Our native types are exactly 8, 16, 32, or 64 bits wide.
 * 2) Our native signed integer types use two's complement representation.
 * 3) The number of bits in a "char" (CHAR_BIT) is 8.
 * 4) Floating point values use standard IEEE 754 types.
 *
 * Implications:
 *
 * - We don't need to check the ranges of the types (1, 2), as they match the
 *   Fast Infoset specification exactly--it is impossible to represent a value
 *   that falls outside the range of the Fast Infoset specification.
 * - We don't need to convert to and from two's complement representation (2).
 * - We can perform byte swapping relatively trivially (3).
 * - Translating in-memory data to and from octets is a no-op (3).
 * - We don't need to translate between floating point formats (4).
 *
 * Solving these problems, even solving them in a portable manner, is not
 * particularly difficult (except #4), but byte swapping is the main
 * portability headache in the modern environment (and even then, x86 seems to
 * have conquered most of the market).
 *
 * In any event, with the non-portable code centralized here, it should be
 * relatively easy to rewrite these routines for those special cases, like
 * freakish PDP-10s with a CHAR_BIT of 9, or Crays with bizarre floating point
 * formats.  If you have such a requirement, feel free to submit an encalg.c
 * that addresses the issue. :-)
 */

#include "autoconf.h"

#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "values.h"

#include "encalg.h"


/*
 * Runtime initialization.
 */

static int did_init = 0;

static int is_big_endian = 0; /* safety value; correct for x86 machines */

int
fi_init_encoding_algorithms(void)
{
	static const FI_UInt64 sample_word = 0xFEDCBA9876543210ull;

	const unsigned char *sample_bytes;

	if (did_init) {
		return 1;
	}

	/*
	 * Endian check.  We could perform this at configuration time, but it's
	 * generally more correct to perform at run time, due to issues with
	 * cross compilation and such.  We'll lose a few cycles to branching.
	 *
	 * This test wouldn't catch every bizarre case under the sun (like an
	 * architecture with mixed endianness), but we probably need to special
	 * case those anyway.  The implementation cost isn't worth it on the
	 * majority, sane architectures.
	 *
	 * TODO: We can probably move this sort of thing out to the general
	 * portability layer.
	 */
	sample_bytes = (unsigned char *)&sample_word;

	if (   sample_bytes[0] == 0xFE
	    && sample_bytes[1] == 0xDC
	    && sample_bytes[2] == 0xBA
	    && sample_bytes[3] == 0x98
	    && sample_bytes[4] == 0x76
	    && sample_bytes[5] == 0x54
	    && sample_bytes[6] == 0x32
	    && sample_bytes[7] == 0x10) {
		/* Big endian.  */
		is_big_endian = 1;
	} else if (   sample_bytes[0] == 0x10
	           && sample_bytes[1] == 0x32
	           && sample_bytes[2] == 0x54
	           && sample_bytes[3] == 0x76
	           && sample_bytes[4] == 0x98
	           && sample_bytes[5] == 0xBA
	           && sample_bytes[6] == 0xDC
	           && sample_bytes[7] == 0xFE) {
		/* Little endian.  */
		is_big_endian = 0;
	} else {
		/* We're on a bizarre architecture we don't understand.  */
		/* FIXME: Report an error.  */
		return 0;
	}

	did_init = 1;
	return 1;
}


/*
 * Forward declarations.
 */

static int copy_encoded_size(FI_EncodingContext *,
                             const FI_Value *);
static int copy_encode(FI_EncodingContext *, FI_Octet *,
                       const FI_Value *);
static int copy_decode(FI_EncodingContext *, FI_Value *,
                       FI_Length, const FI_Octet *);

static int copy_encoded_size_items(FI_EncodingContext *,
                                   const FI_Value *, size_t);

static int swap_16_encode(FI_EncodingContext *, FI_Octet *,
                          const FI_Value *);
static void swap_16_decode(FI_Value *, FI_Length, const FI_Octet *);

static int swap_32_encode(FI_EncodingContext *, FI_Octet *,
                          const FI_Value *);
static void swap_32_decode(FI_Value *, FI_Length, const FI_Octet *);

static int swap_64_encode(FI_EncodingContext *, FI_Octet *,
                          const FI_Value *);
static void swap_64_decode(FI_Value *, FI_Length, const FI_Octet *);


/*
 * 10.2: The "hexadecimal" encoding algorithm.
 * 10.3: The "base64" encoding algorithm.
 *
 * These algorithms take a character string in hexadecimal or base64,
 * respectively, and encode them as their binary equivalent.  Since we store
 * everything in binary natively, these encoding algorithms are essentially
 * no-ops in our implementation, simply copying from source to destination
 * buffers.
 *
 * See the comment at the head of this file about portability.
 */

const FI_EncodingAlgorithm fi_ea_hexadecimal = {
	0,
	copy_encoded_size,
	copy_encode,
	copy_decode
}; /* fi_ea_hexadecimal */

const FI_EncodingAlgorithm fi_ea_base64 = {
	0,
	copy_encoded_size,
	copy_encode,
	copy_decode
}; /* fi_ea_base64 */

/*
 * 10.4: The "short" encoding algorithm.
 * 10.5: The "int" encoding algorithm.
 * 10.6: The "long" encoding algorithm.
 *
 * All of these are integer encoding algorithms.  Each takes a signed, two's
 * complement integer of the respective size as input (10.4.2, 10.5.2, 10.6.2),
 * and serializes it with big endian byte order (10.4.3, 10.5.3, 10.6.3).
 *
 * See the comment at the head of this file about portability.
 */

static int
encoded_size_16(FI_EncodingContext *context,
                const FI_Value *src)
{
	return copy_encoded_size_items(context, src, 2);
}

static int
encoded_size_32(FI_EncodingContext *context,
                const FI_Value *src)
{
	return copy_encoded_size_items(context, src, 4);
}

static int
encoded_size_64(FI_EncodingContext *context,
                const FI_Value *src)
{
	return copy_encoded_size_items(context, src, 8);
}

static int
decode_short(FI_EncodingContext *context, FI_Value *dst,
             FI_Length src_len, const FI_Octet *src)
{
	if (!fi_set_value_type(dst, FI_VALUE_AS_SHORT, src_len / 2)) {
		return 0;
	}

	swap_16_decode(dst, src_len, src);
	return 1;
}

static int
decode_int(FI_EncodingContext *context, FI_Value *dst,
           FI_Length src_len, const FI_Octet *src)
{
	if (!fi_set_value_type(dst, FI_VALUE_AS_INT, src_len / 4)) {
		return 0;
	}

	swap_32_decode(dst, src_len, src);
	return 1;
}

static int
decode_long(FI_EncodingContext *context, FI_Value *dst,
             FI_Length src_len, const FI_Octet *src)
{
	if (!fi_set_value_type(dst, FI_VALUE_AS_LONG, src_len / 8)) {
		return 0;
	}

	swap_64_decode(dst, src_len, src);
	return 1;
}

const FI_EncodingAlgorithm fi_ea_short = {
	0,
	encoded_size_16,
	swap_16_encode,
	decode_short
}; /* fi_ea_short */

const FI_EncodingAlgorithm fi_ea_int = {
	0,
	encoded_size_32,
	swap_32_encode,
	decode_int
}; /* fi_ea_int */

const FI_EncodingAlgorithm fi_ea_long = {
	0,
	encoded_size_32,
	swap_64_encode,
	decode_long
}; /* fi_ea_long */

/*
 * 10.8: The "float" encoding algorithm.
 * 10.9: The "double" encoding algorithm.
 *
 * All of these are floating point encoding algorithms.  Each takes a floating
 * point number of the respective size as input (10.8.2, 10.9.2), and
 * serializes it with big endian byte order (10.8.3, 10.9.3).
 *
 * See the comment at the head of this file about portability.
 */

static int
decode_float(FI_EncodingContext *context, FI_Value *dst,
             FI_Length src_len, const FI_Octet *src)
{
	if (!fi_set_value_type(dst, FI_VALUE_AS_FLOAT, src_len / 4)) {
		return 0;
	}

	swap_32_decode(dst, src_len, src);
	return 1;
}

static int
decode_double(FI_EncodingContext *context, FI_Value *dst,
              FI_Length src_len, const FI_Octet *src)
{
	if (!fi_set_value_type(dst, FI_VALUE_AS_DOUBLE, src_len / 8)) {
		return 0;
	}

	swap_64_decode(dst, src_len, src);
	return 1;
}

const FI_EncodingAlgorithm fi_ea_float = {
	0,
	encoded_size_32,
	swap_32_encode,
	decode_float
}; /* fi_ea_float */

const FI_EncodingAlgorithm fi_ea_double = {
	0,
	encoded_size_64,
	swap_64_encode,
	decode_double
}; /* fi_ea_double */


/*
 * Encoding algorithm implementation that simply copies octets.
 *
 * See the comment at the head of this file about portability.
 */

static int
copy_encoded_size(FI_EncodingContext *context,
                  const FI_Value *src)
{
	return copy_encoded_size_items(context, src, 1);
}

static int
copy_encode(FI_EncodingContext *context, FI_Octet *dst,
            const FI_Value *src)
{
	memcpy(dst, fi_get_value(src), fi_get_value_count(src));
	return 1;
}

static int
copy_decode(FI_EncodingContext *context, FI_Value *dst,
            FI_Length src_len, const FI_Octet *src)
{
	if (!fi_set_value_type(dst, FI_VALUE_AS_OCTETS, src_len)) {
		return 0;
	}

	memcpy(fi_get_value_buffer(dst), src, src_len);
	return 1;
}

static int
copy_encoded_size_items(FI_EncodingContext *context,
                        const FI_Value *src, size_t item_size)
{
	size_t count = fi_get_value_count(src);

	if (count > FI_LENGTH_MAX / item_size) {
		return 0;
	}

	context->encoded_size = (FI_Length)(count * item_size);
	return 1;
}


/*
 * Byte-swapping routines.  Since byte-swapping is a symmetric operation, most
 * of these are stubs that call a single width-specific swapping routine (and
 * even that could be parameterized into a single routine, in theory).
 *
 * See the comment at the head of this file about portability.
 */

static void
swap_16(unsigned char *dst, const unsigned char *src, size_t len)
{
	size_t ii;

	for (ii = 0; ii < len; ii += 2) {
		dst[ii + 0] = src[ii + 1];
		dst[ii + 1] = src[ii + 0];
	}
}

static void
swap_32(unsigned char *dst, const unsigned char *src, size_t len)
{
	size_t ii;

	for (ii = 0; ii < len; ii += 4) {
		dst[ii + 0] = src[ii + 3];
		dst[ii + 1] = src[ii + 2];
		dst[ii + 2] = src[ii + 1];
		dst[ii + 3] = src[ii + 0];
	}
}

static void
swap_64(unsigned char *dst, const unsigned char *src, size_t len)
{
	size_t ii;

	for (ii = 0; ii < len; ii += 8) {
		dst[ii + 0] = src[ii + 7];
		dst[ii + 1] = src[ii + 6];
		dst[ii + 2] = src[ii + 5];
		dst[ii + 3] = src[ii + 4];
		dst[ii + 4] = src[ii + 3];
		dst[ii + 5] = src[ii + 2];
		dst[ii + 6] = src[ii + 1];
		dst[ii + 7] = src[ii + 0];
	}
}

static int
swap_16_encode(FI_EncodingContext *context, FI_Octet *dst,
               const FI_Value *src)
{
	assert(fi_get_value_count(src) <= ((size_t)-1) / 2);

	swap_16(dst, fi_get_value(src), 2 * fi_get_value_count(src));
	return 1;
}

static void
swap_16_decode(FI_Value *dst, FI_Length src_len, const FI_Octet *src)
{
	swap_16(fi_get_value_buffer(dst), src, src_len);
}

static int
swap_32_encode(FI_EncodingContext *context, FI_Octet *dst,
               const FI_Value *src)
{
	assert(fi_get_value_count(src) <= ((size_t)-1) / 4);

	swap_32(dst, fi_get_value(src), 4 * fi_get_value_count(src));
	return 1;
}

static void
swap_32_decode(FI_Value *dst, FI_Length src_len, const FI_Octet *src)
{
	swap_32(fi_get_value_buffer(dst), src, src_len);
}

static int
swap_64_encode(FI_EncodingContext *context, FI_Octet *dst,
               const FI_Value *src)
{
	assert(fi_get_value_count(src) <= ((size_t)-1) / 8);

	swap_64(dst, fi_get_value(src), 8 * fi_get_value_count(src));
	return 1;
}

static void
swap_64_decode(FI_Value *dst, FI_Length src_len, const FI_Octet *src)
{
	swap_64(fi_get_value_buffer(dst), src, src_len);
}


/*
 * Unimplemented encoding algorithms.
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
dummy_decode(FI_EncodingContext *context, FI_Value *dst,
             FI_Length src_len, const FI_Octet *src)
{
	/* Always fails.  */
	return 0;
}

const FI_EncodingAlgorithm fi_ea_boolean = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decode
}; /* fi_ea_boolean */

const FI_EncodingAlgorithm fi_ea_uuid = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decode
}; /* fi_ea_uuid */

const FI_EncodingAlgorithm fi_ea_cdata = {
	0,
	dummy_encoded_size,
	dummy_encode,
	dummy_decode
}; /* fi_ea_cdata */
