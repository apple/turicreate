#ifndef mempool___mempool_h
#define mempool___mempool_h


/*
 * This is where you may alter options to give mempool++
 */

//#define MEMPOOL_DEBUGGING					 //Causes mempool++ to spit out what it's doing to the console
//#define MEMPOOL_ASSERTS						 //Causes mempool++ to check what it's doing and look for impossible cases
#define MEMPOOL_OVERFLOW 3.0f / 4.0f		 //Changes how full a pool is before going to fallbacks
//#define MEMPOOL_DETERMINE_DISTRIBUTION		 //Allows mempool++ to automatically give you the best distribution
#define MEMPOOL_DETERMINE_SCALAR 5.0f / 3.0f //Gives you this times the max number at any given time from distribution
#define MEMPOOL_FALLBACK_DEPTH 3
//#define MEMPOOL_PERFORMANCE_DEBUGGING
//#define private public


//version info
#define __MEMPOOL_MAJOR__ 1
#define __MEMPOOL_MINOR__ 2
#define __MEMPOOL_PATCH__ 0
#define __MEMPOOL_VERSION__ (__MEMPOOL_MAJOR__ * 10000 + __MEMPOOL_MINOR__ * 100 + __MEMPOOL_PATCH__)

/*
 * This is where special function / macro for special options are
 */

#ifdef MEMPOOL_DEBUGGING
	#include <iostream>
	#define MEMPOOL_DEBUG(x) std::cout << x << std::endl;
#else
	#define MEMPOOL_DEBUG(x)
#endif

#ifdef MEMPOOL_ASSERTS
	#include <iostream>
	#define MEMPOOL_ASSERT(condition) if (pool_unlikely(!(condition))){ std::cout << #condition << " isn't true" << std::endl; }
	#define MEMPOOL_ASSERT2(condition, out) if (pool_unlikely(!(condition))){ std::cout << out << std::endl; }
#else
	#define MEMPOOL_ASSERT(condition)
	#define MEMPOOL_ASSERT2(condition, out)
#endif

#ifdef MEMPOOL_PERFORMANCE_DEBUGGING
	#include <iostream>
	#include <sstream>
	#include <string>
	#define MEMPOOL_PERFORMANCE_DEBUG(x) std::cout << x << std::endl;
#else
	#define MEMPOOL_PERFORMANCE_DEBUG(x)
#endif



/*
 * This is where compiler-specific code goes
 */
#ifdef __GNUC__
    #if (__GNUC__ >= 3)
	   #define POOL_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #else
	   #define POOL_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
    #endif

    #if (POOL_GCC_VERSION >= 40300)
	   #define pool_hot pool_nothrow __attribute__ ((hot))
	#else
		#define pool_hot pool_nothrow
	#endif
	
	#if (POOL_GCC_VERSION >= 29600)
		#define pool_likely(x) __builtin_expect((long)((bool)(x)),1)
		#define pool_unlikely(x) __builtin_expect((long)((bool)(x)),0)
	#else
		#define pool_likely(x) x
		#define pool_unlikely(x) x
	#endif
	
	#define pool_nothrow throw()
#else
	#define pool_hot pool_nothrow
	#define pool_likely(x) x
	#define pool_unlikely(x) x
	#define pool_nothrow
#endif

#include <cstring>

/*
 *	This is where the classes are
 */

//Callbacks for fallback if the pool is out of space
class mempool_callbacks {
public:
	typedef void * (*mallocer_t)(size_t);
	typedef void(*freer_t)(void *);
	typedef void * (*reallocer_t)(void *, size_t);

	//Allows the user to alter where the fallbacks point to
	static inline void set(mallocer_t mallocer, reallocer_t reallocer, freer_t freer) pool_nothrow {
		get_instance()._malloc = mallocer;
		get_instance()._free = freer;
		get_instance()._realloc = reallocer;
	}
	
	//allocates memory
	static inline void * allocate(size_t size) pool_nothrow { 
		MEMPOOL_DEBUG("Returing malloced memory:" << size << " bytes");
		return get_instance()._malloc(size); 
	}
	
	//frees memory
	static inline void deallocate(void * ptr) pool_nothrow { 
		MEMPOOL_DEBUG("Freeing malloced memory: " << ptr);
		get_instance()._free(ptr);
	}
	
