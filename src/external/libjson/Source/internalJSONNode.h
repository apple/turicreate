#ifndef INTERNAL_JSONNODE_H
#define INTERNAL_JSONNODE_H

#include "JSONDebug.h"
#include "JSONChildren.h"
#include "JSONMemory.h"
#include "JSONGlobals.h"
#ifdef JSON_DEBUG
    #include <climits>  //to check int value
#endif
#include "JSONSharedString.h"

#ifdef JSON_LESS_MEMORY
    #ifdef __GNUC__
	   #pragma pack(push, 1)
    #elif _MSC_VER
	   #pragma pack(push, internalJSONNode_pack, 1)
    #endif
#endif

/*
    This class is the work horse of libjson, it handles all of the
    functinality of JSONNode.  This object is reference counted for
    speed and memory reasons.

    If JSON_REF_COUNT is not on, this internal structure still has an important
    purpose, as it can be passed around by JSONNoders that are flagged as temporary
*/

class JSONNode;  //forward declaration

#ifndef JSON_LIBRARY
    #define DECL_SET_INTEGER(type) void Set(type) json_nothrow json_write_priority; void Set(unsigned type) json_nothrow json_write_priority;
    #define DECL_CAST_OP(type) operator type() const json_nothrow; operator unsigned type() const json_nothrow;
#endif

#ifdef JSON_MUTEX_CALLBACKS
    #define initializeMutex(x) ,mylock(x)
#else
    #define initializeMutex(x)
#endif

#if defined(JSON_PREPARSE) || !defined(JSON_READ_PRIORITY)
    #define SetFetched(b) (void)0
    #define Fetch() (void)0
    #define initializeFetch(x)
#else
    #define initializeFetch(x) ,fetched(x)
#endif

#ifdef JSON_REF_COUNT
    #define initializeRefCount(x) ,refcount(x)
#else
    #define initializeRefCount(x)
#endif

#ifdef JSON_COMMENTS
    #define initializeComment(x) ,_comment(x)
#else
    #define initializeComment(x)
#endif

#ifndef JSON_UNIT_TEST
    #define incAllocCount() (void)0
    #define decAllocCount() (void)0
    #define incinternalAllocCount() (void)0
    #define decinternalAllocCount() (void)0
#endif

#ifdef JSON_LESS_MEMORY
    #define CHILDREN _value.Children
    #define DELETE_CHILDREN()\
	   if (isContainer()){\
		  jsonChildren::deleteChildren(CHILDREN);\
	   }
    #define CHILDREN_TO_NULL() (void)0
    #define initializeChildren(x)
#else
    #define CHILDREN Children
    #define DELETE_CHILDREN()\
	   if (CHILDREN != 0) jsonChildren::deleteChildren(CHILDREN);
    #define CHILDREN_TO_NULL() CHILDREN = 0
    #define makeNotContainer() (void)0
    #define makeContainer() if (!CHILDREN) CHILDREN = jsonChildren::newChildren()
    #define initializeChildren(x) ,CHILDREN(x)
#endif

class internalJSONNode {
public:
    internalJSONNode(char mytype = JSON_NULL) json_nothrow json_hot;
    #ifdef JSON_READ_PRIORITY
	   internalJSONNode(const json_string & unparsed) json_nothrow json_hot;
	   internalJSONNode(const json_string & name_t, const json_string & value_t) json_nothrow json_read_priority;
    #endif
    internalJSONNode(const internalJSONNode & orig) json_nothrow json_hot;
    internalJSONNode & operator = (const internalJSONNode &) json_nothrow json_hot;
    ~internalJSONNode(void) json_nothrow json_hot;

    static internalJSONNode * newInternal(char mytype = JSON_NULL) json_hot;
    #ifdef JSON_READ_PRIORITY
	   static internalJSONNode * newInternal(const json_string & unparsed) json_hot;
	   static internalJSONNode * newInternal(const json_string & name_t, const json_string & value_t) json_hot;
    #endif
    static internalJSONNode * newInternal(const internalJSONNode & orig) json_hot;  //not copyable, only by this class
    static void deleteInternal(internalJSONNode * ptr) json_nothrow json_hot;

    json_index_t size(void) const json_nothrow json_read_priority;
    bool empty(void) const json_nothrow;
    unsigned char type(void) const json_nothrow json_read_priority;

    json_string name(void) const json_nothrow json_read_priority;
    void setname(const json_string & newname) json_nothrow json_write_priority;
    #ifdef JSON_COMMENTS
	   void setcomment(const json_string & comment) json_nothrow;
	   json_string getcomment(void) const json_nothrow;
    #endif

    #if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
	   void preparse(void) json_nothrow;
    #endif

