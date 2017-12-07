#ifndef LIBJSON_H
#define LIBJSON_H

#include "Source/JSONDefs.h"  //for typedefs of functions, strings, and nodes

/*
    This is the C interface to libjson.

    This file also declares various things that are needed for
    C++ programming
*/

#ifdef JSON_LIBRARY  //compiling the library, hide the interface
    #ifdef __cplusplus
	   extern "C" {
    #endif

		  #ifdef JSON_NO_C_CONSTS
			 /* The interface has no consts in it, but ther must be const_cast internally */
			 #define json_const
			 #define TOCONST_CSTR(x) const_cast<const json_char *>(x)
		  #else
			 #define json_const const
			 #define TOCONST_CSTR(x) x
		  #endif

		  /*
			 stuff that's in namespace libjson
		  */
		  void json_free(void * str);
		  void json_delete(JSONNODE * node);
		  #ifdef JSON_MEMORY_MANAGE
			 void json_free_all(void);
			 void json_delete_all(void);
		  #endif
		  #ifdef JSON_READ_PRIORITY
			 JSONNODE * json_parse(json_const json_char * json);
			 JSONNODE * json_parse_unformatted(json_const json_char * json);
		  #endif
		  json_char * json_strip_white_space(json_const json_char * json);
		  #ifdef JSON_VALIDATE
			 #ifdef JSON_DEPRECATED_FUNCTIONS
				JSONNODE * json_deprecated(json_validate(json_const json_char * json), "json_validate is deprecated, use json_is_valid and json_parse instead");
			 #endif
			 json_bool_t json_is_valid(json_const json_char * json);
			 json_bool_t json_is_valid_unformatted(json_const json_char * json);
		  #endif
		  #if defined JSON_DEBUG && !defined JSON_STDERROR
			 /* When libjson errors, a callback allows the user to know what went wrong */
			 void json_register_debug_callback(json_error_callback_t callback);
		  #endif
		  #ifdef JSON_MUTEX_CALLBACKS
			 #ifdef JSON_MUTEX_MANAGE
				void json_register_mutex_callbacks(json_mutex_callback_t lock, json_mutex_callback_t unlock, json_mutex_callback_t destroy, void * manager_lock);
			 #else
				void json_register_mutex_callbacks(json_mutex_callback_t lock, json_mutex_callback_t unlock, void * manager_lock);
			 #endif
			 void json_set_global_mutex(void * mutex);
			 void json_set_mutex(JSONNODE * node, void * mutex);
			 void json_lock(JSONNODE * node, int threadid);
			 void json_unlock(JSONNODE * node, int threadid);
		  #endif
		  #ifdef JSON_MEMORY_CALLBACKS
			 void json_register_memory_callbacks(json_malloc_t mal, json_realloc_t real, json_free_t fre);
		  #endif

		  #ifdef JSON_STREAM
			 JSONSTREAM * json_new_stream(json_stream_callback_t callback, json_stream_e_callback_t e_callback, void * identifier);
			 void json_stream_push(JSONSTREAM * stream, json_const json_char * addendum);
			 void json_delete_stream(JSONSTREAM * stream);
		     void json_stream_reset(JSONSTREAM * stream);
		  #endif


		  /*
			 stuff that's in class JSONNode
		   */
		  /* ctors */
		  JSONNODE * json_new_a(json_const json_char * name, json_const json_char * value);
		  JSONNODE * json_new_i(json_const json_char * name, json_int_t value);
		  JSONNODE * json_new_f(json_const json_char * name, json_number value);
		  JSONNODE * json_new_b(json_const json_char * name, json_bool_t value);
		  JSONNODE * json_new(char type);
		  JSONNODE * json_copy(json_const JSONNODE * orig);
		  JSONNODE * json_duplicate(json_const JSONNODE * orig);

		  /* assignment */
		  void json_set_a(JSONNODE * node, json_const json_char * value);
		  void json_set_i(JSONNODE * node, json_int_t value);
		  void json_set_f(JSONNODE * node, json_number value);
		  void json_set_b(JSONNODE * node, json_bool_t value);
		  void json_set_n(JSONNODE * node, json_const JSONNODE * orig);

		  /* inspectors */
		  char json_type(json_const JSONNODE * node);
		  json_index_t json_size(json_const JSONNODE * node);
		  json_bool_t json_empty(json_const JSONNODE * node);
		  json_char * json_name(json_const JSONNODE * node);
		  #ifdef JSON_COMMENTS
			 json_char * json_get_comment(json_const JSONNODE * node);
		  #endif
		  json_char * json_as_string(json_const JSONNODE * node);
		  json_int_t json_as_int(json_const JSONNODE * node);
		  json_number json_as_float(json_const JSONNODE * node);
		  json_bool_t json_as_bool(json_const JSONNODE * node);
		  #ifdef JSON_CASTABLE
			 JSONNODE * json_as_node(json_const JSONNODE * node);
			 JSONNODE * json_as_array(json_const JSONNODE * node);
		  #endif
		  #ifdef JSON_BINARY
			 void * json_as_binary(json_const JSONNODE * node, unsigned long * size);
		  #endif
		  #ifdef JSON_WRITE_PRIORITY
			 json_char * json_write(json_const JSONNODE * node);
			 json_char * json_write_formatted(json_const JSONNODE * node);
		  #endif

		  /* modifiers */
		  void json_set_name(JSONNODE * node, json_const json_char * name);
		  #ifdef JSON_COMMENTS
			 void json_set_comment(JSONNODE * node, json_const json_char * comment);
		  #endif
		  void json_clear(JSONNODE * node);
		  void json_nullify(JSONNODE * node);
		  void json_swap(JSONNODE * node, JSONNODE * node2);
		  void json_merge(JSONNODE * node, JSONNODE * node2);
		  #if !defined (JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
			 void json_preparse(JSONNODE * node);
		  #endif
		  #ifdef JSON_BINARY
			 void json_set_binary(JSONNODE * node, json_const void * data, unsigned long length);
		  #endif
		  #ifdef JSON_EXPOSE_BASE64
			 json_char * json_encode64(json_const void * binary, json_index_t bytes);
			 void * json_decode64(json_const json_char * text, unsigned long * size);
		  #endif
		  #ifdef JSON_CASTABLE
			 void json_cast(JSONNODE * node, char type);
		  #endif

		  /* children access */
		  void json_reserve(JSONNODE * node, json_index_t siz);
		  JSONNODE * json_at(JSONNODE * node, json_index_t pos);
		  JSONNODE * json_get(JSONNODE * node, json_const json_char * name);
		  #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
			 JSONNODE * json_get_nocase(JSONNODE * node, json_const json_char * name);
			 JSONNODE * json_pop_back_nocase(JSONNODE * node, json_const json_char * name);
		  #endif
		  void json_push_back(JSONNODE * node, JSONNODE * node2);
		  JSONNODE * json_pop_back_at(JSONNODE * node, json_index_t pos);
		  JSONNODE * json_pop_back(JSONNODE * node, json_const json_char * name);
		  #ifdef JSON_ITERATORS
			 JSONNODE_ITERATOR json_find(JSONNODE * node, json_const json_char * name);
			 #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
				JSONNODE_ITERATOR json_find_nocase(JSONNODE * node, json_const json_char * name);
			 #endif
			 JSONNODE_ITERATOR json_erase(JSONNODE * node, JSONNODE_ITERATOR it);
			 JSONNODE_ITERATOR json_erase_multi(JSONNODE * node, JSONNODE_ITERATOR start, JSONNODE_ITERATOR end);
			 JSONNODE_ITERATOR json_insert(JSONNODE * node, JSONNODE_ITERATOR it, JSONNODE * node2);
			 JSONNODE_ITERATOR json_insert_multi(JSONNODE * node, JSONNODE_ITERATOR it, JSONNODE_ITERATOR start, JSONNODE_ITERATOR end);

			 /* iterator functions */
			 JSONNODE_ITERATOR json_begin(JSONNODE * node);
			 JSONNODE_ITERATOR json_end(JSONNODE * node);
		  #endif

		  /* comparison */
		  json_bool_t json_equal(JSONNODE * node, JSONNODE * node2);

    #ifdef __cplusplus
	   }
    #endif
