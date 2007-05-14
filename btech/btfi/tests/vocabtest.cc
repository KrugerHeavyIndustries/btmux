#include "autoconf.h"

#include <iostream>

#include "encalg.h"

#include "Exception.hh"
#include "Name.hh"
#include "Vocabulary.hh"

using std::cerr;
using std::endl;

using namespace BTech::FI;

namespace {

void
die(const char *cause, const char *reason) throw (int)
{
	cerr << cause << ": " << reason << endl;
	throw 1;
}

template<typename T, typename U>
void
test_max(T& table, const U& sample, FI_VocabIndex max = FI_ONE_MEG)
{
	typename T::EntryRef ref = table.getEntry(sample);

	for (FI_VocabIndex ii = ref.getNewIndex(); ii < max; ii++) {
		if (ref.getNewIndex() != (ii + 1)) {
			die("VocabTable::EntryRef::getNewIndex", "Unexpected index");
		}
	}

	if (ref.getNewIndex() != FI_VOCAB_INDEX_NULL) {
		die("VocabTable::EntryRef::getNewIndex", "Didn't clamp at maximum index");
	}
}

void
run_test()
{
	VocabTable::EntryRef ref1, ref2, ref3;
	FI_VocabIndex idx1, idx2, idx3;

	//
	// Construct a few examples of the various tables.
	//
	RA_VocabTable ra_vt1, ra_vt2;
	EA_VocabTable ea_vt1, ea_vt2;
	DS_VocabTable ds_vt1, ds_vt2, ds_vt3;
	DN_VocabTable dn_vt1, dn_vt2;

	//
	// Test restricted alphabet tables.
	//
	ref1 = ra_vt1.getEntry("hello world");
	ref2 = ra_vt2.getEntry("how now brown cow");
	ref3 = ra_vt1.getEntry("hello world");

	idx1 = ref1.getIndex();
	idx2 = ref2.getIndex();
	idx3 = ref3.getNewIndex();

	if (idx1 != 16 || idx2 != 16 || idx3 != 17) {
		die("RA_VocabTable::getEntry(CharString)",
		    "Incorrect indexes assigned");
	}

	if (*ra_vt1[idx1] != "hello world"
	    || *ra_vt2[idx2] != "how now brown cow") {
		die("RA_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	ref2 = ra_vt1.getEntry("bob's your uncle");
	ref3 = ra_vt1.getEntry("hello world");

	idx2 = ref2.getIndex();
	idx3 = ref3.getIndex();

	if (idx2 != 18 || idx3 != 16) {
		die("RA_VocabTable::getEntry(CharString)",
		    "Incorrect indexes assigned");
	}

	try {
		ra_vt1.getEntry(" ");
		ra_vt1.getEntry("");

		die("RA_VocabTable::getEntry(CharString)",
		    "Didn't throw InvalidArgumentException");
	} catch (const InvalidArgumentException& e) {
	}

	try {
		ra_vt1[0];
		ra_vt2[4];

		die("RA_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	try {
		// Test the hole between last built-in and first user index.
		ra_vt2[4];

		die("RA_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	// The following test of disinternment can only check for fatal errors.
	ref1 = ra_vt1.getEntry("bob's your uncle");
	ref1 = 0;

	//test_max(ra_vt1, "..", 256);
	ra_vt1.clear();

	//
	// Test encoding algorithm tables.
	//
	if (*ea_vt2[1] != &fi_ea_hexadecimal
	    || *ea_vt2[2] != &fi_ea_base64
	    || *ea_vt2[3] != &fi_ea_short
	    || *ea_vt2[4] != &fi_ea_int
	    || *ea_vt2[5] != &fi_ea_long
	    || *ea_vt2[6] != &fi_ea_boolean
	    || *ea_vt2[7] != &fi_ea_float
	    || *ea_vt2[8] != &fi_ea_double
	    || *ea_vt2[9] != &fi_ea_uuid
	    || *ea_vt2[10] != &fi_ea_cdata) {
		die("EA_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	try {
		// Test the hole between last built-in and first user index.
		ea_vt1[0];
		ea_vt2[11];

		die("EA_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	// The following test of disinternment can only check for fatal errors.
	ref1 = ra_vt1.getEntry("bob's your uncle");
	ref1 = 0;

	//test_max(ea_vt1, 0, 256);
	//ea_vt1.clear();

	//
	// Test dynamic string tables.
	//
	ref1 = ds_vt1.getEntry("how");
	ref2 = ds_vt2.getEntry("now://brown");
	ref3 = ds_vt3.getEntry("cow");

	idx3 = ref3.getNewIndex();
	idx1 = ref1.getIndex();
	idx2 = ref2.getIndex();

	if (idx1 != 1 || idx2 != 1 || idx3 != 1) {
		die("DS_VocabTable::getEntry(CharString)",
		    "Incorrect indexes assigned");
	}

	if (*ds_vt1[FI_VOCAB_INDEX_NULL] != ""
	    || *ds_vt1[idx1] != "how"
	    || *ds_vt2[idx2] != "now://brown"
	    || *ds_vt3[idx3] != "cow") {
		die("DS_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	if (ds_vt1.getEntry("how").getIndex() != idx1
	    || ds_vt2.getEntry("now://brown").getIndex() != idx2
	    || ds_vt3.getEntry("cow").getNewIndex() != (idx3 + 1)
	    || ds_vt3.getEntry("cow").getIndex() != idx3) {
		die("DS_VocabTable::getEntry(CharString)",
		    "Reverse mapping failed");
	}

	ref3 = ds_vt1.getEntry("");
	ref3 = ref3;

	if (ref3.getIndex() != FI_VOCAB_INDEX_NULL) {
		die("DS_VocabTable::getEntry(CharString)",
		    "Incorrect indexes assigned");
	}

	try {
		ds_vt1[2];
		ds_vt2[2];
		ds_vt3[2];

		die("DS_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	//test_max(ds_vt1, ".");
	ds_vt1.clear();

	//
	// Test built-in entries of PREFIX and NAMESPACE NAME tables.
	//
	PFX_DS_VocabTable pfx_vt;
	NSN_DS_VocabTable nsn_vt;

	if (*pfx_vt[FI_PFX_XML] != "xml") {
		die("PFX_DS_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	if (*nsn_vt[FI_NSN_XML] != "http://www.w3.org/XML/1998/namespace") {
		die("NSN_DS_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	if (pfx_vt.getEntry("xml").getIndex() != FI_PFX_XML) {
		die("PFX_DS_VocabTable.getEntry(CharString)",
		    "Reverse mapping failed");
	}

	if (nsn_vt.getEntry("http://www.w3.org/XML/1998/namespace").getIndex() != FI_NSN_XML) {
		die("NSN_DS_VocabTable.getEntry(CharString)",
		    "Reverse mapping failed");
	}

	//
	// Test dynamic name tables.
	//
	PFX_DS_VocabTable::TypedEntryRef pfx_ref = pfx_vt.getEntry("how");
	NSN_DS_VocabTable::TypedEntryRef nsn_ref = nsn_vt.getEntry("now://brown");
	DS_VocabTable::TypedEntryRef local_ref = ds_vt3.getEntry("cow");

	const Name n1 (local_ref, nsn_ref, pfx_ref);
	const Name n2 (local_ref);

	pfx_ref.getIndex();
	nsn_ref.getIndex();
	local_ref.getIndex();

	idx1 = dn_vt1.getEntry(n1).getIndex();
	idx2 = dn_vt2.getEntry(n2).getIndex();

	if (idx1 != 1 || idx2 != 1) {
		die("DN_VocabTable::getEntry(Name)",
		    "Incorrect indexes assigned");
	}

	if (*dn_vt1[idx1]->pfx_part != *pfx_ref
	    || *dn_vt1[idx1]->nsn_part != *nsn_ref
	    || *dn_vt2[idx1]->local_part != *local_ref
	    || dn_vt2[idx2]->pfx_part
	    || dn_vt2[idx2]->nsn_part
	    || *dn_vt2[idx2]->local_part != *local_ref) {
		die("DN_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	try {
		dn_vt1[0];
		dn_vt2[2];

		die("DN_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	if (dn_vt1.getEntry(n1).getIndex() != idx1
	    || dn_vt2.getEntry(n2).getIndex() != idx2) {
		die("DN_VocabTable::getEntry(Name)",
		    "Reverse mapping failed");
	}

	test_max(dn_vt1, n1);

	// Test addition when the name table is full.
	local_ref = ds_vt1.getEntry("local");
	nsn_ref = nsn_vt.getEntry("namespace");
	pfx_ref = pfx_vt.getEntry("prefix");

	pfx_ref.getIndex();
	nsn_ref.getIndex();
	local_ref.getIndex();

	Name n3 (local_ref, nsn_ref, pfx_ref);

	if (dn_vt1.getEntry(n3).getIndex() != FI_VOCAB_INDEX_NULL) {
		die("DN_VocabTable::getEntry(Name)",
		    "Didn't clamp at maximum index");
	}

	idx1 = ds_vt1.getEntry("intervening").getIndex();
	idx2 = nsn_vt.getEntry("intervening").getIndex();
	idx3 = pfx_vt.getEntry("intervening").getIndex();

	if ((idx1 != local_ref.getIndex() + 1)
	    || (idx2 != nsn_ref.getIndex() + 1)
	    || (idx3 != pfx_ref.getIndex() + 1)) {
		die("DN_VocabTable::EntryRef::getIndex()",
		    "Didn't properly assign string indexes");
	}

	// Test addition when a string table is full.
	dn_vt1.clear();
	test_max(nsn_vt, "namespace");

	local_ref = ds_vt1.getEntry("local 2");
	nsn_ref = nsn_vt.getEntry("namespace 2");
	pfx_ref = pfx_vt.getEntry("prefix 2");

	pfx_ref.getIndex();
	nsn_ref.getIndex();
	local_ref.getIndex();

	Name n4 (local_ref, nsn_ref, pfx_ref);

	if (dn_vt1.getEntry(n4).getIndex() != FI_VOCAB_INDEX_NULL) {
		die("DN_VocabTable::getEntry(Name)",
		    "Didn't clamp at component string table full");
	}

	if ((idx1 != local_ref.getIndex() - 1)
	    || (FI_VOCAB_INDEX_NULL != nsn_ref.getIndex())
	    || (idx3 != pfx_ref.getIndex() - 1)) {
		die("DS_VocabTable::EntryRef::getIndex()",
		    "Didn't properly assign string indexes");
	}
}

void
dv_table_test()
{
	VocabTable::EntryRef ref1, ref2, ref3;
	FI_VocabIndex idx1, idx2, idx3;

	//
	// Test dynamic value tables.
	//
	DV_VocabTable dv_vt1, dv_vt2, dv_vt3;

	const FI_Int32 an_int32 = 0x12345678;

	const Value an_int32_value (FI_VALUE_AS_INT, 1, &an_int32);
	const Value an_octet_value (FI_VALUE_AS_OCTETS, 3, "how");

	ref1 = dv_vt1.getEntry(an_octet_value);
	ref2 = dv_vt2.getEntry(Value ());
	ref3 = dv_vt3.getEntry(an_int32_value);

	idx3 = ref3.getNewIndex();
	idx1 = ref1.getIndex();
	idx2 = ref2.getIndex();

	if (idx1 != 1 || idx2 != FI_VOCAB_INDEX_NULL || idx3 != 1) {
		die("DV_VocabTable::getEntry(Value)",
		    "Incorrect indexes assigned");
	}

	if (dv_vt1[FI_VOCAB_INDEX_NULL]->getType() != FI_VALUE_AS_NULL
	    || dv_vt1[FI_VOCAB_INDEX_NULL]->getCount() != 0
	    || *dv_vt1[idx1] < an_octet_value || an_octet_value < *dv_vt1[idx1] 
	    || *Value_cast<FI_Int32>(*dv_vt3[idx3]) != an_int32) {
		die("DV_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	if (dv_vt1.getEntry(an_octet_value).getIndex() != idx1
	    || dv_vt2.getEntry(Value ()).getIndex() != idx2
	    || dv_vt3.getEntry(an_int32_value).getNewIndex() != (idx3 + 1)
	    || dv_vt3.getEntry(an_int32_value).getIndex() != idx3) {
		die("DV_VocabTable::getEntry(Value)",
		    "Reverse mapping failed");
	}

	try {
		dv_vt1[2];
		dv_vt2[2];
		dv_vt3[2];

		die("DV_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	test_max(dv_vt1, an_int32_value);
	dv_vt1.clear();
}

} // anonymous namespace

int
main()
{
	try {
		run_test();
		dv_table_test();
	} catch (int e) {
		return e;
	}

	return 0;
}
