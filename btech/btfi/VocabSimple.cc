/*
 * Module implementing the vocabulary table data structure required by X.891.
 *
 * Section 8 contains details on the various vocabulary tables maintained.
 */

#include "autoconf.h"

#include <memory>

#include "common.h" // FIXME: only needed for various vocab index definitions
#include "encalg.h"

#include "Exception.hh"
#include "VocabSimple.hh"


// Use auto_ptr to avoid memory leak warnings from valgrind & friends.
using std::auto_ptr;

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
	// Add, then compare, to deal with overflow for -1 case (used to
	// initialize index 0 for empty string tables).  A few cycles slower,
	// sure, but this isn't on the critical path.
	const FI_VocabIndex next_idx = last_idx + 1;

	if (next_idx > max_idx) {
		return FI_VOCAB_INDEX_NULL;
	}

	// Assign index.
	vocabulary.push_back(entry);
	last_idx = next_idx;
	return next_idx;
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
	static RA_VocabTable builtins (true, 256 /* table limit (7.2.18) */);
	return &builtins;
}

RA_VocabTable::RA_VocabTable(bool read_only, FI_VocabIndex max_idx)
: DynamicTypedVocabTable<value_type> (true, max_idx)
{
	// Add built-in restricted alphabets.
	static auto_ptr<Entry>
	entry_1 (addStaticEntry(FI_RA_NUMERIC, "0123456789-+.e "));

	static auto_ptr<Entry>
	entry_2 (addStaticEntry(FI_RA_DATE_AND_TIME, "012345789-:TZ "));

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
	static EA_VocabTable builtins (true, 256 /* table limit (7.2.18) */);
	return &builtins;
}

EA_VocabTable::EA_VocabTable(bool read_only, FI_VocabIndex max_idx)
: TypedVocabTable<value_type> (true, max_idx)
{
	// Add built-in encoding algorithms.
	static auto_ptr<Entry>
	entry_1 (addStaticEntry(FI_EA_HEXADECIMAL, &fi_ea_hexadecimal));

	static auto_ptr<Entry>
	entry_2 (addStaticEntry(FI_EA_BASE64, &fi_ea_base64));

	static auto_ptr<Entry>
	entry_3 (addStaticEntry(FI_EA_SHORT, &fi_ea_short));

	static auto_ptr<Entry>
	entry_4 (addStaticEntry(FI_EA_INT, &fi_ea_int));

	static auto_ptr<Entry>
	entry_5 (addStaticEntry(FI_EA_LONG, &fi_ea_long));

	static auto_ptr<Entry>
	entry_6 (addStaticEntry(FI_EA_BOOLEAN, &fi_ea_boolean));

	static auto_ptr<Entry>
	entry_7 (addStaticEntry(FI_EA_FLOAT, &fi_ea_float));

	static auto_ptr<Entry>
	entry_8 (addStaticEntry(FI_EA_DOUBLE, &fi_ea_double));

	static auto_ptr<Entry>
	entry_9 (addStaticEntry(FI_EA_UUID, &fi_ea_uuid));

	static auto_ptr<Entry>
	entry_10 (addStaticEntry(FI_EA_CDATA, &fi_ea_cdata));

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
	static DS_VocabTable builtins (true, FI_ONE_MEG);
	return &builtins;
}

DS_VocabTable::DS_VocabTable(bool read_only, FI_VocabIndex max_idx)
: DynamicTypedVocabTable<value_type> (true, max_idx)
{
	// Index 0 is the empty string.
	base_idx = 0;
	last_idx = -1;

	// Add built-in strings.
	static auto_ptr<Entry>
	entry_0 (addStaticEntry(FI_VOCAB_INDEX_NULL, ""));
}

DS_VocabTable::DS_VocabTable()
: DynamicTypedVocabTable<value_type> (false, builtin_table())
{
}

DS_VocabTable::DS_VocabTable(TypedVocabTable *parent)
: DynamicTypedVocabTable<value_type> (parent ? false : true,
                                      parent ? parent : builtin_table())
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
	static PFX_DS_VocabTable builtins (true, FI_ONE_MEG);
	return &builtins;
}

PFX_DS_VocabTable::PFX_DS_VocabTable(bool read_only, FI_VocabIndex idx)
: DS_VocabTable (0)
{
	// Add built-in prefix strings.
	static auto_ptr<Entry>
	entry_1 (addStaticEntry(FI_PFX_XML, "xml"));
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
	static NSN_DS_VocabTable builtins (true, FI_ONE_MEG);
	return &builtins;
}

NSN_DS_VocabTable::NSN_DS_VocabTable(bool read_only, FI_VocabIndex max_idx)
: DS_VocabTable (0)
{
	// Add built-in namespace name strings.
	static auto_ptr<Entry>
	entry_1 (addStaticEntry(FI_NSN_XML,
	                        "http://www.w3.org/XML/1998/namespace"));
}

NSN_DS_VocabTable::NSN_DS_VocabTable()
: DS_VocabTable (builtin_table())
{
}

} // namespace FI
} // namespace BTech
