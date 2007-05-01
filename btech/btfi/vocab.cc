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
 * Generic VocabTable definitions.
 */

void
VocabTable::clear ()
{
	if (parent) {
		base_idx = parent->last_idx + 1;
		last_idx = parent->last_idx;
	}

	for (std::vector<EntryRef>::iterator pp = vocabulary.begin();
	     pp != vocabulary.end();
	     ++pp) {
		pp->entry->resetIndex();
	}

	vocabulary.clear();
}

FI_VocabIndex
VocabTable::acquireIndex(Entry *entry)
{
	if (last_idx >= max_idx) {
		return FI_VOCAB_INDEX_NULL;
	}

	// Assign index.
	vocabulary.push_back(EntryRef(entry));
	return ++last_idx;
}

const VocabTable::EntryRef&
VocabTable::lookupIndex(FI_VocabIndex idx) const
{
	if (idx < base_idx) {
		// Parent index.
		if (!parent) {
			throw IndexOutOfBoundsException ();
		}

		return parent->lookupIndex(idx);
	}

	// TODO: We can assert idx <= last_idx.

	idx -= base_idx;

	if (idx >= vocabulary.size()) {
		throw IndexOutOfBoundsException ();
	}

	return vocabulary[idx];
}


/*
 * 8.2: Restricted alphabets: Character sets for restricted strings.  Not
 * dynamic.  Entries 1 and 2 are defined in section 9.
 *
 * Entries in this table are strings of Unicode characters, representing the
 * character set.
 */

RA_VocabTable::RA_VocabTable()
: TypedVocabTable<value_type> (256), // 7.2.18
  NUMERIC_ALPHABET (get_numeric_alphabet()),
  DATE_AND_TIME_ALPHABET (get_date_and_time_alphabet())
{
	base_idx = 16; // 7.2.19
	last_idx = base_idx - 1;
}

const VocabTable::EntryRef
RA_VocabTable::getEntry(const_value_ref value)
{
	// X.891 section 8.2.2 requires restricted alphabets to contain 2 or
	// more characters.
	if (value.size() < 2) {
		throw InvalidArgumentException ();
	}

	if (value == getValue(NUMERIC_ALPHABET)) {
		return NUMERIC_ALPHABET;
	} else if (value == getValue(DATE_AND_TIME_ALPHABET)) {
		return DATE_AND_TIME_ALPHABET;
	} else {
		return TypedVocabTable<value_type>::getEntry(value);
	}
}

RA_VocabTable::const_value_ref
RA_VocabTable::operator [](FI_VocabIndex idx) const
{
	switch (idx) {
	case FI_RA_NUMERIC:
		return getValue(NUMERIC_ALPHABET);

	case FI_RA_DATE_AND_TIME:
		return getValue(DATE_AND_TIME_ALPHABET);

	default:
		return TypedVocabTable<value_type>::operator [](idx);
	}
}

// Provides singleton NUMERIC_ALPHABET TypedEntry, to avoid static init.
RA_VocabTable::TypedEntry&
RA_VocabTable::get_numeric_alphabet()
{
	static StaticTypedEntry NUMERIC_ALPHABET (FI_RA_NUMERIC,
	                                          "0123456789-+.e ");
	return NUMERIC_ALPHABET;
}

