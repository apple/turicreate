/**
 *
 *  This test suite should get run before releasing a new version of libjson, once all
 *  unit tests have passed.  This asserts that the Options are in the default configuration,
 *  this prevents me from accidentally releasing libjson using options that I had been testing 
 *  with.  It also performs a speed benchmark, so I can keep track of how libjson is performing
 *
 */

#include <iostream>
#include <string>
#include <ctime>
#include "../../libjson.h"

using namespace std;

#ifndef JSON_LIBRARY
#error, JSON_LIBRARY not on
#endif

#ifdef JSON_STRICT
#error, JSON_STRICT on
#endif

#ifdef JSON_DEBUG
#error, JSON_DEBUG on
#endif

#ifdef JSON_ISO_STRICT
#error, JSON_ISO_STRICT on
#endif

#ifndef JSON_SAFE
#error, JSON_SAFE not on
#endif

#ifndef JSON_CASTABLE
#error, JSON_CASTABLE not on
#endif

#ifdef JSON_STDERROR
#error, JSON_STDERROR on
#endif

#ifdef JSON_PREPARSE
#error, JSON_PREPARSE on
#endif

#ifdef JSON_LESS_MEMORY
#error, JSON_LESS_MEMORY on
#endif

#ifdef JSON_UNICODE
#error, JSON_UNICODE on
#endif

#ifndef JSON_REF_COUNT
#error, JSON_REF_COUNT not on
#endif

#ifndef JSON_BINARY
#error, JSON_BINARY not on
#endif

#ifndef JSON_EXPOSE_BASE64
#error, JSON_EXPOSE_BASE64 not on
#endif

#ifndef JSON_ITERATORS
#error, JSON_ITERATORS not on
#endif

#ifndef JSON_STREAM
#error, JSON_STREAM not on
#endif

#ifdef JSON_MEMORY_CALLBACKS
#error, JSON_MEMORY_CALLBACKS on
#endif

#ifdef JSON_MEMORY_MANAGE
#error, JSON_MEMORY_MANAGE on
#endif

#ifdef JSON_MUTEX_CALLBACKS
#error, JSON_MUTEX_CALLBACKS on
#endif

#ifdef JSON_MUTEX_MANAGE
#error, JSON_MUTEX_MANAGE on
#endif

#ifdef JSON_NO_C_CONSTS
#error, JSON_NO_C_CONSTS on
#endif

#ifdef JSON_OCTAL
#error, JSON_OCTAL on
#endif

#if (JSON_READ_PRIORITY != HIGH)
#error JSON_READ_PRIORITY not high
#endif

#if (JSON_WRITE_PRIORITY != MED)
#error JSON_WRITE_PRIORITY not med
#endif

#ifdef JSON_NEWLINE
#error, JSON_NEWLINE on
#endif

#ifdef JSON_INDENT
#error, JSON_INDENT on
#endif

#ifndef JSON_ESCAPE_WRITES
#error, JSON_ESCAPE_WRITES not on
#endif

#ifndef JSON_COMMENTS
#error, JSON_COMMENTS not on
#endif

#ifdef JSON_WRITE_BASH_COMMENTS
#error, JSON_WRITE_BASH_COMMENTS on
#endif

#ifdef JSON_WRITE_SINGLE_LINE_COMMENTS
#error, JSON_WRITE_SINGLE_LINE_COMMENTS on
#endif

#ifdef JSON_ARRAY_ON_ONE_LINE
#error, JSON_ARRAY_ON_ONE_LINE on
#endif

#ifndef JSON_VALIDATE
#error, JSON_VALIDATE not on
#endif

#ifndef JSON_CASE_INSENSITIVE_FUNCTIONS
#error, JSON_CASE_INSENSITIVE_FUNCTIONS not on
#endif

#ifdef JSON_INDEX_TYPE
#error, JSON_INDEX_TYPE on
#endif

#ifdef JSON_BOOL_TYPE
#error, JSON_BOOL_TYPE on
#endif

#ifdef JSON_INT_TYPE
#error, JSON_INT_TYPE on
#endif

#ifdef JSON_STRING_HEADER
#error, JSON_STRING_HEADER on
#endif

#ifdef JSON_NO_EXCEPTIONS
#error, JSON_NO_EXCEPTIONS on
#endif

#ifndef JSON_DEPRECATED_FUNCTIONS
#error, JSON_DEPRECATED_FUNCTIONS not on
#endif

#if (JSON_SECURITY_MAX_NEST_LEVEL != 128)
#error JSON_SECURITY_MAX_NEST_LEVEL not 128
#endif

#if (JSON_SECURITY_MAX_STRING_LENGTH != 33554432)
#error JSON_SECURITY_MAX_STRING_LENGTH not 33554432
#endif

#if (JSON_SECURITY_MAX_STREAM_OBJECTS != 128)
#error JSON_SECURITY_MAX_STREAM_OBJECTS not 128
#endif

