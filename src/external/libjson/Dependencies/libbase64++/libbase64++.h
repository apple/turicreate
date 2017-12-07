#ifndef LIBBASE64_CPP_H
#define LIBBASE64_CPP_H

#include <string>
//#define LIBBASE64_THROW_STD_INVALID_ARGUMENT

//version info
#define __LIBBASE64_MAJOR__ 1
#define __LIBBASE64_MINOR__ 1
#define __LIBBASE64_PATCH__ 0
#define __LIBBASE64_VERSION__ (__LIBBASE64_MAJOR__ * 10000 + __LIBBASE64_MINOR__ * 100 + __LIBBASE64_PATCH__)

//code coverage and asserts
#ifdef NDEBUG
	#define LIBBASE64_ASSERT(cond, msg) (void)0
	#define CREATEBOUNDCHECKER(type, name, ubound, lbound) (void)0
	#define GETITEM_BOUNDCHECK(loc, name) (*(loc))
#else
	#include <iostream>
	#define LIBBASE64_ASSERT(cond, msg) if (!(cond)){ std::cerr << msg << std::endl; throw false; }
	
	template<typename T>
	class libbase64_boundChecker {
	public:
		libbase64_boundChecker(const T * lbound, const T * ubound) : upperbound(ubound), lowerbound(lbound){};
		T getLocation(const T * loc){
			LIBBASE64_ASSERT(loc < upperbound, "Array index above bounds");
			LIBBASE64_ASSERT(loc >= lowerbound, "Array index below bounds");
			return *loc;
		}
	private:
		const T * lowerbound;
		const T * upperbound;
	};
	#define CREATEBOUNDCHECKER(type, name, ubound, lbound) libbase64_boundChecker<type> name(ubound, lbound)
	#define GETITEM_BOUNDCHECK(loc, name) name.getLocation(loc)
	
	#ifdef LIBBASE64CODECOVERAGE
		#define LIBBASE64CODECOVERAGEBRANCH { static bool f_codeCoverage_ = false; if (f_codeCoverage_ == false){ libbase64::getCoverageHits<STRINGTYPE, CHARTYPE, UCHARTYPE, SAFETY>(true); f_codeCoverage_ = true; } }
	#endif
#endif
#ifndef LIBBASE64CODECOVERAGE
	#define LIBBASE64CODECOVERAGEBRANCH (void)0
#endif

//predictive branching optimizations
#ifdef __GNUC__
	#define LIBBASE64_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
	#if (LIBBASE64_GCC_VERSION >= 29600)
		  #define libbase64_likely(x) __builtin_expect((long)((bool)(x)),1)
		  #define libbase64_unlikely(x) __builtin_expect((long)((bool)(x)),0)
	#endif
#endif
#ifndef libbase64_likely
	#define libbase64_likely(x) x
	#define libbase64_unlikely(x) x
#endif


namespace libbase64 {
	#ifdef LIBBASE64CODECOVERAGE  //Gets the number of branches that has been made
		template<class STRINGTYPE, typename CHARTYPE, typename UCHARTYPE, bool SAFETY>
		static size_t getCoverageHits(bool inc){
			static size_t hits = 0;
			if (inc) ++hits;
			return hits;
		}
	#endif
	
	//characters used in convertions
	namespace libbase64_characters {
		template<typename T>
		inline static const T * getChar64(void){
			static const T char64s[64] = {
				(T)'A', (T)'B', (T)'C', (T)'D', (T)'E', (T)'F', (T)'G', (T)'H', (T)'I', (T)'J', (T)'K', (T)'L', (T)'M',
				(T)'N', (T)'O', (T)'P', (T)'Q', (T)'R', (T)'S', (T)'T', (T)'U', (T)'V', (T)'W', (T)'X', (T)'Y', (T)'Z',
				(T)'a', (T)'b', (T)'c', (T)'d', (T)'e', (T)'f', (T)'g', (T)'h', (T)'i', (T)'j', (T)'k', (T)'l', (T)'m',
				(T)'n', (T)'o', (T)'p', (T)'q', (T)'r', (T)'s', (T)'t', (T)'u', (T)'v', (T)'w', (T)'x', (T)'y', (T)'z',
				(T)'0', (T)'1', (T)'2', (T)'3', (T)'4', (T)'5', (T)'6', (T)'7', (T)'8', (T)'9', (T)'+', (T)'/'
			};
			return char64s;
		}
		
		template<typename T>
		inline static T getChar(unsigned char bin){
			CREATEBOUNDCHECKER(T, char64bounds, getChar64<T>(), getChar64<T>() + 64);
			return GETITEM_BOUNDCHECK(getChar64<T>() + bin, char64bounds);
		}
		