// Provides singleton DATE_AND_TIME_ALPHABET TypedEntry, to avoid static init.
RA_VocabTable::TypedEntry&
RA_VocabTable::get_date_and_time_alphabet()
{
	static StaticTypedEntry DATE_AND_TIME_ALPHABET (FI_RA_DATE_AND_TIME,
	                                                "012345789-:TZ ");
	return DATE_AND_TIME_ALPHABET;
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

EA_VocabTable::EA_VocabTable()
: TypedVocabTable<value_type> (256) // 7.2.18
{
	base_idx = 32; // 7.2.20
	last_idx = base_idx - 1;
}

EA_VocabTable::DynamicTypedEntry *
EA_VocabTable::createTypedEntry(const_value_ref value)
{
	throw UnsupportedOperationException ();
}

EA_VocabTable::const_value_ref
EA_VocabTable::operator [](FI_VocabIndex idx) const
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

DS_VocabTable::DS_VocabTable()
: EMPTY_STRING (get_empty_string())
{
}

const VocabTable::EntryRef
DS_VocabTable::getEntry (const_value_ref value)
{
	if (value.empty()) {
		return EMPTY_STRING;
	} else {
		return TypedVocabTable<value_type>::getEntry (value);
	}
}

DS_VocabTable::const_value_ref
DS_VocabTable::operator [](FI_VocabIndex idx) const
{
	if (idx == FI_VOCAB_INDEX_NULL) {
		return getValue(EMPTY_STRING);
	} else {
		return TypedVocabTable<value_type>::operator [](idx);
	}
}

// Provides singleton EMPTY_STRING TypedEntry, to avoid static init.
DS_VocabTable::TypedEntry&
DS_VocabTable::get_empty_string()
{
	static StaticTypedEntry EMPTY_STRING (FI_VOCAB_INDEX_NULL, "");
	return EMPTY_STRING;
}


/*
 * 7.2.21: The PREFIX table has the following built-ins:
 *
 * xml
 */

PFX_DS_VocabTable::PFX_DS_VocabTable()
: XML_PREFIX (get_xml_prefix())
{
	last_idx = FI_PFX_XML;
	base_idx = last_idx + 1;
}

const VocabTable::EntryRef
PFX_DS_VocabTable::getEntry(const_value_ref value)
{
	if (value == getValue(XML_PREFIX)) {
		return XML_PREFIX;
	} else {
		return TypedVocabTable<value_type>::getEntry (value);
	}
}

PFX_DS_VocabTable::const_value_ref
PFX_DS_VocabTable::operator [](FI_VocabIndex idx) const
{
	if (idx == FI_PFX_XML) {
		return getValue(XML_PREFIX);
	} else {
		return TypedVocabTable<value_type>::operator [](idx);
	}
}

// Provides singleton XML_PREFIX TypedEntry, to avoid static init.
PFX_DS_VocabTable::TypedEntry&
PFX_DS_VocabTable::get_xml_prefix()
{
	static StaticTypedEntry XML_PREFIX (FI_PFX_XML, "xml");
	return XML_PREFIX;
}


/*
 * 7.2.22: The NAMESPACE_NAMES has the following built-ins:
 *
 * http://www.w3.org/XML/1998/namespace
 */

NSN_DS_VocabTable::NSN_DS_VocabTable()
: XML_NAMESPACE (get_xml_namespace())
{
	last_idx = FI_NSN_XML;
	base_idx = last_idx + 1;
}

const VocabTable::EntryRef
NSN_DS_VocabTable::getEntry(const_value_ref value)
{
	if (value == getValue(XML_NAMESPACE)) {
		return XML_NAMESPACE;
	} else {
		return TypedVocabTable<value_type>::getEntry (value);
	}
}

NSN_DS_VocabTable::const_value_ref
NSN_DS_VocabTable::operator [](FI_VocabIndex idx) const
{
	if (idx == FI_NSN_XML) {
		return getValue(XML_NAMESPACE);
	} else {
		return TypedVocabTable<value_type>::operator [](idx);
	}
}

// Provides singleton XML_NAMESPACE TypedEntry, to avoid static init.
NSN_DS_VocabTable::TypedEntry&
NSN_DS_VocabTable::get_xml_namespace()
{
	static StaticTypedEntry XML_NAMESPACE (FI_NSN_XML, "http://www.w3.org/XML/1998/namespace");
	return XML_NAMESPACE;
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

// TODO:
//
// 1) Set it up so that name surrogates are derived automatically from the
//    related name tables.
// 2) Expand the FI_NameSurrogate type to cover all possibilities.
// 3) Add a mechanism to test if a table is full before trying to add new
//    entries to it.
// 4) Extend method signatures to handle the more complex type.
#if 0

const VocabTable::EntryRef
DN_VocabTable::getEntry(const_entry_ref entry) throw (Exception)
{
}

#endif

} // namespace FI
} // namespace BTech