    #ifdef JSON_LIBRARY
	   void push_back(JSONNode * node) json_nothrow;
    #else
	   void push_back(const JSONNode & node) json_nothrow;
    #endif
    void reserve(json_index_t siz) json_nothrow;
    void push_front(const JSONNode & node) json_nothrow;
    JSONNode * pop_back(json_index_t pos) json_nothrow;
    JSONNode * pop_back(const json_string & name_t) json_nothrow;
    #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
	   JSONNode * pop_back_nocase(const json_string & name_t) json_nothrow;
    #endif

    JSONNode * at(json_index_t pos) json_nothrow;
    //These return ** because pop_back needs them
    JSONNode ** at(const json_string & name_t) json_nothrow;
    #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
	   JSONNode ** at_nocase(const json_string & name_t) json_nothrow;
    #endif

    void Set(const json_string & val) json_nothrow json_write_priority;
    #ifdef JSON_LIBRARY
	   void Set(json_number val) json_nothrow json_write_priority;
	   void Set(json_int_t val) json_nothrow json_write_priority;
	   operator json_int_t() const json_nothrow;
	   operator json_number() const json_nothrow;
    #else
	   DECL_SET_INTEGER(char)
	   DECL_SET_INTEGER(short)
	   DECL_SET_INTEGER(int)
	   DECL_SET_INTEGER(long)
	   #ifndef JSON_ISO_STRICT
		  DECL_SET_INTEGER(long long)
		  void Set(long double val) json_nothrow json_write_priority;
	   #endif
	   void Set(float val) json_nothrow json_write_priority;
	   void Set(double val) json_nothrow json_write_priority;


	   DECL_CAST_OP(char)
	   DECL_CAST_OP(short)
	   DECL_CAST_OP(int)
	   DECL_CAST_OP(long)
	   #ifndef JSON_ISO_STRICT
		  DECL_CAST_OP(long long)
		  operator long double() const json_nothrow;
	   #endif
	   operator float() const json_nothrow;
	   operator double() const json_nothrow;
    #endif
    operator json_string()const json_nothrow;
    operator bool() const json_nothrow;
    void Set(bool val) json_nothrow;

    bool IsEqualTo(const json_string & val) const json_nothrow;
    bool IsEqualTo(bool val) const json_nothrow;
    bool IsEqualTo(const internalJSONNode * val) const json_nothrow;

    template<typename T>
    bool IsEqualToNum(T val) const json_nothrow;

    internalJSONNode * incRef(void) json_nothrow;
    #ifdef JSON_REF_COUNT
	   void decRef(void) json_nothrow json_hot;
	   bool hasNoReferences(void) json_nothrow json_hot;
    #endif
    internalJSONNode * makeUnique(void) json_nothrow json_hot;

    JSONNode ** begin(void) const json_nothrow;
    JSONNode ** end(void) const json_nothrow;
    bool Fetched(void) const json_nothrow json_hot;
    #ifdef JSON_MUTEX_CALLBACKS
	   void _set_mutex(void * mutex, bool unset = true) json_nothrow json_cold;
	   void _unset_mutex(void) json_nothrow json_cold;
    #endif
    #ifdef JSON_UNIT_TEST
	   static void incinternalAllocCount(void) json_nothrow;
	   static void decinternalAllocCount(void) json_nothrow;
    #endif

    #ifdef JSON_WRITE_PRIORITY
		void DumpRawString(json_string & output) const json_nothrow json_write_priority;
	   void WriteName(bool formatted, bool arrayChild, json_string & output) const json_nothrow json_write_priority;
	   #ifdef JSON_ARRAY_SIZE_ON_ONE_LINE
		  void WriteChildrenOneLine(unsigned int indent, json_string & output) const json_nothrow json_write_priority;
	   #endif
	   void WriteChildren(unsigned int indent, json_string & output) const json_nothrow json_write_priority;
	   void WriteComment(unsigned int indent, json_string & output) const json_nothrow json_write_priority;
	   void Write(unsigned int indent, bool arrayChild, json_string & output) const json_nothrow json_write_priority;
    #endif


    inline bool isContainer(void) const json_nothrow {
	   return (_type == JSON_NODE || _type == JSON_ARRAY);
    }
    inline bool isNotContainer(void) const json_nothrow {
	   return (_type != JSON_NODE && _type != JSON_ARRAY);
    }

    #ifdef JSON_LESS_MEMORY
	   inline void makeNotContainer(void){
		  if (isContainer()){
			 jsonChildren::deleteChildren(CHILDREN);
		  }
	   }
	   inline void makeContainer(void){
		  if (isNotContainer()){
			 CHILDREN = jsonChildren::newChildren();
		  }
	   }
    #endif

    void Nullify(void) const json_nothrow;

    #if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
	   void SetFetched(bool val) const json_nothrow json_hot;
	   void Fetch(void) const json_nothrow json_hot;  //it's const because it doesn't change the VALUE of the function
    #endif

