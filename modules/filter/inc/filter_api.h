// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _FILTER_API_H
#define _FILTER_API_H

typedef void* RESOLVER_HANDLE;

typedef struct RESOLVER_API_TAG RESOLVER_API;

#ifdef __cplusplus
extern "C"
{
#endif

struct RESOLVER_TAG
{
    const RESOLVER_HANDLE* resolver_apis;
    RESOLVER_HANDLE resolver_handle;
};

typedef void* FILTER_MASK;
/* RESULT MAP definition */
typedef union RESULT_MAP_VALUE_tag
{
    const char* stringValue;
    double numericValue;
} RESULT_MAP_VALUE;
typedef enum RESULT_MAP_VALUE_TYPE_tag
{
    STRING,
    NUMERIC
} RESULT_MAP_VALUE_TYPE;

typedef RESULT_MAP_tag {
    RESULT_MAP_VALUE_TYPE valueType;
    RESULT_MAP_VALUE value;
}
#define RESULT_MAP_KEY_FILTER_MASK "filter-mask"
#define RESULT_MAP_KEY_MAC_ADDRESS "mac-address"

typedef void*(rResolver_ParseConfigurationFromJson)(const char* configuration, FILTER_MASK* filterMask);
typedef void*(rResolver_FreeConfiguration)(void* configuration);
typedef RESOLVER_HANDLE(*rResolver_Create)(const void* configuration);
typedef void(*rResolver_Destroy)(RESOLVER_HANDLE resolverHandle);
typedef FILTER_MASK(*rResolver_CreateFilterMask)(RESOLVER_HANDLE resolverHandle);
typedef bool(*rResolver_Resolve)(RESOLVER_HANDLE resolverHandle, const char* characteristics, const char* buf, size_t buf_size, MAP_HANDLE resultMap);

typedef enum RESOLVER_API_VERSION_TAG
{
    RESOLVER_API_VERSION_1
} RESOLVER_API_VERSION;

struct RESOLVER_API_TAG
{
    RESOLVER_API_VERSION version;
}

typedef struct RESOLVER_API_1_TAG
{
    RESOLVER_API base;
    rResolver_ParseConfigurationFromJson Resolver_ParseConfigurationFromJson;
    rResolver_FreeConfiguration Resolver_FreeConfiguration;
    rResolver_Create Resolver_Create;
    rResolver_Destory Resolver_Destroy;
    rResolver_Resolve Resolver_Resolve;
} RESOLVER_API_1;

#define RESOLVER_GETAPI_NAME ("Resolver_GetApi")

#ifdef _WIN32
#define RESOLVER_EXPORT __declspec(dllexport)
#else
#define RESOLVER_EXPORT
#endif // _WIN32

#define RESOLVER_STATIC_GETAPI(RESOLVER_NAME) C2(Resolver_GetApi_, RESOLVER_NAME)

RESOLVER_EXPORT const RESOLVER_API* Resolver_GetApi(RESOLVER_API_VERSION gateway_api_version);


#ifdef __cplusplus
}
#endif

#endif /*_FILTER_API_H*/
