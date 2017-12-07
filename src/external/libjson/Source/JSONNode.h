#ifndef JSONNODE_H
#define JSONNODE_H

#include "JSONDebug.h"   //for string type
#include "internalJSONNode.h"  //internal structure for json value
#include <stdexcept>
#include <cstdarg>  //for the ... parameter

#ifdef JSON_BINARY
    #include "JSON_Base64.h"
#endif

#ifdef JSON_LESS_MEMORY
    #ifdef __GNUC__
	   #pragma pack(push, 1)
    #elif _MSC_VER
	   #pragma pack(push, JSONNode_pack, 1)
    #endif
#endif

#ifndef JSON_REF_COUNT
    #define makeUniqueInternal() (void)0
#endif

#define JSON_CHECK_INTERNAL() JSON_ASSERT(internal != 0, JSON_TEXT("no internal"))

#ifdef JSON_MUTEX_CALLBACKS
    #define JSON_MUTEX_COPY_DECL ,void * parentMutex
    #define JSON_MUTEX_COPY_DECL2 ,void * parentMutex = 0
#else
    #define JSON_MUTEX_COPY_DECL
    #define JSON_MUTEX_COPY_DECL2
#endif

#ifdef JSON_LIBRARY
    #define JSON_PTR_LIB *
    #define JSON_NEW(x) JSONNode::newJSONNode_Shallow(x)


    #define DECLARE_FOR_ALL_TYPES(foo)\
	   foo(json_int_t)json_nothrow;\
	   foo(json_number) json_nothrow;\
	   foo(bool) json_nothrow;\
	   foo(const json_string &) json_nothrow;

    #define DECLARE_FOR_ALL_CAST_TYPES_CONST(foo)\
	   foo(json_int_t) const json_nothrow;\
	   foo(json_number) const json_nothrow;\
	   foo(bool) const json_nothrow;\
	   foo(const json_string &) const json_nothrow;\

    #define DECLARE_FOR_ALL_TYPES_CONST(foo)\
	   DECLARE_FOR_ALL_CAST_TYPES_CONST(foo)\
	   foo(const JSONNode &) const json_nothrow;

    #define IMPLEMENT_FOR_ALL_NUMBERS(foo)\
	   foo(json_int_t)\
	   foo(json_number)


#else
    #define JSON_PTR_LIB
    #define JSON_NEW(x) x

    #ifdef JSON_ISO_STRICT
	   #define DECLARE_FOR_LONG_LONG(foo)
	   #define DECLARE_FOR_LONG_LONG_CONST(foo)
	   #define IMPLEMENT_FOR_LONG_LONG(foo)
	   #define DECLARE_FOR_LONG_DOUBLE(foo)
	   #define DECLARE_FOR_LONG_DOUBLE_CONST(foo)
	   #define IMPLEMENT_FOR_LONG_DOUBLE(foo)
    #else
	   #define DECLARE_FOR_LONG_LONG(foo) foo(long long) json_nothrow; foo(unsigned long long) json_nothrow;
	   #define DECLARE_FOR_LONG_LONG_CONST(foo) foo(long long) const json_nothrow; foo(unsigned long long) const json_nothrow;
	   #define IMPLEMENT_FOR_LONG_LONG(foo) foo(long long) foo(unsigned long long)
	   #define DECLARE_FOR_LONG_DOUBLE(foo) foo(long double) json_nothrow;
	   #define DECLARE_FOR_LONG_DOUBLE_CONST(foo) foo(long double) const json_nothrow;
	   #define IMPLEMENT_FOR_LONG_DOUBLE(foo) foo(long double)
    #endif

    #define DECLARE_FOR_ALL_TYPES(foo)\
	   foo(char) json_nothrow;	 foo(unsigned char) json_nothrow;\
	   foo(short) json_nothrow;	 foo(unsigned short) json_nothrow;\
	   foo(int) json_nothrow;	 foo(unsigned int) json_nothrow;\
	   foo(long) json_nothrow;	 foo(unsigned long) json_nothrow;\
	   foo(float) json_nothrow;	 foo(double) json_nothrow;\
	   foo(bool) json_nothrow;\
	   foo(const json_string &) json_nothrow;\
	   foo(const json_char *) json_nothrow;\
	   DECLARE_FOR_LONG_LONG(foo)\
	   DECLARE_FOR_LONG_DOUBLE(foo)

    #define DECLARE_FOR_ALL_CAST_TYPES_CONST(foo)\
	   foo(char) const json_nothrow;	foo(unsigned char) const json_nothrow;\
	   foo(short) const json_nothrow;	foo(unsigned short) const json_nothrow;\
	   foo(int) const json_nothrow;	foo(unsigned int) const json_nothrow;\
	   foo(long) const json_nothrow;	foo(unsigned long) const json_nothrow;\
	   foo(float) const json_nothrow;	foo(double) const json_nothrow;\
	   foo(bool) const json_nothrow;\
	   foo(const json_string &) const json_nothrow;\
	   DECLARE_FOR_LONG_LONG_CONST(foo)\
	   DECLARE_FOR_LONG_DOUBLE_CONST(foo)

    #define DECLARE_FOR_ALL_TYPES_CONST(foo)\
	   DECLARE_FOR_ALL_CAST_TYPES_CONST(foo)\
	   foo(const JSONNode &) const json_nothrow;\
	   foo(const json_char *) const json_nothrow;

    #define IMPLEMENT_FOR_ALL_NUMBERS(foo)\
	   foo(char) foo(unsigned char)\
	   foo(short) foo(unsigned short)\
	   foo(int) foo(unsigned int)\
	   foo(long) foo(unsigned long)\
	   foo(float) foo(double)\
	   IMPLEMENT_FOR_LONG_LONG(foo)\
	   IMPLEMENT_FOR_LONG_DOUBLE(foo)