    #ifdef JSON_READ_PRIORITY
	   void FetchString(void) const json_nothrow json_read_priority;
	   void FetchNode(void) const json_nothrow json_read_priority;
	   void FetchArray(void) const json_nothrow json_read_priority;
    #endif
    void FetchNumber(void) const json_nothrow json_read_priority;

    #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
	   static bool AreEqualNoCase(const json_char * ch_one, const json_char * ch_two) json_nothrow json_read_priority;
    #endif

    inline void clearname(void) json_nothrow {
	   clearString(_name);
    }

    #ifdef JSON_DEBUG
	   #ifndef JSON_LIBRARY
		  JSONNode Dump(size_t & totalmemory) const json_nothrow;
		  JSONNode DumpMutex(void) const json_nothrow;
	   #endif
    #endif


    mutable unsigned char _type BITS(3);

    json_string _name;
    mutable bool _name_encoded BITS(1);  //must be above name due to initialization list order

    mutable json_string _string;   //these are both mutable because the string can change when it's fetched
    mutable bool _string_encoded BITS(1);

    //the value of the json
    union value_union_t {
	   bool _bool BITS(1);
	   json_number _number;
	   #ifdef JSON_LESS_MEMORY
		  jsonChildren * Children;
	   #endif
    };
    mutable value_union_t _value; //internal structure changes depending on type

    #ifdef JSON_MUTEX_CALLBACKS
	   void * mylock;
    #endif

    #ifdef JSON_REF_COUNT
	   size_t refcount PACKED(20);
    #endif

    #if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
	   mutable bool fetched BITS(1);
    #endif

    #ifdef JSON_COMMENTS
	   json_string _comment;
    #endif

    #ifndef JSON_LESS_MEMORY
	   jsonChildren * CHILDREN;
    #endif
};

inline internalJSONNode::internalJSONNode(char mytype) json_nothrow : _type(mytype), _name(), _name_encoded(), _string(), _string_encoded(), _value()
    initializeMutex(0)
    initializeRefCount(1)
    initializeFetch(true)
    initializeComment(json_global(EMPTY_JSON_STRING))
    initializeChildren((_type == JSON_NODE || _type == JSON_ARRAY) ? jsonChildren::newChildren() : 0){

    incinternalAllocCount();

    #ifdef JSON_LESS_MEMORY
	   //if not less memory, its in the initialization list
	   if (isContainer()){
		  CHILDREN = jsonChildren::newChildren();
	   }
    #endif
}

inline internalJSONNode * internalJSONNode::incRef(void) json_nothrow {
    #ifdef JSON_REF_COUNT
	   ++refcount;
	   return this;
    #else
	   return makeUnique();
    #endif
}

inline json_index_t internalJSONNode::size(void) const json_nothrow {
    if (isNotContainer()) return 0;
    Fetch();
    return CHILDREN -> size();
}

inline bool internalJSONNode::empty(void) const json_nothrow {
    if (isNotContainer()) return true;
    Fetch();
    return CHILDREN -> empty();
}

inline unsigned char internalJSONNode::type(void) const json_nothrow {
    return _type;
}

inline json_string internalJSONNode::name(void) const json_nothrow {
    return _name;
}

inline void internalJSONNode::setname(const json_string & newname) json_nothrow {
    #ifdef JSON_LESS_MEMORY
	   JSON_ASSERT(newname.capacity() == newname.length(), JSON_TEXT("name object too large"));
    #endif
    _name = newname;
    _name_encoded = true;
}

#ifdef JSON_COMMENTS
    inline void internalJSONNode::setcomment(const json_string & comment) json_nothrow {
	   _comment = comment;
    }

    inline json_string internalJSONNode::getcomment(void) const json_nothrow {
	   return _comment;
    }
#endif

inline bool internalJSONNode::IsEqualTo(const json_string & val) const json_nothrow {
    if (type() != JSON_STRING) return false;
    Fetch();
    return _string == val;
}

inline bool internalJSONNode::IsEqualTo(bool val) const json_nothrow {
    if (type() != JSON_BOOL) return false;
    Fetch();
    return val == _value._bool;
}

template<typename T>
inline bool internalJSONNode::IsEqualToNum(T val) const json_nothrow {
    if (type() != JSON_NUMBER) return false;
    Fetch();
    return (json_number)val == _value._number;
}

#ifdef JSON_REF_COUNT
    inline void internalJSONNode::decRef(void) json_nothrow {
	   JSON_ASSERT(refcount != 0, JSON_TEXT("decRef on a 0 refcount internal"));
	   --refcount;
    }

    inline bool internalJSONNode::hasNoReferences(void) json_nothrow {
	   return refcount == 0;
    }
#endif

