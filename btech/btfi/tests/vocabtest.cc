#include "autoconf.h"

#include <iostream>

#include "encalg.h"

#include "vocab.hh"
#include "Name.hh"

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
	for (FI_VocabIndex ii = table.createEntry(sample).getIndex();
	     ii < max;
	     ii++) {
		if (table.createEntry(sample).getIndex() != (ii + 1)) {
			die("VocabTable<T>::createEntry", "Unexpected index");
		}
	}

	if (table.createEntry(sample).getIndex() != FI_VOCAB_INDEX_NULL) {
		die("VocabTable<T>::add", "Didn't clamp at maximum index");
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
	ref3 = ra_vt1.createEntry("hello world");

	idx3 = ref3.getIndex();
	idx1 = ref1.getIndex();
	idx2 = ref2.getIndex();

	if (idx1 != 17 || idx2 != 16 || idx3 != 16) {
		die("RA_VocabTable::getEntry(CharString)",
		    "Incorrect indexes assigned");
	}

	if (ra_vt1[idx1] != "hello world"
	    || ra_vt2[idx2] != "how now brown cow") {
		die("RA_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	ref2 = ra_vt1.getEntry("bob's your uncle");
	ref3 = ra_vt1.getEntry("hello world");

	idx2 = ref2.getIndex();
	idx3 = ref3.getIndex();

	if (idx2 != 18 || idx3 != 17) {
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
	ref1.release();

	//test_max(ra_vt1, "..", 256);
	ra_vt1.clear();

	//
	// Test encoding algorithm tables.
	//
	try {
		ea_vt1.createEntry(0);
		ea_vt2.createEntry(0);

		die("EA_VocabTable::createEntry(FI_EncodingAlgorithm *)",
		    "Didn't throw UnsupportedOperationException");
	} catch (const UnsupportedOperationException& e) {
	}

	try {
		ea_vt1.getEntry(0);
		ea_vt2.getEntry(0);

		die("EA_VocabTable::getEntry(FI_EncodingAlgorithm *)",
		    "Didn't throw UnsupportedOperationException");
	} catch (const UnsupportedOperationException& e) {
	}

	if (ea_vt2[1] != &fi_ea_hexadecimal
	    || ea_vt2[2] != &fi_ea_base64
	    || ea_vt2[3] != &fi_ea_short
	    || ea_vt2[4] != &fi_ea_int
	    || ea_vt2[5] != &fi_ea_long
	    || ea_vt2[6] != &fi_ea_boolean
	    || ea_vt2[7] != &fi_ea_float
	    || ea_vt2[8] != &fi_ea_double
	    || ea_vt2[9] != &fi_ea_uuid
	    || ea_vt2[10] != &fi_ea_cdata) {
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
	ref1.release();

	// XXX: test_max() doesn't make sense when we can't createEntry().
	//test_max(ea_vt1, 0, 256);
	//ea_vt1.clear();

	//
	// Test dynamic string tables.
	//
	ref1 = ds_vt1.getEntry("how");
	ref2 = ds_vt2.getEntry("now://brown");
	ref3 = ds_vt3.createEntry("cow");

	idx3 = ref3.getIndex();
	idx1 = ref1.getIndex();
	idx2 = ref2.getIndex();

	if (idx1 != 1 || idx2 != 1 || idx3 != 1) {
		die("DS_VocabTable::getEntry(CharString)",
		    "Incorrect indexes assigned");
	}

	if (ds_vt1[FI_VOCAB_INDEX_NULL] != ""
	    || ds_vt1[idx1] != "how"
	    || ds_vt2[idx2] != "now://brown"
	    || ds_vt3[idx3] != "cow") {
		die("DS_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	if (ds_vt1.getEntry("how").getIndex() != idx1
	    || ds_vt2.getEntry("now://brown").getIndex() != idx2
	    || ds_vt3.getEntry("cow").getIndex() != (idx3 + 1)) {
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

	if (pfx_vt[FI_PFX_XML] != "xml") {
		die("PFX_DS_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	if (nsn_vt[FI_NSN_XML] != "http://www.w3.org/XML/1998/namespace") {
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
	ref1 = pfx_vt.getEntry("how");
	ref2 = nsn_vt.getEntry("now://brown");
	ref3 = ds_vt3.getEntry("cow");

	const Name n1 (ref3, ref2, ref1);
	const Name n2 (ref3);

	idx1 = dn_vt1.getEntry(n1).getIndex();
	idx2 = dn_vt2.getEntry(n2).getIndex();

	if (idx1 != 1 || idx2 != 1) {
		die("DN_VocabTable::getEntry(Name)",
		    "Incorrect indexes assigned");
	}

	if (dn_vt1[idx1].pfx_part != ref1
	    || dn_vt1[idx1].nsn_part != ref2
	    || dn_vt2[idx1].local_part != ref3
	    || dn_vt2[idx2].pfx_part != 0
	    || dn_vt2[idx2].nsn_part != 0
	    || dn_vt2[idx2].local_part != ref3) {
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
	ref1 = ds_vt1.getEntry("local");
	ref2 = nsn_vt.getEntry("namespace");
	ref3 = pfx_vt.getEntry("prefix");

	Name n3 (ref1, ref2, ref3);

	if (dn_vt1.getEntry(n3).getIndex() != FI_VOCAB_INDEX_NULL) {
		die("DN_VocabTable::getEntry(Name)",
		    "Didn't clamp at maximum index");
	}

	idx1 = ds_vt1.getEntry("intervening").getIndex();
	idx2 = nsn_vt.getEntry("intervening").getIndex();
	idx3 = pfx_vt.getEntry("intervening").getIndex();

	fprintf(stderr, "%d %d %d\n", idx1, idx2, idx3);
	fprintf(stderr, "%d %d %d\n", ref1.getIndex(), ref2.getIndex(), ref3.getIndex());
}

} // anonymous namespace

int
main()
{
	try {
		run_test();
	} catch (int e) {
		return e;
	}

	return 0;
}
