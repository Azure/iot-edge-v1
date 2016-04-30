// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstdlib>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "micromock.h"
#include "micromockcharstararenullterminatedstrings.h"
#include "azure_c_shared_utility/lock.h"

#include "gateway.h"

#define DUMMY_JSON_PATH "x.json"
#define MISCONFIG_JSON_PATH "invalid_json.json"
#define MISSING_INFO_JSON_PATH "missing_info_json.json"
#define VALID_JSON_PATH "valid_json.json"
#define VALID_JSON_NULL_ARGS_PATH "valid_json_null.json"

#define GBALLOC_H

extern "C" int gballoc_init(void);
extern "C" void gballoc_deinit(void);
extern "C" void* gballoc_malloc(size_t size);
extern "C" void* gballoc_calloc(size_t nmemb, size_t size);
extern "C" void* gballoc_realloc(void* ptr, size_t size);
extern "C" void gballoc_free(void* ptr);

namespace BASEIMPLEMENTATION
{

	
	/*if malloc is defined as gballoc_malloc at this moment, there'd be serious trouble*/

#define Lock(x) (LOCK_OK + gballocState - gballocState) /*compiler warning about constant in if condition*/
#define Unlock(x) (LOCK_OK + gballocState - gballocState)
#define Lock_Init() (LOCK_HANDLE)0x42
#define Lock_Deinit(x) (LOCK_OK + gballocState - gballocState)
#include "gballoc.c"
#undef Lock
#undef Unlock
#undef Lock_Init
#undef Lock_Deinit
#include "vector.c"

#include <stddef.h>   /* size_t */    
    
/* Types and enums */
typedef struct json_object_t JSON_Object;
typedef struct json_array_t  JSON_Array;
typedef struct json_value_t  JSON_Value;

enum json_value_type {
    JSONError   = -1,
    JSONNull    = 1,
    JSONString  = 2,
    JSONNumber  = 3,
    JSONObject  = 4,
    JSONArray   = 5,
    JSONBoolean = 6
};
typedef int JSON_Value_Type;
    
enum json_result_t {
    JSONSuccess = 0,
    JSONFailure = -1
};
typedef int JSON_Status;
    
typedef void * (*JSON_Malloc_Function)(size_t);
typedef void   (*JSON_Free_Function)(void *);

/* Call only once, before calling any other function from parson API. If not called, malloc and free
   from stdlib will be used for all allocations */
void json_set_allocation_functions(JSON_Malloc_Function malloc_fun, JSON_Free_Function free_fun);
    
/* Parses first JSON value in a file, returns NULL in case of error */
JSON_Value * json_parse_file(const char *filename);

/* Parses first JSON value in a file and ignores comments (/ * * / and //),
   returns NULL in case of error */
JSON_Value * json_parse_file_with_comments(const char *filename);
    
/*  Parses first JSON value in a string, returns NULL in case of error */
JSON_Value * json_parse_string(const char *string);

/*  Parses first JSON value in a string and ignores comments (/ * * / and //),
    returns NULL in case of error */
JSON_Value * json_parse_string_with_comments(const char *string);
    
/* Serialization */
size_t      json_serialization_size(const JSON_Value *value); /* returns 0 on fail */
JSON_Status json_serialize_to_buffer(const JSON_Value *value, char *buf, size_t buf_size_in_bytes);
JSON_Status json_serialize_to_file(const JSON_Value *value, const char *filename);
char *      json_serialize_to_string(const JSON_Value *value);

/* Pretty serialization */
size_t      json_serialization_size_pretty(const JSON_Value *value); /* returns 0 on fail */
JSON_Status json_serialize_to_buffer_pretty(const JSON_Value *value, char *buf, size_t buf_size_in_bytes);
JSON_Status json_serialize_to_file_pretty(const JSON_Value *value, const char *filename);
char *      json_serialize_to_string_pretty(const JSON_Value *value);

void        json_free_serialized_string(char *string); /* frees string from json_serialize_to_string and json_serialize_to_string_pretty */

/* Comparing */
int  json_value_equals(const JSON_Value *a, const JSON_Value *b);
    
/* Validation
   This is *NOT* JSON Schema. It validates json by checking if object have identically 
   named fields with matching types.
   For example schema {"name":"", "age":0} will validate 
   {"name":"Joe", "age":25} and {"name":"Joe", "age":25, "gender":"m"},
   but not {"name":"Joe"} or {"name":"Joe", "age":"Cucumber"}.
   In case of arrays, only first value in schema is checked against all values in tested array.
   Empty objects ({}) validate all objects, empty arrays ([]) validate all arrays,
   null validates values of every type.
 */
JSON_Status json_validate(const JSON_Value *schema, const JSON_Value *value);
    
/*
 * JSON Object
 */
JSON_Value  * json_object_get_value  (const JSON_Object *object, const char *name);
const char  * json_object_get_string (const JSON_Object *object, const char *name);
JSON_Object * json_object_get_object (const JSON_Object *object, const char *name);
JSON_Array  * json_object_get_array  (const JSON_Object *object, const char *name);
double        json_object_get_number (const JSON_Object *object, const char *name); /* returns 0 on fail */
int           json_object_get_boolean(const JSON_Object *object, const char *name); /* returns -1 on fail */

/* dotget functions enable addressing values with dot notation in nested objects,
 just like in structs or c++/java/c# objects (e.g. objectA.objectB.value).
 Because valid names in JSON can contain dots, some values may be inaccessible
 this way. */
JSON_Value  * json_object_dotget_value  (const JSON_Object *object, const char *name);
const char  * json_object_dotget_string (const JSON_Object *object, const char *name);
JSON_Object * json_object_dotget_object (const JSON_Object *object, const char *name);
JSON_Array  * json_object_dotget_array  (const JSON_Object *object, const char *name);
double        json_object_dotget_number (const JSON_Object *object, const char *name); /* returns 0 on fail */
int           json_object_dotget_boolean(const JSON_Object *object, const char *name); /* returns -1 on fail */

/* Functions to get available names */
size_t        json_object_get_count(const JSON_Object *object);
const char  * json_object_get_name (const JSON_Object *object, size_t index);
    
/* Creates new name-value pair or frees and replaces old value with a new one. 
 * json_object_set_value does not copy passed value so it shouldn't be freed afterwards. */
JSON_Status json_object_set_value(JSON_Object *object, const char *name, JSON_Value *value);
JSON_Status json_object_set_string(JSON_Object *object, const char *name, const char *string);
JSON_Status json_object_set_number(JSON_Object *object, const char *name, double number);
JSON_Status json_object_set_boolean(JSON_Object *object, const char *name, int boolean);
JSON_Status json_object_set_null(JSON_Object *object, const char *name);

/* Works like dotget functions, but creates whole hierarchy if necessary.
 * json_object_dotset_value does not copy passed value so it shouldn't be freed afterwards. */
JSON_Status json_object_dotset_value(JSON_Object *object, const char *name, JSON_Value *value);
JSON_Status json_object_dotset_string(JSON_Object *object, const char *name, const char *string);
JSON_Status json_object_dotset_number(JSON_Object *object, const char *name, double number);
JSON_Status json_object_dotset_boolean(JSON_Object *object, const char *name, int boolean);
JSON_Status json_object_dotset_null(JSON_Object *object, const char *name);

/* Frees and removes name-value pair */
JSON_Status json_object_remove(JSON_Object *object, const char *name);

/* Works like dotget function, but removes name-value pair only on exact match. */
JSON_Status json_object_dotremove(JSON_Object *object, const char *key);

/* Removes all name-value pairs in object */
JSON_Status json_object_clear(JSON_Object *object);
    
/* 
 *JSON Array 
 */
JSON_Value  * json_array_get_value  (const JSON_Array *array, size_t index);
const char  * json_array_get_string (const JSON_Array *array, size_t index);
JSON_Object * json_array_get_object (const JSON_Array *array, size_t index);
JSON_Array  * json_array_get_array  (const JSON_Array *array, size_t index);
double        json_array_get_number (const JSON_Array *array, size_t index); /* returns 0 on fail */
int           json_array_get_boolean(const JSON_Array *array, size_t index); /* returns -1 on fail */
size_t        json_array_get_count  (const JSON_Array *array);
    
/* Frees and removes value at given index, does nothing and returns JSONFailure if index doesn't exist.
 * Order of values in array may change during execution.  */
JSON_Status json_array_remove(JSON_Array *array, size_t i);

/* Frees and removes from array value at given index and replaces it with given one.
 * Does nothing and returns JSONFailure if index doesn't exist. 
 * json_array_replace_value does not copy passed value so it shouldn't be freed afterwards. */
JSON_Status json_array_replace_value(JSON_Array *array, size_t i, JSON_Value *value);
JSON_Status json_array_replace_string(JSON_Array *array, size_t i, const char* string);
JSON_Status json_array_replace_number(JSON_Array *array, size_t i, double number);
JSON_Status json_array_replace_boolean(JSON_Array *array, size_t i, int boolean);
JSON_Status json_array_replace_null(JSON_Array *array, size_t i);

/* Frees and removes all values from array */
JSON_Status json_array_clear(JSON_Array *array);

/* Appends new value at the end of array.
 * json_array_append_value does not copy passed value so it shouldn't be freed afterwards. */
JSON_Status json_array_append_value(JSON_Array *array, JSON_Value *value);
JSON_Status json_array_append_string(JSON_Array *array, const char *string);
JSON_Status json_array_append_number(JSON_Array *array, double number);
JSON_Status json_array_append_boolean(JSON_Array *array, int boolean);
JSON_Status json_array_append_null(JSON_Array *array);
    
/*
 *JSON Value
 */
JSON_Value * json_value_init_object (void);
JSON_Value * json_value_init_array  (void);
JSON_Value * json_value_init_string (const char *string); /* copies passed string */
JSON_Value * json_value_init_number (double number);
JSON_Value * json_value_init_boolean(int boolean);
JSON_Value * json_value_init_null   (void);
JSON_Value * json_value_deep_copy   (const JSON_Value *value);
void         json_value_free        (JSON_Value *value);

JSON_Value_Type json_value_get_type   (const JSON_Value *value);
JSON_Object *   json_value_get_object (const JSON_Value *value);
JSON_Array  *   json_value_get_array  (const JSON_Value *value);
const char  *   json_value_get_string (const JSON_Value *value);
double          json_value_get_number (const JSON_Value *value);
int             json_value_get_boolean(const JSON_Value *value);

/* Same as above, but shorter */
JSON_Value_Type json_type   (const JSON_Value *value);
JSON_Object *   json_object (const JSON_Value *value);
JSON_Array  *   json_array  (const JSON_Value *value);
const char  *   json_string (const JSON_Value *value);
double          json_number (const JSON_Value *value);
int             json_boolean(const JSON_Value *value);

#define parson_parson_h
#ifdef _CRT_SECURE_NO_WARNINGS
#undef _CRT_SECURE_NO_WARNINGS
#include "parson.c"
#define _CRT_SECURE_NO_WARNINGS
#else
#include "parson.c"
#endif
};