	static inline void * reallocate(void * ptr, size_t size) pool_nothrow {
		MEMPOOL_DEBUG("Reallocating memory: " << ptr << " to " << size << " bytes");
		return get_instance()._realloc(ptr, size);
	}
private:
	//Retrieves a Meyers singleton
	static inline mempool_callbacks & get_instance(void) pool_nothrow {
		static mempool_callbacks _single(std::malloc, std::realloc, std::free);
		return _single;
	}
				
	//The constructor
	inline mempool_callbacks(mallocer_t mallocer, reallocer_t reallocer, freer_t freer) :
		_malloc(mallocer),
		_free(freer),
		_realloc(reallocer){
	}
	
	//not copyable
	mempool_callbacks & operator = (const mempool_callbacks & other);
	mempool_callbacks(const mempool_callbacks & other);
	
	//member callbacks
	mallocer_t _malloc;
	reallocer_t _realloc;
	freer_t _free;
};

//The workhorse of the templates library, a class that holds a pool that it allocates memory from
template <typename T, size_t size>
class object_memory_pool;  //forward declaration

template <size_t bytes, size_t size>
class memory_pool_no_fullflag {  //forward declaration
public:
	memory_pool_no_fullflag<bytes, size>():
	_link(NULL),
	current(0),
	depth(0),
	threshold((size_t)((float)size * (MEMPOOL_OVERFLOW))),
	memoryPool_end(memoryPool_start + (size * bytes)),
	used_end(used_start + size),
	runningPointer(used_start)
	{
		std::memset(used_start, 0, size * sizeof(bool));
	}
	
	virtual ~memory_pool_no_fullflag(void){
		if (_link){
			_link -> ~memory_pool_no_fullflag();
			mempool_callbacks::deallocate(_link);
		}
	}
	
	inline size_t load(void) const pool_nothrow {
		return current;
	}
	
	inline void * allocate(void) pool_hot {
		if (void * res = allocate_nofallback()){
			return res;
		}
		return _link_allocate();
	}
	
	inline void deallocate(void * ptr) pool_hot {
		if (memory_pool_no_fullflag<bytes, size> * container = contains(ptr)){
			container -> deallocate_nofallback(ptr);
		} else {
			mempool_callbacks::deallocate(ptr);
		}
	}
	
	void * allocate_nofallback() pool_hot {
		if (!(*runningPointer)) return _return_current();
		if (++runningPointer >= used_end) runningPointer = used_start;
		if (current < threshold){
			//make sure it doesnt loop around infinity so point it to itself
			const bool * position = runningPointer; 
			do {
				if (!(*runningPointer)) return _return_current();
				if (++runningPointer >= used_end) runningPointer = used_start;
			} while (position != runningPointer);
			MEMPOOL_ASSERT2(false, "Got to impossible code location");
		}
	
		MEMPOOL_DEBUG("Returing null");
		return NULL;
	}
	
	void deallocate_nofallback(void * ptr) pool_hot {
		MEMPOOL_ASSERT2(current, "current not positive");
		--current;
		MEMPOOL_DEBUG("Freeing slot " << ((char*)ptr - memoryPool_start) / bytes);
		MEMPOOL_DEBUG("  pointer=" << ptr);
		MEMPOOL_ASSERT2((((char*)ptr - memoryPool_start) / bytes) < size, "Freeing slot " << (((char*)ptr - memoryPool_start) / bytes) << " in a pool with only " << size << " items");
		MEMPOOL_ASSERT2(used_start[((char*)ptr - memoryPool_start) / bytes], "Freeing " << ptr << " and it's already been freed");
		used_start[(((char*)ptr - memoryPool_start) / bytes)] = false;
	}
	
	inline memory_pool_no_fullflag<bytes, size> * contains(void * ptr) pool_hot {
		if ((ptr >= memoryPool_start) && (ptr < memoryPool_end)) return this;
		return (_link) ? _link -> contains(ptr) : NULL;
	}
	
	#ifdef MEMPOOL_PERFORMANCE_DEBUGGING
		const char * const getName(void){ return "memory_pool_no_fullflag"; }
		
		const char * getDepth(){
			static const char * depths[15] = {
				" ",
				"  ",
				"   ",
				"    ",
				"     ",
				"      ",
				"       ",
				"        ",
				"         ",
				"          ",
				"           ",
				"            ",
				"             ",
				"              ",
				"-              ",
			};
			if (depth > 14) return depths[14];
			return depths[depth];
		}
		