#endif

#define IMPLEMENT_FOR_ALL_TYPES(foo)\
    IMPLEMENT_FOR_ALL_NUMBERS(foo)\
    foo(const json_string &)\
    foo(bool)

/*
    This class is mostly just a wrapper class around internalJSONNode, this class keeps
    the reference count and handles copy on write and such.  This class is also responsible
    for argument checking and throwing exceptions if needed.
*/


class JSONNode {
public:
    explicit JSONNode(char mytype = JSON_NODE) json_nothrow json_hot;
    #define DECLARE_CTOR(type) explicit JSONNode(const json_string & name_t, type value_t)
    DECLARE_FOR_ALL_TYPES(DECLARE_CTOR)

    JSONNode(const JSONNode & orig) json_nothrow json_hot;
    ~JSONNode(void) json_nothrow json_hot;
    
    #if (defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY))
        static JSONNode stringType(const json_string & str);
        void set_name_(const json_string & newname) json_nothrow json_write_priority;
    #endif

    json_index_t size(void) const json_nothrow json_read_priority;
    bool empty(void) const json_nothrow json_read_priority;
    void clear(void) json_nothrow json_cold;
    unsigned char type(void) const json_nothrow json_read_priority;

    json_string name(void) const json_nothrow json_read_priority;
    void set_name(const json_string & newname) json_nothrow json_write_priority;
    #ifdef JSON_COMMENTS
	   void set_comment(const json_string & comment) json_nothrow;
	   json_string get_comment(void) const json_nothrow;
    #endif
    #if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
	   void preparse(void) json_nothrow json_read_priority;
    #endif


    json_string as_string(void) const json_nothrow json_read_priority;
    json_int_t as_int(void) const json_nothrow json_read_priority;
    json_number as_float(void) const json_nothrow json_read_priority;
    bool as_bool(void) const json_nothrow json_read_priority;
    
    #ifdef JSON_CASTABLE
	   JSONNode as_node(void) const json_nothrow json_read_priority;
	   JSONNode as_array(void) const json_nothrow json_read_priority;
	   void cast(char newtype) json_nothrow;
    #endif
    
    #ifdef JSON_BINARY
	   std::string as_binary(void) const json_nothrow json_cold;
	   void set_binary(const unsigned char * bin, size_t bytes) json_nothrow json_cold;
    #endif

    JSONNode & at(json_index_t pos) json_throws(std::out_of_range);
    const JSONNode & at(json_index_t pos) const json_throws(std::out_of_range);

    JSONNode & operator[](json_index_t pos) json_nothrow;
    const JSONNode & operator[](json_index_t pos) const json_nothrow;

    JSONNode & at(const json_string & name_t) json_throws(std::out_of_range);
    const JSONNode & at(const json_string & name_t) const json_throws(std::out_of_range);
    #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
	   JSONNode & at_nocase(const json_string & name_t) json_throws(std::out_of_range);
	   const JSONNode & at_nocase(const json_string & name_t) const json_throws(std::out_of_range);
    #endif
    JSONNode & operator[](const json_string & name_t) json_nothrow;
    const JSONNode & operator[](const json_string & name_t) const json_nothrow;

    #ifdef JSON_LIBRARY
	   void push_back(JSONNode * node) json_nothrow;
    #else
	   void push_back(const JSONNode & node) json_nothrow;
    #endif
    void reserve(json_index_t siz) json_nothrow;
    JSONNode JSON_PTR_LIB pop_back(json_index_t pos) json_throws(std::out_of_range);
    JSONNode JSON_PTR_LIB pop_back(const json_string & name_t) json_throws(std::out_of_range);
    #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
	   JSONNode JSON_PTR_LIB pop_back_nocase(const json_string & name_t) json_throws(std::out_of_range);
    #endif

    DECLARE_FOR_ALL_TYPES(JSONNode & operator =)
    JSONNode & operator = (const JSONNode &) json_nothrow;

    DECLARE_FOR_ALL_TYPES_CONST(bool operator ==)
    DECLARE_FOR_ALL_TYPES_CONST(bool operator !=)


    void nullify(void) json_nothrow;
    void swap(JSONNode & other) json_nothrow;
    void merge(JSONNode & other) json_nothrow json_cold;
    void merge(unsigned int num, ...) json_nothrow json_cold;
    JSONNode duplicate(void) const json_nothrow;


    //iterator
    #ifdef JSON_ITERATORS
	   #ifndef JSON_LIBRARY
		  #define json_iterator_ptr(iter) iter.it
		  #define ptr_to_json_iterator(iter) json_iterator(iter)

