// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNETCORE_COMMON_H
#define DOTNETCORE_COMMON_H

#include "broker.h"
#include "module.h"

#ifdef _WIN64
#define DOTNET_CORE_CALLING_CONVENTION
#elif WIN32
#define DOTNET_CORE_CALLING_CONVENTION __stdcall
#else
#define DOTNET_CORE_CALLING_CONVENTION
#endif

typedef int(DOTNET_CORE_CALLING_CONVENTION *coreclr_initialize_ptr)(const char* exePath,
    const char* appDomainFriendlyName,
    int propertyCount,
    const char** propertyKeys,
    const char** propertyValues,
    void** hostHandle,
    unsigned int* domainId);


typedef int(DOTNET_CORE_CALLING_CONVENTION *coreclr_shutdown_ptr)(
    void* hostHandle,
    unsigned int domainId);


typedef int(DOTNET_CORE_CALLING_CONVENTION *coreclr_create_delegate_ptr)(void* hostHandle,
    unsigned int domainId,
    const char* entryPointAssemblyName,
    const char* entryPointTypeName,
    const char* entryPointMethodName,
    void** delegate);


namespace dotnetcore_module
{
    struct DOTNET_CORE_HOST_HANDLE_DATA
    {
        DOTNET_CORE_HOST_HANDLE_DATA()
            :
            module_id(0), 
            broker(nullptr),
            assembly_name(nullptr)
        {

        };

        DOTNET_CORE_HOST_HANDLE_DATA(const char* input_assembly_name)
            :
            module_id(0),
            broker(nullptr)        
        {
            this->assembly_name = STRING_construct(input_assembly_name);
        };

        DOTNET_CORE_HOST_HANDLE_DATA(BROKER_HANDLE broker, const char* input_assembly_name)
            :
            module_id(0),
            broker(broker)        
        {
            this->assembly_name = STRING_construct(input_assembly_name);
        };

        DOTNET_CORE_HOST_HANDLE_DATA(DOTNET_CORE_HOST_HANDLE_DATA&& rhs)
        {
            module_id = rhs.module_id;
            broker = rhs.broker;        
            this->assembly_name = STRING_clone(rhs.assembly_name);
        };

        DOTNET_CORE_HOST_HANDLE_DATA(const DOTNET_CORE_HOST_HANDLE_DATA& rhs)
        {
            module_id = rhs.module_id;
            broker = rhs.broker;        
            this->assembly_name = STRING_clone(rhs.assembly_name);
        };

        DOTNET_CORE_HOST_HANDLE_DATA(const DOTNET_CORE_HOST_HANDLE_DATA& rhs, size_t module_id)
        {
            this->module_id = module_id;
            broker = rhs.broker;        
            this->assembly_name = STRING_clone(rhs.assembly_name);
        };

        ~DOTNET_CORE_HOST_HANDLE_DATA()
        {
            STRING_delete(assembly_name);
        };


        size_t module_id;

        BROKER_HANDLE broker;

        STRING_HANDLE assembly_name;
    };
}

#endif /*DOTNETCORE_COMMON_H*/