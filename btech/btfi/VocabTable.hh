/*
 * Module implementing the vocabulary table data structure required by X.891.
 */

#ifndef BTECH_FI_VOCABTABLE_HH
#define BTECH_FI_VOCABTABLE_HH

#include <cstddef>

#include <vector>

#include "fiptypes.h" // FIXME: move FI_VocabIndex, FI_VOCAB_INDEX_NULL out

#include "Exception.hh"
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
 *
 * TODO: Add explicit keyword where useful.  This is relatively new to C++, so
 * it should only be used to catch mistakes, not to enforce behavior.
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
		operator const void * () const {
			return entry;
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
public:
	class TypedEntryRef;

	typedef T value_type;
	typedef const T& const_reference;

protected:
	class TypedEntry;

	// Note that we specifically use a regular TypedEntry pointer here.
	typedef ObjectPool<value_type,TypedEntry *> EntryPool;
	typedef typename EntryPool::PoolPtr EntryPoolPtr;

public:
	~TypedVocabTable () {
		// Delete our entry pool, held by our wildly dangerous pointer.
		if (own_entry_pool) {
			delete entry_pool;
		}
	}

	// Get a TypedEntry by vocabulary index.  Throws OutOfBoundsException
	// if given a non-existent index.
	const TypedEntryRef operator [] (FI_VocabIndex idx) const {
		return static_cast<const TypedEntryRef&>(lookupIndex(idx));
	}

	/*
	 * Type-safe EntryRef.
	 */
	class TypedEntryRef : public EntryRef {
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
				       < src.getTypedEntry()->value;
			}
		}

	private:
		TypedEntry *getTypedEntry () const {
			return static_cast<TypedEntry *>(entry);
		}
	}; // template class TypedVocabTable::TypedEntryRef

protected:
	TypedVocabTable (bool read_only, FI_VocabIndex max_idx)
	: VocabTable (read_only, max_idx), typed_parent (0),
	  own_entry_pool (true),
	  entry_pool (new EntryPool ()) {}

	TypedVocabTable (bool read_only, TypedVocabTable *parent,
	                 EntryPool *shared_pool = 0)
	: VocabTable (read_only, parent), typed_parent (parent),
	  own_entry_pool (!shared_pool),
	  entry_pool (shared_pool ? shared_pool : new EntryPool ()) {}

	// Search the entry pool by value for an existing TypedEntry.  Returns
	// a null EntryPoolPtr if the TypedEntry can not be found.
	const EntryPoolPtr findEntry (const_reference value) {
		// Check the local entry pool.
		const EntryPoolPtr pool_ptr = entry_pool->findObject(value);
		if (pool_ptr) {
			// TODO: Assert pool_ptr.metadata() != 0.
			return pool_ptr;
		}

		// Fall back to the parent.
		if (typed_parent) {
			return typed_parent->findEntry(value);
		}

		// Give up.
		return 0;
	}

	// Convenience method for adding static entries.  A pointer to the
	// allocated memory is returned, which the caller is responsible for
	// deallocating when the table is no longer needed.  For the intended
	// use of allocating static entries, a static auto_ptr is probably a
	// good way of handling it.
	TypedEntry *addStaticEntry (FI_VocabIndex idx, const_reference value) {
		if (findEntry(value)) {
			throw AssertionFailureException ();
		}

		const EntryPoolPtr pool_ptr = entry_pool->createObject(value);

		TypedEntry *const entry = new StaticTypedEntry (idx, pool_ptr);
		pool_ptr.metadata() = entry;

		if ((base_idx + vocabulary.size()) != idx
		    || acquireIndex(entry) != idx) {  
			throw AssertionFailureException ();
		}

		return entry;
	}

	TypedVocabTable *const typed_parent;

	// Pool of singleton value_type values (and their associated TypeEntry
	// objects).  own_entry_pool is true if this object created the pool.
	const bool own_entry_pool;
	EntryPool *const entry_pool;

	/*
	 * TypedEntry for vocabulary table values.
	 */
	class TypedEntry : public Entry {
	public:
		// Provide read-only access to the pooled value.
		const_reference value;

	protected:
		TypedEntry (const EntryPoolPtr& pool_ptr)
		: value (*pool_ptr), pool_ptr (pool_ptr) {}

		// Retain a reference to the pooled value.
		const EntryPoolPtr pool_ptr;
	}; // template class TypedVocabTable::TypedEntry

	/*
	 * A TypedEntry implementation that statically assigns its index.
	 * Intended for use with constants.  Note that the passed value will be
	 * retained by reference, and needs to have sufficient lifetime.
	 *
	 * For example, you can't directly initialize CharString tables with a
	 * string constant like "", because that actually creates a temporary
	 * CharString copy, and passes a reference to the temporary, which
	 * doesn't have a sufficient lifetime.
	 */
	class StaticTypedEntry : public TypedEntry {
	public:
		StaticTypedEntry (FI_VocabIndex idx,
		                  const EntryPoolPtr& pool_ptr)
		: TypedEntry (pool_ptr), static_idx (idx) {}

		bool hasIndex () const {
			return true;
		}

		FI_VocabIndex getIndex () {
			return static_idx;
		}

	private:
		const FI_VocabIndex static_idx;
	}; // template class TypedVocabTable::StaticTypedEntry
}; // template class TypedVocabTable

