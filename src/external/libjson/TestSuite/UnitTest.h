#ifndef _TEST_SUITE_H_
#define _TEST_SUITE_H_

#include <string>
#include <sstream>
#include <cstring>

#ifdef __GNUC__
    #define TEST_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
    #if (TEST_GCC_VERSION >= 29600)
	   #define test_likely(x) __builtin_expect((long)((bool)(x)),1)
	   #define test_unlikely(x) __builtin_expect((long)((bool)(x)),0)
    #else
	   #define test_likely(x) x
	   #define test_unlikely(x) x
    #endif
#else
    #define test_likely(x) x
    #define test_unlikely(x) x
#endif

#include <limits>

template <typename T>
union unittest_numUnion {
	unittest_numUnion(T _v) : val(_v){}
	T val;
	unsigned char c[sizeof(T)];
};

template<typename T>
static bool unittest_isNAN(T num){
	unittest_numUnion<T> orig(num);
	
	static unittest_numUnion<T> sig_nan(std::numeric_limits<T>::signaling_NaN());
	
	bool isNAN = true;
	for(size_t i = 0; i < sizeof(T); ++i){
		if (orig.c[i] != sig_nan.c[i]){
			isNAN = false;
			break;
		}
	}
	if (isNAN) return true;
	
	static unittest_numUnion<T> quiet_nan(std::numeric_limits<T>::quiet_NaN());
	
	for(size_t i = 0; i < sizeof(T); ++i){
		if (orig.c[i] != quiet_nan.c[i]){
			return false;
		}
	}
	return true;
}

class UnitTest {
public:
    static void SelfCheck(void);
    static void PushFailure(const std::string & fail);
    static void PushSuccess(const std::string & pass);
    static void echo_(const std::string & out);
    static std::string ToString(void);
    static std::string ToHTML(void);
    static void SaveTo(const std::string & location);
    static void SetReturnOnFail(bool option);
    static bool GetReturnOnFail(void);
    static void SetEcho(bool option);
    static void SetPrefix(const std::string & prefix);
    static std::string GetPrefix(void);
    static void StartTime(void);
	static inline bool _floatsAreEqual(const double & one, const double & two){
		return (one > two) ? (one - two) < .000001 : (one - two) > -.000001;
	}
};

#define MakePre()\
    std::string pre = UnitTest::GetPrefix();\
    if (test_unlikely(pre.empty())){\
        std::stringstream out;\
        out << __FILE__ << ":" << __LINE__;\
        pre = out.str();\
    }\
    pre += ":  ";

#define FAIL(stri)\
    MakePre()\
    UnitTest::PushFailure(pre + std::string(stri));\
    if (UnitTest::GetReturnOnFail()) return;

#define PASS(stri)\
    MakePre();\
    UnitTest::PushSuccess(pre + std::string(stri));\

#define assertUnitTest()\
    UnitTest::SelfCheck();

#define assertTrue(cond)\
    if (test_unlikely(!(cond))){\
	   FAIL(#cond);\
    } else {\
	   PASS(#cond);\
    }

#define assertFalse(cond)\
    if (test_unlikely(cond)){\
	   FAIL(#cond);\
    } else {\
	   PASS(#cond);\
    }

#define assertTrue_Primitive(cond, leftside, rightside)\
    if (test_unlikely(!(cond))){\
	   std::stringstream unit_out;\
	   unit_out << #cond;\
	   unit_out << ", Left side: " << leftside;\
	   unit_out << ", Right side: " << rightside;\
	   FAIL(unit_out.str());\
    } else {\
	   PASS(#cond);\
    }

//needs to copy it so that if its a function call it only does it once
#define assertNAN(type, one)\
	{\
		type val = (type)one;\
		std::string lag(#one);\
		lag += " not a number";\
		if (test_likely(unittest_isNAN<type>(one))){\
			PASS(lag)\
		} else {\
			FAIL(lag)\
		}\
	}

#define assertFloatEquals(one, two)\
	assertTrue(UnitTest::_floatsAreEqual(one, two))

#define assertEquals(one, two)\
    assertTrue((one) == (two))

#define assertNotEquals(one, two)\
    assertTrue((one) != (two))

#define assertGreaterThan(one, two)\
    assertTrue((one) > (two))

#define assertGreaterThanEqualTo(one, two)\
    assertTrue((one) >= (two))

#define assertLessThan(one, two)\
    assertTrue((one) < (two))

#define assertLessThanEqualTo(one, two)\
    assertTrue((one) <= (two))



#define assertEquals_Primitive(one, two)\
    assertTrue_Primitive((one) == (two), one, two)

#define assertNotEquals_Primitive(one, two)\
    assertTrue_Primitive((one) != (two), one, two)

#define assertGreaterThan_Primitive(one, two)\
    assertTrue_Primitive((one) > (two), one, two)

#define assertGreaterThanEqualTo_Primitive(one, two)\
    assertTrue_Primitive((one) >= (two), one, two)

#define assertLessThan_Primitive(one, two)\
    assertTrue_Primitive((one) < (two), one, two)

#define assertLessThanEqualTo_Primitive(one, two)\
    assertTrue_Primitive((one) <= (two), one, two)

#define assertNull(one)\
    assertTrue(one == NULL);

#define assertNotNull(one)\
    assertTrue(one != NULL);

#define assertCStringEquals(one, two)\
    if (test_unlikely(strcmp(one, two))){\
	   FAIL(std::string(#one) + "==" + #two);\
    } else {\
	   PASS(std::string(#one) + "==" + #two);\
    }

#define assertCStringNotEquals(one, two)\
    if (test_unlikely(!strcmp(one, two))){\
	   FAIL(std::string(#one) + "!=" + #two);\
    } else {\
	   PASS(std::string(#one) + "!=" + #two);\
    }

#define assertCStringEqualsW(one, two)\
    if (test_unlikely(wcscmp(one, two))){\
	   FAIL(std::string(#one) + "==" + #two);\
    } else {\
	   PASS(std::string(#one) + "==" + #two);\
    }

#define assertCStringNotEqualsW(one, two)\
    if (test_unlikely(!wcscmp(one, two))){\
	   FAIL(std::string(#one) + "!=" + #two);\
    } else {\
	   PASS(std::string(#one) + "!=" + #two);\
    }

#define assertException(code, exc)\
    {\
	   bool failed = false;\
	   try {\
		  code;\
	   } catch (exc){\
		  PASS(std::string(#exc) + " caught");\
		  failed = true;\
	   }\
	   if (test_unlikely(!failed)){ FAIL(std::string(#exc) + " not caught");}\
    }

#define echo(something)\
    {\
	   std::stringstream somet;\
	   somet << something;\
	   UnitTest::echo_(somet.str());\
    }

#endif
