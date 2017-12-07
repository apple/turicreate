#ifndef JSON_ALLOCATOR_H
#define JSON_ALLOCATOR_H

#include "../JSONOptions.h"
#if defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)

#include <cstddef>

//need these for the json_nothrow
#include "JSONDefs/Visual_C.h"
#include "JSONDefs/GNU_C.h"
#include "JSONDefs/Unknown_C.h"

class JSONAllocatorRelayer {
public:
#ifdef JSON_UNIT_TEST
    static size_t getAllocationCount(void);
	static size_t getAllocationByteCount(void);
	static size_t getDeallocationCount(void);
#endif
	static void * alloc(size_t bytes) json_nothrow json_hot;
	static void dealloc(void * ptr) json_nothrow json_hot;
};

template <class T> class json_allocator;

// specialize for void:
template <> class json_allocator<void> {
public:
	typedef void*       pointer;
	typedef const void* const_pointer;
	// reference to void members are impossible.
	typedef void value_type;
	template <class U> struct rebind { typedef json_allocator<U> other; };
};

template <class T> class json_allocator {
public:
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef T*        pointer;
	typedef const T*  const_pointer;
	typedef T&        reference;
	typedef const T&  const_reference;
	typedef T         value_type;
	template <class U> struct rebind { typedef json_allocator<U> other; };
	
	inline json_allocator() json_nothrow {}
	inline json_allocator(const json_allocator&) json_nothrow {}
	template <class U> inline json_allocator(const json_allocator<U>&) json_nothrow {}
	inline ~json_allocator() json_nothrow {}
	
	inline pointer address(reference x) const { return &x; }
	inline const_pointer address(const_reference x) const { return &x; }
	
	inline pointer allocate(size_type n, json_allocator<void>::const_pointer = 0) json_hot {
		return (pointer)JSONAllocatorRelayer::alloc(n * sizeof(T));
	}
	inline void deallocate(pointer p, size_type) json_hot {
		JSONAllocatorRelayer::dealloc(p);
	}
	
	inline size_type max_size() const json_nothrow { return 0xEFFFFFFF; }
	
	inline void construct(pointer p, const T& val){
		new(p)T(val);
	};
	inline void destroy(pointer p){
		((T*)p) -> ~T();
	}
};

template <class T1, class T2> inline bool operator==(const json_allocator<T1>&, const json_allocator<T2>&) json_nothrow { return true; }
template <class T1, class T2> inline bool operator!=(const json_allocator<T1>&, const json_allocator<T2>&) json_nothrow { return false; }

#endif
#endif
