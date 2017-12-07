#include "JSONChildren.h"
#include "JSONNode.h"

#ifdef JSON_UNIT_TEST
    void jsonChildren::addAllocCount(void){ JSONNode::incChildrenAllocCount();}
    void jsonChildren::subAllocCount(void){ JSONNode::decChildrenAllocCount();}
#endif

/*
 *	reserves a certain number of bytes, in memory saving mode it creates a special
 *	type of child container that will not autoshrink
 */
void jsonChildren::reserve2(jsonChildren *& mine, json_index_t amount) json_nothrow {
    if (mine -> array != 0){
	   if (mine -> mycapacity < amount){
		  mine -> inc(amount - mine -> mycapacity);
		  #ifdef JSON_LESS_MEMORY
			 mine = jsonChildren_Reserved::newChildren_Reserved(mine, amount);
		  #endif
	   }
    } else {
	   mine -> reserve(amount);
    }
}

void jsonChildren::inc(void) json_nothrow {
    JSON_ASSERT(this != 0, JSON_TEXT("Children is null inc"));
    if (json_unlikely(mysize == mycapacity)){  //it's full
	   if (json_unlikely(mycapacity == 0)){  //the array hasn't been created yet
		  JSON_ASSERT(!array, JSON_TEXT("Expanding a 0 capacity array, but not null"));
		  #ifdef JSON_LESS_MEMORY
			 array = json_malloc<JSONNode*>(1);
			 mycapacity = 1;
		  #else
			 array = json_malloc<JSONNode*>(8);  //8 seems average for JSON, and it's only 64 bytes
			 mycapacity = 8;
		  #endif
	   } else {
		  #ifdef JSON_LESS_MEMORY
			 mycapacity += 1;  //increment the size of the array
		  #else
			 mycapacity <<= 1;  //double the size of the array
		  #endif
		  array = json_realloc<JSONNode*>(array, mycapacity);
	   }
    }
}


void jsonChildren::inc(json_index_t amount) json_nothrow {
    JSON_ASSERT(this != 0, JSON_TEXT("Children is null inc(amount)"));
    if (json_unlikely(amount == 0)) return;
    if (json_likely(mysize + amount >= mycapacity)){  //it's full
	   if (json_unlikely(mycapacity == 0)){  //the array hasn't been created yet
		  JSON_ASSERT(!array, JSON_TEXT("Expanding a 0 capacity array, but not null"));
		  #ifdef JSON_LESS_MEMORY
			 array = json_malloc<JSONNode*>(amount);
			 mycapacity = amount;
		  #else
			 array = json_malloc<JSONNode*>(amount > 8 ? amount : 8);  //8 seems average for JSON, and it's only 64 bytes
			 mycapacity = amount > 8 ? amount : 8;
		  #endif
	   } else {
		  #ifdef JSON_LESS_MEMORY
			 mycapacity = mysize + amount;  //increment the size of the array
		  #else
			 while(mysize + amount > mycapacity){
				mycapacity <<= 1;  //double the size of the array
			 }
		  #endif
		  array = json_realloc<JSONNode*>(array, mycapacity);
	   }
    }
}

//actually deletes everything within the vector, this is safe to do on an empty or even a null array
void jsonChildren::deleteAll(void) json_nothrow {
    JSON_ASSERT(this != 0, JSON_TEXT("Children is null deleteAll"));
    json_foreach(this, runner){
        JSON_ASSERT(*runner != JSON_TEXT('\0'), JSON_TEXT("a null pointer within the children"));
	   JSONNode::deleteJSONNode(*runner);  //this is why I can't do forward declaration
    }
}

void jsonChildren::doerase(JSONNode ** position, json_index_t number) json_nothrow {
    JSON_ASSERT(this != 0, JSON_TEXT("Children is null doerase"));
    JSON_ASSERT(array != 0, JSON_TEXT("erasing something from a null array 2"));
    JSON_ASSERT(position >= array, JSON_TEXT("position is beneath the start of the array 2"));
    JSON_ASSERT(position + number <= array + mysize, JSON_TEXT("erasing out of bounds 2"));
    if (position + number >= array + mysize){
	   mysize = (json_index_t)(position - array);
	   #ifndef JSON_ISO_STRICT
		  JSON_ASSERT((long long)position - (long long)array >= 0, JSON_TEXT("doing negative allocation"));
	   #endif
    } else {
	   std::memmove(position, position + number, (mysize - (position - array) - number) * sizeof(JSONNode *));
	   mysize -= number;
    }
}