		std::string dump(void){
			std::stringstream output;
			output << getDepth() << getName() << "<" << bytes << ", " << size << ">: " << (void*)this << std::endl;
			output << getDepth() << "Currently holding: " << current << " items." << std::endl;
			//_____
			output << getDepth() << "+";
			for(int i = 0; i < 78; ++i){
				output << "_";
			}
			output << "+" << std::endl << getDepth() << "|";
			
			//Fill in
			int i;
			for(i = 0; i < size; ++i){
				if ((i % 80 == 0) && (i != 0)){
					output << "|" << std::endl << getDepth() << "|";
				}
				if (i == (runningPointer - used_start)){
					if (used_start[i]){
						output << "R";
					} else {
						output << "P";
					}
				} else if (used_start[i]){
					output << "X";
				} else {
					output << " ";
				}
			}
			
			for(; (i % 80) != 0; ++i){
				output << "+";
			}
			
			//-------
			output << getDepth() << "+";
			for(i = 0; i < 78; ++i){
				output << "-";
			}
			output << "+";
			if (_link){
				output << "----+" << std::endl;
			}
			return output.str();
		}
	#endif
protected:
	//copy ctors and assignment operator
	memory_pool_no_fullflag & operator = (const memory_pool_no_fullflag & other);
	memory_pool_no_fullflag(const memory_pool_no_fullflag & other);
	
	inline void * _return_current(void) pool_hot {
		*runningPointer = true;
		++current;
		MEMPOOL_DEBUG("Returning slot " << runningPointer - used_start << " at depth " << depth);
		MEMPOOL_DEBUG("  memoryPool_start=" << (void*)memoryPool_start);
		MEMPOOL_DEBUG("  memoryPool_end  =" << (void*)memoryPool_end);
		MEMPOOL_DEBUG("  return value    =" << (void*)(memoryPool_start + ((runningPointer - used_start) * bytes)));
		MEMPOOL_ASSERT2(((memoryPool_start + ((runningPointer - used_start) * bytes))) < memoryPool_end, "Returning pointer outside the high end of the pool");
		MEMPOOL_ASSERT2(((memoryPool_start + ((runningPointer - used_start) * bytes))) >= memoryPool_start, "Returning pointer outside the low end of the pool");
		const bool * const pre = runningPointer;
		if (++runningPointer >= used_end) runningPointer = used_start;
		return memoryPool_start + ((pre - used_start) * bytes);
	}
	
	void * _link_allocate(void) pool_nothrow {
		if (depth >= MEMPOOL_FALLBACK_DEPTH) return mempool_callbacks::allocate(bytes);
		if (!_link){
			_link = new(mempool_callbacks::allocate(sizeof(memory_pool_no_fullflag<bytes, size>))) memory_pool_no_fullflag<bytes, size>();
			_link -> depth = depth + 1;
		}
		return _link -> allocate();
	}

	size_t current;							//The current number of items in the pool
	size_t threshold;						//The number of items in the pool before it starts using fallback
	char memoryPool_start[size * bytes];	//The memory pool
	char * memoryPool_end;					//The end of the memory pool
	bool used_start[size];					//A pool to know whether or not an item is currently used
	bool * used_end;						//The end of the boolean flags
	bool * runningPointer;					//A pointer that loops, keeping an eye on what is taken and what isn't
	memory_pool_no_fullflag<bytes, size> * _link;		//Creates a linked list when expanding
	size_t depth;
};

template <size_t bytes, size_t size>
class memory_pool : public memory_pool_no_fullflag<bytes, size> {
public:
	memory_pool<bytes, size>() : 
	memory_pool_no_fullflag<bytes, size>(),
	_full(false){}
	
	virtual ~memory_pool(void){}
	
	inline void * allocate(void) pool_hot {
		if (_full) return mempool_callbacks::allocate(bytes);
		return memory_pool_no_fullflag<bytes, size>::allocate();
	}
	
	inline void deallocate(void * ptr) pool_hot {
		_full = false;
		return memory_pool_no_fullflag<bytes, size>::deallocate(ptr);
	}
	
	#ifdef MEMPOOL_PERFORMANCE_DEBUGGING
		const char * const getName(void){ return "memory_pool"; }
	#endif
private:
	//copy ctors and assignment operator
	memory_pool & operator = (const memory_pool & other);
	memory_pool(const memory_pool & other);
	
