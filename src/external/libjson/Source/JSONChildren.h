#ifndef JSONCHILDREN_H
#define JSONCHILDREN_H

#include "JSONMemory.h"
#include "JSONDebug.h"  //for JSON_ASSERT macro

#ifdef JSON_LESS_MEMORY
    #ifdef __GNUC__
	   #pragma pack(push, 1)
    #elif _MSC_VER
	   #pragma pack(push, jsonChildren, 1)
    #endif
#endif

#define json_foreach(chldrn, itrtr)\
    JSONNode ** itrtr = chldrn -> begin();\
    for(JSONNode ** itrtr##_end = chldrn -> end(); itrtr != itrtr##_end; ++itrtr)

/*
 This class is essentially a vector that has been heavily optimized for the specific purpose
 of holding JSONNode children.  It acts the same way as a vector, it has a automatically
 expanding array.  On destruction, this container automatically destroys everything contained
 in it as well, so that you libjson doesn't have to do that.

 T is JSONNode*, I can't define it that way directly because JSONNode uses this container, and because
 the container deletes the children automatically, forward declaration can't be used
 */

class JSONNode;  //forward declaration

#ifdef JSON_LESS_MEMORY
    #define childrenVirtual virtual
#else
    #define childrenVirtual
#endif

#ifndef JSON_UNIT_TEST
    #define addAllocCount() (void)0
    #define subAllocCount() (void)0
#endif

class jsonChildren {
public:
    //starts completely empty and the array is not allocated
    jsonChildren(void) json_nothrow : array(0), mysize(0), mycapacity(0) {
	   addAllocCount();
    }

    #ifdef JSON_LESS_MEMORY
	   jsonChildren(JSONNode** ar, json_index_t si, json_index_t ca) json_nothrow : array(ar), mysize(si), mycapacity(ca) {
		  addAllocCount();
	   }
    #endif

    //deletes the array and everything that is contained within it (using delete)
    childrenVirtual ~jsonChildren(void) json_nothrow {
	   if (json_unlikely(array != 0)){  //the following function calls are safe, but take more time than a check here
		  deleteAll();
		  libjson_free<JSONNode*>(array);
	   }
	   subAllocCount();
    }

    #ifdef JSON_UNIT_TEST
	   void addAllocCount(void);
	   void subAllocCount(void);
    #endif

    //increase the size of the array
    void inc(json_index_t amount) json_nothrow;
    void inc(void) json_nothrow;

    //Adds something to the vector, doubling the array if necessary
    void push_back(JSONNode * item) json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null push_back"));
	   inc();
	   array[mysize++] = item;
    }

    //Adds something to the front of the vector, doubling the array if necessary
    void push_front(JSONNode * item) json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null push_front"));
	   inc();
	   std::memmove(array + 1, array, mysize++ * sizeof(JSONNode *));
	   array[0] = item;
    }

    //gets an item out of the vector by it's position
    inline JSONNode * operator[] (json_index_t position) const json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null []"));
	   JSON_ASSERT(position < mysize, JSON_TEXT("Using [] out of bounds"));
	   JSON_ASSERT(position < mycapacity, JSON_TEXT("Using [] out of bounds"));
	   JSON_ASSERT(array != 0, JSON_TEXT("Array is null"));
	   return array[position];
    }

    //returns the allocated capacity, but keep in mind that some might not be valid
    inline json_index_t capacity() const json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null capacity"));
	   return mycapacity;
    }

    //returns the number of valid objects within the vector
    inline json_index_t size() const json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null size"));
	   return mysize;
    }

    //tests whether or not the vector is empty
    inline bool empty() const json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null empty"));
	   return mysize == 0;
    }

    //clears (and deletes) everything from the vector and sets it's size to 0
    inline void clear() json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null clear"));
	   if (json_likely(array != 0)){  //don't bother clearing anything if there is nothing in it
		  JSON_ASSERT(mycapacity != 0, JSON_TEXT("mycapacity is not zero, but array is null"));
		  deleteAll();
		  mysize = 0;
	   }
	   JSON_ASSERT(mysize == 0, JSON_TEXT("mysize is not zero after clear"));
    }

    //returns the beginning of the array
    inline JSONNode ** begin(void) const json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null begin"));
	   return array;
    }

    //returns the end of the array
    inline JSONNode ** end(void) const json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null end"));
	   return array + mysize;
    }

    //makes sure that even after shirnking and expanding, the iterator is in same relative position
	template <bool reverse>
    struct iteratorKeeper {
    public:
	  iteratorKeeper(jsonChildren * pthis, JSONNode ** & position) json_nothrow :
		 myRelativeOffset(reverse ? (json_index_t)(pthis -> array + (size_t)pthis -> mysize - position) : (json_index_t)(position - pthis -> array)),
		 myChildren(pthis),
		 myPos(position){}

	   ~iteratorKeeper(void) json_nothrow {
		 if (reverse){
			myPos = myChildren -> array + myChildren -> mysize - myRelativeOffset;
		 } else {
			myPos = myChildren -> array + myRelativeOffset;
		 }
	   }
    private:
	   iteratorKeeper(const iteratorKeeper &);
	   iteratorKeeper & operator = (const iteratorKeeper &);

	   json_index_t myRelativeOffset;
	   jsonChildren * myChildren;
	   JSONNode ** & myPos;
    };

    //This function DOES NOT delete the item it points to
    inline void erase(JSONNode ** & position) json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null erase"));
	   JSON_ASSERT(array != 0, JSON_TEXT("erasing something from a null array 1"));
	   JSON_ASSERT(position >= array, JSON_TEXT("position is beneath the start of the array 1"));
	   JSON_ASSERT(position <= array + mysize, JSON_TEXT("erasing out of bounds 1"));
	   std::memmove(position, position + 1, (mysize-- - (position - array) - 1) * sizeof(JSONNode *));
	   iteratorKeeper<false> ik(this, position);
	   shrink();
    }

    //This function DOES NOT delete the item it points to
    inline void erase(JSONNode ** & position, json_index_t number) json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null erase 2"));
	   doerase(position, number);
	   iteratorKeeper<false> ik(this, position);
	   shrink();
    }


    //This function DOES NOT delete the item it points to
    inline void erase(JSONNode ** position, json_index_t number, JSONNode ** & starter) json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null erase 3"));
	   doerase(position, number);
	   iteratorKeeper<false> ik(this, starter);
	   shrink();
    }

    #ifdef JSON_LIBRARY
	   void insert(JSONNode ** & position, JSONNode * item) json_nothrow{
    #else
	   void insert(JSONNode ** & position, JSONNode * item, bool reverse = false) json_nothrow {
    #endif
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null insert"));
	   //position isnt relative to array because of realloc
	   JSON_ASSERT(position >= array, JSON_TEXT("position is beneath the start of the array insert 1"));
	   JSON_ASSERT(position <= array + mysize, JSON_TEXT("position is above the end of the array insert 1"));
		#ifndef JSON_LIBRARY
		if (reverse){
			iteratorKeeper<true> ik(this, position);
			inc();
		} else 
		#endif
		{
			iteratorKeeper<false> ik(this, position);
			inc();
		}

	   std::memmove(position + 1, position, (mysize++ - (position - array)) * sizeof(JSONNode *));
	   *position = item;
    }

    void insert(JSONNode ** & position, JSONNode ** items, json_index_t num) json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null insert 2"));
	   JSON_ASSERT(position >= array, JSON_TEXT("position is beneath the start of the array insert 2"));
	   JSON_ASSERT(position <= array + mysize, JSON_TEXT("position is above the end of the array insert 2"));
	   {
		  iteratorKeeper<false> ik(this, position);
		  inc(num);
	   }
	   const size_t ptrs = ((JSONNode **)(array + mysize)) - position;
	   std::memmove(position + num, position, ptrs * sizeof(JSONNode *));
	   std::memcpy(position, items, num * sizeof(JSONNode *));
	   mysize += num;
    }

    inline void reserve(json_index_t amount) json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null reserve"));
	   JSON_ASSERT(array == 0, JSON_TEXT("reserve is not meant to expand a preexisting array"));
	   JSON_ASSERT(mycapacity == 0, JSON_TEXT("reservec is not meant to expand a preexisting array"));
	   JSON_ASSERT(mysize == 0, JSON_TEXT("reserves is not meant to expand a preexisting array"));
	   array = json_malloc<JSONNode*>(mycapacity = amount);
    }

	//it is static because mine might change pointers entirely
    static void reserve2(jsonChildren *& mine, json_index_t amount) json_nothrow;

    //shrinks the array to only as large as it needs to be to hold everything within it
    inline childrenVirtual void shrink() json_nothrow {
	   JSON_ASSERT(this != 0, JSON_TEXT("Children is null shrink"));
	   if (json_unlikely(mysize == 0)){  //size is zero, we should completely free the array
		  libjson_free<JSONNode*>(array);  //free does checks for a null pointer, so don't bother checking
		  array = 0;
	   #ifdef JSON_LESS_MEMORY
		  } else {  //need to shrink it, using realloc
			 JSON_ASSERT(array != 0, JSON_TEXT("shrinking a null array that is not size 0"));
			 array = json_realloc<JSONNode*>(array, mysize);
	   #endif
	   }
	   mycapacity = mysize;
    }


    inline static void deleteChildren(jsonChildren * ptr) json_nothrow {
	   #ifdef JSON_MEMORY_CALLBACKS
		  ptr -> ~jsonChildren();
		  libjson_free<jsonChildren>(ptr);
	   #else
		  delete ptr;
	   #endif
    }

    inline static jsonChildren * newChildren(void) {
	   #ifdef JSON_MEMORY_CALLBACKS
		  return new(json_malloc<jsonChildren>(1)) jsonChildren();
	   #else
		  return new jsonChildren();
	   #endif
    }

    JSONNode ** array;  //the expandable array

    json_index_t mysize;	     //the number of valid items
    json_index_t mycapacity;   //the number of possible items
