/*
 * Module implementing the vocabulary table data structure required by X.891.
 *
 * Section 8 contains details on the various vocabulary tables maintained.
 */

#include "autoconf.h"

#include "common.h"
#include "encalg.h"

#include "vocab.hh"


namespace BTech {
namespace FI {

/*
 * 8.2: Restricted alphabets: Character sets for restricted strings.  Not
 * dynamic.  Entries 1 and 2 are defined in section 9.
 *
 * Entries in this table are strings of Unicode characters, representing the
 * character set.
 */

namespace {

const CharString NUMERIC_ALPHABET ("0123456789-+.e ");
const CharString DATE_AND_TIME_ALPHABET ("012345789-:TZ ");

} // anonymous namespace

const VocabIndex RA_VocabTable::MAX = 256; // 7.2.18
const VocabIndex RA_VocabTable::LAST_BUILTIN = FI_RA_DATE_AND_TIME;
const VocabIndex RA_VocabTable::FIRST_ADDED = 16; // 7.2.19

void
RA_VocabTable::clear() throw ()
{
	alphabets.clear();
}

VocabIndex
RA_VocabTable::add(const_entry_ref entry) throw (Exception)
{
	// X.891 section 8.2.2 requires restricted alphabets to contain 2 or
	// more characters.
	if (entry.size() < 2) {
		throw InvalidArgumentException ();
	}

	VocabIndex nextIndex = FIRST_ADDED + alphabets.size();

	if (nextIndex > MAX) {
		return FI_VOCAB_INDEX_NULL;
	}

	alphabets.push_back(entry);
	return nextIndex;
}

RA_VocabTable::const_entry_ref
RA_VocabTable::operator[](VocabIndex idx) const throw (Exception)
{
	switch (idx) {
	case FI_RA_NUMERIC:
		return NUMERIC_ALPHABET;

	case FI_RA_DATE_AND_TIME:
		return DATE_AND_TIME_ALPHABET;

	default:
		// Fall back to look-up table.
		if (idx <= FI_VOCAB_INDEX_NULL) {
			throw IndexOutOfBoundsException ();
		} else if (idx < FIRST_ADDED) {
			throw InvalidArgumentException ();
		}

		idx -= FIRST_ADDED;

		if (idx >= alphabets.size()) {
			throw IndexOutOfBoundsException ();
		}

		return alphabets[idx];
	}
}

VocabIndex
RA_VocabTable::size() const
{
	if (alphabets.empty()) {
		return LAST_BUILTIN;
	} else {
		return FIRST_ADDED + alphabets.size() - 1;
	}
}


/*
 * 8.3: Encoding algorithms: Algorithms for compactly encoding values (as
 * characters) into octet strings.  Not dynamic.  Entries 1 through 10 are
 * defined in section 10.
 *
 * Entries in this table (for non-built-in algorithms) are identified by URI.
 * We're not going to support external algorithms for the foreseeable future.
 *
 * Encoding algorithms 1-31 are reserved for standard use (7.2.20).
 */

namespace {

const FI_EncodingAlgorithm *ea_hexadecimal_ptr = &fi_ea_hexadecimal;
const FI_EncodingAlgorithm *ea_base64_ptr = &fi_ea_base64;
const FI_EncodingAlgorithm *ea_short_ptr = &fi_ea_short;
const FI_EncodingAlgorithm *ea_int_ptr = &fi_ea_int;
const FI_EncodingAlgorithm *ea_long_ptr = &fi_ea_long;
const FI_EncodingAlgorithm *ea_boolean_ptr = &fi_ea_boolean;
const FI_EncodingAlgorithm *ea_float_ptr = &fi_ea_float;
const FI_EncodingAlgorithm *ea_double_ptr = &fi_ea_double;
const FI_EncodingAlgorithm *ea_uuid_ptr = &fi_ea_uuid;
const FI_EncodingAlgorithm *ea_cdata_ptr = &fi_ea_cdata;

} // anonymous namespace

const VocabIndex EA_VocabTable::MAX = 256; // 7.2.18
const VocabIndex EA_VocabTable::LAST_BUILTIN = FI_EA_CDATA;
const VocabIndex EA_VocabTable::FIRST_ADDED = 32; // 7.2.20

void
EA_VocabTable::clear() throw ()
{
	// XXX: No support for external encoding algorithms.
}

EA_VocabTable::const_entry_ref
EA_VocabTable::operator[](VocabIndex idx) const throw (Exception)
{
	switch (idx) {
	case FI_EA_HEXADECIMAL:
		return ea_hexadecimal_ptr;

	case FI_EA_BASE64:
		return ea_base64_ptr;

	case FI_EA_SHORT:
		return ea_short_ptr;

	case FI_EA_INT:
		return ea_int_ptr;

	case FI_EA_LONG:
		return ea_long_ptr;

	case FI_EA_BOOLEAN:
		return ea_boolean_ptr;

	case FI_EA_FLOAT:
		return ea_float_ptr;

	case FI_EA_DOUBLE:
		return ea_double_ptr;

	case FI_EA_UUID:
		return ea_uuid_ptr;

	case FI_EA_CDATA:
		return ea_cdata_ptr;

	default:
		// XXX: No support for external encoding algorithms.
		throw IndexOutOfBoundsException ();
	}
}

VocabIndex
EA_VocabTable::size() const
{
	return LAST_BUILTIN;
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

namespace {

const CharString EMPTY_STRING ("");

} // anonymous namespace

const VocabIndex DS_VocabTable::MAX = FI_ONE_MEG; // 7.2.18

void
DS_VocabTable::clear() throw ()
{
	strings.clear();
	reverse_map.clear();
}

VocabIndex
DS_VocabTable::add(const_entry_ref entry) throw (Exception)
{
	if (entry.empty()) {
		throw InvalidArgumentException ();
	}

	VocabIndex nextIndex = size() + 1;

	if (nextIndex > MAX) {
		return FI_VOCAB_INDEX_NULL;
	}

	strings.push_back(entry);
	reverse_map[entry] = nextIndex;
	return nextIndex;
}

DS_VocabTable::const_entry_ref
DS_VocabTable::operator[](VocabIndex idx) const throw (Exception)
{
	if (idx == FI_VOCAB_INDEX_NULL) {
		return EMPTY_STRING;
	} else if (idx < FI_VOCAB_INDEX_NULL || idx > size()) {
		throw IndexOutOfBoundsException ();
	}

	return strings[idx - 1];
}

VocabIndex
DS_VocabTable::find(const_entry_ref entry) const throw (Exception)
{
	if (entry.empty()) {
		throw InvalidArgumentException ();
	}

	string_map_type::const_iterator p = reverse_map.find(entry);
	if (p == reverse_map.end()) {
		return FI_VOCAB_INDEX_NULL;
	}

	return p->second;
}


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

const VocabIndex DN_VocabTable::MAX = FI_ONE_MEG; // 7.2.18

void
DN_VocabTable::clear() throw ()
{
	names.clear();
	reverse_map.clear();
}

VocabIndex
DN_VocabTable::add(const_entry_ref entry) throw (Exception)
{
	if (entry.local_idx == FI_VOCAB_INDEX_NULL) {
		throw InvalidArgumentException ();
	}

	VocabIndex nextIndex = size() + 1;

	if (nextIndex > MAX) {
		return FI_VOCAB_INDEX_NULL;
	}

	names.push_back(entry);
	reverse_map[entry] = nextIndex;
	return nextIndex;
}

DN_VocabTable::const_entry_ref
DN_VocabTable::operator[](VocabIndex idx) const throw (Exception)
{
	if (idx <= FI_VOCAB_INDEX_NULL || idx > size()) {
		throw IndexOutOfBoundsException ();
	}

	return names[idx - 1];
}

VocabIndex
DN_VocabTable::find(const_entry_ref entry) const throw (Exception)
{
	name_map_type::const_iterator p = reverse_map.find(entry);
	if (p == reverse_map.end()) {
		return FI_VOCAB_INDEX_NULL;
	}

	return p->second;
}

} // namespace FI
} // namespace BTech