	bool _full;
	
	template <typename T, size_t s> friend class object_memory_pool;
};


//A memory pool for a specific type of object
#define new_object(pool, ctor) new (pool.allocate_noctor()) ctor  //allows user to call a specific ctor on the object [ new_object(mypool, T(x, y)) ] 
template <typename T, size_t size>
class object_memory_pool {
public:	
	inline size_t load(void) const pool_nothrow {
		return _pool.load();
	}
	
	virtual ~object_memory_pool() pool_nothrow { } //so that it can be overloaded
	
	inline T * allocate(void) pool_hot { return new (_pool.allocate()) T(); }
	inline void * allocate_noctor(void) pool_nothrow { return _pool.allocate(); }
	
	inline void deallocate(T * ptr) pool_hot {
		ptr -> ~T();
		_pool.deallocate(ptr);
	}
	
	inline memory_pool<sizeof(T), size> * contains(T * ptr) const pool_hot {
		return _pool.contains((void*)ptr);	
	}
	
	inline T * alloc_nofallback() pool_hot {
		if (void * res = _pool.allocate_nofallback()){
			return new (res) T();
		}
		return NULL;
	}
	
	inline void deallocate_nofallback(T * ptr) pool_hot {
		ptr -> ~T();
		_pool.deallocate_nofallback(ptr);
	}
	#ifdef MEMPOOL_PERFORMANCE_DEBUGGING
		const std::string dump(void){ return _pool.dump(); }
	#endif
private:
	memory_pool<sizeof(T), size> _pool;
};


#define MEMPOOL_TEMPLATE_PAIR(x) size_t bytes ## x , size_t count ## x
#define MEMPOOL_ALLOC_CHECK(x) if (bytes <= bytes ## x ){ if (void * res = (_pool ## x).allocate_nofallback()) return res; }
#define MEMPOOL_DEALLOC_CHECK(x) if (memory_pool_no_fullflag< bytes ## x , count ## x > * container = (_pool ## x).contains(ptr)){ container -> deallocate_nofallback(ptr); return; }
#define MEMPOOL_MEMBER_POOL(x) memory_pool< bytes ## x , count ## x > _pool ## x;

