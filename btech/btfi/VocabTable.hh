/*
 * Module implementing the vocabulary table data structure required by X.891.
 */

#ifndef BTECH_FI_VOCABTABLE_HH
#define BTECH_FI_VOCABTABLE_HH

#include <cstddef>

#include <vector>

#include "common.h"

#include "ObjectPool.hh"


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
	 * An EntryRef will normally be acquired from the getEntry() method of
	 * a TypedVocabTable, or copied from another EntryRef.
	 *
	 * Note that EntryRef may use reference counting to manage memory, so
	 * circular references must carefully be avoided.  In practice, this
	 * means an Entry should never hold an EntryRef that potentially points
	 * to itself, whether directly or indirectly.  An Entry may freely hold
	 * an EntryRef that points to a different object, however.
	 *
	 * Also note that an EntryRef (and its backing Entry and value) are
	 * only guaranteed to be valid for the same lifetime as the vocabulary
	 * table they were acquired from.
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

		EntryRef& operator = (const EntryRef& src) {
			// Add reference to new entry, then drop reference to
			// old entry.  This order is important for handling the
			// self-assignment case.
			if (src.entry) {
				src.entry->addRef();
			}

			if (entry) {
				entry->delRef();
			}

			// Perform assignment.
			entry = src.entry;
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

		// Test whether the reference points anywhere.
		bool isValid () const {
			return entry != 0;
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
	VocabTable (bool read_only, FI_VocabIndex max_idx)
	: parent (0), read_only (read_only), max_idx (max_idx),
	  base_idx (1), last_idx (0) {}

	VocabTable (bool read_only, VocabTable *parent)
	: parent (parent), read_only (read_only), max_idx (parent->max_idx),
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

	// Whether this table is mutable.
	bool read_only;

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
		virtual bool hasIndex () const = 0;

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

		// Unassign a dynamically-assigned index, allowing a new index
		// to be returned by the next invocation of getIndex().  Has no
		// effect on statically-assigned indexes.
		virtual void resetIndex () {}

	protected:
		virtual void addRef () {}
		virtual void delRef () {}
	}; // class VocabTable::Entry
}; // class VocabTable

/*
 * Template for a type-specific vocabulary table.
 */
template<typename T>
class TypedVocabTable : public VocabTable {
protected:
	class TypedEntry;

public:
	class TypedEntryRef;

	typedef T value_type;
	typedef const T& const_reference;

	// Get a TypedEntry by vocabulary index.  Throws OutOfBoundsException
	// if given a non-existent index.
	const TypedEntryRef operator [] (FI_VocabIndex idx) const {
		return static_cast<const TypedEntryRef&>(lookupIndex(idx));
	}

	/*
	 * Type-safe EntryRef.
	 */
	class TypedEntryRef : public EntryRef {
		friend class EntryPool;

	public:
		TypedEntryRef (TypedEntry *entry = 0) : EntryRef (entry) {}

		TypedEntryRef (const TypedEntryRef& src)
		: EntryRef (src.entry) {}

		TypedEntryRef& operator = (const TypedEntryRef& src) {
			EntryRef::operator =(src);
			return *this;
		}

		// Pointer-styled dereferencing.
		const_reference operator * () const {
			return getTypedEntry()->value;
		}

		const value_type *operator -> () const {
			return &getTypedEntry()->value;
		}

		// Ordering comparison.
		bool operator < (const TypedEntryRef& src) const {
			// We need to compare by entry value here.
			if (!entry) {
				if (!src.entry) {
					// 0 == 0
					return false;
				} else {
					// 0 < *
					return true;
				}
			} else if (!src.entry) {
				// * > 0
				return false;
			} else {
				// L < R ?
				return getTypedEntry()->value
				       < getTypedEntry()->value;
			}
		}

	private:
		TypedEntry *getTypedEntry () const {
			return static_cast<TypedEntry *>(entry);
		}
	}; // template class TypedVocabTable::TypedEntryRef

protected:
	TypedVocabTable (bool read_only, FI_VocabIndex max_idx)
	: VocabTable (read_only, max_idx) {}

	TypedVocabTable (bool read_only, TypedVocabTable *parent)
	: VocabTable (read_only, parent) {}

	/*
	 * TypedEntry for vocabulary table values.
	 */
	class TypedEntry : public Entry {
	public:
		const_reference value;

	protected:
		TypedEntry (const_reference value) : value (value) {}
	}; // template class TypedVocabTable::TypedEntry

	/*
	 * A TypedEntry implementation that statically assigns its index.
	 * Intended for use with constants.  Note that the passed value will be
	 * retained by reference, and needs to have sufficient lifetime.
	 *
	 * For example, you can't directly initialize CharString tables with a
	 * string constant like "", because that actually creates a temporary
	 * std::string copy, and passes that by reference.
	 */
	class StaticTypedEntry : public TypedEntry {
	public:
		StaticTypedEntry (FI_VocabIndex idx, const_reference value)
		: TypedEntry (value), static_idx (idx) {}

