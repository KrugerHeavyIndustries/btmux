#include "autoconf.h"

#include <iostream>

#include "encalg.h"

#include "vocab.hh"

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
test_max(T& table, const U& entry)
{
	for (FI_VocabIndex ii = table.add(entry); ii < T::MAX; ii++) {
		if (table.add(entry) != (ii + 1)
		    || table.size() != (ii + 1)) {
			die("VocabTable<T>::add", "Index out of order");
		}
	}

	if (table.add(entry) != FI_VOCAB_INDEX_NULL
	    || table.size() != T::MAX) {
		die("VocabTable<T>::add", "Didn't clamp at maximum index");
	}

	table.clear();
}

void
run_test()
{
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
	idx1 = ra_vt1.add("hello world");
	idx2 = ra_vt2.add("how now brown cow");

	if (idx1 != 16 || idx2 != 16) {
		die("RA_VocabTable::add(CharString)",
		    "Incorrect indexes assigned");
	}

	try {
		ra_vt1.add(" ");
		ra_vt1.add("");

		die("RA_VocabTable::add(CharString)",
		    "Didn't throw InvalidArgumentException");
	} catch (const InvalidArgumentException& e) {
	}

	if (ra_vt1[idx1] != "hello world"
	    || ra_vt2[idx2] != "how now brown cow") {
		die("RA_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	try {
		ra_vt1[0];
		ra_vt2[4];

		die("RA_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	try {
		ra_vt2[4];

		die("RA_VocabTable[FI_VocabIndex]",
		    "Didn't throw InvalidArgumentException");
	} catch (const InvalidArgumentException& e) {
	}

	try {
		ra_vt1.find("hello world");
		ra_vt2.find("how now brown cow");

		die("RA_VocabTable::find(CharString)",
		    "Didn't throw UnsupportedOperationException");
	} catch (const UnsupportedOperationException& e) {
	}

	test_max(ra_vt1, "..");

	//
	// Test encoding algorithm tables.
	//
	try {
		ea_vt1.add(0);
		ea_vt2.add(0);

		die("EA_VocabTable::add(FI_EncodingAlgorithm *)",
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
		ea_vt1[0];
		ea_vt2[11];

		die("EA_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	try {
		ea_vt1.find(0);
		ea_vt2.find(0);

		die("EA_VocabTable::find(FI_EncodingAlgorithm *)",
		    "Didn't throw UnsupportedOperationException");
	} catch (const UnsupportedOperationException& e) {
	}

	// XXX: test_max() doesn't make sense when we can't add().
	//test_max(ea_vt1, 256);

	//
	// Test dynamic string tables.
	//

	idx1 = ds_vt1.add("how");
	idx2 = ds_vt2.add("now://brown");
	idx3 = ds_vt3.add("cow");

	if (idx1 != 1 || idx2 != 1 || idx3 != 1) {
		die("DS_VocabTable::add(CharString)",
		    "Incorrect indexes assigned");
	}

	try {
		ds_vt1.add("");

		die("DS_VocabTable::add(CharString)",
		    "Didn't throw InvalidArgumentException");
	} catch (const InvalidArgumentException& e) {
	}

	if (ds_vt1[FI_VOCAB_INDEX_NULL] != ""
	    || ds_vt1[idx1] != "how"
	    || ds_vt2[idx2] != "now://brown"
	    || ds_vt3[idx3] != "cow") {
		die("DS_VocabTable[FI_VocabIndex]",
		    "Forward mapping failed");
	}

	try {
		ds_vt1[2];
		ds_vt2[2];
		ds_vt3[2];

		die("DS_VocabTable[FI_VocabIndex]",
		    "Didn't throw IndexOutOfBoundsException");
	} catch (const IndexOutOfBoundsException& e) {
	}

	if (ds_vt1.find("how") != idx1
	    || ds_vt2.find("now://brown") != idx2
	    || ds_vt3.find("cow") != idx3
	    || ds_vt1.find("now://brown") != FI_VOCAB_INDEX_NULL
	    || ds_vt2.find("cow") != FI_VOCAB_INDEX_NULL
	    || ds_vt3.find("how") != FI_VOCAB_INDEX_NULL) {
		die("DS_VocabTable::find(CharString)",
		    "Reverse mapping failed");
	}

	try {
		ds_vt1.find("");

		die("DS_VocabTable::find(CharString)",
		    "Didn't throw InvalidArgumentException");
	} catch (const InvalidArgumentException& e) {
	}

	test_max(ds_vt1, ".");

	//
	// Test dynamic name tables.
	//

	FI_NameSurrogate ns1 (idx3, idx2, idx1);
	FI_NameSurrogate ns2 (idx3);

	idx1 = dn_vt1.add(ns1);
	idx2 = dn_vt2.add(ns2);

	if (idx1 != 1 || idx2 != 1) {
		die("DN_VocabTable::add(FI_NameSurrogate)",
		    "Incorrect indexes assigned");
	}

	try {
		dn_vt1.add(FI_NameSurrogate (FI_VOCAB_INDEX_NULL));

		die("DN_VocabTable::add(CharString)",
		    "Didn't throw InvalidArgumentException");
	} catch (const InvalidArgumentException& e) {
	}

	if (dn_vt1[idx1] != ns1 || dn_vt2[idx2] != ns2) {
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

	if (dn_vt1.find(ns1) != idx1 || dn_vt2.find(ns2) != idx2
	    || dn_vt1.find(ns2) != FI_VOCAB_INDEX_NULL
	    || dn_vt2.find(ns1) != FI_VOCAB_INDEX_NULL) {
		die("DN_VocabTable::find(FI_NameSurrogate)",
		    "Reverse mapping failed");
	}

	test_max(dn_vt1, ns1);
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
