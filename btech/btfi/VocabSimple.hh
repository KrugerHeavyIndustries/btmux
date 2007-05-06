/*
 * Module implementing the specific vocabulary tables required by X.891, which
 * don't depend on any other vocabulary table types.
 */

#ifndef BTECH_FI_VOCABSIMPLE_HH
#define BTECH_FI_VOCABSIMPLE_HH

#include "VocabTable.hh"

#include "common.h"
#include "encalg.h"


namespace BTech {
namespace FI {

//
// Restricted alphabet table implementation.
//
class RA_VocabTable : public DynamicTypedVocabTable<CharString> {
public:
	RA_VocabTable ();

	const TypedEntryRef getEntry(const_reference value);

private:
	RA_VocabTable (bool read_only);

	static TypedVocabTable *builtin_table ();
}; // class RA_VocabTable

//
// Encoding algorithm table implementation.
//
class EA_VocabTable : public TypedVocabTable<const FI_EncodingAlgorithm> {
public:
	EA_VocabTable ();

private:
	EA_VocabTable (bool read_only);

	static EA_VocabTable *builtin_table ();
}; // class EA_VocabTable

//
// Dynamic string table implementation.
//
class DS_VocabTable : public DynamicTypedVocabTable<CharString> {
public:
	DS_VocabTable ();

protected:
	DS_VocabTable (TypedVocabTable *parent);

private:
	DS_VocabTable (bool read_only);

	static TypedVocabTable *builtin_table ();
}; // class DS_VocabTable

//
// Prefix table implementation.
//
class PFX_DS_VocabTable : public DS_VocabTable {
public:
	PFX_DS_VocabTable ();

private:
	PFX_DS_VocabTable (bool read_only);

	static DS_VocabTable *builtin_table ();
}; // class PFX_DS_VocabTable

//
// Namespace name table implementation.
//
class NSN_DS_VocabTable : public DS_VocabTable {
public:
	NSN_DS_VocabTable ();

private:
	NSN_DS_VocabTable (bool read_only);

	static DS_VocabTable *builtin_table ();
}; // class NSN_DS_VocabTable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_VOCABSIMPLE_HH */