#define MEMPOOL_REALLOC_CHECK(x)\
	if (memory_pool_no_fullflag< bytes ## x , count ## x > * container = (_pool ## x).contains(ptr)){\
		if (bytes <= bytes ## x) return ptr;\
		void * newvalue = allocate(bytes);\
		std::memcpy(newvalue, ptr, bytes ## x);\
		container -> deallocate_nofallback(ptr);\
		return newvalue;\
	}


#ifdef MEMPOOL_DETERMINE_DISTRIBUTION
	#include <map>
	#include <iostream>
	#define MEMPOOL_ALLOC_METHOD(number, code)\
		bucket_pool_ ## number (void) : _profile_on_delete(0) { }\
		~bucket_pool_ ## number (void) { \
			if (_profile_on_delete){ \
				dump_atonce(_profile_on_delete, _profile_on_delete / 40); \
				dump_template(_profile_on_delete); \
			} \
		}\
		void * allocate(size_t bytes) pool_hot {\
			if (mapping.find(bytes) != mapping.end()){\
				++mapping[bytes];\
				++current_mapping[bytes];\
				if (current_mapping[bytes] > max_mapping[bytes]) max_mapping[bytes] = current_mapping[bytes];\
			} else {\
				mapping[bytes] = 1;\
				max_mapping[bytes] = 1;\
				current_mapping[bytes] = 1;\
			}\
			void * res = mempool_callbacks::allocate(bytes);\
			mem_mapping[res] = bytes;\
			return res;\
		}
	#define MEMPOOL_DEALLOC_METHOD(code)\
		void deallocate(void * ptr) pool_hot {\
			--current_mapping[mem_mapping[ptr]];\
			mem_mapping.erase(ptr);\
			mempool_callbacks::deallocate(ptr);\
		}
	
	#define MEMPOOL_ANALYZERS(macro_count)\
		inline size_t _max(size_t one, size_t two){ return (one > two) ? one : two; }\
		void dump_total(size_t max, size_t sep = 16, size_t tlen = 30){\
			std::cout << "-------- Total --------" << std::endl;\
			size_t max_amount = 0;\
			for(size_t i = 0; i < max;){\
				size_t amount = 0;\
				for(size_t j = 0; j < sep; ++j, ++i){\
					if (mapping.find(i) != mapping.end()){\
						amount += mapping[i];\
					}\
				}\
				if (amount > max_amount) max_amount = amount;\
			}\
			float scalar = ((float)max_amount) / ((float)tlen);\
			\
			for(size_t i = 0; i < max;){\
				size_t amount = 0;\
				for(size_t j = 0; j < sep; ++j, ++i){\
					if (mapping.find(i) != mapping.end()){\
						amount += mapping[i];\
					}\
				}\
				\
				if (i < 10) std::cout << ' ';\
				if (i < 100) std::cout << ' ';\
				if (i < 1000) std::cout << ' ';\
				if (i < 10000) std::cout << ' ';\
				std::cout << i << ':';\
				\
				for(size_t j = 0; j < (size_t)((float)amount / scalar); ++j){\
					std::cout << '*';\
				}\
				std::cout << '(' << amount << ')' << std::endl;\
			}\
		}\
		\
		void dump_atonce(size_t max, size_t sep = 16, size_t tlen = 30){\
			std::cout << "------ Distribution for \"" << _str << "\" ------" << std::endl;\
			size_t max_amount = 0;\
			for(size_t i = 0; i < max;){\
				size_t amount = 0;\
				for(size_t j = 0; j < sep; ++j, ++i){\
					if (max_mapping.find(i) != max_mapping.end()){\
						amount += max_mapping[i];\
					}\
				}\
				if (amount > max_amount) max_amount = amount;\
			}\
			float scalar = ((float)max_amount) / ((float)tlen);\
			\
			for(size_t i = 0; i < max;){\
				size_t amount = 0;\
				for(size_t j = 0; j < sep; ++j, ++i){\
					if (max_mapping.find(i) != max_mapping.end()){\
						amount += max_mapping[i];\
					}\
				}\
				\
				if (i < 10) std::cout << ' ';\
				if (i < 100) std::cout << ' ';\
				if (i < 1000) std::cout << ' ';\
				if (i < 10000) std::cout << ' ';\
				std::cout << i << ':';\
				\
				for(size_t j = 0; j < (size_t)((float)amount / scalar); ++j){\
					std::cout << '*';\
				}\
				std::cout << '(' << amount << ')' << std::endl;\
			}\
		}\
		\
		void dump_template(size_t max){\
			std::cout << "Recommended Template for \"" << _str << "\" = ";\
			size_t total_at_once = 0;\
			size_t highest = 0;\
			for(size_t i = 0; i < max; ++i){\
				if (max_mapping.find(i) != max_mapping.end()){\
					total_at_once += max_mapping[i];\
					highest = i;\
				}\
			}\
			\
			size_t count = 0;\
			size_t total_at_once_part = total_at_once / macro_count;\
			size_t current = 0;\
			size_t totalsofar = 0;\
			std::cout << '<';\
			for(size_t i = 0; ((i < max) && (count < (macro_count -1))); ++i){\
				if (max_mapping.find(i) != max_mapping.end()){\
					current += max_mapping[i];\
					totalsofar += max_mapping[i];\
					if (current > total_at_once_part){\
						std::cout << (i - 1)  << ", " << (size_t)(((float)current - max_mapping[i]) * (MEMPOOL_DETERMINE_SCALAR)) << ", ";\
						current = max_mapping[i];\
						++count;\
					}\
				}\
			}\
			std::cout << max << ", " << _max((size_t)((float)(total_at_once - totalsofar) * (MEMPOOL_DETERMINE_SCALAR)), total_at_once_part / 2) << '>' << std::endl;\
		}\
		\
		inline void profile_on_delete(size_t var, const std::string & str){ _profile_on_delete = var; _str = str; }

	#define MEMPOOL_MEMBERS(code)\
		 std::map<size_t, size_t> mapping;\
		 std::map<size_t, size_t> current_mapping;\
		 std::map<size_t, size_t> max_mapping;\
		 std::map<void *, size_t> mem_mapping;\
		 size_t _profile_on_delete;\
		 std::string _str;
	#define MEMPOOL_LOAD(number, code) inline size_t * load(void) const pool_nothrow { static size_t _load[number] = {0}; return &_load[0]; }
#else
	#define MEMPOOL_ALLOC_METHOD(number, code)\
		void * allocate(size_t bytes) pool_hot {\
			code\
			return mempool_callbacks::allocate(bytes);\
		}
	#define MEMPOOL_DEALLOC_METHOD(code)\
		void deallocate(void * ptr) pool_hot {\
			code\
			mempool_callbacks::deallocate(ptr);\
		}
	#define MEMPOOL_ANALYZERS(macro_count)
	#define MEMPOOL_MEMBERS(code) code
	#define MEMPOOL_LOAD(number, code) inline size_t * load(void) const pool_nothrow { static size_t _load[number]; code return &_load[0]; }
#endif

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2)>
class bucket_pool_2 {
public:
	MEMPOOL_ALLOC_METHOD(
		2,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
	}
	MEMPOOL_LOAD(
		2,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
	)	
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
	)
	MEMPOOL_ANALYZERS(2)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3)>