#undef parson_parson_h
#include "parson.h"

TYPED_MOCK_CLASS(CGatewayMocks, CGlobalMock)
{
public:

	/*Parson Mocks*/
	MOCK_STATIC_METHOD_1(, JSON_Value*, json_parse_file, const char *, filename)
		JSON_Value* value = NULL;
		if (filename != NULL)
		{
			value = (JSON_Value*)malloc(sizeof(BASEIMPLEMENTATION::JSON_Value));
		}
	MOCK_METHOD_END(JSON_Value*, value);

	MOCK_STATIC_METHOD_1(, JSON_Object*, json_value_get_object, const JSON_Value*, value)
		JSON_Object* object = NULL;
		if (value != NULL)
		{
			object = (JSON_Object*)0x42;
		}
	MOCK_METHOD_END(JSON_Object*, object);
	
	MOCK_STATIC_METHOD_2(, JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name)
		JSON_Array* arr = NULL;
		if (object != NULL && name != NULL)
		{
			arr = (JSON_Array*)0x42;
		}
	MOCK_METHOD_END(JSON_Array*, arr);

	MOCK_STATIC_METHOD_1(, size_t, json_array_get_count, const JSON_Array*, arr)
		size_t size = 0;
	MOCK_METHOD_END(size_t, size);

	MOCK_STATIC_METHOD_2(, JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index)
		JSON_Object* object = NULL;
		if (arr != NULL && index >= 0)
		{
			object = (JSON_Object*)0x42;
		}
	MOCK_METHOD_END(JSON_Object*, object);

	MOCK_STATIC_METHOD_2(, const char*, json_object_get_string, const JSON_Object*, object, const char*, name)
		const char* string = NULL;
		if (object != NULL && name != NULL)
		{
			string = name;
		}
	MOCK_METHOD_END(const char*, string);

	MOCK_STATIC_METHOD_2(, JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name)
		JSON_Value* value = NULL;
		if (object != NULL && name != NULL)
		{
			value = (JSON_Value*)0x42;
		}
	MOCK_METHOD_END(JSON_Value*, value);

	MOCK_STATIC_METHOD_1(, char*, json_serialize_to_string, const JSON_Value*, value)
		char* serialized_string = NULL;
		const char* text = "[serialized string]";
		if (value != NULL)
		{
			serialized_string = (char*)malloc(sizeof(char) * strlen(text) + 1);
			strcpy(serialized_string, text);
		}
	MOCK_METHOD_END(char*, serialized_string);

	MOCK_STATIC_METHOD_1(, void, json_value_free, JSON_Value*, value)
		free(value);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_1(, void, json_free_serialized_string, char*, string)
		free(string);
	MOCK_VOID_METHOD_END();

	/*Gateway Mocks*/
	MOCK_STATIC_METHOD_1(, GATEWAY_HANDLE, Gateway_LL_Create, const GATEWAY_PROPERTIES*, properties)
		GATEWAY_HANDLE handle = (GATEWAY_HANDLE)BASEIMPLEMENTATION::gballoc_malloc(1);
	MOCK_METHOD_END(GATEWAY_HANDLE, handle);

	MOCK_STATIC_METHOD_1(, void, Gateway_LL_Destroy, GATEWAY_HANDLE, gw)
		BASEIMPLEMENTATION::gballoc_free(gw);
	MOCK_VOID_METHOD_END();

	/*Vector Mocks*/
	MOCK_STATIC_METHOD_1(, VECTOR_HANDLE, VECTOR_create, size_t, elementSize)
		VECTOR_HANDLE vector = BASEIMPLEMENTATION::VECTOR_create(elementSize);
	MOCK_METHOD_END(VECTOR_HANDLE, vector);

	MOCK_STATIC_METHOD_1(, void, VECTOR_destroy, VECTOR_HANDLE, handle)
		BASEIMPLEMENTATION::VECTOR_destroy(handle);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_3(, int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements)
	int result1 = BASEIMPLEMENTATION::VECTOR_push_back(handle, elements, numElements);
	MOCK_METHOD_END(int, result1);

	MOCK_STATIC_METHOD_3(, void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index)
		BASEIMPLEMENTATION::VECTOR_erase(handle, elements, index);
	MOCK_VOID_METHOD_END();

	MOCK_STATIC_METHOD_2(, void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index)
		auto element = BASEIMPLEMENTATION::VECTOR_element(handle, index);
	MOCK_METHOD_END(void*, element);

	MOCK_STATIC_METHOD_1(, void*, VECTOR_front, const VECTOR_HANDLE, handle)
		auto element = BASEIMPLEMENTATION::VECTOR_front(handle);
	MOCK_METHOD_END(void*, element);

	MOCK_STATIC_METHOD_1(, size_t, VECTOR_size, const VECTOR_HANDLE, handle)
		auto size = BASEIMPLEMENTATION::VECTOR_size(handle);
	MOCK_METHOD_END(size_t, size);

	MOCK_STATIC_METHOD_3(, void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value)
		void* element = BASEIMPLEMENTATION::VECTOR_find_if(handle, pred, value);
	MOCK_METHOD_END(void*, element);

	MOCK_STATIC_METHOD_1(, void*, gballoc_malloc, size_t, size)
		void* result2 = BASEIMPLEMENTATION::gballoc_malloc(size);
	MOCK_METHOD_END(void*, result2);

	MOCK_STATIC_METHOD_2(, void*, gballoc_realloc, void*, ptr, size_t, size)
	MOCK_METHOD_END(void*, BASEIMPLEMENTATION::gballoc_realloc(ptr, size));

	MOCK_STATIC_METHOD_1(, void, gballoc_free, void*, ptr)
		BASEIMPLEMENTATION::gballoc_free(ptr);
	MOCK_VOID_METHOD_END()

};

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , JSON_Value*, json_parse_file, const char *, filename);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , JSON_Object*, json_value_get_object, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , JSON_Array*, json_object_get_array, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , size_t, json_array_get_count, const JSON_Array*, arr);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , JSON_Object*, json_array_get_object, const JSON_Array*, arr, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , const char*, json_object_get_string, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , JSON_Value*, json_object_get_value, const JSON_Object*, object, const char*, name);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , char*, json_serialize_to_string, const JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, json_value_free, JSON_Value*, value);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, json_free_serialized_string, char*, string);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , GATEWAY_HANDLE, Gateway_LL_Create, const GATEWAY_PROPERTIES*, properties);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, Gateway_LL_Destroy, GATEWAY_HANDLE, gw);

DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , VECTOR_HANDLE, VECTOR_create, size_t, elementSize);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, VECTOR_destroy, VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , int, VECTOR_push_back, VECTOR_HANDLE, handle, const void*, elements, size_t, numElements);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , void, VECTOR_erase, VECTOR_HANDLE, handle, void*, elements, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void*, VECTOR_element, const VECTOR_HANDLE, handle, size_t, index);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void*, VECTOR_front, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , size_t, VECTOR_size, const VECTOR_HANDLE, handle);
DECLARE_GLOBAL_MOCK_METHOD_3(CGatewayMocks, , void*, VECTOR_find_if, const VECTOR_HANDLE, handle, PREDICATE_FUNCTION, pred, const void*, value);


DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void*, gballoc_malloc, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_2(CGatewayMocks, , void*, gballoc_realloc, void*, ptr, size_t, size);
DECLARE_GLOBAL_MOCK_METHOD_1(CGatewayMocks, , void, gballoc_free, void*, ptr)

static MICROMOCK_GLOBAL_SEMAPHORE_HANDLE g_dllByDll;
static MICROMOCK_MUTEX_HANDLE g_testByTest;


BEGIN_TEST_SUITE(gateway_unittests)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = MicroMockCreateMutex();
	ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
	MicroMockDestroyMutex(g_testByTest);
    TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
	if (!MicroMockAcquireMutex(g_testByTest))
	{
		ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
	}
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
	if (!MicroMockReleaseMutex(g_testByTest))
	{
		ASSERT_FAIL("failure in test framework at ReleaseMutex");
	}
}