JSON_PROTECTED
    //to make sure it's not copyable
    jsonChildren(const jsonChildren &);
    jsonChildren & operator = (const jsonChildren &);

    void deleteAll(void) json_nothrow json_hot;  //implemented in JSONNode.cpp
    void doerase(JSONNode ** position, json_index_t number) json_nothrow;
};

#ifdef JSON_LESS_MEMORY
    class jsonChildren_Reserved : public jsonChildren {
    public:
	   jsonChildren_Reserved(jsonChildren * orig, json_index_t siz) json_nothrow : jsonChildren(orig -> array, orig -> mysize, orig -> mycapacity), myreserved(siz) {
		  orig -> array = 0;
		  deleteChildren(orig);
		  addAllocCount();
	   }
	   jsonChildren_Reserved(const jsonChildren_Reserved & orig) json_nothrow  : jsonChildren(orig.array, orig.mysize, orig.mycapacity), myreserved(orig.myreserved){
		  addAllocCount();
	   }
	   inline virtual ~jsonChildren_Reserved() json_nothrow {
		  subAllocCount();
	   };
	   inline virtual void shrink() json_nothrow {
		  JSON_ASSERT(this != 0, JSON_TEXT("Children is null shrink reserved"));
		  if (json_unlikely(mysize == 0)){  //size is zero, we should completely free the array
			 libjson_free<JSONNode*>(array);  //free does checks for a null pointer, so don't bother checking
			 array = 0;
		  } else if (mysize > myreserved){
			 JSON_ASSERT(array != 0, JSON_TEXT("shrinking a null array that is not size 0"));
			 array = json_realloc<JSONNode*>(array, mysize);
		  }
	   }

	   #ifdef JSON_LESS_MEMORY
		  inline static jsonChildren * newChildren_Reserved(jsonChildren * orig, json_index_t siz) json_nothrow {
			 #ifdef JSON_MEMORY_CALLBACKS
				return new(json_malloc<jsonChildren_Reserved>(1)) jsonChildren_Reserved(orig, siz);
			 #else
				return new jsonChildren_Reserved(orig, siz);
			 #endif
		  }
	   #endif
    JSON_PRIVATE
	   jsonChildren_Reserved & operator = (const jsonChildren_Reserved &);
	   json_index_t myreserved;
    };
#endif

#ifdef JSON_LESS_MEMORY
    #ifdef __GNUC__
	   #pragma pack(pop)
    #elif _MSC_VER
	   #pragma pack(pop, jsonChildren)
    #endif
#endif
#endif