class bucket_pool_3 {
public:
	MEMPOOL_ALLOC_METHOD(
		3,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		3,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
	)
	MEMPOOL_ANALYZERS(3)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3),
MEMPOOL_TEMPLATE_PAIR(4)>
class bucket_pool_4 {
public:
	MEMPOOL_ALLOC_METHOD(
		4,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
		MEMPOOL_ALLOC_CHECK(4)
	)
	void * reallocate(void * ptr, size_t bytes){
        MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		MEMPOOL_REALLOC_CHECK(4)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		4,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
		_load[3] = _pool4.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
		MEMPOOL_DEALLOC_CHECK(4)
	)
	MEMPOOL_ANALYZERS(4)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
		MEMPOOL_MEMBER_POOL(4)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3),
MEMPOOL_TEMPLATE_PAIR(4),
MEMPOOL_TEMPLATE_PAIR(5)>
class bucket_pool_5 {
public:
	MEMPOOL_ALLOC_METHOD(
		5,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
		MEMPOOL_ALLOC_CHECK(4)
		MEMPOOL_ALLOC_CHECK(5)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		MEMPOOL_REALLOC_CHECK(4)
		MEMPOOL_REALLOC_CHECK(5)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		5,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
		_load[3] = _pool4.load();
		_load[4] = _pool5.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
		MEMPOOL_DEALLOC_CHECK(4)
		MEMPOOL_DEALLOC_CHECK(5)
	)
	MEMPOOL_ANALYZERS(5)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
		MEMPOOL_MEMBER_POOL(4)
		MEMPOOL_MEMBER_POOL(5)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3),
MEMPOOL_TEMPLATE_PAIR(4),
MEMPOOL_TEMPLATE_PAIR(5),
MEMPOOL_TEMPLATE_PAIR(6)>
class bucket_pool_6 {
public:
	MEMPOOL_ALLOC_METHOD(
		6,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
		MEMPOOL_ALLOC_CHECK(4)
		MEMPOOL_ALLOC_CHECK(5)
		MEMPOOL_ALLOC_CHECK(6)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		MEMPOOL_REALLOC_CHECK(4)
		MEMPOOL_REALLOC_CHECK(5)
		MEMPOOL_REALLOC_CHECK(6)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		6,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
		_load[3] = _pool4.load();
		_load[4] = _pool5.load();
		_load[5] = _pool6.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
		MEMPOOL_DEALLOC_CHECK(4)
		MEMPOOL_DEALLOC_CHECK(5)
		MEMPOOL_DEALLOC_CHECK(6)
	)
	MEMPOOL_ANALYZERS(6)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
		MEMPOOL_MEMBER_POOL(4)
		MEMPOOL_MEMBER_POOL(5)
		MEMPOOL_MEMBER_POOL(6)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3),