		template<typename T>
		inline static T toBinary(T c) {
			static T binaryConvert[80] = {62,48,49,50,63,52,53,54,55,56,57,58,59,60,61,249,250,251,252,253,254,255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
			CREATEBOUNDCHECKER(T, binaryConvertsbounds, binaryConvert, binaryConvert + 80);
			return GETITEM_BOUNDCHECK(binaryConvert + c - 43, binaryConvertsbounds);
		}
		
		template<typename T>
		static inline T & emptyString(void){
			static T t;
			return t;
		}
	}

	namespace libbase64_Calculator {
		inline static size_t getEncodingSize(size_t bytes){
			return (bytes + 2 - ((bytes + 2) % 3)) / 3 * 4;
		}
		inline static size_t getDecodingSize(size_t res){
			return res * 3 / 4;
		}
	}


	/**
	 *	Encodes data into a base64 string of STRINGTYPE
	 */
	template<class STRINGTYPE, typename CHARTYPE, typename UCHARTYPE, bool SAFETY>
	static STRINGTYPE encode(const unsigned char * binary, size_t bytes){
		CREATEBOUNDCHECKER(unsigned char, binarybounds, binary, binary + bytes);	
	
		//make sure that there is actually something to encode
		if (SAFETY){
			if (libbase64_unlikely(bytes == 0)){
				LIBBASE64CODECOVERAGEBRANCH;
				return libbase64_characters::emptyString<STRINGTYPE>();
			}
		}
		
		//calculate length and how misaligned it is
		size_t misaligned = bytes % 3;
		STRINGTYPE result;
		result.reserve(libbase64_Calculator::getEncodingSize(bytes));
		
		//do all of the ones that are 3 byte aligned
		for (size_t i = 0, aligned((bytes - misaligned) / 3); i < aligned; ++i){
			LIBBASE64CODECOVERAGEBRANCH;
			result += libbase64_characters::getChar<CHARTYPE>((GETITEM_BOUNDCHECK(binary, binarybounds) & 0xFC) >> 2);
			result += libbase64_characters::getChar<CHARTYPE>(((GETITEM_BOUNDCHECK(binary, binarybounds) & 0x03) << 4) + ((GETITEM_BOUNDCHECK(binary + 1, binarybounds) & 0xF0) >> 4));
			result += libbase64_characters::getChar<CHARTYPE>(((GETITEM_BOUNDCHECK(binary + 1, binarybounds) & 0x0F) << 2) + ((GETITEM_BOUNDCHECK(binary + 2, binarybounds) & 0xC0) >> 6));
			result += libbase64_characters::getChar<CHARTYPE>(GETITEM_BOUNDCHECK(binary + 2, binarybounds) & 0x3F);
			binary += 3;
		}
		
		//handle any additional characters at the end of it
		if (libbase64_likely(misaligned != 0)){
			LIBBASE64CODECOVERAGEBRANCH;
			//copy the rest into a temporary buffer, need it for the null terminators
			unsigned char temp[3] = { '\0', '\0', '\0' };
			for (unsigned char i = 0; i < (unsigned char)misaligned; ++i){
				LIBBASE64CODECOVERAGEBRANCH;
				temp[i] = GETITEM_BOUNDCHECK(binary++, binarybounds);
			}
			
			//now do the final three bytes
			result += libbase64_characters::getChar<CHARTYPE>((temp[0] & 0xFC) >> 2);
			result += libbase64_characters::getChar<CHARTYPE>(((temp[0] & 0x03) << 4) + ((temp[1] & 0xF0) >> 4));
			if (misaligned == 2){
				LIBBASE64CODECOVERAGEBRANCH;
				result += libbase64_characters::getChar<CHARTYPE>(((temp[1] & 0x0F) << 2) + ((temp[2] & 0xC0) >> 6));
			} else {
				LIBBASE64CODECOVERAGEBRANCH;
				result += (CHARTYPE)'=';
			}
			result += (CHARTYPE)'=';
		} else {
			LIBBASE64CODECOVERAGEBRANCH;
		}
		
		LIBBASE64_ASSERT(libbase64_Calculator::getEncodingSize(bytes) == result.length(), "Reserve wasn't the correct guess");
		return result;
	}	
		
