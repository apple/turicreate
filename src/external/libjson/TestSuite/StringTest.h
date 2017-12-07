#ifndef STRING_TEST_H
#define STRING_TEST_H

/*
 *  Developer note:  This is not a fully functionaly string and is not meant to be used as such.
 *  It is merely to serve as a testing module
 */

#include <cstring>
#include <cstdlib>

typedef char mychar;

static size_t mystrlen(const mychar * str){
    unsigned int i = 0;
    for(const mychar * it = str; *it; ++it, ++i){
	   //dummy
    }
    return i;
}

class json_string {
public:
    struct const_iterator {
	   inline const_iterator&  operator ++(void)  { ++it; return *this; }
	   inline const_iterator&  operator --(void)  { --it; return *this; }
	   inline const_iterator&  operator +=(long i)  { it += i; return *this; }
	   inline const_iterator&  operator -=(long i)  { it -= i; return *this; }
	   inline const_iterator  operator ++(int)  {
		  const_iterator result(*this);
		  ++it;
		  return result;
	   }
	   inline const_iterator  operator --(int)  {
		  const_iterator result(*this);
		  --it;
		  return result;
	   }
	   inline const_iterator  operator +(long i) const  {
		  const_iterator result(*this);
		  result.it += i;
		  return result;
	   }
	   inline const_iterator  operator -(long i) const  {
		  const_iterator result(*this);
		  result.it -= i;
		  return result;
	   }
		inline size_t operator -(const_iterator other) const  {
			return it - other.it;
		}
	   inline mychar & operator [](size_t pos) const  { return it[pos]; };
	   inline mychar & operator *(void) const  { return *it; }
	   inline bool  operator == (const const_iterator & other) const  { return it == other.it; }
	   inline bool  operator != (const const_iterator & other) const  { return it != other.it; }
	   inline bool  operator > (const const_iterator & other) const  { return it > other.it; }
	   inline bool  operator >= (const const_iterator & other) const  { return it >= other.it; }
	   inline bool  operator < (const const_iterator & other) const  { return it < other.it; }
	   inline bool  operator <= (const const_iterator & other) const  { return it <= other.it; }
	   inline const_iterator &  operator = (const const_iterator & orig)  { it = orig.it; return *this; }
	   const_iterator (const const_iterator & orig)  : it(orig.it) {}
	   const_iterator (const mychar * place)  : it((mychar*)place) {}
	   const_iterator(void) : it(0) {};

	   mychar * it;
    };

    struct iterator {
	   inline iterator&  operator ++(void)  { ++it; return *this; }
	   inline iterator&  operator --(void)  { --it; return *this; }
	   inline iterator&  operator +=(long i)  { it += i; return *this; }
	   inline iterator&  operator -=(long i)  { it -= i; return *this; }
	   inline iterator  operator ++(int)  {
		  iterator result(*this);
		  ++it;
		  return result;
	   }
	   inline iterator  operator --(int)  {
		  iterator result(*this);
		  --it;
		  return result;
	   }
	   inline iterator  operator +(long i) const  {
		  iterator result(*this);
		  result.it += i;
		  return result;
	   }
	   inline iterator  operator -(long i) const  {
		  iterator result(*this);
		  result.it -= i;
		  return result;
	   }
	   inline mychar & operator [](size_t pos) const  { return it[pos]; };
	   inline mychar & operator *(void) const  { return *it; }
	   inline bool  operator == (const iterator & other) const  { return it == other.it; }
	   inline bool  operator != (const iterator & other) const  { return it != other.it; }
	   inline bool  operator > (const iterator & other) const  { return it > other.it; }
	   inline bool  operator >= (const iterator & other) const  { return it >= other.it; }
	   inline bool  operator < (const iterator & other) const  { return it < other.it; }
	   inline bool  operator <= (const iterator & other) const  { return it <= other.it; }
	   inline iterator &  operator = (const iterator & orig)  { it = orig.it; return *this; }
       inline operator const_iterator() const json_nothrow { return const_iterator(it); }
	   iterator (const iterator & orig)  : it(orig.it) {}
	   iterator (const mychar * place)  : it((mychar*)place) {}

	   mychar * it;
    };



    const static size_t npos = 0xFFFFFFFF;
    json_string(void) : len(0), str(0){
	   setToCStr("", 0);
    }

    json_string(const mychar * meh) : len(0), str(0){
	   setToCStr(meh, mystrlen(meh));
    }
	
	json_string(const mychar * meh, size_t l) : len(l), str(0){
	   setToCStr(meh, l);
	   str[len] = '\0';
    }

    json_string(const iterator & beg, const iterator & en) : len(0), str(0){
	   setToCStr(beg.it, en.it - beg.it);
	   str[len] = '\0';
    }

    json_string(const const_iterator & beg, const const_iterator & en) : len(0), str(0){
	   setToCStr(beg.it, en.it - beg.it);
	   str[len] = '\0';
    }