MEMPOOL_TEMPLATE_PAIR(4),
MEMPOOL_TEMPLATE_PAIR(5),
MEMPOOL_TEMPLATE_PAIR(6),
MEMPOOL_TEMPLATE_PAIR(7)>
class bucket_pool_7 {
public:
	MEMPOOL_ALLOC_METHOD(
		7,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
		MEMPOOL_ALLOC_CHECK(4)
		MEMPOOL_ALLOC_CHECK(5)
		MEMPOOL_ALLOC_CHECK(6)
		MEMPOOL_ALLOC_CHECK(7)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		MEMPOOL_REALLOC_CHECK(4)
		MEMPOOL_REALLOC_CHECK(5)
		MEMPOOL_REALLOC_CHECK(6)
		MEMPOOL_REALLOC_CHECK(7)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		7,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
		_load[3] = _pool4.load();
		_load[4] = _pool5.load();
		_load[5] = _pool6.load();
		_load[6] = _pool7.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
		MEMPOOL_DEALLOC_CHECK(4)
		MEMPOOL_DEALLOC_CHECK(5)
		MEMPOOL_DEALLOC_CHECK(6)
		MEMPOOL_DEALLOC_CHECK(7)
	)
	MEMPOOL_ANALYZERS(7)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
		MEMPOOL_MEMBER_POOL(4)
		MEMPOOL_MEMBER_POOL(5)
		MEMPOOL_MEMBER_POOL(6)
		MEMPOOL_MEMBER_POOL(7)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3),
MEMPOOL_TEMPLATE_PAIR(4),
MEMPOOL_TEMPLATE_PAIR(5),
MEMPOOL_TEMPLATE_PAIR(6),
MEMPOOL_TEMPLATE_PAIR(7),
MEMPOOL_TEMPLATE_PAIR(8)>
class bucket_pool_8 {
public:
	MEMPOOL_ALLOC_METHOD(
		8,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
		MEMPOOL_ALLOC_CHECK(4)
		MEMPOOL_ALLOC_CHECK(5)
		MEMPOOL_ALLOC_CHECK(6)
		MEMPOOL_ALLOC_CHECK(7)
		MEMPOOL_ALLOC_CHECK(8)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		MEMPOOL_REALLOC_CHECK(4)
		MEMPOOL_REALLOC_CHECK(5)
		MEMPOOL_REALLOC_CHECK(6)
		MEMPOOL_REALLOC_CHECK(7)
		MEMPOOL_REALLOC_CHECK(8)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		8,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
		_load[3] = _pool4.load();
		_load[4] = _pool5.load();
		_load[5] = _pool6.load();
		_load[6] = _pool7.load();
		_load[7] = _pool8.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
		MEMPOOL_DEALLOC_CHECK(4)
		MEMPOOL_DEALLOC_CHECK(5)
		MEMPOOL_DEALLOC_CHECK(6)
		MEMPOOL_DEALLOC_CHECK(7)
		MEMPOOL_DEALLOC_CHECK(8)
	)
	MEMPOOL_ANALYZERS(8)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
		MEMPOOL_MEMBER_POOL(4)
		MEMPOOL_MEMBER_POOL(5)
		MEMPOOL_MEMBER_POOL(6)
		MEMPOOL_MEMBER_POOL(7)
		MEMPOOL_MEMBER_POOL(8)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3),
MEMPOOL_TEMPLATE_PAIR(4),
MEMPOOL_TEMPLATE_PAIR(5),
MEMPOOL_TEMPLATE_PAIR(6),
MEMPOOL_TEMPLATE_PAIR(7),
MEMPOOL_TEMPLATE_PAIR(8),
MEMPOOL_TEMPLATE_PAIR(9)>
class bucket_pool_9 {
public:
	MEMPOOL_ALLOC_METHOD(
		9,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
		MEMPOOL_ALLOC_CHECK(4)
		MEMPOOL_ALLOC_CHECK(5)
		MEMPOOL_ALLOC_CHECK(6)
		MEMPOOL_ALLOC_CHECK(7)
		MEMPOOL_ALLOC_CHECK(8)
		MEMPOOL_ALLOC_CHECK(9)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		MEMPOOL_REALLOC_CHECK(4)
		MEMPOOL_REALLOC_CHECK(5)
		MEMPOOL_REALLOC_CHECK(6)
		MEMPOOL_REALLOC_CHECK(7)
		MEMPOOL_REALLOC_CHECK(8)
		MEMPOOL_REALLOC_CHECK(9)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		9,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
		_load[3] = _pool4.load();
		_load[4] = _pool5.load();
		_load[5] = _pool6.load();
		_load[6] = _pool7.load();
		_load[7] = _pool8.load();
		_load[8] = _pool9.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
		MEMPOOL_DEALLOC_CHECK(4)
		MEMPOOL_DEALLOC_CHECK(5)
		MEMPOOL_DEALLOC_CHECK(6)
		MEMPOOL_DEALLOC_CHECK(7)
		MEMPOOL_DEALLOC_CHECK(8)
		MEMPOOL_DEALLOC_CHECK(9)
	)
	MEMPOOL_ANALYZERS(9)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
		MEMPOOL_MEMBER_POOL(4)
		MEMPOOL_MEMBER_POOL(5)
		MEMPOOL_MEMBER_POOL(6)
		MEMPOOL_MEMBER_POOL(7)
		MEMPOOL_MEMBER_POOL(8)
		MEMPOOL_MEMBER_POOL(9)
	)
};