		  struct iterator;
		  struct const_iterator {
			 inline const_iterator& operator ++(void) json_nothrow { ++it; return *this; }
			 inline const_iterator& operator --(void) json_nothrow { --it; return *this; }
			 inline const_iterator& operator +=(long i) json_nothrow { it += i; return *this; }
			 inline const_iterator& operator -=(long i) json_nothrow { it -= i; return *this; }
			 inline const_iterator operator ++(int) json_nothrow {
				const_iterator result(*this);
				++it;
				return result;
			 }
			 inline const_iterator operator --(int) json_nothrow {
				const_iterator result(*this);
				--it;
				return result;
			 }
			 inline const_iterator operator +(long i) const json_nothrow {
				const_iterator result(*this);
				result.it += i;
				return result;
			 }
			 inline const_iterator operator -(long i) const json_nothrow {
				const_iterator result(*this);
				result.it -= i;
				return result;
			 }
			 inline const JSONNode& operator [](size_t pos) const json_nothrow { return const_cast<const JSONNode&>(*it[pos]); };
			 inline const JSONNode& operator *(void) const json_nothrow { return const_cast<const JSONNode&>(*(*it)); }
			 inline const JSONNode* operator ->(void) const json_nothrow { return const_cast<const JSONNode*>(*it); }
			 inline bool operator == (const const_iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const const_iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator > (const const_iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator >= (const const_iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator < (const const_iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator <= (const const_iterator & other) const json_nothrow { return it <= other.it; }

			 inline bool operator == (const iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator > (const iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator >= (const iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator < (const iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator <= (const iterator & other) const json_nothrow { return it <= other.it; }

			 inline const_iterator & operator =(const const_iterator & orig) json_nothrow { it = orig.it; return *this; }
			 const_iterator (const const_iterator & orig) json_nothrow : it(orig.it) {}
		  private:
			 JSONNode ** it;
			 const_iterator(JSONNode ** starter) : it(starter) {}
			 friend class JSONNode;
			 friend struct iterator;
		  };
		  const_iterator begin(void) const json_nothrow;
		  const_iterator end(void) const json_nothrow;

		  struct iterator {
			 inline iterator& operator ++(void) json_nothrow { ++it; return *this; }
			 inline iterator& operator --(void) json_nothrow { --it; return *this; }
			 inline iterator& operator +=(long i) json_nothrow { it += i; return *this; }
			 inline iterator& operator -=(long i) json_nothrow { it -= i; return *this; }
			 inline iterator operator ++(int) json_nothrow {
				iterator result(*this);
				++it;
				return result;
			 }
			 inline iterator operator --(int) json_nothrow {
				iterator result(*this);
				--it;
				return result;
			 }
			 inline iterator operator +(long i) const json_nothrow {
				iterator result(*this);
				result.it += i;
				return result;
			 }
			 inline iterator operator -(long i) const json_nothrow {
				iterator result(*this);
				result.it -= i;
				return result;
			 }
			 inline JSONNode& operator [](size_t pos) const json_nothrow { return *it[pos]; };
			 inline JSONNode& operator *(void) const json_nothrow { return *(*it); }
			 inline JSONNode* operator ->(void) const json_nothrow { return *it; }
			 inline bool operator == (const iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator > (const iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator >= (const iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator < (const iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator <= (const iterator & other) const json_nothrow { return it <= other.it; }
			 inline iterator & operator = (const iterator & orig) json_nothrow { it = orig.it; return *this; }

			 inline bool operator == (const const_iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const const_iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator > (const const_iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator >= (const const_iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator < (const const_iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator <= (const const_iterator & other) const json_nothrow { return it <= other.it; }
			 inline iterator & operator = (const const_iterator & orig) json_nothrow { it = orig.it; return *this; }

			 iterator (const iterator & orig) json_nothrow : it(orig.it) {}
			 inline operator const_iterator() const json_nothrow { return const_iterator(it); }
		  private:
			 JSONNode ** it;
			 iterator(JSONNode ** starter) json_nothrow : it(starter) {}
			 friend class JSONNode;
			 friend struct const_iterator;
		  };
		  typedef iterator json_iterator;

		  struct reverse_iterator;
		  struct reverse_const_iterator {
			 inline reverse_const_iterator& operator ++(void) json_nothrow{ --it; return *this; }
			 inline reverse_const_iterator& operator --(void) json_nothrow{ ++it; return *this; }
			 inline reverse_const_iterator& operator +=(long i) json_nothrow{ it -= i; return *this; }
			 inline reverse_const_iterator& operator -=(long i) json_nothrow{ it += i; return *this; }
			 inline reverse_const_iterator operator ++(int) json_nothrow{
				reverse_const_iterator result(*this);
				--it;
				return result;
			 }
			 inline reverse_const_iterator operator --(int) json_nothrow{
				reverse_const_iterator result(*this);
				++it;
				return result;
			 }
			 inline reverse_const_iterator operator +(long i) const json_nothrow {
				reverse_const_iterator result(*this);
				result.it -= i;
				return result;
			 }
			 inline reverse_const_iterator operator -(long i) const json_nothrow {
				reverse_const_iterator result(*this);
				result.it += i;
				return result;
			 }
			 inline const JSONNode& operator [](size_t pos) const json_nothrow { return const_cast<const JSONNode&>(*it[pos]); };
			 inline const JSONNode& operator *(void) const json_nothrow { return const_cast<const JSONNode&>(*(*it)); }
			 inline const JSONNode* operator ->(void) const json_nothrow { return const_cast<const JSONNode*>(*it); }
			 inline bool operator == (const reverse_const_iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const reverse_const_iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator < (const reverse_const_iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator <= (const reverse_const_iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator > (const reverse_const_iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator >= (const reverse_const_iterator & other) const json_nothrow { return it <= other.it; }

			 inline bool operator == (const reverse_iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const reverse_iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator < (const reverse_iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator <= (const reverse_iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator > (const reverse_iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator >= (const reverse_iterator & other) const json_nothrow { return it <= other.it; }

			 inline reverse_const_iterator & operator = (const reverse_const_iterator & orig) json_nothrow { it = orig.it; return *this; }
			 reverse_const_iterator (const reverse_const_iterator & orig) json_nothrow : it(orig.it) {}
		  private:
			 JSONNode ** it;
			 reverse_const_iterator(JSONNode ** starter) json_nothrow : it(starter) {}
			 friend class JSONNode;
			 friend struct reverse_iterator;
		  };
		  reverse_const_iterator rbegin(void) const json_nothrow;
		  reverse_const_iterator rend(void) const json_nothrow;

		  struct reverse_iterator {
			 inline reverse_iterator& operator ++(void) json_nothrow { --it; return *this; }
			 inline reverse_iterator& operator --(void) json_nothrow { ++it; return *this; }
			 inline reverse_iterator& operator +=(long i) json_nothrow { it -= i; return *this; }
			 inline reverse_iterator& operator -=(long i) json_nothrow { it += i; return *this; }
			 inline reverse_iterator operator ++(int) json_nothrow {
				reverse_iterator result(*this);
				--it;
				return result;
			 }
			 inline reverse_iterator operator --(int) json_nothrow {
				reverse_iterator result(*this);
				++it;
				return result;
			 }
			 inline reverse_iterator operator +(long i) const json_nothrow {
				reverse_iterator result(*this);
				result.it -= i;
				return result;
			 }
			 inline reverse_iterator operator -(long i) const json_nothrow {
				reverse_iterator result(*this);
				result.it += i;
				return result;
			 }
			 inline JSONNode& operator [](size_t pos) const json_nothrow { return *it[pos]; };
			 inline JSONNode& operator *(void) const json_nothrow { return *(*it); }
			 inline JSONNode* operator ->(void) const json_nothrow { return *it; }
			 inline bool operator == (const reverse_iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const reverse_iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator < (const reverse_iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator <= (const reverse_iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator > (const reverse_iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator >= (const reverse_iterator & other) const json_nothrow { return it <= other.it; }

			 inline bool operator == (const reverse_const_iterator & other) const json_nothrow { return it == other.it; }
			 inline bool operator != (const reverse_const_iterator & other) const json_nothrow { return it != other.it; }
			 inline bool operator < (const reverse_const_iterator & other) const json_nothrow { return it > other.it; }
			 inline bool operator <= (const reverse_const_iterator & other) const json_nothrow { return it >= other.it; }
			 inline bool operator > (const reverse_const_iterator & other) const json_nothrow { return it < other.it; }
			 inline bool operator >= (const reverse_const_iterator & other) const json_nothrow { return it <= other.it; }

			 inline reverse_iterator & operator = (const reverse_iterator & orig) json_nothrow { it = orig.it; return *this; }
			 reverse_iterator (const reverse_iterator & orig) json_nothrow : it(orig.it) {}
			 inline operator reverse_const_iterator() const json_nothrow { return reverse_const_iterator(it); }
		  private:
			 JSONNode ** it;
			 reverse_iterator(JSONNode ** starter) json_nothrow : it(starter) {}
			 friend class JSONNode;
			 friend struct reverse_const_iterator;
		  };
		  reverse_iterator rbegin(void) json_nothrow;
		  reverse_iterator rend(void) json_nothrow;

		  const_iterator find(const json_string & name_t) const json_nothrow;
		  #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
			 const_iterator find_nocase(const json_string & name_t) const json_nothrow;
		  #endif

		  reverse_iterator erase(reverse_iterator pos) json_nothrow;
		  reverse_iterator erase(reverse_iterator start, const reverse_iterator & end) json_nothrow;

		  iterator insert(iterator pos, const JSONNode & x) json_nothrow;
		  reverse_iterator insert(reverse_iterator pos, const JSONNode & x) json_nothrow;
		  iterator insert(iterator pos, const reverse_iterator & _start, const reverse_iterator & _end) json_nothrow;
		  reverse_iterator insert(reverse_iterator pos, const iterator & _start, const iterator & _end) json_nothrow;
		  reverse_iterator insert(reverse_iterator pos, const reverse_iterator & _start, const reverse_iterator & _end) json_nothrow;

		  json_iterator insert(json_iterator pos, const const_iterator & _start, const const_iterator & _end) json_nothrow;
		  reverse_iterator insert(reverse_iterator pos, const const_iterator & _start, const const_iterator & _end) json_nothrow;
		  json_iterator insert(json_iterator pos, const reverse_const_iterator & _start, const reverse_const_iterator & _end) json_nothrow;
		  reverse_iterator insert(reverse_iterator pos, const reverse_const_iterator & _start, const reverse_const_iterator & _end) json_nothrow;
	   #else
		  typedef JSONNode** json_iterator;
		  #define json_iterator_ptr(iter) iter
		  #define ptr_to_json_iterator(iter) iter
		  json_iterator insert(json_iterator pos, JSONNode * x) json_nothrow;
	   #endif

	   json_iterator begin(void) json_nothrow;
	   json_iterator end(void) json_nothrow;

	   json_iterator find(const json_string & name_t) json_nothrow;
	   #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
		  json_iterator find_nocase(const json_string & name_t) json_nothrow;
	   #endif
	   json_iterator erase(json_iterator pos) json_nothrow;
	   json_iterator erase(json_iterator start, const json_iterator & end) json_nothrow;
	   json_iterator insert(json_iterator pos, const json_iterator & _start, const json_iterator & _end) json_nothrow;
    #endif


    #ifdef JSON_MUTEX_CALLBACKS
	   static void register_mutex_callbacks(json_mutex_callback_t lock, json_mutex_callback_t unlock, void * manager_lock) json_nothrow json_cold;
	   #ifdef JSON_MUTEX_MANAGE
		  static void register_mutex_destructor(json_mutex_callback_t destroy) json_nothrow json_cold;
	   #endif
	   static void set_global_mutex(void * mutex) json_nothrow json_cold;
	   void set_mutex(void * mutex) json_nothrow json_cold;
	   void lock(int thread) json_nothrow json_cold;
	   void unlock(int thread) json_nothrow json_cold;
	   struct auto_lock {
		  public:
			 auto_lock(JSONNode & node, int thread)  json_nothrow: mynode(&node), mythread(thread){
				mynode -> lock(mythread);
			 }
			 auto_lock(JSONNode * node, int thread)  json_nothrow: mynode(node), mythread(thread){
				mynode -> lock(mythread);
			 }
			 ~auto_lock(void) json_nothrow{
				mynode -> unlock(mythread);
			 }
		  private:
			 auto_lock & operator = (const auto_lock &);
			 auto_lock(const auto_lock &);
			 JSONNode * mynode;
			 int mythread;
	   };
	   static void * getThisLock(JSONNode * pthis) json_nothrow json_cold;
    #endif

    #ifdef JSON_UNIT_TEST
	   static int getNodeAllocationCount(void);
	   static int getNodeDeallocationCount(void);
	   static int getInternalAllocationCount(void);
	   static int getInternalDeallocationCount(void);
	   static int getChildrenAllocationCount(void);
	   static int getChildrenDeallocationCount(void);
	   static void incAllocCount(void);
	   static void decAllocCount(void);
	   static void incinternalAllocCount(void);
	   static void decinternalAllocCount(void);
	   static void incChildrenAllocCount(void);
	   static void decChildrenAllocCount(void);
    #endif

    #ifdef JSON_WRITE_PRIORITY
		#ifdef JSON_LESS_MEMORY
			#define DEFAULT_APPROX_SIZE 8
			#define DEFAULT_APPROX_SIZE_FORMATTED 16
		#else
			#define DEFAULT_APPROX_SIZE 1024
			#define DEFAULT_APPROX_SIZE_FORMATTED 2048
		#endif
	   json_string write(size_t approxsize = DEFAULT_APPROX_SIZE) const json_nothrow json_write_priority;
	   json_string write_formatted(size_t approxsize = DEFAULT_APPROX_SIZE_FORMATTED) const json_nothrow json_write_priority;
    #endif

    #ifdef JSON_DEBUG
	   #ifndef JSON_LIBRARY
		  JSONNode dump(void) const json_nothrow;
	   #endif
    #endif
    static void deleteJSONNode(JSONNode * ptr) json_nothrow json_hot;
    static JSONNode * newJSONNode_Shallow(const JSONNode & orig) json_hot;

    #define DECLARE_CAST_OP(type) operator type()
    //DECLARE_FOR_ALL_CAST_TYPES_CONST(DECLARE_CAST_OP)
JSON_PRIVATE
    static JSONNode * newJSONNode(const JSONNode & orig     JSON_MUTEX_COPY_DECL2) json_hot;
    static JSONNode * newJSONNode(internalJSONNode * internal_t) json_hot;
    #ifdef JSON_READ_PRIORITY
	   //used by JSONWorker
	   JSONNode(const json_string & unparsed) json_nothrow : internal(internalJSONNode::newInternal(unparsed)){ //root, specialized because it can only be array or node
		  incAllocCount();
	   }
    #endif
    JSONNode(internalJSONNode * internal_t) json_nothrow : internal(internal_t){ //do not increment anything, this is only used in one case and it's already taken care of
	   incAllocCount();
    }
    JSONNode(bool, JSONNode & orig) json_nothrow json_hot;

    void decRef(void) json_nothrow json_hot;  //decrements internal's counter, deletes it if needed
    #ifdef JSON_REF_COUNT
	   void makeUniqueInternal(void) json_nothrow; //makes internal it's own
	   void merge(JSONNode * other) json_nothrow json_cold;
    #endif

    #ifdef JSON_DEBUG
	   #ifndef JSON_LIBRARY
		  JSONNode dump(size_t & totalmemory) json_nothrow;
	   #endif
    #endif

    #ifdef JSON_ITERATORS
	   #ifndef JSON_LIBRARY
		  json_iterator insertFRR(json_iterator pos, JSONNode ** const _start, JSONNode ** const _end) json_nothrow;
		  reverse_iterator insertRRR(reverse_iterator pos, JSONNode ** const _start, JSONNode ** const _end) json_nothrow;
		  reverse_iterator insertRFF(reverse_iterator pos, JSONNode ** const _start, JSONNode ** const _end) json_nothrow;
	   #endif
	   json_iterator insertFFF(json_iterator pos, JSONNode ** const _start, JSONNode ** const _end) json_nothrow;
    #endif

	inline void clear_name(void) json_nothrow {
	    JSON_CHECK_INTERNAL();
	    makeUniqueInternal();
	    internal -> clearname();
	}

    mutable internalJSONNode * internal;
    friend class JSONWorker;
    friend class internalJSONNode;
};



/*
    Implementations are here to keep the class declaration cleaner.  They can't be placed in a different
    file because they are inlined.
*/

#define CAST_OP(type)\
    inline JSONNode::operator type() const json_nothrow {\
	   return static_cast<type>(*internal);\
    }
//IMPLEMENT_FOR_ALL_TYPES(CAST_OP)


inline JSONNode::JSONNode(char mytype) json_nothrow : internal(internalJSONNode::newInternal(mytype)){
    JSON_ASSERT((mytype == JSON_NULL) ||
			 (mytype == JSON_STRING) ||
			 (mytype == JSON_NUMBER) ||
			 (mytype == JSON_BOOL) ||
			 (mytype == JSON_ARRAY) ||
			 (mytype == JSON_NODE), JSON_TEXT("Not a proper JSON type"));
    incAllocCount();
}

inline JSONNode::JSONNode(const JSONNode & orig) json_nothrow : internal(orig.internal -> incRef()){
    incAllocCount();
}

//this allows a temp node to simply transfer its contents, even with ref counting off
inline JSONNode::JSONNode(bool, JSONNode & orig) json_nothrow : internal(orig.internal){
    orig.internal = 0;
    incAllocCount();
}

inline JSONNode::~JSONNode(void) json_nothrow{
    if (internal != 0) decRef();
    decAllocCount();
}

inline json_index_t JSONNode::size(void) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return internal -> size();
}

inline bool JSONNode::empty(void) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return internal -> empty();
}

inline void JSONNode::clear(void) json_nothrow {
    JSON_CHECK_INTERNAL();
    if (!empty()){
	   makeUniqueInternal();
	   internal -> CHILDREN -> clear();
    }
}

inline unsigned char JSONNode::type(void) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return internal -> type();
}

inline json_string JSONNode::name(void) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return internal -> name();
}

inline void JSONNode::set_name(const json_string & newname) json_nothrow{
    JSON_CHECK_INTERNAL();
    makeUniqueInternal();
    internal -> setname(newname);
}

#ifdef JSON_COMMENTS
    inline void JSONNode::set_comment(const json_string & newname) json_nothrow{
	   JSON_CHECK_INTERNAL();
	   makeUniqueInternal();
	   internal -> setcomment(newname);
    }

    inline json_string JSONNode::get_comment(void) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   return internal -> getcomment();
    }
#endif

//#ifdef JSON_DEPRECATED_FUNCTIONS
    inline json_string JSONNode::as_string(void) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   return static_cast<json_string>(*internal);
    }

    inline json_int_t JSONNode::as_int(void) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   return static_cast<json_int_t>(*internal);
    }

    inline json_number JSONNode::as_float(void) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   return static_cast<json_number>(*internal);
    }

    inline bool JSONNode::as_bool(void) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   return static_cast<bool>(*internal);
    }
//#endif

#ifdef JSON_BINARY
    inline void JSONNode::set_binary(const unsigned char * bin, size_t bytes) json_nothrow{
	   JSON_CHECK_INTERNAL();
	   *this = JSONBase64::json_encode64(bin, bytes);
    }

    inline std::string JSONNode::as_binary(void) const json_nothrow {
	   JSON_ASSERT_SAFE(type() == JSON_STRING, JSON_TEXT("using as_binary for a non-string type"), return json_global(EMPTY_STD_STRING););
	   JSON_CHECK_INTERNAL();
	   return JSONBase64::json_decode64(as_string());
    }
#endif

inline JSONNode & JSONNode::operator[](const json_string & name_t) json_nothrow {
    JSON_CHECK_INTERNAL();
    makeUniqueInternal();
    return *(*(internal -> at(name_t)));
}

inline const JSONNode & JSONNode::operator[](const json_string & name_t) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return *(*(internal -> at(name_t)));
}

#ifdef JSON_LIBRARY
inline void JSONNode::push_back(JSONNode * child) json_nothrow{
#else
inline void JSONNode::push_back(const JSONNode & child) json_nothrow{
#endif
    JSON_CHECK_INTERNAL();
    makeUniqueInternal();
    internal -> push_back(child);
}

inline void JSONNode::reserve(json_index_t siz) json_nothrow{
    makeUniqueInternal();
    internal -> reserve(siz);
}

inline JSONNode & JSONNode::operator = (const JSONNode & orig) json_nothrow {
    JSON_CHECK_INTERNAL();
    #ifdef JSON_REF_COUNT
	   if (internal == orig.internal) return *this;  //don't want it accidentally deleting itself
    #endif
    decRef();  //dereference my current one
    internal = orig.internal -> incRef();  //increase reference of original
    return *this;
}

#ifndef JSON_LIBRARY
    inline JSONNode & JSONNode::operator = (const json_char * val) json_nothrow {
	   JSON_CHECK_INTERNAL();
	   *this = json_string(val);
	   return *this;
    }
#endif

#define NODE_SET_TYPED(type)\
    inline JSONNode & JSONNode::operator = (type val) json_nothrow {\
	   JSON_CHECK_INTERNAL();\
	   makeUniqueInternal();\
	   internal -> Set(val);\
	   return *this;\
    }
IMPLEMENT_FOR_ALL_TYPES(NODE_SET_TYPED)


/*
    This section is the equality operators
*/

#define NODE_CHECK_EQUALITY(type)\
    inline bool JSONNode::operator == (type val) const json_nothrow {\
	   JSON_CHECK_INTERNAL();\
	   return internal -> IsEqualToNum<type>(val);\
    }

IMPLEMENT_FOR_ALL_NUMBERS(NODE_CHECK_EQUALITY)

inline bool JSONNode::operator == (const json_string & val) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return internal -> IsEqualTo(val);
}

#ifndef JSON_LIBRARY
    inline bool JSONNode::operator == (const json_char * val) const json_nothrow {
	   JSON_CHECK_INTERNAL();
	   return *this == json_string(val);
    }
#endif

inline bool JSONNode::operator == (bool val) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return internal -> IsEqualTo(val);
}
inline bool JSONNode::operator == (const JSONNode & val) const json_nothrow {
    JSON_CHECK_INTERNAL();
    return internal -> IsEqualTo(val.internal);
}


/*
    This section is the inequality operators
*/


#define NODE_CHECK_INEQUALITY(type)\
    inline bool JSONNode::operator != (type val) const json_nothrow {\
	   JSON_CHECK_INTERNAL();\
	   return !(*this == val);\
    }

IMPLEMENT_FOR_ALL_TYPES(NODE_CHECK_INEQUALITY)
NODE_CHECK_INEQUALITY(const JSONNode &)
#ifndef JSON_LIBRARY
    NODE_CHECK_INEQUALITY(const json_char * )
#endif

inline void JSONNode::nullify(void) json_nothrow {
    JSON_CHECK_INTERNAL();
    makeUniqueInternal();
    internal -> Nullify();
}

inline void JSONNode::swap(JSONNode & other) json_nothrow {
    JSON_CHECK_INTERNAL();
    internalJSONNode * temp = other.internal;
    other.internal = internal;
    internal = temp;
    JSON_CHECK_INTERNAL();
}

inline void JSONNode::decRef(void) json_nothrow { //decrements internal's counter, deletes it if needed
    JSON_CHECK_INTERNAL();
    #ifdef JSON_REF_COUNT
	   internal -> decRef();
	   if (internal -> hasNoReferences()){
		  internalJSONNode::deleteInternal(internal);
	   }
    #else
	   internalJSONNode::deleteInternal(internal);
    #endif
}

#ifdef JSON_REF_COUNT
    inline void JSONNode::makeUniqueInternal() json_nothrow { //makes internal it's own
	   JSON_CHECK_INTERNAL();
	   internal = internal -> makeUnique();  //might return itself or a new one that's exactly the same
    }
#endif

#ifdef JSON_ITERATORS
    inline JSONNode::json_iterator JSONNode::begin(void) json_nothrow {
	   JSON_CHECK_INTERNAL();
	   JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("begin"));
	   makeUniqueInternal();
	   return json_iterator(internal -> begin());
    }

    inline JSONNode::json_iterator JSONNode::end(void) json_nothrow {
	   JSON_CHECK_INTERNAL();
	   JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("end"));
	   makeUniqueInternal();
	   return json_iterator(internal -> end());
    }

    #ifndef JSON_LIBRARY
	   inline JSONNode::const_iterator JSONNode::begin(void) const json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("begin"));
		  return JSONNode::const_iterator(internal -> begin());
	   }

	   inline JSONNode::const_iterator JSONNode::end(void) const json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("end"));
		  return JSONNode::const_iterator(internal -> end());
	   }

	   inline JSONNode::reverse_iterator JSONNode::rbegin(void) json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("rbegin"));
		  makeUniqueInternal();
		  return JSONNode::reverse_iterator(internal -> end() - 1);
	   }

	   inline JSONNode::reverse_iterator JSONNode::rend(void) json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("rend"));
		  makeUniqueInternal();
		  return JSONNode::reverse_iterator(internal -> begin() - 1);
	   }

	   inline JSONNode::reverse_const_iterator JSONNode::rbegin(void) const json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("rbegin"));
		  return JSONNode::reverse_const_iterator(internal -> end() - 1);
	   }

	   inline JSONNode::reverse_const_iterator JSONNode::rend(void) const json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSON_ASSERT(type() == JSON_NODE || type() == JSON_ARRAY, json_global(ERROR_NON_ITERATABLE) + JSON_TEXT("rend"));
		  return JSONNode::reverse_const_iterator(internal -> begin() - 1);
	   }

	   inline JSONNode::iterator JSONNode::insert(json_iterator pos, const const_iterator & _start, const const_iterator & _end) json_nothrow {
		  return insertFFF(pos, _start.it, _end.it);
	   }

	   inline JSONNode::reverse_iterator JSONNode::insert(reverse_iterator pos, const const_iterator & _start, const const_iterator & _end) json_nothrow {
		  return insertRFF(pos, _start.it, _end.it);
	   }

	   inline JSONNode::reverse_iterator JSONNode::insert(reverse_iterator pos, const iterator & _start, const iterator & _end) json_nothrow {
		  return insertRFF(pos, _start.it, _end.it);
	   }

	   inline JSONNode::reverse_iterator JSONNode::insert(reverse_iterator pos, const reverse_const_iterator & _start, const reverse_const_iterator & _end) json_nothrow {
		  return insertRRR(pos, _start.it, _end.it);
	   }

	   inline JSONNode::reverse_iterator JSONNode::insert(reverse_iterator pos, const reverse_iterator & _start, const reverse_iterator & _end) json_nothrow {
		  return insertRRR(pos, _start.it, _end.it);
	   }

	   inline JSONNode::iterator JSONNode::insert(json_iterator pos, const reverse_const_iterator & _start, const reverse_const_iterator & _end) json_nothrow {
		  return insertFRR(pos, _start.it, _end.it);
	   }

	   inline JSONNode::iterator JSONNode::insert(iterator pos, const reverse_iterator & _start, const reverse_iterator & _end) json_nothrow {
		  return insertFRR(pos, _start.it, _end.it);
	   }
    #endif

    inline JSONNode::json_iterator JSONNode::insert(json_iterator pos, const json_iterator & _start, const json_iterator & _end) json_nothrow {
	   return insertFFF(pos, json_iterator_ptr(_start), json_iterator_ptr(_end));
    }
#endif

#ifdef JSON_WRITE_PRIORITY
    inline json_string JSONNode::write(size_t approxsize) const json_nothrow {
	    JSON_CHECK_INTERNAL();
	    JSON_ASSERT_SAFE(type() == JSON_NODE || type() == JSON_ARRAY, JSON_TEXT("Writing a non-writable node"), return json_global(EMPTY_JSON_STRING););
		json_string result;
		result.reserve(approxsize);
		internal -> Write(0xFFFFFFFF, true, result);
		return result;
    }

    inline json_string JSONNode::write_formatted(size_t approxsize) const json_nothrow {
	    JSON_CHECK_INTERNAL();
	    JSON_ASSERT_SAFE(type() == JSON_NODE || type() == JSON_ARRAY, JSON_TEXT("Writing a non-writable node"), return json_global(EMPTY_JSON_STRING););
		json_string result;
		result.reserve(approxsize);
		internal -> Write(0, true, result);
		return result;
    }

#endif

#if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
    inline void JSONNode::preparse(void) json_nothrow {
	   JSON_CHECK_INTERNAL();
	   internal -> preparse();
    }
#endif

#ifdef JSON_DEBUG
    #ifndef JSON_LIBRARY
	   inline JSONNode JSONNode::dump(void) const json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSONNode dumpage(JSON_NODE);
		  dumpage.push_back(JSON_NEW(JSONNode(JSON_TEXT("this"), (long)this)));
		  size_t total = 0;
		  JSONNode node(internal -> Dump(total));
		  dumpage.push_back(JSON_NEW(JSONNode(JSON_TEXT("total bytes used"), total)));
		  dumpage.push_back(JSON_NEW(JSONNode(JSON_TEXT("bytes used"), sizeof(JSONNode))));
		  dumpage.push_back(JSON_NEW(node));
		  return dumpage;
	   }

	   inline JSONNode JSONNode::dump(size_t & totalmemory) json_nothrow {
		  JSON_CHECK_INTERNAL();
		  JSONNode dumpage(JSON_NODE);
		  dumpage.push_back(JSON_NEW(JSONNode(JSON_TEXT("this"), (long)this)));
		  dumpage.push_back(JSON_NEW(JSONNode(JSON_TEXT("bytes used"), sizeof(JSONNode))));
		  dumpage.push_back(JSON_NEW(internal -> Dump(totalmemory)));
		  return dumpage;
	   }
    #endif
#endif

#ifdef JSON_LESS_MEMORY
    #ifdef __GNUC__
	   #pragma pack(pop)
    #elif _MSC_VER
	   #pragma pack(pop, JSONNode_pack)
    #endif
#endif
#endif