	template<class STRINGTYPE, typename CHARTYPE, typename UCHARTYPE, bool SAFETY>
    static std::string decode(const STRINGTYPE & encoded){
		//check length to be sure its acceptable for base64
		const size_t length = encoded.length();
		
		if (SAFETY){
			if (libbase64_unlikely((length % 4) != 0)){
				LIBBASE64CODECOVERAGEBRANCH;
				return libbase64_characters::emptyString<std::string>();
			}
			if (libbase64_unlikely(length == 0)){
				LIBBASE64CODECOVERAGEBRANCH;
				return libbase64_characters::emptyString<std::string>();
			}
			
			//check to be sure there aren't odd characters or characters in the wrong places
			size_t pos = encoded.find_first_not_of(libbase64_characters::getChar64<CHARTYPE>());
			if (libbase64_unlikely(pos != STRINGTYPE::npos)){
				LIBBASE64CODECOVERAGEBRANCH;
				if (libbase64_unlikely(encoded[pos] != (CHARTYPE)'=')){
					LIBBASE64CODECOVERAGEBRANCH;  //INVALID_CHAR
					#ifdef LIBBASE64_THROW_STD_INVALID_ARGUMENT
						throw std::invalid_argument("invalid character in base64");
					#else
						return libbase64_characters::emptyString<std::string>();
					#endif
				}
				if (pos != length - 1){
					LIBBASE64CODECOVERAGEBRANCH;
					if (libbase64_unlikely(pos != length - 2)){
						LIBBASE64CODECOVERAGEBRANCH; //EQUAL_WRONG_PLACE
						#ifdef LIBBASE64_THROW_STD_INVALID_ARGUMENT
							throw std::invalid_argument("equal sign in wrong place in base64");
						#else
							return libbase64_characters::emptyString<std::string>();
						#endif
					}
					if (libbase64_unlikely(encoded[pos + 1] != (CHARTYPE)'=')){
						LIBBASE64CODECOVERAGEBRANCH;  //EQUAL_NOT_LAST
						#ifdef LIBBASE64_THROW_STD_INVALID_ARGUMENT
							throw std::invalid_argument("invalid character in base64");
						#else
							return libbase64_characters::emptyString<std::string>();
						#endif
					}
					LIBBASE64CODECOVERAGEBRANCH;
				} else {
					LIBBASE64CODECOVERAGEBRANCH;
				}
			} else {
				LIBBASE64CODECOVERAGEBRANCH;
			}
		}
		
		const CHARTYPE * runner = encoded.data();
		const CHARTYPE * end = runner + encoded.length();
		CREATEBOUNDCHECKER(CHARTYPE, encodedbounds, runner, end);
		size_t aligned = length / 4; //don't do the last ones as they might be = padding
		std::string result;
		--aligned;
		result.reserve(libbase64_Calculator::getDecodingSize(length));
		
		//first do the ones that can not have any padding
		for (unsigned int i = 0; i < aligned; ++i){
			const CHARTYPE second = libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner + 1, encodedbounds));
			const CHARTYPE third = libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner + 2, encodedbounds));
			result += (libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner, encodedbounds)) << 2) + ((second & 0x30) >> 4);
			result += ((second & 0xf) << 4) + ((third & 0x3c) >> 2);
			result += ((third & 0x3) << 6) + libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner + 3, encodedbounds));
			runner += 4;
		}
		
		//now do the ones that might have padding, the first two characters can not be padding, so do them quickly
		const CHARTYPE second = libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner + 1, encodedbounds));
		result += (libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner + 0, encodedbounds)) << 2) + ((second & 0x30) >> 4);
		runner += 2;
		if ((runner != end) && (*runner != (CHARTYPE)'=')){  //not two = pads
			LIBBASE64CODECOVERAGEBRANCH;
			const CHARTYPE third = libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner, encodedbounds));
			result += ((second & 0xf) << 4) + ((third & 0x3c) >> 2);
			++runner;
			if ((runner != end) && (*runner != (CHARTYPE)'=')){  //no padding
				LIBBASE64CODECOVERAGEBRANCH;
				result += ((third & 0x3) << 6) + libbase64_characters::toBinary<UCHARTYPE>(GETITEM_BOUNDCHECK(runner, encodedbounds));
			} else {
				LIBBASE64CODECOVERAGEBRANCH;
			}
		} else {
			LIBBASE64CODECOVERAGEBRANCH;
		}
		
		LIBBASE64_ASSERT(libbase64_Calculator::getDecodingSize(length) >= result.length(), "Reserve wasn't the correct guess, too small");
		LIBBASE64_ASSERT((result.length() <= 3) || (libbase64_Calculator::getDecodingSize(length) > result.length() - 3), "Reserve wasn't the correct guess, too big");
		return result;
	}
}

#endif
