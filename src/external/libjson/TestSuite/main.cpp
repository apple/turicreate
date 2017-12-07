#include <iostream>
#include <cstdlib> //for malloc, realloc, and free
#include "TestSuite.h"
#include "../libjson.h"

void DoTests(void);
void DoTests(void){
    TestSuite::TestStreams();
    TestSuite::TestValidator();
    TestSuite::TestString();
    TestSuite::TestConverters();
    #ifdef JSON_BINARY
	   TestSuite::TestBase64();
    #endif

    TestSuite::TestReferenceCounting();
    TestSuite::TestConstructors();
    TestSuite::TestAssigning();
    TestSuite::TestEquality();
    TestSuite::TestInequality();
    TestSuite::TestChildren();
    TestSuite::TestFunctions();
    TestSuite::TestIterators();
    TestSuite::TestInspectors();
    TestSuite::TestNamespace();
    #ifdef JSON_WRITE_PRIORITY
	   TestSuite::TestWriter();
    #endif
    #ifdef JSON_COMMENTS
	   TestSuite::TestComments();
    #endif
    #ifdef JSON_MUTEX_CALLBACKS
	   TestSuite::TestMutex();
	   TestSuite::TestThreading();
    #endif
	TestSuite::TestSharedString();
    TestSuite::TestFinal();
}

#ifdef JSON_MEMORY_CALLBACKS
    long mallocs = 0;
    long reallocs = 0;
    long frees = 0;
    long bytes = 0;

    //used to check load
    size_t maxBytes = 0;
    size_t currentBytes = 0;
    #ifdef JSON_LIBRARY
        #define MEMTYPE unsigned long
    #else
        #define MEMTYPE size_t
    #endif
    #include <map>
	#include <vector>
    std::map<void *, MEMTYPE> mem_mapping;
	std::vector<size_t> bytesallocated;

    void * testmal(MEMTYPE siz);
	void * testmal(MEMTYPE siz){ 
        ++mallocs; 
        bytes += (long)siz; 
        currentBytes += siz;
        if (currentBytes > maxBytes) maxBytes = currentBytes;
		bytesallocated.push_back(currentBytes);

        void * res = std::malloc(siz); 
        mem_mapping[res] = siz;
        return res;
     }

    void testfree(void * ptr);
    void testfree(void * ptr){ 
        ++frees; 

        std::map<void *, MEMTYPE>::iterator i = mem_mapping.find(ptr);
        if (i != mem_mapping.end()){  //globals
            currentBytes -= mem_mapping[ptr];
            mem_mapping.erase(ptr);
        }
		
		bytesallocated.push_back(currentBytes);

        std::free(ptr); 
    }

	void * testreal(void * ptr, MEMTYPE siz);
	void * testreal(void * ptr, MEMTYPE siz){ 
        ++reallocs; 

        std::map<void *, MEMTYPE>::iterator i = mem_mapping.find(ptr);
        if (i != mem_mapping.end()){  //globals
            currentBytes -= mem_mapping[ptr];
            mem_mapping.erase(ptr);
        }
        currentBytes += siz;
        if (currentBytes > maxBytes) maxBytes = currentBytes;
		bytesallocated.push_back(currentBytes);
        

        void * res = std::realloc(ptr, siz); 
        mem_mapping[res] = siz;
        return res;
    }

    void doMemTests(void);
    void doMemTests(void){
	   #ifdef JSON_LIBRARY
		  json_register_memory_callbacks(testmal, testreal, testfree);
	   #else
		  libjson::register_memory_callbacks(testmal, testreal, testfree);
	   #endif
	   DoTests();
	   echo("mallocs: " << mallocs);
	   echo("frees: " << frees);
	   echo("reallocs: " << reallocs);
	   echo("bytes: " << bytes  << " (" << (int)(bytes / 1024) << " KB)");
       echo("max bytes at once: " << maxBytes  << " (" << (int)(maxBytes / 1024) << " KB)");
	   std::vector<size_t>::iterator i = bytesallocated.begin();
	   std::vector<size_t>::iterator e = bytesallocated.end();
	   size_t bbytes = 0;
	   for(; i != e; ++i){
			bbytes += *i;
	   }
	   bbytes = (size_t)(((double)bbytes) / ((double)bytesallocated.size()));
	   echo("avg bytes at once: " << bbytes  << " (" << (int)(bbytes / 1024) << " KB)");
	   echo("still allocated: " << currentBytes  << " (" << (int)(currentBytes / 1024) << " KB) (Global variables)");
	   assertEquals(mallocs, frees);
    }
#endif

#include "RunTestSuite2.h"

int main () {	
    UnitTest::StartTime();
	TestSuite::TestSelf();
	
    DoTests();

    #ifdef JSON_MEMORY_CALLBACKS
	   doMemTests();
    #endif

	RunTestSuite2::RunTests();
	
    UnitTest::SaveTo("out.html");

    return 0;
}
