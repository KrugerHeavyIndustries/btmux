/*
 * Module implementing a singleton pool of objects.  Memory is automatically
 * managed using a reference-counting smart pointer.
 */

#ifndef BTECH_FI_OBJECTPOOL_HH
#define BTECH_FI_OBJECTPOOL_HH

#include <cstddef>

#include <map>

#include "Exception.hh"


namespace BTech {
namespace FI {

template<typename T, typename U = void *>
class ObjectPool {
public:
	// value_type must be CopyConstructible.
	typedef T value_type;

	typedef const T& const_reference;

	typedef const T *const_pointer;

	// meta_value_type must be Assignable, and must also allow the
	// following expression: meta_value_type (0).
	typedef U meta_value_type;

private:
	class PoolData;
	class PoolComp;

	typedef std::map<const_pointer,PoolData *,PoolComp> PoolMap;
	typedef typename PoolMap::value_type PoolMapValue;
	typedef typename PoolMap::const_iterator PoolMapIterator;

public:
	class PoolPtr;

	~ObjectPool () {
		// Unintern any items still in this pool.
		for (PoolMapIterator pp = pool_map.begin();
		     pp != pool_map.end();
		     ++pp) {
			// TODO: Assert pp->second->ref_count > 0.
			pp->second->is_interned = false;
		}
	}

	PoolPtr getObject (const_reference value) {
		PoolMapIterator pp = pool_map.find(&value);
		if (pp == pool_map.end()) {
			// Construct a new PoolData for this value.  It will
			// automatically intern itself.
			return new PoolData (*this, value);
		} else {
			// Use existing PoolData for this value.
			return pp->second;
		}
	}

	/*
	 * Reference-counting smart pointer.
	 */
	class PoolPtr {
		friend class ObjectPool;

	public:
		// Reference-counting construction/destruction.
		~PoolPtr () {
			data_ptr->delRef();
		}

		// Reference-counting copy/assignment.
		PoolPtr (const PoolPtr& src) : data_ptr (src.data_ptr) {
			data_ptr->addRef();
		}

		PoolPtr& operator = (const PoolPtr& src) {
			// Add reference to source item, then delete the
			// reference to the old item.  This order is important
			// for handling the self-assignment case.
			src.data_ptr->addRef();
			data_ptr->delRef();

			// Perform the actual assignment.
			data_ptr = src.data_ptr;
			return *this;
		}

		// Pointer-style access to the value.
		const_reference operator * () const {
			return data_ptr->value;
		}

		const value_type *operator -> () const {
			return &data_ptr->value;
		}

		// Get the metadata variable associated with the value.
		meta_value_type& metadata () const {
			return data_ptr->metadata;
		}

	private:
		// Reference-counting construction/destruction.
		PoolPtr (PoolData *data_ptr) : data_ptr (data_ptr) {
			data_ptr->addRef();
		}

		// The managed pointer.
		PoolData *data_ptr;
	}; // template class ObjectPool::PoolPtr

private:
	// The value's reference-counted owner object.
	class PoolData {
		friend class ObjectPool;

	public:
		PoolData (ObjectPool& owner, const_reference value_ref)
		: value (value_ref), metadata (0),
		  owner (owner), ref_count (0), is_interned (true) {
			owner.pool_map.insert(PoolMapValue (&value, this));
		}

		void addRef () {
			// TODO: Throw Exception on overflow.
			ref_count++;
		}

		void delRef () {
			// TODO: Assert ref_count > 0.
			if (--ref_count < 1) {
				if (is_interned) {
					is_interned = false;
					owner.pool_map.erase(&value);
				}

				// Yay! We're crazy!
				delete this;
			}
		}

		const value_type value;

		meta_value_type metadata;

	private:
		ObjectPool& owner;
		size_t ref_count;
		bool is_interned;
	}; // template class ObjectPool::PoolData

	// Value pointer comparator functor.
	struct PoolComp {
		bool operator () (const_pointer lhs, const_pointer rhs) const {
			return *lhs < *rhs;
		}
	}; // template struct ObjectPool::PoolComp

	PoolMap pool_map;
}; // template class ObjectPool

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_OBJECTPOOL_HH */
