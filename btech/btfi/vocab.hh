/*
 * Module implementing the vocabulary table data structure required by X.891.
 */

#ifndef BTECH_FI_VOCAB_HH
#define BTECH_FI_VOCAB_HH

#include <cstddef>

#include <vector>
#include <set>

#include "common.h"
#include "encalg.h"


namespace BTech {
namespace FI {

/*
 * Generic vocabulary table base class.
 *
 * Vocabulary tables are meant to be arranged in a hierarchy, going from
 * built-ins, to an external vocabulary, to the initial vocabulary, and finally
 * the dynamic vocabulary.  This eases sharing and re-use of vocabulary tables.
 *
 * This class provides an abstract interface for clearing the contents of
 * vocabulary tables.  Specific vocabulary tables will generally be derived
 * from the TypedVocabTable<T> template, which provides interfaces for actually
 * creating and accessing vocabulary table entries.
 */
class VocabTable {
protected:
	class Entry;

public:
	virtual ~VocabTable () {}

	// Clears this vocabulary table's contents (but not those in higher
	// levels of the resolution hierarchy).  Higher levels should be
	// cleared before lower levels, to avoid reindexing problems.
	void clear ();

	// Parent of this table in the resolution hierarchy (or 0 if this is a
	// root-level table).
	VocabTable *getParent () const {
		return parent;
	}

	/*
	 * Proxy for a vocabulary table entry.  Provides an abstract interface
	 * for a reference to a table entry, which may have its vocabulary
	 * index dynamically assigned.
	 *
	 * An EntryRef will normally be created by the getEntryRef() method of
	 * a TypedVocabTable.
	 *
	 * Note that EntryRef may use reference counting to manage memory, so
	 * circular references must carefully be avoided.  In practice, this
	 * means an Entry should never hold an EntryRef that potentially points
	 * to itself, whether directly or indirectly.  An Entry may freely hold
	 * an EntryRef that points to a different object, however.
	 */
	class EntryRef {
		friend class VocabTable;

	public:
		// Reference-counting construction/destruction.
		EntryRef (Entry *entry = 0) : entry (entry) {
			if (entry) {
				entry->addRef();
			}
		}

		~EntryRef () {
			if (entry) {
				entry->delRef();
			}
		}

		// Reference-counting copy/assignment.
		EntryRef (const EntryRef& ref) : entry (ref.entry) {
			if (entry) {
				entry->addRef();
			}
		}

		EntryRef& operator = (const EntryRef& rValue) {
			// Add reference to new entry, then drop reference to
			// old entry.  This order is important for handling the
			// self-assignment case.
			if (rValue.entry) {
				rValue.entry->addRef();
			}

			if (entry) {
				entry->delRef();
			}

			// Perform assignment.
			entry = rValue.entry;
			return *this;
		}

		// Invoke hasIndex() on the associated Entry.
		bool hasIndex () const {
			return entry->hasIndex();
		}

		// Invoke getIndex() on the associated Entry.
		FI_VocabIndex getIndex () const {
			return entry->getIndex();
		}

		// Ordering comparison.
		bool operator < (const EntryRef& rValue) const {
			// We need to compare by entry value here.
			if (!entry) {
				if (!rValue.entry) {
					// 0 == 0
					return false;
				} else {
					// 0 < *
					return true;
				}
			} else if (!rValue.entry) {
				// * > 0
				return false;
			} else {
				// L < R ?
				return *entry < *rValue.entry;
			}
		}

		// Test whether this reference points to a given Entry.  Useful
		// for doing things like ref != 0.
		bool operator != (const Entry *rValue) const {
			return entry != rValue;
		}

		bool operator == (const Entry *rValue) const {
			return entry == rValue;
		}

		// Explicitly release the reference to the managed pointer.
		void release () {
			if (entry) {
				entry->delRef();
				entry = 0;
			}
		}

	protected:
		// Reference-counted pointer.
		Entry *entry;
	}; // class VocabTable::EntryRef

protected:
	// Construct a table with an optional parent table.
	VocabTable (FI_VocabIndex initial_last_idx = 0,
	            FI_VocabIndex max_idx = FI_ONE_MEG)
	: parent (0), initial_last_idx (initial_last_idx), max_idx (max_idx),
	  base_idx (initial_last_idx + 1), last_idx (initial_last_idx) {}

	VocabTable (VocabTable *parent)
	: parent (parent), initial_last_idx (0), max_idx (parent->max_idx),
	  base_idx (parent->last_idx + 1), last_idx (parent->last_idx) {}

	// Acquire a vocabulary index for the entry.  If the table's index
	// limit is reached, return FI_VOCAB_INDEX_NULL.  Throws an Exception
	// for all types of errors.
	FI_VocabIndex acquireIndex (Entry *entry);

