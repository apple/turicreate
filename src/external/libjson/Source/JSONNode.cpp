#include "JSONNode.h"

#ifdef JSON_UNIT_TEST
    int allocCount(0);
    int deallocCount(0);
    int internalAllocCount(0);
    int internalDeallocCount(0);
    int childrenAllocCount(0);
    int childrenDeallocCount(0);
    int JSONNode::getNodeAllocationCount(void){ return allocCount; }
    int JSONNode::getNodeDeallocationCount(void){ return deallocCount; }
    int JSONNode::getInternalAllocationCount(void){ return internalAllocCount; }
    int JSONNode::getInternalDeallocationCount(void){ return internalDeallocCount; }
    int JSONNode::getChildrenAllocationCount(void){ return childrenAllocCount; }
    int JSONNode::getChildrenDeallocationCount(void){ return childrenDeallocCount; }
    void JSONNode::incAllocCount(void){ ++allocCount; }
    void JSONNode::decAllocCount(void){ ++deallocCount; }
    void JSONNode::incinternalAllocCount(void){ ++internalAllocCount; }
    void JSONNode::decinternalAllocCount(void){ ++internalDeallocCount; }
    void JSONNode::incChildrenAllocCount(void){ ++childrenAllocCount; }
    void JSONNode::decChildrenAllocCount(void){ ++childrenDeallocCount; }
#endif

#define IMPLEMENT_CTOR(type)\
    JSONNode::JSONNode(const json_string & name_t, type value_t) json_nothrow : internal(internalJSONNode::newInternal()){\
	   internal -> Set(value_t);\
	   internal -> setname(name_t);\
	   incAllocCount();\
    }
IMPLEMENT_FOR_ALL_TYPES(IMPLEMENT_CTOR)

#ifndef JSON_LIBRARY
    JSONNode::JSONNode(const json_string & name_t, const json_char * value_t) json_nothrow : internal(internalJSONNode::newInternal()){
	   internal -> Set(json_string(value_t));
	   internal -> setname(name_t);
	   incAllocCount();
    }
#endif

#if (defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY))
    #include "JSONWorker.h"
    JSONNode JSONNode::stringType(const json_string & str){
        JSONNode res;
        res.set_name(json_global(EMPTY_JSON_STRING));
        #ifdef JSON_LESS_MEMORY
            res = JSONWorker::FixString(str, res.internal, false);
        #else
            res = JSONWorker::FixString(str, res.internal -> _string_encoded);
        #endif
        return res;
    }

    void JSONNode::set_name_(const json_string & newname) json_nothrow {
        #ifdef JSON_LESS_MEMORY
            json_string _newname = JSONWorker::FixString(newname, internal, true);
        #else
            json_string _newname = JSONWorker::FixString(newname, internal -> _name_encoded);
        #endif
        set_name(_newname);
    }
#endif

#ifdef JSON_CASTABLE
    JSONNode JSONNode::as_node(void) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   if (type() == JSON_NODE){
		  return *this;
	   } else if (type() == JSON_ARRAY){
		  JSONNode res(duplicate());
		  res.internal -> _type = JSON_NODE;
		  return res;
	   }
	   #ifdef JSON_MUTEX_CALLBACKS
		  if (internal -> mylock != 0){
			 JSONNode res(JSON_NODE);
			 res.set_mutex(internal -> mylock);
			 return res;
		  }
	   #endif
	   return JSONNode(JSON_NODE);
    }

    JSONNode JSONNode::as_array(void) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   if (type() == JSON_ARRAY){
		  return *this;
	   } else if (type() == JSON_NODE){
		  JSONNode res(duplicate());
		  res.internal -> _type = JSON_ARRAY;
		  json_foreach(res.internal -> CHILDREN, runner){
			 (*runner) -> clear_name();
		  }
		  return res;
	   }
	   #ifdef JSON_MUTEX_CALLBACKS
		  if (internal -> mylock != 0){
			 JSONNode res(JSON_ARRAY);
			 res.set_mutex(internal -> mylock);
			 return res;
		  }
	   #endif
	   return JSONNode(JSON_ARRAY);
    }

    void JSONNode::cast(char newtype) json_nothrow {
	   JSON_CHECK_INTERNAL();
	   if (newtype == type()) return;

	   switch(newtype){
		  case JSON_NULL:
			 nullify();
			 return;
		  case JSON_STRING:
			 *this = as_string();
			 return;
		  case JSON_NUMBER:
			 *this = as_float();
			 return;
		  case JSON_BOOL:
			 *this = as_bool();
			 return;
		  case JSON_ARRAY:
			 *this = as_array();
			 return;
		  case JSON_NODE:
			 *this = as_node();
			 return;
	   }
	   JSON_FAIL(JSON_TEXT("cast to unknown type"));
    }
#endif