inline internalJSONNode * internalJSONNode::makeUnique(void) json_nothrow {
    #ifdef JSON_REF_COUNT
	   if (refcount > 1){
		  decRef();
		  return newInternal(*this);
	   }
	   JSON_ASSERT(refcount == 1, JSON_TEXT("makeUnique on a 0 refcount internal"));
	   return this;
    #else
	   return newInternal(*this);
    #endif
}

#if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
    inline void internalJSONNode::SetFetched(bool val) const json_nothrow {
	   fetched = val;
    }
#endif

inline bool internalJSONNode::Fetched(void) const json_nothrow {
    #if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
	   return fetched;
    #else
	   return true;
    #endif
}

inline JSONNode ** internalJSONNode::begin(void) const json_nothrow {
    JSON_ASSERT_SAFE(isContainer(), json_global(ERROR_NON_CONTAINER) + JSON_TEXT("begin"), return 0;);
    Fetch();
    return CHILDREN -> begin();
}

inline JSONNode ** internalJSONNode::end(void) const json_nothrow {
    JSON_ASSERT_SAFE(isContainer(), json_global(ERROR_NON_CONTAINER) + JSON_TEXT("end"), return 0;);
    Fetch();
    return CHILDREN -> end();
}

inline JSONNode * internalJSONNode::at(json_index_t pos) json_nothrow {
    JSON_ASSERT_SAFE(isContainer(), JSON_TEXT("calling at on non-container type"), return 0;);
    Fetch();
    return (*CHILDREN)[pos];
}

#if defined(JSON_LESS_MEMORY) && defined(__GNUC__)
    inline void internalJSONNode::reserve(json_index_t __attribute__((unused)) siz) json_nothrow
#else
    inline void internalJSONNode::reserve(json_index_t siz) json_nothrow
#endif
{
    JSON_ASSERT_SAFE(isContainer(), json_global(ERROR_NON_CONTAINER) + JSON_TEXT("reserve"), return;);
    Fetch();
    jsonChildren::reserve2(CHILDREN, siz);
}



/*
    cast operators
*/
#ifndef JSON_LIBRARY
    #ifdef JSON_ISO_STRICT
	   #define BASE_CONVERT_TYPE long
    #else
	   #define BASE_CONVERT_TYPE long long
    #endif

    #define IMP_SMALLER_INT_CAST_OP(_type, type_max, type_min)\
	   inline internalJSONNode::operator _type() const json_nothrow {\
		  JSON_ASSERT(_value._number > type_min, _string + json_global(ERROR_LOWER_RANGE) + JSON_TEXT(#_type));\
		  JSON_ASSERT(_value._number < type_max, _string + json_global(ERROR_UPPER_RANGE) + JSON_TEXT(#_type));\
		  JSON_ASSERT(_value._number == (json_number)((_type)(_value._number)), json_string(JSON_TEXT("(")) + json_string(JSON_TEXT(#_type)) + json_string(JSON_TEXT(") will truncate ")) + _string);\
		  return (_type)static_cast<BASE_CONVERT_TYPE>(*this);\
	   }

    IMP_SMALLER_INT_CAST_OP(char, CHAR_MAX, CHAR_MIN)
    IMP_SMALLER_INT_CAST_OP(unsigned char, UCHAR_MAX, 0)
    IMP_SMALLER_INT_CAST_OP(short, SHRT_MAX, SHRT_MIN)
    IMP_SMALLER_INT_CAST_OP(unsigned short, USHRT_MAX, 0)
    IMP_SMALLER_INT_CAST_OP(int, INT_MAX, INT_MIN)
    IMP_SMALLER_INT_CAST_OP(unsigned int, UINT_MAX, 0)

    #ifndef JSON_ISO_STRICT
	   IMP_SMALLER_INT_CAST_OP(long, LONG_MAX, LONG_MIN)
	   IMP_SMALLER_INT_CAST_OP(unsigned long, ULONG_MAX, 0)
    #endif
#endif

inline internalJSONNode::operator json_string() const json_nothrow {
    Fetch();
    return _string;
}


#ifndef JSON_LIBRARY
    #ifndef JSON_ISO_STRICT
	   inline internalJSONNode::operator float() const json_nothrow {
		  return static_cast<float>(static_cast<long double>(*this));
	   }
	   inline internalJSONNode::operator double() const json_nothrow {
		  return static_cast<double>(static_cast<long double>(*this));
	   }
    #else
	   inline internalJSONNode::operator float() const json_nothrow {
		  return static_cast<float>(static_cast<double>(*this));
	   }
    #endif
#endif

#ifdef JSON_LESS_MEMORY
    #ifdef __GNUC__
	   #pragma pack(pop)
    #elif _MSC_VER
	   #pragma pack(pop, internalJSONNode_pack,)
    #endif
#endif
#endif
