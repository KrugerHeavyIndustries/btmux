/*
 * Module implementing the vocabulary table data structure required by X.891.
 */

#ifndef BTECH_FI_VOCAB_H
#define BTECH_FI_VOCAB_H

#include "common.h"
#include "encalg.h"

#include <exception>
#include <string>
#include <functional>

#include <vector>
#include <map>

/* Sections 6.5 and 6.10: Vocabulary table indexes range from 1 to 2^20.  */
typedef unsigned long VocabIndex;

#define FI_VOCAB_INDEX_NULL 0
#define FI_VOCAB_INDEX_MAX FI_ONE_MEG

/* Convenience constants for built-in restricted alphabets.  */
typedef enum {
	FI_RA_NUMERIC = 1,		/* numeric (section 9.1) */
	FI_RA_DATE_AND_TIME		/* date and time (section 9.2) */
} FI_RA_VocabIndex;

/* Convenience constants for built-in encoding algorithms.  */
typedef enum {
	FI_EA_HEXADECIMAL = 1,		/* hexadecimal (section 10.2) */
	FI_EA_BASE64,			/* Base64 (section 10.3) */
	FI_EA_SHORT,			/* 16-bit integer (section 10.4) */
	FI_EA_INT,			/* 32-bit integer (section 10.5) */
	FI_EA_LONG,			/* 64-bit integer (section 10.6) */
	FI_EA_BOOLEAN,			/* boolean (section 10.7) */
	FI_EA_FLOAT,			/* single precision (section 10.8) */
	FI_EA_DOUBLE,			/* double precision (section 10.9) */
	FI_EA_UUID,			/* UUID (section 10.10) */
	FI_EA_CDATA			/* CDATA (section 10.11) */
} FI_EA_VocabIndex;

typedef struct FI_NameSurrogate {
	VocabIndex prefix_idx;		/* optional */
	VocabIndex namespace_idx;	/* optional, required by prefix_idx */
	VocabIndex local_idx;		/* required */

#ifdef __cplusplus
	FI_NameSurrogate (VocabIndex local_idx,
	                  VocabIndex namespace_idx = FI_VOCAB_INDEX_NULL,
	                  VocabIndex prefix_idx = FI_VOCAB_INDEX_NULL)
	: prefix_idx(prefix_idx),
	  namespace_idx(namespace_idx),
	  local_idx(local_idx) {}

	bool operator< (const FI_NameSurrogate& rhs) const throw () {
		if (prefix_idx < rhs.prefix_idx) {
			return true;
		} else if (namespace_idx < rhs.namespace_idx) {
			return true;
		} else {
			return local_idx < rhs.local_idx;
		}
	}

	bool operator== (const FI_NameSurrogate& rhs) const throw () {
		return (local_idx == rhs.local_idx
		        && namespace_idx == rhs.namespace_idx
		        && prefix_idx == rhs.prefix_idx);
	}

	bool operator!= (const FI_NameSurrogate& rhs) const throw () {
		return !(*this == rhs);
	}
#endif /* __cplusplus */
} FI_NameSurrogate;

namespace BTech {
namespace FI {

// TODO: Make this a common typedef.
typedef std::basic_string<FI_Char> CharString;

// TODO: Make this a common class.
class Exception : public std::exception {
}; // Exception

class IndexOutOfBoundsException : public Exception {
}; // IndexOutOfBoundsException

class InvalidArgumentException : public Exception {
}; // InvalidArgumentException

class UnsupportedOperationException : public Exception {
}; // UnsupportedOperationException

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

	virtual VocabIndex add (const_entry_ref entry) throw (Exception) {
		throw UnsupportedOperationException ();
	}

	virtual const_entry_ref operator[] (VocabIndex idx) const
	                                    throw (Exception) = 0;

	virtual VocabIndex find (const_entry_ref entry) const
	                         throw (Exception) {
		throw UnsupportedOperationException ();
	}
}; // class VocabTable

//
// Restricted alphabet table implementation.
//
class RA_VocabTable : public VocabTable<CharString> {
public:
	RA_VocabTable () {}
	~RA_VocabTable () {}

	void clear () throw ();

	VocabIndex add (const_entry_ref entry) throw (Exception);
	const_entry_ref operator[] (VocabIndex idx) const throw (Exception);

private:
	typedef std::vector<entry_type> alphabet_table_type;

	alphabet_table_type alphabets;
}; // class RA_VocabTable

//
// Encoding algorithm table implementation.
//
class EA_VocabTable : public VocabTable<const FI_EncodingAlgorithm *> {
public:
	EA_VocabTable () {}
	~EA_VocabTable () {}

	void clear () throw ();

	const_entry_ref operator[] (VocabIndex idx) const throw (Exception);

private:
}; // class EA_VocabTable

//
// Dynamic string table implementation.
//
class DS_VocabTable : public VocabTable<CharString> {
public:
	DS_VocabTable () {}
	~DS_VocabTable () {}

	void clear () throw ();

	VocabIndex add (const_entry_ref entry) throw (Exception);
	const_entry_ref operator[] (VocabIndex idx) const throw (Exception);
	VocabIndex find (const_entry_ref entry) const throw (Exception);

private:
	typedef std::vector<entry_type> string_table_type;
	typedef std::map<entry_type,VocabIndex> string_map_type;

	string_table_type strings;
	string_map_type reverse_map;
}; // class DS_VocabTable

//
// Dynamic name table implementation.
//
class DN_VocabTable : public VocabTable<FI_NameSurrogate> {
public:
	DN_VocabTable () {}
	~DN_VocabTable () {}

	void clear () throw ();

	VocabIndex add (const_entry_ref entry) throw (Exception);
	const_entry_ref operator[] (VocabIndex idx) const throw (Exception);
	VocabIndex find (const_entry_ref entry) const throw (Exception);

private:
	typedef std::vector<entry_type> name_table_type;
	typedef std::map<entry_type,VocabIndex> name_map_type;

	name_table_type names;
	name_map_type reverse_map;
}; // class DN_VocabTable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_VOCAB_H */
