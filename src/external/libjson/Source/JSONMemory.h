#ifndef JSON_MEMORY_H
#define JSON_MEMORY_H

#include <cstdlib> //for malloc, realloc, and free
#include <cstring> //for memmove
#include "../JSONOptions.h"
#include "JSONDebug.h"

#if defined(JSON_DEBUG) || defined(JSON_SAFE)
    #define JSON_FREE_PASSTYPE &
#else
    #define JSON_FREE_PASSTYPE
#endif

#if defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
    class JSONMemory {
    public:
		  static void * json_malloc(size_t siz) json_malloc_attr;
	      static void * json_realloc(void * ptr, size_t siz) json_malloc_attr;
	   static void json_free(void * ptr) json_nothrow;
	   static void registerMemoryCallbacks(json_malloc_t mal, json_realloc_t real, json_free_t fre) json_nothrow json_cold;
    private:
        JSONMemory(void);
    };

    template <typename T> static inline T * json_malloc(size_t count) json_malloc_attr;
    template <typename T> static inline T * json_malloc(size_t count) json_nothrow {
	   return (T *)JSONMemory::json_malloc(sizeof(T) * count);
    }

    template <typename T> static inline T * json_realloc(T * ptr, size_t count) json_malloc_attr;
    template <typename T> static inline T * json_realloc(T * ptr, size_t count) json_nothrow {
       return (T *)JSONMemory::json_realloc(ptr, sizeof(T) * count);
    }

    template <typename T> static inline void libjson_free(T * JSON_FREE_PASSTYPE ptr) json_nothrow {
	   JSONMemory::json_free(ptr);
	   #if defined(JSON_DEBUG) || defined(JSON_SAFE)  //in debug or safe mode, set the pointer to 0 so that it can't be used again
		  ptr = 0;
	   #endif
    }
#else

    template <typename T> static inline T * json_malloc(size_t count) json_malloc_attr;
    template <typename T> static inline T * json_malloc(size_t count) json_nothrow {
	   #ifdef JSON_DEBUG  //in debug mode, see if the malloc was successful
		  void * result = std::malloc(count * sizeof(T));
		  JSON_ASSERT(result != 0, JSON_TEXT("Out of memory"));
		  #ifdef JSON_NULL_MEMORY
			 std::memset(result, '\0', count  * sizeof(T));
		  #endif
		  return (T *)result;
	   #else
		  return (T *)std::malloc(count * sizeof(T));
	   #endif
    }

    template <typename T> static inline void libjson_free(T * JSON_FREE_PASSTYPE ptr) json_nothrow {
	   std::free(ptr);
	   #if defined(JSON_DEBUG) || defined(JSON_SAFE)  //in debug or safe mode, set the pointer to 0 so that it can't be used again
		  ptr = 0;
	   #endif
    }

    template <typename T> static inline T * json_realloc(T * ptr, size_t count) json_malloc_attr;
    template <typename T> static inline T * json_realloc(T * ptr, size_t count) json_nothrow {
	   #ifdef JSON_DEBUG  //in debug mode, check the results of realloc to be sure it was successful
		  void * result = std::realloc(ptr, count * sizeof(T));
		  JSON_ASSERT(result != 0, JSON_TEXT("Out of memory"));
		  return (T *)result;
	   #else
		  return (T *)std::realloc(ptr, count * sizeof(T));
	   #endif
    }
#endif

#ifdef JSON_MEMORY_MANAGE
    #include <map>
    class JSONNode;
    struct auto_expand {
    public:
	   auto_expand(void) json_nothrow : mymap(){}
	   ~auto_expand(void) json_nothrow { purge(); }
	   void purge(void) json_nothrow;
	   inline void clear(void) json_nothrow { purge(); mymap.clear(); }
	   inline void * insert(void * ptr) json_nothrow { mymap[ptr] = ptr; return ptr; }
	   inline void remove(void * ptr) json_nothrow {
		  JSON_MAP(void *, void *)::iterator i = mymap.find(ptr);
		  JSON_ASSERT(i != mymap.end(), JSON_TEXT("Removing a non-managed item"));
		  mymap.erase(i);
	   }
	   JSON_MAP(void *, void *) mymap;
    private:
        auto_expand(const auto_expand &);
        auto_expand & operator = (const auto_expand &);
    };

    struct auto_expand_node {
    public:
	   auto_expand_node(void) json_nothrow : mymap(){}
	   ~auto_expand_node(void) json_nothrow { purge(); }
	   void purge(void) json_nothrow ;
	   inline void clear(void) json_nothrow { purge(); mymap.clear(); }
	   inline JSONNode * insert(JSONNode * ptr) json_nothrow { mymap[ptr] = ptr; return ptr; }
	   inline void remove(void * ptr) json_nothrow {
		  JSON_MAP(void *, JSONNode *)::iterator i = mymap.find(ptr);
		  if(json_likely(i != mymap.end())) mymap.erase(i);
	   }
	   JSON_MAP(void *, JSONNode *) mymap;
    private:
        auto_expand_node(const auto_expand_node &);
        auto_expand_node & operator = (const auto_expand_node &);
    };

    #ifdef JSON_STREAM
	   class JSONStream;
	   struct auto_expand_stream {
        public:
		  auto_expand_stream(void) json_nothrow : mymap(){}
		  ~auto_expand_stream(void) json_nothrow { purge(); }
		  void purge(void) json_nothrow ;
		  inline void clear(void) json_nothrow { purge(); mymap.clear(); }
		  inline JSONStream * insert(JSONStream * ptr) json_nothrow { mymap[ptr] = ptr; return ptr; }
		  inline void remove(void * ptr) json_nothrow {
			 JSON_MAP(void *, JSONStream *)::iterator i = mymap.find(ptr);
			 if(json_likely(i != mymap.end())) mymap.erase(i);
		  }
		  JSON_MAP(void *, JSONStream *) mymap;
        private:
            auto_expand_stream(const auto_expand_stream &);
            auto_expand_stream & operator = (const auto_expand_stream &);
	   };
    #endif
#endif

//The C++ way, use an self-deleting pointer and let the optimizer decide when it gets destroyed
template <typename T>
class json_auto {
    public:
	   json_auto(void) json_nothrow : ptr(0){}
	   json_auto(size_t count) json_nothrow : ptr(json_malloc<T>(count)){}
	   json_auto(T * arg) json_nothrow : ptr(arg){}
	   ~json_auto(void) json_nothrow {
		  libjson_free<T>(ptr);
	   }
	   inline void set(T * p) json_nothrow{
		  ptr = p;
	   }
	   T * ptr;
    private:
	   json_auto(const json_auto &);
	   json_auto & operator =(const json_auto &);
};

//Clears a string, if required, frees the memory
static inline void clearString(json_string & str) json_nothrow {
    #ifdef JSON_LESS_MEMORY
	   json_string().swap(str);
    #else
	   str.clear();
    #endif
}

//Shrinks a string
static inline void shrinkString(json_string & str) json_nothrow {
    #ifdef JSON_LESS_MEMORY
        if (str.capacity() != str.length()) str = json_string(str.begin(), str.end());
    #endif
}

#endif