		bool hasIndex () const {
			return true;
		}

		FI_VocabIndex getIndex () {
			return static_idx;
		}

	private:
		FI_VocabIndex static_idx;
	}; // template class TypedVocabTable::StaticTypedEntry
}; // template class TypedVocabTable

/*
 * Template for a type-specific vocabulary table with dynamic index assignment.
 */
template<typename T>
class DynamicTypedVocabTable : public TypedVocabTable<T> {
protected:
	typedef TypedVocabTable<T> TypedVocabTable;
	typedef typename TypedVocabTable::TypedEntry TypedEntry;

public:
	typedef typename TypedVocabTable::TypedEntryRef TypedEntryRef;

	typedef typename TypedVocabTable::value_type value_type;
	typedef typename TypedVocabTable::const_reference const_reference;

protected:
	class DynamicTypedEntry;

	typedef ObjectPool<T,DynamicTypedEntry *> EntryPool;
	typedef typename EntryPool::PoolPtr EntryPoolPtr;

public:
	~DynamicTypedVocabTable () {
		if (!dynamic_root) {
			// We could theoretically destroy the dynamic_root of a
			// table and leave it with a dangling reference, but if
			// you delete the parent of a table first, and then
			// expect to be able to use it normally, then you've
			// got other problems.
			//
			// In short, no fancy reference counting needed.
			//
			// TODO: Document for the API user that deleting parent
			// tables before children is really bad, even if it
			// seems really obvious to us.
			delete entry_pool;
		}
	}

	// Create a new TypedEntry for a value.  Will always return a new
	// entry, rather than reusing an existing one.  The entry may be
	// interned, just like with getEntry(), so this may also be slow.
	virtual const TypedEntryRef createEntry (const_reference value) {
		return createEntryObject(entry_pool->getObject(value));
	}

	// Get a TypedEntry for a value.  Can (but is not required to) return
	// an existing entry with the same value.
	//
	// This may be relatively slow, so avoid calling it too much.  The
	// suggested usage is to call it once for each known value, and cache
	// the returned EntryRef for later usage.
	//
	// This must return an EntryRef by value, NOT by reference.
	virtual const TypedEntryRef getEntry (const_reference value) {
		const EntryPoolPtr value_ptr = entry_pool->getObject(value);

		if (!value_ptr.metadata()) {
			// Initialize entry.
			value_ptr.metadata() = createEntryObject(value_ptr);
		}

		return value_ptr.metadata();
	}

protected:
	DynamicTypedVocabTable (bool read_only, FI_VocabIndex max_idx)
	: TypedVocabTable (read_only, max_idx),
	  dynamic_root (0),
	  entry_pool (new EntryPool ()) {}

	DynamicTypedVocabTable (bool read_only, TypedVocabTable *parent)
	: TypedVocabTable (read_only, parent),
	  dynamic_root (0),
	  entry_pool (new EntryPool ()) {}

	DynamicTypedVocabTable (bool read_only, DynamicTypedVocabTable *parent)
	: TypedVocabTable (read_only, parent),
	  dynamic_root (parent->getDynamicRoot()),
	  entry_pool (dynamic_root->entry_pool) {}

	// Create a new DynamicTypedEntry for a value.
	virtual DynamicTypedEntry *
	createEntryObject (const EntryPoolPtr& value_ptr) {
		return new DynamicTypedEntry (*this, value_ptr);
	}

	/*
	 * A TypedEntry implementation that dynamically assigns its index.
	 */
	class DynamicTypedEntry : public TypedEntry {
	public:
		DynamicTypedEntry (DynamicTypedVocabTable& owner,
		                   const EntryPoolPtr& value_ptr)
		: TypedEntry (*value_ptr), value_ptr (value_ptr),
		  owner (owner), has_cached_idx (false),
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
				// TODO: Assert ref_count == 0.
				// XXX: We're crazy! Yay!
				delete this;
			} else if (is_interned && ref_count == 1) {
				//owner.disintern(value);
				// disintern() will drop ref_count to 0.
			}
		}

		virtual FI_VocabIndex acquireIndex () {
			return owner.acquireIndex(this);
		}

	private:
		using TypedEntry::value;
		const EntryPoolPtr value_ptr;

		DynamicTypedVocabTable& owner;

		bool has_cached_idx;
		FI_VocabIndex cached_idx;

		size_t ref_count;

		bool is_interned;
	}; // template class DynamicTypedVocabTable::DynamicTypedEntry

private:
	// Get the first DynamicTypedVocabTable in the search hierarchy.
	DynamicTypedVocabTable *getDynamicRoot () {
		return dynamic_root ? dynamic_root : this;
	}

	DynamicTypedVocabTable *const dynamic_root;

	EntryPool *const entry_pool;
}; // template class DynamicTypedVocabTable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_VOCABTABLE_HH */
