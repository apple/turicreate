#include "JSONMemory.h"

#ifdef JSON_MEMORY_MANAGE
    #include "JSONNode.h"
    void auto_expand::purge(void) json_nothrow {
	   for(JSON_MAP(void *, void *)::iterator i = mymap.begin(), en = mymap.end(); i != en; ++i){
		  #if defined(JSON_DEBUG) || defined(JSON_SAFE)
			 void * temp = (void*)i -> first;  //because its pass by reference
			 libjson_free<void>(temp);
		  #else
			 libjson_free<void>((void*)i -> first);
		  #endif
	   }
    }

    void auto_expand_node::purge(void) json_nothrow {
	   for(JSON_MAP(void *, JSONNode *)::iterator i = mymap.begin(), en = mymap.end(); i != en; ++i){
		  JSONNode::deleteJSONNode((JSONNode *)i -> second);
	   }
    }

    #ifdef JSON_STREAM
	   #include "JSONStream.h"
	   void auto_expand_stream::purge(void) json_nothrow {
		  for(JSON_MAP(void *, JSONStream *)::iterator i = mymap.begin(), en = mymap.end(); i != en; ++i){
			 JSONStream::deleteJSONStream((JSONStream *)i -> second);
		  }
	   }
    #endif
#endif

#if defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)

#ifdef JSON_MEMORY_POOL
	#include "JSONMemoryPool.h"
    static bucket_pool_8<MEMPOOL_1, MEMPOOL_2, MEMPOOL_3, MEMPOOL_4, MEMPOOL_5, MEMPOOL_6, MEMPOOL_7, MEMPOOL_8> json_generic_mempool;
    
	//This class is only meant to initiate the mempool to start out using std::malloc/realloc/free
	class mempool_callback_setter {
    public:
        inline mempool_callback_setter(void) json_nothrow {
            mempool_callbacks::set(std::malloc, std::realloc, std::free);
        }
    private:
        inline mempool_callback_setter(const mempool_callback_setter & o){}
        inline mempool_callback_setter & operator = (const mempool_callback_setter & o){}
    };
    static mempool_callback_setter __mempoolcallbacksetter;
#endif

#include "JSONSingleton.h"

void * JSONMemory::json_malloc(size_t siz) json_nothrow {
    #ifdef JSON_MEMORY_POOL
		return json_generic_mempool.allocate(siz);
	#else
        if (json_malloc_t callback = JSONSingleton<json_malloc_t>::get()){
	       #if(defined(JSON_DEBUG) && (!defined(JSON_MEMORY_CALLBACKS))) //in debug mode without mem callback, see if the malloc was successful
		      void * result = callback(siz);
		      JSON_ASSERT(result, JSON_TEXT("Out of memory"));
		      return result;
	       #else
		      return callback(siz);
	       #endif
        }
        #if(defined(JSON_DEBUG) && (!defined(JSON_MEMORY_CALLBACKS))) //in debug mode without mem callback, see if the malloc was successful
	       void * result = std::malloc(siz);
	       JSON_ASSERT(result, JSON_TEXT("Out of memory"));
	       return result;
        #else
	       return std::malloc(siz);
        #endif
    #endif
}

void JSONMemory::json_free(void * ptr) json_nothrow {
    #ifdef JSON_MEMORY_POOL
		json_generic_mempool.deallocate(ptr);
	#else
        if (json_free_t callback = JSONSingleton<json_free_t>::get()){
	       callback(ptr);
        } else {
	       std::free(ptr);
        }
    #endif
}

void * JSONMemory::json_realloc(void * ptr, size_t siz) json_nothrow {
    #ifdef JSON_MEMORY_POOL
	    return json_generic_mempool.reallocate(ptr, siz);
    #else
        if (json_realloc_t callback = JSONSingleton<json_realloc_t>::get()){
        #if(defined(JSON_DEBUG) && (!defined(JSON_MEMORY_CALLBACKS))) //in debug mode without mem callback, see if the malloc was successful
	          void * result = callback(ptr, siz);
	          JSON_ASSERT(result, JSON_TEXT("Out of memory"));
	          return result;
           #else
	          return callback(ptr, siz);
           #endif
        }
        #if(defined(JSON_DEBUG) && (!defined(JSON_MEMORY_CALLBACKS))) //in debug mode without mem callback, see if the malloc was successful
           void * result = std::realloc(ptr, siz);
           JSON_ASSERT(result, JSON_TEXT("Out of memory"));
           return result;
        #else
           return std::realloc(ptr, siz);
        #endif
    #endif
}

#ifdef JSON_MEMORY_POOL
    //it is okay to pass null to these callbacks, no make sure they function exists
    static void * malloc_proxy(size_t siz) json_nothrow {
       if (json_malloc_t callback = JSONSingleton<json_malloc_t>::get()){
	       return callback(siz);
       }
       return std::malloc(siz);
    }

    static void * realloc_proxy(void * ptr, size_t siz) json_nothrow {
       if (json_realloc_t callback = JSONSingleton<json_realloc_t>::get()){
          return callback(ptr, siz);
       }
       return std::realloc(ptr, siz);
    }

    static void free_proxy(void * ptr){
        if (json_free_t callback = JSONSingleton<json_free_t>::get()){
	       callback(ptr);
        } else {
	       std::free(ptr);
        }
    }
#endif


void JSONMemory::registerMemoryCallbacks(json_malloc_t mal, json_realloc_t real, json_free_t fre) json_nothrow {
    JSONSingleton<json_malloc_t>::set(mal);
    JSONSingleton<json_realloc_t>::set(real);
    JSONSingleton<json_free_t>::set(fre);
    #ifdef JSON_MEMORY_POOL
        mempool_callbacks::set(malloc_proxy, realloc_proxy, free_proxy);
    #endif
}


#endif