    json_string(const json_string & meh) : len(0), str(0){
	   setToCStr(meh.c_str(), meh.len);
    }

    ~json_string(void){ std::free(str); };

    json_string(unsigned int l, mychar meh) : len(0), str(0){
	   str = (mychar*)std::malloc((l + 1) * sizeof(mychar));
	   len = l;
	   for (unsigned int i = 0; i < l; ++i){
		  str[i] = meh;
	   }
	   str[l] = '\0';
    }

    void swap(json_string & meh){
	   size_t _len = len;
	   mychar * _str = str;
	   len = meh.len;
	   str = meh.str;
	   meh.len = _len;
	   meh.str = _str;
    }

    iterator begin(void){ return iterator(str); };
    iterator end(void){ return iterator(str + length()); };
    const iterator begin(void) const { return iterator(str); };
    const iterator end(void) const { return iterator(str + length()); };
    void assign(const iterator & beg, const iterator & en){
	   json_string(beg, en).swap(*this);
    }
    json_string & append(const iterator & beg, const iterator & en){
	   json_string temp(beg, en);
	   return *this += temp;
    }

    const mychar * c_str(void) const { return str; };
	const mychar * data(void) const { return str; };
    size_t length(void) const { return len; };
    size_t capacity(void) const { return len; };
    bool empty(void) const { return len == 0; };

    bool operator ==(const json_string & other) const {
	   if (len != other.len) return false;
	   return memcmp(str, other.str, len * sizeof(mychar)) == 0;
    }

    bool operator !=(const json_string & other) const {
	   return !(*this == other);
    }

    const char & operator[] (size_t pos) const { return str[pos]; }
    char & operator[] ( size_t pos ){ return str[pos]; }

    json_string & operator = (const json_string & meh) {
	   std::free(str);
	   setToCStr(meh.c_str(), meh.len);
	   return *this;
    }

    json_string & operator = (const mychar * meh) {
	   std::free(str);
	   setToCStr(meh, mystrlen(meh));
	   return *this;
    }

    json_string & operator += (const json_string & other) {
	   size_t newlen = len + other.len;
	   mychar * newstr = (mychar*)std::malloc((newlen + 1) * sizeof(mychar));
	   std::memcpy(newstr, str, len * sizeof(mychar));
	   std::memcpy(newstr + len, other.str, (other.len + 1) * sizeof(mychar));
	   len = newlen;
	   std::free(str);
	   str = newstr;
	   return *this;
    }

    const json_string operator + (const json_string & other) const {
	   json_string result = *this;
	   result += other;
	   return result;
    }

    json_string & operator += (const mychar other) {
	   mychar temp[2] = {other, '\0'};
	   json_string temp_s(temp);
	   return (*this) += temp_s;
    }

    const json_string operator + (const mychar other) const {
	   json_string result = *this;
	   result += other;
	   return result;
    }

    void reserve(size_t){};  //noop, its just a test
    void clear(void){setToCStr("", 0);}

    json_string substr(size_t pos = 0, size_t n = npos) const {
	   json_string res(false, false, false);
	   if (n > len) n = len;
	   if (n + pos > len) n = len - pos;
	   res.setToCStr(str + pos, n);
	   res.str[n] = L'\0';
	   return res;
    }


    size_t find ( mychar c, size_t pos = 0 ) const {
	   if (pos > len) return npos;
	   for(mychar * i = str + pos; *i; ++i){
		  if (*i == c) return i - str;
	   }
	   return npos;
    }

    size_t find_first_not_of ( const mychar* s, size_t pos = 0 ) const {
	   if (pos > len) return npos;
	   for(mychar * i = str + pos; *i; ++i){
		  bool found = false;
		  for(const mychar * k = s; *k; ++k){
			 if (*i == *k){
				found = true;
				break;
			 }
		  }
		  if (!found) return i - str;
	   }
	   return npos;
    }

    size_t find_first_of ( const mychar* s, size_t pos = 0 ) const {
	   if (pos > len) return npos;
	   for(mychar * i = str + pos; *i; ++i){
		  for(const mychar * k = s; *k; ++k){
			 if (*i == *k){
				return i - str;
			 }
		  }
	   }
	   return npos;
    }

    iterator erase(iterator it, iterator it2){
	   size_t mov = it2.it - it.it;
	   std::memmove(str, it2.it, (len - mov + 1) * sizeof(mychar));  //+1 for null terminator
	   len -= mov;
	   return it;
    }
private:
    json_string(bool, bool, bool) : len(0), str(0){};

    void setToCStr(const mychar * st, size_t l){
	   len = l;
	   str = (mychar*)std::memcpy(std::malloc((len + 1) * sizeof(mychar)), st, (len + 1) * sizeof(mychar));
    }

    size_t len;
    mychar * str;

};

#endif
