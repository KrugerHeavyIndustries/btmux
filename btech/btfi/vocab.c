/*
 * Module implementing the vocabulary table data structure required by X.891.
 *
 * Section 8 contains details on the various vocabulary tables maintained.
 */

#include "autoconf.h"

#include <stddef.h>
#include <stdlib.h>

#include <assert.h>

#include "common.h"
#include "encalg.h"

#include "vocab.h"


/*
 * Common declarations.
 */


/*
 * 8.2: Restricted alphabets: Character sets for restricted strings.  Not
 * dynamic.  Entries 1 and 2 are defined in section 9.
 *
 * Entries in this table are strings of Unicode characters, representing the
 * character set.
 */

static const char *const FI_NUMERIC_ALPHABET = "0123456789-+.e ";
static const char *const FI_DATE_AND_TIME_ALPHABET = "012345789-:TZ ";

struct FI_tag_RA_VocabTable {
}; /* FI_tag_RA_VocabTable */

FI_VocabIndex
fi_add_ra(FI_RA_VocabTable *tbl, const FI_Char *alphabet)
{
	return FI_VOCAB_INDEX_NULL;
}

const FI_Char *
fi_get_ra(const FI_RA_VocabTable *tbl, FI_VocabIndex idx)
{
	const FI_Char *alphabet;

	switch (idx) {
	case FI_RA_NUMERIC:
		alphabet = FI_NUMERIC_ALPHABET;
		break;

	case FI_RA_DATE_AND_TIME:
		alphabet = FI_DATE_AND_TIME_ALPHABET;
		break;

	default: /* fall back to lookup table */
		alphabet = NULL;
		break;
	}

	return alphabet;
}


/*
 * 8.3: Encoding algorithms: Algorithms for compactly encoding values (as
 * characters) into octet strings.  Not dynamic.  Entries 1 through 10 are
 * defined in section 10.
 *
 * Entries in this table (for non-built-in algorithms) are identified by URI.
 * We're not going to support external algorithms for the foreseeable future.
 */

struct FI_tag_EA_VocabTable {
}; /* FI_tag_EA_VocabTable */

static FI_EA_VocabTable dummy_ea_table; /* dummy_ea_table */

FI_EA_VocabTable *
fi_create_ea_table(void)
{
	/* We don't support external algorithms, so just use a dummy table.  */
	return &dummy_ea_table;
}

void
fi_destroy_ea_table(FI_EA_VocabTable *tbl)
{
}

FI_VocabIndex
fi_add_ea(FI_EA_VocabTable *tbl, const FI_EncodingAlgorithm *alg)
{
	/* No support for external encoding algorithms.  */
	return FI_VOCAB_INDEX_NULL;
}

const FI_EncodingAlgorithm *
fi_get_ea(const FI_EA_VocabTable *tbl, FI_VocabIndex idx)
{
	const FI_EncodingAlgorithm *alg;

	switch (idx) {
	case FI_EA_HEXADECIMAL:
		alg = &fi_ea_hexadecimal;
		break;

	case FI_EA_BASE64:
		alg = &fi_ea_base64;
		break;

	case FI_EA_SHORT:
		alg = &fi_ea_short;
		break;

	case FI_EA_INT:
		alg = &fi_ea_int;
		break;

	case FI_EA_LONG:
		alg = &fi_ea_long;
		break;

	case FI_EA_BOOLEAN:
		alg = &fi_ea_boolean;
		break;

	case FI_EA_FLOAT:
		alg = &fi_ea_float;
		break;

	case FI_EA_DOUBLE:
		alg = &fi_ea_double;
		break;

	case FI_EA_UUID:
		alg = &fi_ea_uuid;
		break;

	case FI_EA_CDATA:
		alg = &fi_ea_cdata;
		break;

	default:
		/* No support for external encoding algorithms.  */
		alg = NULL;
		break;
	}

	return alg;
}


/*
 * 8.4: Dynamic strings: Character strings.  Dynamic.  Each document has 8:
 *
 * PREFIX
 * NAMESPACE NAME
 * LOCAL NAME
 * OTHER NCNAME
 * OTHER URI
 * ATTRIBUTE VALUE
 * CONTENT CHARACTER CHUNK
 * OTHER STRING
 *
 * Processing rules are defined in section 7.13 and 7.14.
 */

struct FI_tag_DS_VocabTable {
}; /* FI_tag_DS_VocabTable */


/*
 * 8.5: Dynamic names: Name surrogates.  Dynamic.  Each document has 2:
 *
 * ELEMENT NAME
 * ATTRIBUTE NAME
 *
 * Name surrogates are triplets of indices into the PREFIX, NAMESPACE NAME,
 * and LOCAL NAME tables, representing qualified names.  Only the LOCAL NAME
 * index is mandatory.  The PREFIX index requires the NAMESPACE NAME index.
 *
 * The meaning of the three possible kinds of name surrogates is defined in
 * section 8.5.3.  Processing rules are defined in section 7.15 and 7.16.
 */

struct FI_tag_DN_VocabTable {
}; /* FI_tag_DN_VocabTable */
