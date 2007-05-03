/*
 * Module implementing the specific vocabulary tables required by X.891.
 */

#ifndef BTECH_FI_VOCABULARY_HH
#define BTECH_FI_VOCABULARY_HH

#include "VocabTable.hh"

#include "common.h"
#include "encalg.h"


namespace BTech {
namespace FI {

//
// Restricted alphabet table implementation.
//
class RA_VocabTable : public TypedVocabTable<CharString> {
public:
	RA_VocabTable ();

	const TypedEntryRef getEntry (const_value_ref value);

	const_value_ref operator [] (FI_VocabIndex idx) const;

private:
	const TypedEntryRef NUMERIC_ALPHABET;
	const TypedEntryRef DATE_AND_TIME_ALPHABET;

	static TypedEntry *get_numeric_alphabet ();
	static TypedEntry *get_date_and_time_alphabet();
}; // class RA_VocabTable

//
// Encoding algorithm table implementation.
//
class EA_VocabTable : public TypedVocabTable<const FI_EncodingAlgorithm *> {
public:
	EA_VocabTable ();

	const_value_ref operator [] (FI_VocabIndex idx) const;

protected:
	DynamicTypedEntry *createTypedEntry (const_value_ref value);
}; // class EA_VocabTable

//
// Dynamic string table implementation.
//
class DS_VocabTable : public TypedVocabTable<CharString> {
public:
	DS_VocabTable ();

	const TypedEntryRef getEntry (const_value_ref value);

	const_value_ref operator [] (FI_VocabIndex idx) const;

protected:
	DS_VocabTable (FI_VocabIndex initial_last_idx);

private:
	const TypedEntryRef EMPTY_STRING;

	static TypedEntry *get_empty_string ();
}; // class DS_VocabTable

//
// Prefix table implementation.
//
class PFX_DS_VocabTable : public DS_VocabTable {
public:
	PFX_DS_VocabTable ();

	const TypedEntryRef getEntry (const_value_ref value);

	const_value_ref operator [] (FI_VocabIndex idx) const;

private:
	const TypedEntryRef XML_PREFIX;

	static TypedEntry *get_xml_prefix ();
}; // class PFX_DS_VocabTable

//
// Namespace name table implementation.
//
class NSN_DS_VocabTable : public DS_VocabTable {
public:
	NSN_DS_VocabTable ();

	const TypedEntryRef getEntry (const_value_ref value);

	const_value_ref operator [] (FI_VocabIndex idx) const;

private:
	const TypedEntryRef XML_NAMESPACE;

	static TypedEntry *get_xml_namespace ();
}; // class NSN_DS_VocabTable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_VOCABULARY_HH */