/*Tests_SRS_GATEWAY_14_001: [If file_path is NULL the function shall return NULL.]*/
TEST_FUNCTION(Gateway_Create_Returns_NULL_For_NULL_JSON_Input)
{
	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(NULL);

	//Assert
	ASSERT_IS_NULL(gateway);
}

/*Tests_SRS_GATEWAY_14_003: [The function shall return NULL if the file contents could not be read and / or parsed to a JSON_Value.]*/
TEST_FUNCTION(Gateway_Create_Returns_NULL_If_File_Not_Exist)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(DUMMY_JSON_PATH))
		.SetFailReturn((JSON_Value*)NULL);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(DUMMY_JSON_PATH);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_14_008: [ This function shall return NULL upon any memory allocation failure. ]*/
TEST_FUNCTION(Gateway_Create_Returns_NULL_On_Properties_Malloc_Failure)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(DUMMY_JSON_PATH));
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)))
		.SetFailReturn((void*)NULL);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(DUMMY_JSON_PATH);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

}

/*Tests_SRS_GATEWAY_14_002: [The function shall use parson to read the file and parse the JSON string to a parson JSON_Value structure.]*/
/*Tests_SRS_GATEWAY_14_005: [The function shall set the value of const void* module_properties in the GATEWAY_PROPERTIES instance to a char* representing the serialized args value for the particular module.]*/
/*Tests_SRS_GATEWAY_14_007: [The function shall use the GATEWAY_PROPERTIES instance to create and return a GATEWAY_HANDLE using the lower level API.]*/
TEST_FUNCTION(Gateway_Create_Parses_Valid_JSON_Configuration_File)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));

	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn(2);
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_PROPERTIES_ENTRY)));

	STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module name"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module path"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module name"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module path"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, Gateway_LL_Create(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(VALID_JSON_PATH);

	//Assert
	ASSERT_IS_NOT_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_14_002: [The function shall use parson to read the file and parse the JSON string to a parson JSON_Value structure.]*/
/*Tests_SRS_GATEWAY_14_004: [The function shall traverse the JSON_Value object to initialize a GATEWAY_PROPERTIES instance.]*/
/*Tests_SRS_GATEWAY_14_005: [The function shall set the value of const void* module_properties in the GATEWAY_PROPERTIES instance to a char* representing the serialized args value for the particular module.]*/
/*Tests_SRS_GATEWAY_14_007: [The function shall use the GATEWAY_PROPERTIES instance to create and return a GATEWAY_HANDLE using the lower level API.]*/
TEST_FUNCTION(Gateway_Create_Traverses_JSON_Value_Success)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn(2);
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_PROPERTIES_ENTRY)));

	STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module name"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module path"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module name"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module path"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, Gateway_LL_Create(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(VALID_JSON_PATH);

	//Assert
	ASSERT_IS_NOT_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_14_002: [The function shall use parson to read the file and parse the JSON string to a parson JSON_Value structure.]*/
/*Tests_SRS_GATEWAY_14_004: [The function shall traverse the JSON_Value object to initialize a GATEWAY_PROPERTIES instance.]*/
/*Tests_SRS_GATEWAY_14_005: [The function shall set the value of const void* module_properties in the GATEWAY_PROPERTIES instance to a char* representing the serialized args value for the particular module.]*/
/*Tests_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_Create_Traverses_JSON_Push_Back_Fail)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn(2);
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_PROPERTIES_ENTRY)));

	STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module name"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module path"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2);

	STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module name"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module path"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_value(IGNORED_PTR_ARG, "args"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_serialize_to_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_push_back(IGNORED_PTR_ARG, IGNORED_PTR_ARG, 1))
		.IgnoreArgument(1)
		.IgnoreArgument(2)
		.SetFailReturn(-1);

	STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_element(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_free_serialized_string(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(VALID_JSON_PATH);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

}

/*Tests_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_Create_Traverses_JSON_Value_NULL_Array)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
		.IgnoreArgument(1)
		.SetFailReturn((JSON_Array*)NULL);

	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(VALID_JSON_PATH);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_14_008: [This function shall return NULL upon any memory allocation failure.]*/
TEST_FUNCTION(Gateway_Create_Traverses_JSON_Value_VECTOR_Create_Fail)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(VALID_JSON_PATH));
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_PROPERTIES_ENTRY)))
		.SetFailReturn((VECTOR_HANDLE)NULL);

	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(VALID_JSON_PATH);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();
}