	// Helper to find the entry corresponding to a given value.  Throws
	// an IndexOutOfBoundsException if it can't find the relevant index.
	const EntryRef& lookupIndex (FI_VocabIndex idx) const;

	// Pointer to the parent table, if any.
	VocabTable *const parent;

	// Initial last index.  Only meaningful on root tables, as a
	// convenience for handling built-ins.
	const FI_VocabIndex initial_last_idx;

	// Maximum vocabulary index.  Set at construction time, and should be
	// identical for all levels of the hierarchy.
	const FI_VocabIndex max_idx;

	// First index in use by this table.  It should be the same as the
	// parent table's last_idx + 1.
	FI_VocabIndex base_idx;

	// Last index in use by this table, including any reserved indexes.
	// Child tables should use this to determine where to start assigning
	// vocabulary indexes from.  It is meaningless for dynamic tables.
	//
	// The value should only be read from the parent at construction or
	// clear() time, to establish the child's index state.  Changing the
	// value without performing a clear() on all children (particularly in
	// the positive direction), will result in undefined behavior.
	FI_VocabIndex last_idx;

	std::vector<EntryRef> vocabulary;

	/*
	 * A vocabulary table entry, representing an (index, value) pair.
	 */
	class Entry {
		friend class EntryRef;

	public:
		virtual ~Entry () {}

		// Returns true if the entry has been assigned an index.  The
		// index may still be FI_VOCAB_INDEX_NULL, in case the
		// associated table's limit was reached.
		virtual bool hasIndex () const { return true; }

		// Get the entry's index.  Return FI_VOCAB_INDEX_NULL if an
		// index can not be assigned, for whatever reason. (The index
		// may also simply be FI_VOCAB_INDEX_NULL; it may be necessary
		// to disambiguate from context, such as by checking if the
		// value is an empty string.) Errors should be reported by
		// throwing an appropriate Exception.
		//
		// Once assigned, the index should never change.  Also, other
		// than a FI_VOCAB_INDEX_NULL, indicating a missing index, the
		// index should be unique within its context for any given
		// Entry instance (though multiple Entry instances may have the
		// same value).
		virtual FI_VocabIndex getIndex () = 0;

		// Unassign index.  A new index may be returned by the next
		// invocation of getIndex().
		virtual void resetIndex () {}

		virtual bool operator < (const Entry& rValue) const = 0;

	protected:
		Entry () {}

		virtual void addRef () {}
		virtual void delRef () {}
	}; // class VocabTable::Entry
}; // class VocabTable

/*
 * Type-specific vocabulary table template.
 */
template<typename T>
class TypedVocabTable : public VocabTable {
private:
	class EntryPool;

protected:
	class TypedEntry;
	class DynamicTypedEntry;

public:
	class TypedEntryRef;

	typedef T value_type;
	typedef const T& const_value_ref;

	~TypedVocabTable () {
		if (!parent) {
			// We could theoretically destroy the parent of a table
			// and leave it with a dangling entry_pool reference,
			// but if you delete the parent of a table first, and
			// then expect to be able to use it in any way that
			// would access the entry_pool, then you've got other
			// problems.
			//
			// In short, no fancy reference counting needed.
			//
			// TODO: Document for the API user that deleting parent
			// tables
			// before children is really bad, even if it seems
			// obvious.
			delete entry_pool;
		}
	}

	// Create a new TypedEntry for a value.  Will always return a new
	// entry, rather than reusing an existing one.
	virtual const TypedEntryRef createEntry (const_value_ref value) {
		return createTypedEntry(value);
	}

	// Get a TypedEntry for a value.  Can (but is not required to) return
	// an existing entry with the same value.
	//
	// This may be relatively slow, so avoid calling it too much.  The
	// suggested usage is to call it once for each known value, and cache
	// the returned EntryRef for later usage.
	//
	// This must return an EntryRef by value, NOT by reference.
	virtual const TypedEntryRef getEntry (const_value_ref value) {
		return entry_pool->intern(createTypedEntry(value));
	}

	// Get stored value by vocabulary index.  Throws OutOfBoundsException
	// if given a non-existent index.
	virtual const_value_ref operator [] (FI_VocabIndex idx) const {
		return static_cast<const TypedEntryRef&>(lookupIndex(idx)).getValue();
	}

	/*
	 * Type-safe EntryRef.
	 */
	class TypedEntryRef : public EntryRef {
		friend class TypedVocabTable;

	public:
		TypedEntryRef (TypedEntry *entry = 0) : EntryRef (entry) {}

		TypedEntryRef (const TypedEntryRef& rValue)
		: EntryRef (rValue.entry) {}

		TypedEntryRef& operator = (const TypedEntryRef& rValue) {
			EntryRef::operator =(rValue);
			return *this;
		}

		const_value_ref getValue () const {
			return getTypedEntry()->value;
		}

