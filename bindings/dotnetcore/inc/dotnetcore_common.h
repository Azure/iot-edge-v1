// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOTNETCORE_COMMON_H
#define DOTNETCORE_COMMON_H

#include "broker.h"
#include "module.h"

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