/*
 * Test for the ASN.1 DER module.
 */

#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>

#include "fi/asn1der.h"

#define TEST_FILE "asn1der.test"

static void
die(const char *cause)
{
	perror(cause);
	exit(EXIT_FAILURE);
}

static void
die_asn1(const char *cause, ASN1_Error_State *error_state)
{
	fprintf(stderr, "%s: %s\n", cause, error_state->error_string);
	exit(EXIT_FAILURE);
}

static void
write_file(FILE *fp, const ASN1_Octet *data, ASN1_Length length)
{
	size_t result;

	do {
		result = fwrite(data, sizeof(ASN1_Octet), length, fp);

		if (ferror(fp)) {
			die("fwrite");
		}

		data += result;
		length -= result;
	} while (result < length);
}

static void
read_file(FILE *fp, ASN1_Octet *data, ASN1_Length length)
{
	size_t result;

	do {
		result = fread(data, sizeof(ASN1_Octet), length, fp);

		if (ferror(fp)) {
			die("fread");
		}

		if (feof(fp)) {
			return;
		}

		data += result;
		length -= result;
	} while (result < length);
}

static void
check_contents(const ASN1_TLV *value)
{
	ASN1_Length ii;

	for (ii = 0; ii < value->length; ii++) {
		if (value->contents[ii] != (ASN1_Octet)ii) {
			fprintf(stderr,
			        "check_contents() failed on octet %lu/%lu\n",
			        ii, value->length);
			exit(EXIT_FAILURE);
		}
	}
}

int
main(void)
{
	FILE *fp;

	ASN1_Octet contents[256];
	int ii;

	ASN1_Encoder_State encoder;
	ASN1_Decoder_State decoder;

	/* Generate some test data.  */
	for (ii = 0; ii < sizeof(contents) / sizeof(char); ii++) {
		contents[ii] = ii;
	}

	/*
	 * Test writing.
	 */
	asn1_init_encoder(&encoder);

	if (!(fp = fopen(TEST_FILE, "wb"))) {
		die("fopen");
	}

	/* Test short form.  */
	encoder.current_field.tag_number = 1;
	encoder.current_field.length = 127;

	if (!asn1_encode_tlv(&encoder)) {
		die_asn1("asn1_encode_tlv", &encoder.error_state);
	}

	write_file(fp,
	           asn1_get_buffer_contents(&encoder.buffer),
	           asn1_get_buffer_length(&encoder.buffer));

	write_file(fp, contents, encoder.current_field.length);

	/* Test long form.  */
	encoder.current_field.tag_number = 2;
	encoder.current_field.length = 128;

	if (!asn1_encode_tlv(&encoder)) {
		die_asn1("asn1_encode_tlv", &encoder.error_state);
	}

	write_file(fp,
	           asn1_get_buffer_contents(&encoder.buffer),
	           asn1_get_buffer_length(&encoder.buffer));

	write_file(fp, contents, encoder.current_field.length);

	/* Test longer form.  */
	encoder.current_field.tag_number = 3;
	encoder.current_field.length = 256;

	if (!asn1_encode_tlv(&encoder)) {
		die_asn1("asn1_encode_tlv", &encoder.error_state);
	}

	write_file(fp,
	           asn1_get_buffer_contents(&encoder.buffer),
	           asn1_get_buffer_length(&encoder.buffer));

	write_file(fp, contents, encoder.current_field.length);

	if (fclose(fp) != 0) {
		die("fclose");
	}

	asn1_free_encoder(&encoder);

	/*
	 * Test reading.
	 */
	asn1_init_decoder(&decoder);

	if (!(fp = fopen(TEST_FILE, "rb"))) {
		die("fopen");
	}

	ii = 1;

	while (!feof(fp)) {
		if (!asn1_decode_tlv(&decoder)) {
			ASN1_Octet *buffer;

			if (decoder.error_state.error_code != ASN1_ERROR_OOO) {
				die_asn1("asn1_decode_tlv",
				         &decoder.error_state);
			}

			/* Non-fatal error, get more bytes.  */
			buffer = asn1_write_reserve(&decoder.buffer,
			                            decoder.min_octets);
			if (!buffer) {
				die_asn1("asn1_write_reserve",
				         &decoder.error_state);
			}

			read_file(fp, buffer, decoder.min_octets);
		} else {
			/* Check that value matches.  */
			if (decoder.current_field.tag_number != ii++) {
				fprintf(stderr, "Tag mismatch: %d != %u\n",
				        (ii - 1),
				        decoder.current_field.tag_number);
				exit(EXIT_FAILURE);
			}

			check_contents(&decoder.current_field);
		}
	}

	if (fclose(fp) != 0) {
		die("fclose");
	}

	asn1_free_decoder(&decoder);

	/* Clean up.  */
	remove(TEST_FILE);

	exit(EXIT_SUCCESS);
}