	private:
		TypedEntry *getTypedEntry () const {
			return static_cast<TypedEntry *>(entry);
		}
	}; // template class TypedVocabTable::TypedEntryRef

protected:
	// Create a new DynamicTypedEntry for a value.
	virtual DynamicTypedEntry *createTypedEntry (const_value_ref value) {
		return new DynamicTypedEntry (*this, value);
	}

	TypedVocabTable (FI_VocabIndex initial_last_idx = 0,
	                 FI_VocabIndex max_idx = FI_ONE_MEG)
	: VocabTable (initial_last_idx, max_idx),
	  entry_pool (new EntryPool ()) {}

	TypedVocabTable (TypedVocabTable *parent)
	: VocabTable (parent),
	  entry_pool (parent->getEntryPool()) {}

	/*
	 * TypedEntry for vocabulary table values.
	 */
	class TypedEntry : public Entry {
		friend class TypedEntryRef;

	public:
		bool operator < (const Entry& entry) const {
			return value < static_cast<const TypedEntry&>(entry).value;
		}

	protected:
		TypedEntry (const_value_ref value) : value (value) {}

		const value_type value;
	}; // template class TypedVocabTable::TypedEntry

	/*
	 * A TypedEntry implementation that dynamically assigns its index.
	 */
	class DynamicTypedEntry : public TypedEntry {
		friend class EntryPool;

	public:
		DynamicTypedEntry (TypedVocabTable& owner,
		                   const_value_ref value)
		: TypedEntry (value), owner (owner), has_cached_idx (false),
		  ref_count (0), is_interned (false) {}

		bool hasIndex () const {
			return has_cached_idx;
		}

		FI_VocabIndex getIndex () {
			if (!has_cached_idx) {
				cached_idx = acquireIndex();
				has_cached_idx = true;
			}

			// Note that cached_idx may be FI_VOCAB_INDEX_NULL.
			return cached_idx;
		}

		void resetIndex () {
			has_cached_idx = false;
		}

	protected:
		void addRef () {
			ref_count++;
		}

		void delRef () {
			ref_count--;
			if (ref_count < 1) {
				// TODO: Assert ref_count == 1.
				// XXX: We're crazy! Yay!
				delete this;
			} else if (is_interned && ref_count == 1) {
				owner.entry_pool->disintern(this);
				// disintern() will drop ref_count to 0.
			}
		}

		virtual FI_VocabIndex acquireIndex () {
			return owner.acquireIndex(this);
		}

	private:
		TypedVocabTable& owner;

		bool has_cached_idx;
		FI_VocabIndex cached_idx;

		size_t ref_count;

		bool is_interned;
	}; // template class TypedVocabTable::DynamicTypedEntry

	/*
	 * A TypedEntry implementation that statically assigns its index.
	 * Intended for use with constants, it does not participate in
	 * automatic memory management using reference counting.
	 */
	class StaticTypedEntry : public TypedEntry {
	public:
		StaticTypedEntry (FI_VocabIndex idx, const_value_ref value)
		: TypedEntry (value), cached_idx (idx) {}

		FI_VocabIndex getIndex () {
			return cached_idx;
		}

	private:
		FI_VocabIndex cached_idx;
	}; // template class TypedVocabTable::StaticTypedEntry

private:
	// Return the root-level entry pool.
	EntryPool *getEntryPool () const {
		if (parent) {
			return getParent()->getEntryPool();
		} else {
			return entry_pool;
		}
	}

	EntryPool *const entry_pool;

	// Pool of interned Entry objects.
	class EntryPool {
		typedef std::set<TypedEntryRef> RefSet;
		typedef typename RefSet::const_iterator RefSetIterator;

	public:
		// Unintern entries before destruction, to avoid triggering
		// automatic disinternment by the EntryRef.
		~EntryPool () {
			for (RefSetIterator pp = interned.begin();
			     pp != interned.end();
			     ++pp) {
				setInterned(pp->getTypedEntry(), false);
			}
		}

		// Intern entry.
		const TypedEntryRef& intern (DynamicTypedEntry *entry) {
			const TypedEntryRef& ref = *interned.insert(entry).first;
			setInterned(ref.getTypedEntry(), true);
			return ref;
		}

		// Disintern entry.
		void disintern (DynamicTypedEntry *entry) {
			setInterned(entry, false);
			interned.erase(entry);
		}

	private:
		static DynamicTypedEntry *castTypedEntry (TypedEntry *entry) {
			return static_cast<DynamicTypedEntry *>(entry);
		}

		static void setInterned (Entry *entry, bool state) {
			DynamicTypedEntry *const dyn_entry
			= static_cast<DynamicTypedEntry *>(entry);

			dyn_entry->is_interned = state;
		}

		RefSet interned;
	}; // class VocabTable::EntryPool
}; // template class TypedVocabTable


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

#endif /* !BTECH_FI_VOCAB_HH */