template<	
MEMPOOL_TEMPLATE_PAIR(1),
MEMPOOL_TEMPLATE_PAIR(2),
MEMPOOL_TEMPLATE_PAIR(3),
MEMPOOL_TEMPLATE_PAIR(4),
MEMPOOL_TEMPLATE_PAIR(5),
MEMPOOL_TEMPLATE_PAIR(6),
MEMPOOL_TEMPLATE_PAIR(7),
MEMPOOL_TEMPLATE_PAIR(8),
MEMPOOL_TEMPLATE_PAIR(9),
MEMPOOL_TEMPLATE_PAIR(10)>
class bucket_pool_10 {
public:
	MEMPOOL_ALLOC_METHOD(
		10,
		MEMPOOL_ALLOC_CHECK(1)
		MEMPOOL_ALLOC_CHECK(2)
		MEMPOOL_ALLOC_CHECK(3)
		MEMPOOL_ALLOC_CHECK(4)
		MEMPOOL_ALLOC_CHECK(5)
		MEMPOOL_ALLOC_CHECK(6)
		MEMPOOL_ALLOC_CHECK(7)
		MEMPOOL_ALLOC_CHECK(8)
		MEMPOOL_ALLOC_CHECK(9)
		MEMPOOL_ALLOC_CHECK(10)
	)
	void * reallocate(void * ptr, size_t bytes){
		MEMPOOL_REALLOC_CHECK(1)
		MEMPOOL_REALLOC_CHECK(2)
		MEMPOOL_REALLOC_CHECK(3)
		MEMPOOL_REALLOC_CHECK(4)
		MEMPOOL_REALLOC_CHECK(5)
		MEMPOOL_REALLOC_CHECK(6)
		MEMPOOL_REALLOC_CHECK(7)
		MEMPOOL_REALLOC_CHECK(8)
		MEMPOOL_REALLOC_CHECK(9)
		MEMPOOL_REALLOC_CHECK(10)
		return mempool_callbacks::reallocate(ptr, bytes);
	}
	MEMPOOL_LOAD(
		10,
		_load[0] = _pool1.load();
		_load[1] = _pool2.load();
		_load[2] = _pool3.load();
		_load[3] = _pool4.load();
		_load[4] = _pool5.load();
		_load[5] = _pool6.load();
		_load[6] = _pool7.load();
		_load[7] = _pool8.load();
		_load[8] = _pool9.load();
		_load[9] = _pool10.load();
	)
	MEMPOOL_DEALLOC_METHOD(
		MEMPOOL_DEALLOC_CHECK(1)
		MEMPOOL_DEALLOC_CHECK(2)
		MEMPOOL_DEALLOC_CHECK(3)
		MEMPOOL_DEALLOC_CHECK(4)
		MEMPOOL_DEALLOC_CHECK(5)
		MEMPOOL_DEALLOC_CHECK(6)
		MEMPOOL_DEALLOC_CHECK(7)
		MEMPOOL_DEALLOC_CHECK(8)
		MEMPOOL_DEALLOC_CHECK(9)
		MEMPOOL_DEALLOC_CHECK(10)
	)
	MEMPOOL_ANALYZERS(10)
private:
	MEMPOOL_MEMBERS(
		MEMPOOL_MEMBER_POOL(1)
		MEMPOOL_MEMBER_POOL(2)
		MEMPOOL_MEMBER_POOL(3)
		MEMPOOL_MEMBER_POOL(4)
		MEMPOOL_MEMBER_POOL(5)
		MEMPOOL_MEMBER_POOL(6)
		MEMPOOL_MEMBER_POOL(7)
		MEMPOOL_MEMBER_POOL(8)
		MEMPOOL_MEMBER_POOL(9)
		MEMPOOL_MEMBER_POOL(10)
	)
};

#endif