#else
    #ifndef __cplusplus
	   #error Turning off JSON_LIBRARY requires C++
    #endif
    #include "Source/JSONNode.h"  //not used in this file, but libjson.h should be the only file required to use it embedded
    #include "Source/JSONWorker.h"
    #include "Source/JSONValidator.h"
    #include "Source/JSONStream.h"
    #include "Source/JSONPreparse.h"
    #ifdef JSON_EXPOSE_BASE64
	   #include "Source/JSON_Base64.h"
    #endif
    #ifndef JSON_NO_EXCEPTIONS
	   #include <stdexcept>  //some methods throw exceptions
    #endif

	#include <cwchar>  /* need wide characters */
	#include <string>

    namespace libjson {	   
	   #ifdef JSON_EXPOSE_BASE64
		  inline static json_string encode64(const unsigned char * binary, size_t bytes) json_nothrow {
			 return JSONBase64::json_encode64(binary, bytes);
		  }

		  inline static std::string decode64(const json_string & encoded) json_nothrow {
			 return JSONBase64::json_decode64(encoded);
		  }
	   #endif
	   
	   //useful if you have json that you don't want to parse, just want to strip to cut down on space
	   inline static json_string strip_white_space(const json_string & json) json_nothrow {
		  return JSONWorker::RemoveWhiteSpaceAndComments(json, false);
	   }
		
		#ifndef JSON_STRING_HEADER
			inline static std::string to_std_string(const json_string & str){
				#if defined(JSON_UNICODE) ||defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
					return std::string(str.begin(), str.end());		
				#else
					return str;
				#endif
			}
			inline static std::wstring to_std_wstring(const json_string & str){
				#if (!defined(JSON_UNICODE)) || defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
					return std::wstring(str.begin(), str.end());		
				#else
					return str;
				#endif
			}
			
			inline static json_string to_json_string(const std::string & str){
				#if defined(JSON_UNICODE) ||defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
					return json_string(str.begin(), str.end());		
				#else
					return str;
				#endif
			}
			inline static json_string to_json_string(const std::wstring & str){
				#if (!defined(JSON_UNICODE)) || defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
					return json_string(str.begin(), str.end());		
				#else
					return str;
				#endif
			}
		#endif

	   #ifdef JSON_READ_PRIORITY
		  //if json is invalid, it throws a std::invalid_argument exception
		  inline static JSONNode parse(const json_string & json) json_throws(std::invalid_argument) {
			 #ifdef JSON_PREPARSE
				size_t len;
				json_auto<json_char> buffer(JSONWorker::RemoveWhiteSpace(json, len, false));
				return JSONPreparse::isValidRoot(buffer.ptr);
			 #else
				return JSONWorker::parse(json);
			 #endif
		  }

		  inline static JSONNode parse_unformatted(const json_string & json) json_throws(std::invalid_argument) {
			 #ifdef JSON_PREPARSE
				return JSONPreparse::isValidRoot(json);
			 #else
				return JSONWorker::parse_unformatted(json);
			 #endif
		  }

		  #ifdef JSON_VALIDATE
			 inline static bool is_valid(const json_string & json) json_nothrow {
				#ifdef JSON_SECURITY_MAX_STRING_LENGTH
				    if (json_unlikely(json.length() > JSON_SECURITY_MAX_STRING_LENGTH)){
					   JSON_FAIL(JSON_TEXT("Exceeding JSON_SECURITY_MAX_STRING_LENGTH"));
					   return false;
				    }
				#endif
				json_auto<json_char> s;
				s.set(JSONWorker::RemoveWhiteSpaceAndCommentsC(json, false));
				return JSONValidator::isValidRoot(s.ptr);
			 }

			 inline static bool is_valid_unformatted(const json_string & json) json_nothrow {
				#ifdef JSON_SECURITY_MAX_STRING_LENGTH
				    if (json_unlikely(json.length() > JSON_SECURITY_MAX_STRING_LENGTH)){
					   JSON_FAIL(JSON_TEXT("Exceeding JSON_SECURITY_MAX_STRING_LENGTH"));
					   return false;
				    }
				#endif
				return JSONValidator::isValidRoot(json.c_str());
			 }
			 #ifdef JSON_DEPRECATED_FUNCTIONS
				#ifdef JSON_NO_EXCEPTIONS
				    #error, JSON_DEPRECATED_FUNCTIONS requires JSON_NO_EXCEPTIONS be off
				#endif
				//if json is invalid, it throws a std::invalid_argument exception (differs from parse because this checks the entire tree)
				inline static JSONNode json_deprecated(validate(const json_string & json), "libjson::validate is deprecated, use libjson::is_valid and libjson::parse instead");
			 #endif
		  #endif
	   #endif

	   //When libjson errors, a callback allows the user to know what went wrong
	   #if defined JSON_DEBUG && !defined JSON_STDERROR
		  inline static void register_debug_callback(json_error_callback_t callback) json_nothrow {
			 JSONDebug::register_callback(callback);
		  }
	   #endif

	   #ifdef JSON_MUTEX_CALLBACKS
		  #ifdef JSON_MUTEX_MANAGE
			 inline static void register_mutex_callbacks(json_mutex_callback_t lock, json_mutex_callback_t unlock, json_mutex_callback_t destroy, void * manager_lock) json_nothrow {
				JSONNode::register_mutex_callbacks(lock, unlock, manager_lock);
				JSONNode::register_mutex_destructor(destroy);
			 }
		  #else
			 inline static void register_mutex_callbacks(json_mutex_callback_t lock, json_mutex_callback_t unlock, void * manager_lock) json_nothrow {
				JSONNode::register_mutex_callbacks(lock, unlock, manager_lock);
			 }
		  #endif

		  inline static void set_global_mutex(void * mutex) json_nothrow {
			 JSONNode::set_global_mutex(mutex);
		  }
	   #endif

	   #ifdef JSON_MEMORY_CALLBACKS
		  inline static void register_memory_callbacks(json_malloc_t mal, json_realloc_t real, json_free_t fre) json_nothrow {
			 JSONMemory::registerMemoryCallbacks(mal, real, fre);
		  }
	   #endif

    }
    #ifdef JSON_VALIDATE
	   #ifdef JSON_DEPRECATED_FUNCTIONS
		  //if json is invalid, it throws a std::invalid_argument exception (differs from parse because this checks the entire tree)
		  inline static JSONNode libjson::validate(const json_string & json) {
			 if (json_likely(is_valid(json))){
				return parse(json);
			 }
			 throw std::invalid_argument("");
		  }
	   #endif
    #endif
#endif  //JSON_LIBRARY

#endif  //LIBJSON_H
