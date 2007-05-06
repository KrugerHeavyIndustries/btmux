/*
 * Module implementing the vocabulary table data structure required by X.891.
 *
 * Section 8 contains details on the various vocabulary tables maintained.
 */

#include "autoconf.h"

#include "common.h"
#include "encalg.h"

#include "Exception.hh"
#include "VocabSimple.hh"


namespace BTech {
namespace FI {

/*
 * Generic VocabTable definitions.
 */

void
VocabTable::clear()
{
	if (read_only) {
		throw UnsupportedOperationException ();
	}

	if (parent) {
		last_idx = parent->last_idx;
	} else {
		last_idx = 0;
	}

	base_idx = last_idx + 1;

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
	vocabulary.push_back(entry);
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

RA_VocabTable::TypedVocabTable *
RA_VocabTable::builtin_table()
{
	static RA_VocabTable builtins (true);
	return &builtins;
}

RA_VocabTable::RA_VocabTable(bool read_only)
: DynamicTypedVocabTable<value_type> (true, 256 /* table limit (7.2.18) */)
{
	// Built-in restricted alphabets.
	static CharString NUMERIC_ALPHABET = "0123456789-+.e ";
	static CharString DATE_AND_TIME_ALPHABET = "012345789-:TZ ";

	// Built-in restricted alphabet entries.
	static StaticTypedEntry
	NUMERIC_ALPHABET_ENTRY (FI_RA_NUMERIC, NUMERIC_ALPHABET);

	static StaticTypedEntry
	DATE_AND_TIME_ALPHABET_ENTRY (FI_RA_DATE_AND_TIME,
	                              DATE_AND_TIME_ALPHABET);

	// Add built-in restricted alphabet entries.
	// TODO: Assert we acquire the correct indexes.
	acquireIndex(&NUMERIC_ALPHABET_ENTRY);
	acquireIndex(&DATE_AND_TIME_ALPHABET_ENTRY);

	// Up to 15 reserved for built-in (7.2.19).
	last_idx = 15;
}

RA_VocabTable::RA_VocabTable()
: DynamicTypedVocabTable<value_type> (false, builtin_table())
{
}

const RA_VocabTable::TypedEntryRef
RA_VocabTable::getEntry(const_reference value)
{
	// X.891 section 8.2.2 requires restricted alphabets to contain 2 or
	// more characters.
	if (value.size() < 2) {
		throw InvalidArgumentException ();
	}

	return DynamicTypedVocabTable<value_type>::getEntry(value);
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

EA_VocabTable *
EA_VocabTable::builtin_table()
{
	static EA_VocabTable builtins (true);
	return &builtins;
}

EA_VocabTable::EA_VocabTable(bool read_only)
: TypedVocabTable<value_type> (true, 256 /* table limit (7.2.18) */)
{
	// Built-in encoding algorithm entries.
	static StaticTypedEntry
	HEXADECIMAL_ENTRY (FI_EA_HEXADECIMAL, fi_ea_hexadecimal);

	static StaticTypedEntry
	BASE64_ENTRY (FI_EA_BASE64, fi_ea_base64);

	static StaticTypedEntry
	SHORT_ENTRY (FI_EA_SHORT, fi_ea_short);

	static StaticTypedEntry
	INT_ENTRY (FI_EA_INT, fi_ea_int);

	static StaticTypedEntry
	LONG_ENTRY (FI_EA_LONG, fi_ea_long);

	static StaticTypedEntry
	BOOLEAN_ENTRY (FI_EA_BOOLEAN, fi_ea_boolean);

	static StaticTypedEntry
	FLOAT_ENTRY (FI_EA_FLOAT, fi_ea_float);

	static StaticTypedEntry
	DOUBLE_ENTRY (FI_EA_DOUBLE, fi_ea_double);

	static StaticTypedEntry
	CDATA_ENTRY (FI_EA_CDATA, fi_ea_cdata);

	static StaticTypedEntry
	UUID_ENTRY (FI_EA_UUID, fi_ea_uuid);

	// Add built-in encoding algorithm entries.
	// TODO: Assert we acquire the correct indexes.
	acquireIndex(&HEXADECIMAL_ENTRY);
	acquireIndex(&BASE64_ENTRY);
	acquireIndex(&SHORT_ENTRY);
	acquireIndex(&INT_ENTRY);
	acquireIndex(&LONG_ENTRY);
	acquireIndex(&BOOLEAN_ENTRY);
	acquireIndex(&FLOAT_ENTRY);
	acquireIndex(&DOUBLE_ENTRY);
	acquireIndex(&CDATA_ENTRY);
	acquireIndex(&UUID_ENTRY);

	// Up to 31 reserved for built-in (7.2.20).
	last_idx = 31;
}

EA_VocabTable::EA_VocabTable()
: TypedVocabTable<value_type> (false, builtin_table())
{
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

DS_VocabTable::TypedVocabTable *
DS_VocabTable::builtin_table()
{
	static DS_VocabTable builtins (true);
	return &builtins;
}

DS_VocabTable::DS_VocabTable(bool read_only)
: DynamicTypedVocabTable<value_type> (true, FI_ONE_MEG)
{
	// Built-in strings.
	static CharString EMPTY_STRING = "";

	// Built-in string entries.
	static StaticTypedEntry
	EMPTY_STRING_ENTRY (FI_VOCAB_INDEX_NULL, EMPTY_STRING);

	// Add built-in string entries.
	// TODO: Assert we acquire the correct indexes.
	acquireIndex(&EMPTY_STRING_ENTRY);

	// Index 0 is the empty string.
	base_idx = 0;
	last_idx = 0;
}

DS_VocabTable::DS_VocabTable()
: DynamicTypedVocabTable<value_type> (false, builtin_table())
{
}

DS_VocabTable::DS_VocabTable(TypedVocabTable *parent)
: DynamicTypedVocabTable<value_type> (true, parent)
{
}


/*
 * 7.2.21: The PREFIX table has the following built-ins:
 *
 * xml
 */

DS_VocabTable *
PFX_DS_VocabTable::builtin_table()
{
	static PFX_DS_VocabTable builtins (true);
	return &builtins;
}

PFX_DS_VocabTable::PFX_DS_VocabTable(bool read_only)
{
	// Built-in prefix strings.
	static CharString XML_PREFIX = "xml";

	// Built-in prefix string entries.
	static StaticTypedEntry XML_PREFIX_ENTRY (FI_PFX_XML, XML_PREFIX);

	// Add built-in prefix string entries.
	// TODO: Assert we acquire the correct indexes.
	acquireIndex(&XML_PREFIX_ENTRY);
}

PFX_DS_VocabTable::PFX_DS_VocabTable()
: DS_VocabTable (builtin_table())
{
}


/*
 * 7.2.22: The NAMESPACE_NAMES has the following built-ins:
 *
 * http://www.w3.org/XML/1998/namespace
 */

DS_VocabTable *
NSN_DS_VocabTable::builtin_table()
{
	static NSN_DS_VocabTable builtins (true);
	return &builtins;
}

NSN_DS_VocabTable::NSN_DS_VocabTable(bool read_only)
{
	// Built-in namespace name strings.
	static CharString
	XML_NAMESPACE = "http://www.w3.org/XML/1998/namespace";

	// Built-in namespace name string entries.
	static StaticTypedEntry
	XML_NAMESPACE_ENTRY (FI_NSN_XML, XML_NAMESPACE);

	// Add built-in namespace name string entries.
	// TODO: Assert we acquire the correct indexes.
	acquireIndex(&XML_NAMESPACE_ENTRY);
}

NSN_DS_VocabTable::NSN_DS_VocabTable()
: DS_VocabTable (builtin_table())
{
}

} // namespace FI
} // namespace BTech