/*
 * Template for a type-specific vocabulary table with dynamic index assignment.
 */
template<typename T>
class DynamicTypedVocabTable : public TypedVocabTable<T> {
protected:
	class DynamicTypedEntry;

	typedef TypedVocabTable<T> TypedVocabTable;
	typedef typename TypedVocabTable::TypedEntry TypedEntry;

	typedef typename TypedVocabTable::EntryPool EntryPool;
	typedef typename TypedVocabTable::EntryPoolPtr EntryPoolPtr;

public:
	typedef typename TypedVocabTable::TypedEntryRef TypedEntryRef;

	typedef typename TypedVocabTable::value_type value_type;
	typedef typename TypedVocabTable::const_reference const_reference;

	// Create a new TypedEntry for a value.  Will always return a new
	// entry, rather than reusing an existing one.  The entry may be
	// interned, just like with getEntry(), so this may also be slow.
	virtual const TypedEntryRef createEntry (const_reference value) {
		// Look for an existing entry.
		EntryPoolPtr pool_ptr = findEntry(value);
		if (!pool_ptr) {
			// Add a new interned entry.
			pool_ptr = entry_pool->createObject(value);
			pool_ptr.metadata() = createEntryObject(pool_ptr);
			return pool_ptr.metadata();
		} else {
			// Create new, uninterned entry.
			return createEntryObject(pool_ptr);
		}
	}

	// Get a TypedEntry for a value.  Can (but is not required to) return
	// an existing entry with the same value.
	//
	// This may be relatively slow, so avoid calling it too much.  The
	// suggested usage is to call it once for each known value, and cache
	// the returned TypedEntryRef for later usage.
	//
	// This must return an TypedEntryRef by value, NOT by reference.
	virtual const TypedEntryRef getEntry (const_reference value) {
		// Look for an existing entry.
		EntryPoolPtr pool_ptr = findEntry(value);
		if (!pool_ptr) {
			// Add a new interned entry.
			pool_ptr = entry_pool->createObject(value);
			pool_ptr.metadata() = createEntryObject(pool_ptr);
			return pool_ptr.metadata();
		} else {
			// Use existing, interned entry.
			return pool_ptr.metadata();
		}
	}

protected:
	DynamicTypedVocabTable (bool read_only, FI_VocabIndex max_idx)
	: TypedVocabTable (read_only, max_idx),
	  dynamic_root (0) {}

	DynamicTypedVocabTable (bool read_only, TypedVocabTable *parent)
	: TypedVocabTable (read_only, parent),
	  dynamic_root (0) {}

	DynamicTypedVocabTable (bool read_only, DynamicTypedVocabTable *parent)
	: TypedVocabTable (read_only, parent,
	                   parent->getDynamicRoot()->entry_pool),
	  dynamic_root (parent->getDynamicRoot()) {}

	// Create a new DynamicTypedEntry for a value.
	virtual DynamicTypedEntry *
	createEntryObject (const EntryPoolPtr& pool_ptr) {
		return new DynamicTypedEntry (*this, pool_ptr);
	}

	/*
	 * A TypedEntry implementation that dynamically assigns its index.
	 */
	class DynamicTypedEntry : public TypedEntry {
	public:
		DynamicTypedEntry (DynamicTypedVocabTable& owner,
		                   const EntryPoolPtr& pool_ptr)
		: TypedEntry (pool_ptr),
		  owner (owner), has_cached_idx (false), ref_count (0) {}

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
			if (--ref_count < 1) {
				// TODO: Assert ref_count == 0.
				// XXX: We're crazy! Yay!
				delete this;

				// When this object is deleted, there are no
				// more references to it.  The value_ptr will
				// be released, and there should be no more
				// references to that, either.  This will
				// automatically take care of de-registering
				// the value-to-Entry mapping, so there will be
				// no dangling pointers.
				//
				// To ensure this, we should never allow
				// external, reference-counted access to
				// value_ptr; the user holding a reference to
				// the DynamicTypedEntry (and thus implicitly
				// to the value) is sufficient.
			}
		}

		virtual FI_VocabIndex acquireIndex () {
			return owner.acquireIndex(this);
		}

	private:
		DynamicTypedVocabTable& owner;

		bool has_cached_idx;
		FI_VocabIndex cached_idx;

		size_t ref_count;
	}; // template class DynamicTypedVocabTable::DynamicTypedEntry

private:
	using TypedVocabTable::entry_pool;

	// Get the root dynamic table in the lookup hierarchy.
	DynamicTypedVocabTable *getDynamicRoot () {
		return dynamic_root ? dynamic_root : this;
	}

	DynamicTypedVocabTable *const dynamic_root;
}; // template class DynamicTypedVocabTable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_VOCABTABLE_HH */