//different just to supress the warning
#ifdef JSON_REF_COUNT
void JSONNode::merge(JSONNode & other) json_nothrow {
#else
void JSONNode::merge(JSONNode &) json_nothrow {
#endif
    JSON_CHECK_INTERNAL();
    #ifdef JSON_REF_COUNT
	   if (internal == other.internal) return;
	   JSON_ASSERT(*this == other, JSON_TEXT("merging two nodes that aren't equal"));
	   if (internal -> refcount < other.internal -> refcount){
		  *this = other;
	   } else {
		  other = *this;
	   }
    #endif
}

#ifdef JSON_REF_COUNT
    void JSONNode::merge(JSONNode * other) json_nothrow {
	   JSON_CHECK_INTERNAL();
	   if (internal == other -> internal) return;
	   *other = *this;
    }

    //different just to supress the warning
    void JSONNode::merge(unsigned int num, ...) json_nothrow {
#else
    void JSONNode::merge(unsigned int, ...) json_nothrow {
#endif
    JSON_CHECK_INTERNAL();
    #ifdef JSON_REF_COUNT
	   va_list args;
	   va_start(args, num);
	   for(unsigned int i = 0; i < num; ++i){
		  merge(va_arg(args, JSONNode*));
	   }
	   va_end(args);
    #endif
}

JSONNode JSONNode::duplicate(void) const json_nothrow {
    JSON_CHECK_INTERNAL();
    JSONNode mycopy(*this);
    #ifdef JSON_REF_COUNT
	   JSON_ASSERT(internal == mycopy.internal, JSON_TEXT("copy ctor failed to ref count correctly"));
	   mycopy.makeUniqueInternal();
    #endif
    JSON_ASSERT(internal != mycopy.internal, JSON_TEXT("makeUniqueInternal failed"));
    return mycopy;
}

JSONNode & JSONNode::at(json_index_t pos) json_throws(std::out_of_range) {
    JSON_CHECK_INTERNAL();
    if (json_unlikely(pos >= internal -> size())){
	   JSON_FAIL(JSON_TEXT("at() out of bounds"));
	   json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
    }
    return (*this)[pos];
}

const JSONNode & JSONNode::at(json_index_t pos) const json_throws(std::out_of_range) {
    JSON_CHECK_INTERNAL();
    if (json_unlikely(pos >= internal -> size())){
	   JSON_FAIL(JSON_TEXT("at() const out of bounds"));
	   json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
    }
    return (*this)[pos];
}

JSONNode & JSONNode::operator[](json_index_t pos) json_nothrow {
    JSON_CHECK_INTERNAL();
    JSON_ASSERT(pos < internal -> size(), JSON_TEXT("[] out of bounds"));
    makeUniqueInternal();
    return *(internal -> at(pos));
}

const JSONNode & JSONNode::operator[](json_index_t pos) const json_nothrow {
    JSON_CHECK_INTERNAL();
    JSON_ASSERT(pos < internal -> size(), JSON_TEXT("[] const out of bounds"));
    return *(internal -> at(pos));
}

JSONNode & JSONNode::at(const json_string & name_t) json_throws(std::out_of_range) {
    JSON_CHECK_INTERNAL();
    JSON_ASSERT(type() == JSON_NODE, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("at"));
    makeUniqueInternal();
    if (JSONNode ** res = internal -> at(name_t)){
	   return *(*res);
    }
    JSON_FAIL(json_string(JSON_TEXT("at could not find child by name: ")) + name_t);
    json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
}

const JSONNode & JSONNode::at(const json_string & name_t) const json_throws(std::out_of_range) {
    JSON_CHECK_INTERNAL();
    JSON_ASSERT(type() == JSON_NODE, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("at"));
    if (JSONNode ** res = internal -> at(name_t)){
	   return *(*res);
    }
    JSON_FAIL(json_string(JSON_TEXT("at const could not find child by name: ")) + name_t);
    json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
}

#ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
    JSONNode & JSONNode::at_nocase(const json_string & name_t) json_throws(std::out_of_range) {
	   JSON_CHECK_INTERNAL();
	   JSON_ASSERT(type() == JSON_NODE, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("at_nocase"));
	   makeUniqueInternal();
	   if (JSONNode ** res = internal -> at_nocase(name_t)){
		  return *(*res);
	   }
	   JSON_FAIL(json_string(JSON_TEXT("at_nocase could not find child by name: ")) + name_t);
	   json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
    }

    const JSONNode & JSONNode::at_nocase(const json_string & name_t) const json_throws(std::out_of_range) {
	   JSON_CHECK_INTERNAL();
	   JSON_ASSERT(type() == JSON_NODE, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("at_nocase"));
	   if (JSONNode ** res = internal -> at_nocase(name_t)){
		  return *(*res);
	   }
	   JSON_FAIL(json_string(JSON_TEXT("at_nocase const could not find child by name: ")) + name_t);
	   json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
    }
#endif

#ifndef JSON_LIBRARY
    struct auto_delete {
	   public:
		  auto_delete(JSONNode * node) json_nothrow : mynode(node){};
		  ~auto_delete(void) json_nothrow { JSONNode::deleteJSONNode(mynode); };
		  JSONNode * mynode;
	   private:
		  auto_delete(const auto_delete &);
		  auto_delete & operator = (const auto_delete &);
    };
#endif

JSONNode JSON_PTR_LIB JSONNode::pop_back(json_index_t pos) json_throws(std::out_of_range) {
    JSON_CHECK_INTERNAL();
    if (json_unlikely(pos >= internal -> size())){
	   JSON_FAIL(JSON_TEXT("pop_back out of bounds"));
	   json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
    }
    makeUniqueInternal();
    #ifdef JSON_LIBRARY
	   return internal -> pop_back(pos);
    #else
	   auto_delete temp(internal -> pop_back(pos));
	   return *temp.mynode;
    #endif
}

JSONNode JSON_PTR_LIB JSONNode::pop_back(const json_string & name_t) json_throws(std::out_of_range) {
    JSON_CHECK_INTERNAL();
    JSON_ASSERT(type() == JSON_NODE, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("pop_back"));
    #ifdef JSON_LIBRARY
	   return internal -> pop_back(name_t);
    #else
	   if (JSONNode * res = internal -> pop_back(name_t)){
		  auto_delete temp(res);
		  return *(temp.mynode);
	   }
	   JSON_FAIL(json_string(JSON_TEXT("pop_back const could not find child by name: ")) + name_t);
	   json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
    #endif
}

#ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
    JSONNode JSON_PTR_LIB JSONNode::pop_back_nocase(const json_string & name_t) json_throws(std::out_of_range) {
	   JSON_CHECK_INTERNAL();
	   JSON_ASSERT(type() == JSON_NODE, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("pop_back_no_case"));
	   #ifdef JSON_LIBRARY
		  return internal -> pop_back_nocase(name_t);
	   #else
		  if (JSONNode * res = internal -> pop_back_nocase(name_t)){
			 auto_delete temp(res);
			 return *(temp.mynode);
		  }
		  JSON_FAIL(json_string(JSON_TEXT("pop_back_nocase could not find child by name: ")) + name_t);
		  json_throw(std::out_of_range(json_global(EMPTY_STD_STRING)));
	   #endif
    }
#endif
		
#ifdef JSON_MEMORY_POOL
	#include "JSONMemoryPool.h"
	memory_pool<NODEPOOL> json_node_mempool;
#endif
		
void JSONNode::deleteJSONNode(JSONNode * ptr) json_nothrow {
	#ifdef JSON_MEMORY_POOL
		ptr -> ~JSONNode();
		json_node_mempool.deallocate((void*)ptr);
	#elif defined(JSON_MEMORY_CALLBACKS)
		ptr -> ~JSONNode();
		libjson_free<JSONNode>(ptr);
	#else
		delete ptr;
	#endif
}
		
inline JSONNode * _newJSONNode(const JSONNode & orig) {
	#ifdef JSON_MEMORY_POOL
		return new((JSONNode*)json_node_mempool.allocate()) JSONNode(orig);
	#elif defined(JSON_MEMORY_CALLBACKS)
		return new(json_malloc<JSONNode>(1)) JSONNode(orig);
	#else
		return new JSONNode(orig);
	#endif
}
		
JSONNode * JSONNode::newJSONNode(const JSONNode & orig    JSON_MUTEX_COPY_DECL) {
	#ifdef JSON_MUTEX_CALLBACKS
		if (parentMutex != 0){
			JSONNode * temp = _newJSONNode(orig);
			temp -> set_mutex(parentMutex);
			return temp;
		}
	#endif
	return _newJSONNode(orig);
}
		
JSONNode * JSONNode::newJSONNode(internalJSONNode * internal_t) {
	#ifdef JSON_MEMORY_POOL
		return new((JSONNode*)json_node_mempool.allocate()) JSONNode(internal_t);
	#elif defined(JSON_MEMORY_CALLBACKS)
		return new(json_malloc<JSONNode>(1)) JSONNode(internal_t);
	#else
		return new JSONNode(internal_t);
	#endif
}
		
JSONNode * JSONNode::newJSONNode_Shallow(const JSONNode & orig) {
	#ifdef JSON_MEMORY_POOL
		return new((JSONNode*)json_node_mempool.allocate()) JSONNode(true, const_cast<JSONNode &>(orig));
	#elif defined(JSON_MEMORY_CALLBACKS)
		return new(json_malloc<JSONNode>(1)) JSONNode(true, const_cast<JSONNode &>(orig));
	#else
		return new JSONNode(true, const_cast<JSONNode &>(orig));
	#endif
}
		

