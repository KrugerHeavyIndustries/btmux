/*
 * Module implementing the vocabulary table data structure required by X.891.
 */

#ifndef BTECH_FI_VOCAB_HH
#define BTECH_FI_VOCAB_HH

#include "common.h"
#include "encalg.h"

#include <vector>
#include <map>

namespace BTech {
namespace FI {

//
// Vocabulary table base class template.
//
template<typename T>
class VocabTable {
protected:
	typedef T entry_type;
	typedef const T& const_entry_ref;

public:
	virtual ~VocabTable () {}

	virtual void clear () throw () = 0;

	virtual FI_VocabIndex add (const_entry_ref entry) throw (Exception) {
		throw UnsupportedOperationException ();
	}

	virtual const_entry_ref operator[] (FI_VocabIndex idx) const
	                                   throw (Exception) = 0;

	virtual FI_VocabIndex find (const_entry_ref entry) const
	                           throw (Exception) {
		throw UnsupportedOperationException ();
	}

	virtual FI_VocabIndex size () const throw () = 0;
}; // class VocabTable

//
// Restricted alphabet table implementation.
//
class RA_VocabTable : public VocabTable<CharString> {
public:
	static const FI_VocabIndex MAX; // maximum index
	static const FI_VocabIndex LAST_BUILTIN; // last built-in alphabet index
	static const FI_VocabIndex FIRST_ADDED; // first added alphabet index

	void clear () throw ();

	FI_VocabIndex add (const_entry_ref entry) throw (Exception);
	const_entry_ref operator[] (FI_VocabIndex idx) const throw (Exception);

	FI_VocabIndex size () const throw ();

private:
	typedef std::vector<entry_type> alphabet_table_type;

	alphabet_table_type alphabets;
}; // class RA_VocabTable

//
// Encoding algorithm table implementation.
//
class EA_VocabTable : public VocabTable<const FI_EncodingAlgorithm *> {
public:
	static const FI_VocabIndex MAX; // maximum index
	static const FI_VocabIndex LAST_BUILTIN; // last built-in encoding index
	static const FI_VocabIndex FIRST_ADDED; // first added encoding index

	void clear () throw ();

	const_entry_ref operator[] (FI_VocabIndex idx) const throw (Exception);

	FI_VocabIndex size () const throw ();

 private:
}; // class EA_VocabTable

//
// Dynamic string table implementation.
//
class DS_VocabTable : public VocabTable<CharString> {
public:
	static const FI_VocabIndex MAX; // maximum index

	const FI_VocabIndex last_builtin; // last built-in string index
	const FI_VocabIndex first_added; // first added string index

	DS_VocabTable (FI_VocabIndex last_builtin = FI_VOCAB_INDEX_NULL)
	              throw ()
	: last_builtin (last_builtin), first_added (last_builtin + 1) {}

	void clear () throw ();

	FI_VocabIndex add (const_entry_ref entry) throw (Exception);
	const_entry_ref operator[] (FI_VocabIndex idx) const throw (Exception);
	FI_VocabIndex find (const_entry_ref entry) const throw (Exception);

	FI_VocabIndex size () const throw () {
		return strings.size();
	}

private:
	typedef std::vector<entry_type> string_table_type;
	typedef std::map<entry_type,FI_VocabIndex> string_map_type;

	string_table_type strings;
	string_map_type reverse_map;
}; // class DS_VocabTable

//
// Dynamic name table implementation.
//
class DN_VocabTable : public VocabTable<FI_NameSurrogate> {
public:
	static const FI_VocabIndex MAX; // maximum index

	void clear () throw ();

	FI_VocabIndex add (const_entry_ref entry) throw (Exception);
	const_entry_ref operator[] (FI_VocabIndex idx) const throw (Exception);
	FI_VocabIndex find (const_entry_ref entry) const throw (Exception);

	FI_VocabIndex size () const throw () {
		return names.size();
	}

private:
	typedef std::vector<entry_type> name_table_type;
	typedef std::map<entry_type,FI_VocabIndex> name_map_type;

	name_table_type names;
	name_map_type reverse_map;
}; // class DN_VocabTable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_VOCAB_HH */
