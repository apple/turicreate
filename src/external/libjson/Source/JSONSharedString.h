#ifndef JSON_SHARED_STRING_H
#define JSON_SHARED_STRING_H

/*
 *	This class allows json objects to share string
 *	Since libjson is a parser, it does a lot of substrings, but since
 *	a string with all of the information already exists, those substrings
 *	can be infered by an offset and length and a pointer to the master
 *	string
 *
 *	EXPERIMENTAL, Not used yet
 */

#include "JSONDebug.h"
#include "JSONGlobals.h"
#include "JSONMemory.h"

/*
mallocs: 3351
frees: 3351
reallocs: 3
bytes: 298751 (291 KB)
max bytes at once: 3624 (3 KB)
avg bytes at once: 970 (0 KB)
*/

#ifdef JSON_LESS_MEMORY
	#ifdef __GNUC__
		#pragma pack(push, 1)
	#elif _MSC_VER
		#pragma pack(push, json_shared_string_pack, 1)
	#endif
#endif

class json_shared_string {
public:


	struct iterator;
	  struct const_iterator {
		//const_iterator(const json_char * p, const json_shared_string * pa) : parent(pa), it(p){}
	  const_iterator(const json_char * p, const json_shared_string * pa) : it(p){}

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
		 inline const json_char & operator [](size_t pos) const json_nothrow { return it[pos]; };
		 inline const json_char & operator *(void) const json_nothrow { return *it; }
		 inline const json_char * operator ->(void) const json_nothrow { return it; }
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
    // const json_shared_string * parent; // creates annoying warning
		 const json_char * it;
		 friend class json_shared_string;
		 friend struct iterator;
	  };

	  struct iterator {
		iterator(const json_char * p, const json_shared_string * pa) : parent(pa), it(p){}

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
		 inline const json_char & operator [](size_t pos) const json_nothrow { return it[pos]; };
		 inline const json_char & operator *(void) const json_nothrow { return *it; }
		 inline const json_char * operator ->(void) const json_nothrow { return it; }
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

		 inline iterator & operator =(const iterator & orig) json_nothrow { it = orig.it; return *this; }
		 iterator (const iterator & orig) json_nothrow : it(orig.it) {}
	  private:
		 const json_shared_string * parent;
		 const json_char * it;
		 friend class json_shared_string;
		 friend struct const_iterator;
	  };



	inline json_shared_string::iterator begin(void){
		iterator res = iterator(data(), this);
		return res;
	}
	inline json_shared_string::iterator end(void){
		iterator res = iterator(data() + len, this);
		return res;
	}
	inline json_shared_string::const_iterator begin(void) const {
		const_iterator res = const_iterator(data(), this);
		return res;
	}
	inline json_shared_string::const_iterator end(void) const {
		const_iterator res = const_iterator(data() + len, this);
		return res;
	}


	inline json_string::iterator std_begin(void){
		return _str -> mystring.begin() + offset;
	}
	inline json_string::iterator std_end(void){
		return std_begin() + len;
	}

	inline json_string::const_iterator std_begin(void) const{
		return _str -> mystring.begin() + offset;
	}
	inline json_string::const_iterator std_end(void) const{
		return std_begin() + len;
	}

	inline json_shared_string(void) : offset(0), len(0), _str(new(json_malloc<json_shared_string_internal>(1)) json_shared_string_internal(json_global(EMPTY_JSON_STRING))) {}

	inline json_shared_string(const json_string & str) : offset(0), len(str.length()), _str(new(json_malloc<json_shared_string_internal>(1)) json_shared_string_internal(str)) {}

	inline json_shared_string(const json_shared_string & str, size_t _offset, size_t _len) : _str(str._str), offset(str.offset + _offset), len(_len) {
		++_str -> refCount;
	}

	inline json_shared_string(const json_shared_string & str, size_t _offset) : _str(str._str), offset(str.offset + _offset), len(str.len - _offset) {
		++_str -> refCount;
	}

	inline json_shared_string(const iterator & s, const iterator & e) : _str(s.parent -> _str), offset(s.it - s.parent -> _str -> mystring.data()), len(e.it - s.it){
		++_str -> refCount;
	}

	inline ~json_shared_string(void){
		deref();
	}

	inline bool empty(void) const { return len == 0; }

	size_t find(json_char ch, size_t pos = 0) const {
		if (_str -> refCount == 1) return _str -> mystring.find(ch, pos);
		json_string::const_iterator e = std_end();
		for(json_string::const_iterator b = std_begin() + pos; b != e; ++b){
			if (*b == ch) return b - std_begin();
		}
		return json_string::npos;
	}

	inline json_char & operator[] (size_t loc){
		return _str -> mystring[loc + offset];
	}
	inline json_char operator[] (size_t loc) const {
		return _str -> mystring[loc + offset];
	}
	inline void clear(){ len = 0; }
	inline size_t length() const { return len; }
	inline const json_char * c_str() const { return toString().c_str(); }
	inline const json_char * data() const { return _str -> mystring.data() + offset; }

	inline bool operator != (const json_shared_string & other) const {
		if ((other._str == _str) && (other.len == len) && (other.offset == offset)) return false;
		return other.toString() != toString();
	}

	inline bool operator == (const json_shared_string & other) const {
		if ((other._str == _str) && (other.len == len) && (other.offset == offset)) return true;
		return other.toString() == toString();
	}

	inline bool operator == (const json_string & other) const {
		return other == toString();
	}

	json_string & toString(void) const {
		//gonna have to do a real substring now anyway, so do it completely
		if (_str -> refCount == 1){
			if (offset || len != _str -> mystring.length()){
				_str -> mystring = json_string(std_begin(), std_end());
			}
		} else if (offset || len != _str -> mystring.length()){
			--_str -> refCount;  //dont use deref because I know its not going to be deleted
			_str = new(json_malloc<json_shared_string_internal>(1)) json_shared_string_internal(json_string(std_begin(), std_end()));
		}
		offset = 0;
		return _str -> mystring;
	}


	inline void assign(const json_shared_string & other, size_t _offset, size_t _len){
		if (other._str != _str){
			deref();
			_str = other._str;
		}
		++_str -> refCount;
		offset = other.offset + _offset;
		len = _len;
	}

	json_shared_string(const json_shared_string & other) : _str(other._str), offset(other.offset), len(other.len){
		++_str -> refCount;
	}

	json_shared_string & operator =(const json_shared_string & other){
		if (other._str != _str){
			deref();
			_str = other._str;
			++_str -> refCount;
		}
		offset = other.offset;
		len = other.len;
		return *this;
	}

	json_shared_string & operator += (const json_char c){
		toString() += c;
		++len;
		return *this;
	}

	//when doing a plus equal of another string, see if it shares the string and starts where this one left off, in which case just increase len
JSON_PRIVATE
	struct json_shared_string_internal {
		inline json_shared_string_internal(const json_string & _mystring) : mystring(_mystring), refCount(1) {}
		json_string mystring;
		size_t refCount PACKED(20);
	};
	inline void deref(void){
		if (--_str -> refCount == 0){
			_str -> ~json_shared_string_internal();
			libjson_free<json_shared_string_internal>(_str);
		}
	}
	mutable json_shared_string_internal * _str;
	mutable size_t offset PACKED(20);
	mutable size_t len PACKED(20);
};

#ifdef JSON_LESS_MEMORY
	#ifdef __GNUC__
		#pragma pack(pop)
	#elif _MSC_VER
		#pragma pack(pop, json_shared_string_pack,)
	#endif
#endif

#endif