#ifdef JSON_MEMORY_POOL
#error JSON_MEMORY_POOL is on
#endif

#ifdef JSON_UNIT_TEST
#error, JSON_UNIT_TEST on
#endif

#define IT_COUNT 50000
static string makeBigFormatted(){
    string json = "{\n";
    for(unsigned int i = 0; i < IT_COUNT; ++i){
	   json += "\t//This is an object\r\n\t{\n\t\t\"name\" : 14.783,\n\t\t/* This is a multilen commenet */\n\t\t\"another\" : \"I am a stirng\"\n\t},"; 
	   json += "\n\n\t//This is an array\r\n\t[4, 16, true, false, 78.98],\n"; 
    }
    json += "\t\"number\" : null\n}";
    return json;
}

static string makeBig(){
    string json = "{";
    for(unsigned int i = 0; i < IT_COUNT; ++i){
	   json += "{\"name\":14.783,\"another\":\"I am a stirng\"},"; 
	   json += "[4, 16, true, false, 78.98],"; 
    }
    json += "\"number\":null}";
    return json;
}

int main (int argc, char * const argv[]) {
    JSONNODE * node;
    string mystr = makeBigFormatted();
    clock_t start = clock();
    for(unsigned int i = 0; i < 100; ++i){
	   node = json_parse(mystr.c_str());
	   for (unsigned int j = 0; j < IT_COUNT; ++j){
		  JSONNODE * meh = json_at(node, j * 2);
		  json_as_float(json_get(meh, "name"));
		  char * str = json_as_string(json_get(meh, "another"));
		  json_free(str);
		  
		  meh = json_at(node, j * 2 + 1);
		  json_as_int(json_at(meh, 0));
		  json_as_int(json_at(meh, 1));
		  json_as_bool(json_at(meh, 2));
		  json_as_bool(json_at(meh, 3));
		  json_as_int(json_at(meh, 4));
	   }
	   json_delete(node);
    }
    cout << "Reading:             " << clock() - start << endl;
    
    
    
    mystr = makeBig();
    start = clock();
    for(unsigned int i = 0; i < 100; ++i){
	   node = json_parse(mystr.c_str());
	   for (unsigned int j = 0; j < IT_COUNT; ++j){
		  JSONNODE * meh = json_at(node, j * 2);
		  json_as_float(json_get(meh, "name"));
		  char * str = json_as_string(json_get(meh, "another"));
		  json_free(str);
		  
		  meh = json_at(node, j * 2 + 1);
		  json_as_int(json_at(meh, 0));
		  json_as_int(json_at(meh, 1));
		  json_as_bool(json_at(meh, 2));
		  json_as_bool(json_at(meh, 3));
		  json_as_int(json_at(meh, 4));
	   }
	   json_delete(node);
    }
    cout << "Reading Unformatted: " << clock() - start << endl;
    
    
    start = clock();
    for(unsigned int i = 0; i < 100; ++i){
	   node = json_new(JSON_NODE);
	   for (unsigned int j = 0; j < IT_COUNT; ++j){
		  JSONNODE * meh = json_new(JSON_NODE);
		  json_push_back(meh, json_new_f("name", 14.783));
		  json_push_back(meh, json_new_a("another", "I am a string"));
		  json_push_back(node, meh);
		  
		  meh = json_new(JSON_ARRAY);
		  json_push_back(meh, json_new_i(NULL, 14));
		  json_push_back(meh, json_new_i("", 1));
		  json_push_back(meh, json_new_b(NULL, true));
		  json_push_back(meh, json_new_b("", false));
		  json_push_back(meh, json_new_f(NULL, 14.3243));
		  json_push_back(node, meh);
	   }
	   json_delete(node);
    }
    cout << "Building:            " << clock() - start << endl;
    
    
    
    node = json_new(JSON_NODE);
    for (unsigned int j = 0; j < IT_COUNT; ++j){
	   JSONNODE * meh = json_new(JSON_NODE);
	   json_push_back(meh, json_new_f("name", 14.783));
	   json_push_back(meh, json_new_a("another", "I am a string"));
	   json_push_back(node, meh);
	   
	   meh = json_new(JSON_ARRAY);
	   json_push_back(meh, json_new_i(NULL, 14));
	   json_push_back(meh, json_new_i("", 1));
	   json_push_back(meh, json_new_b(NULL, true));
	   json_push_back(meh, json_new_b("", false));
	   json_push_back(meh, json_new_f(NULL, 14.3243));
	   json_push_back(node, meh);
    }
    start = clock();
    for(unsigned int i = 0; i < 100; ++i){
	   char * str = json_write_formatted(node);
	   json_free(str);
    }
    cout << "Writing:             " << clock() - start << endl;
    
    start = clock();
    for(unsigned int i = 0; i < 100; ++i){
	   char * str = json_write(node);
	   json_free(str);
    }
    cout << "Writing Unformatted: " << clock() - start << endl;
    json_delete(node);
    
    return 0;
}