/*Tests_SRS_GATEWAY_14_004: [The function shall traverse the JSON_Value object to initialize a GATEWAY_PROPERTIES instance.]*/
/*Tests_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_Create_Traverses_JSON_Value_NULL_Root_Value_Failure)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(DUMMY_JSON_PATH));
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetFailReturn((JSON_Object*)NULL);
	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(DUMMY_JSON_PATH);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();

	//Cleanup
	Gateway_LL_Destroy(gateway);
}

/*Tests_SRS_GATEWAY_14_006: [The function shall return NULL if the JSON_Value contains incomplete information.]*/
TEST_FUNCTION(Gateway_Create_Fails_For_Missing_Info_In_JSON_Configuration)
{
	//Arrange
	CGatewayMocks mocks;

	STRICT_EXPECTED_CALL(mocks, json_parse_file(MISSING_INFO_JSON_PATH));
	STRICT_EXPECTED_CALL(mocks, gballoc_malloc(sizeof(GATEWAY_PROPERTIES)));
	STRICT_EXPECTED_CALL(mocks, json_value_get_object(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_array(IGNORED_PTR_ARG, "modules"))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_array_get_count(IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn(2);
	STRICT_EXPECTED_CALL(mocks, VECTOR_create(sizeof(GATEWAY_PROPERTIES_ENTRY)));

	STRICT_EXPECTED_CALL(mocks, json_array_get_object(IGNORED_PTR_ARG, 0))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module name"))
		.IgnoreArgument(1)
		.SetFailReturn((char*)NULL);
	STRICT_EXPECTED_CALL(mocks, json_object_get_string(IGNORED_PTR_ARG, "module path"))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mocks, VECTOR_size(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, VECTOR_destroy(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(mocks, json_value_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);
	STRICT_EXPECTED_CALL(mocks, gballoc_free(IGNORED_PTR_ARG))
		.IgnoreArgument(1);

	//Act
	GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(MISSING_INFO_JSON_PATH);

	//Assert
	ASSERT_IS_NULL(gateway);
	mocks.AssertActualAndExpectedCalls();
}

END_TEST_SUITE(gateway_unittests)
