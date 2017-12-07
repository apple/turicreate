#include "JSONAllocator.h"

#if defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
#include "JSONMemory.h"

#ifdef JSON_UNIT_TEST
    static size_t alloccount = 0;
	static size_t dealloccount = 0;
	static size_t allocbyte = 0;
    size_t JSONAllocatorRelayer::getAllocationCount(void){ return alloccount; }
	size_t JSONAllocatorRelayer::getDeallocationCount(void){ return alloccount; }
	size_t JSONAllocatorRelayer::getAllocationByteCount(void){ return allocbyte; }
    #define INC_ALLOC() ++alloccount;
	#define INC_ALLODEALLOC() ++dealloccount;
	#define INC_ALLOC_BYTES(bytes) allocbyte += bytes;
#else
    #define INC_ALLOC() (void)0
	#define INC_ALLODEALLOC() (void)0
	#define INC_ALLOC_BYTES(bytes) (void)0
#endif

void * JSONAllocatorRelayer::alloc(size_t bytes) json_nothrow {
    INC_ALLOC();
	INC_ALLOC_BYTES(bytes);
	return JSONMemory::json_malloc(bytes);
}

void JSONAllocatorRelayer::dealloc(void * ptr) json_nothrow {
	INC_ALLODEALLOC();
	JSONMemory::json_free(ptr);
}
#endif
