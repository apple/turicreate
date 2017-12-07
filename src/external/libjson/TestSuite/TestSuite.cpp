#include "TestSuite.h"
#include "../Source/JSONNode.h"

#ifndef JSON_STDERROR
    #ifdef JSON_DEBUG
	   #ifdef JSON_LIBRARY
	   static void callback(const json_char * msg_c){
		  json_string msg(msg_c);
	   #else
	   static void callback(const json_string & msg){
	   #endif
		  #ifdef JSON_STRING_HEADER
			 echo("callback triggered, but can't display string");
		  #else
			 #ifdef JSON_UNICODE
				const std::string res = std::string(msg.begin(), msg.end());
				echo(res);
			 #else
				echo(msg);
			 #endif
		  #endif
	   }
    #endif
#endif

void TestSuite::TestSelf(void){
    UnitTest::SetPrefix("TestSuite.cpp - Self Test");
    #ifndef JSON_STDERROR
	   #ifdef JSON_DEBUG
		  #ifdef JSON_LIBRARY
			 json_register_debug_callback(callback);
		  #else
			 libjson::register_debug_callback(callback);
		  #endif
	   #endif
    #endif
    assertUnitTest();

    #if defined(JSON_SAFE) && ! defined(JSON_LIBRARY)
	   bool temp = false;
	   JSON_ASSERT_SAFE(true, JSON_TEXT(""), temp = true;);
	   assertFalse(temp);
	   JSON_ASSERT_SAFE(false, JSON_TEXT(""), temp = true;);
	   assertTrue(temp);

	   temp = false;
	   JSON_FAIL_SAFE(JSON_TEXT(""), temp = true;);
	   assertTrue(temp);
    #endif

    echo("If this fails, then edit JSON_INDEX_TYPE in JSONOptions.h");
    assertLessThanEqualTo(sizeof(json_index_t), sizeof(void*));
}

//makes sure that libjson didn't leak memory somewhere
void TestSuite::TestFinal(void){
    #ifdef JSON_UNIT_TEST
	   UnitTest::SetPrefix("TestSuite.cpp - Memory Leak");
	   echo("Node allocations: " << JSONNode::getNodeAllocationCount());
	   echo("Node deallocations: " << JSONNode::getNodeDeallocationCount());
	   assertEquals(JSONNode::getNodeAllocationCount(), JSONNode::getNodeDeallocationCount());

	   echo("internal allocations: " << JSONNode::getInternalAllocationCount());
	   echo("internal deallocations: " << JSONNode::getInternalDeallocationCount());
	   assertEquals(JSONNode::getInternalAllocationCount(), JSONNode::getInternalDeallocationCount());

	   echo("children allocations: " << JSONNode::getChildrenAllocationCount());
	   echo("children deallocations: " << JSONNode::getChildrenDeallocationCount());
	   assertEquals(JSONNode::getChildrenAllocationCount(), JSONNode::getChildrenDeallocationCount());

		#if defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
		   echo("stl allocations: " << JSONAllocatorRelayer::getAllocationCount());
		   echo("stl deallocations: " << JSONAllocatorRelayer::getDeallocationCount());
		   echo("stl bytes: " << JSONAllocatorRelayer::getAllocationByteCount());
		   assertEquals(JSONAllocatorRelayer::getAllocationCount(), JSONAllocatorRelayer::getDeallocationCount());
		#endif
    #endif
}
