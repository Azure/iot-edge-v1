// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef NODEJS_COMMON_H
#define NODEJS_COMMON_H

#include <string>

#include "v8.h"

#include "message_bus.h"

struct NODEJS_MODULE_HANDLE_DATA;

typedef void(*PFNMODULE_START)(NODEJS_MODULE_HANDLE_DATA* handle_data);

struct NODEJS_MODULE_HANDLE_DATA
{
    NODEJS_MODULE_HANDLE_DATA()
        :
        bus{ NULL },
        on_module_start{ NULL },
        module_id{ 0 },
        v8_isolate{ nullptr }
    {}

    NODEJS_MODULE_HANDLE_DATA(
        MESSAGE_BUS_HANDLE message_bus,
        const char* path,
        const char* config,
        PFNMODULE_START module_start)
        :
        bus{ message_bus },
        main_path{ path },
        configuration_json{ config },
        on_module_start{ module_start },
        module_id{ 0 },
        v8_isolate{ nullptr }
    {}

    NODEJS_MODULE_HANDLE_DATA(const NODEJS_MODULE_HANDLE_DATA&& rhs)
    {
        bus = rhs.bus;
        main_path = rhs.main_path;
        configuration_json = rhs.configuration_json;
        v8_isolate = rhs.v8_isolate;
        on_module_start = rhs.on_module_start;
        module_id = rhs.module_id;

        if (v8_isolate != nullptr && rhs.module_object.IsEmpty() == false)
        {
            module_object.Reset(v8_isolate, rhs.module_object);
        }
    }

    NODEJS_MODULE_HANDLE_DATA(const NODEJS_MODULE_HANDLE_DATA& rhs, size_t module_id)
    {
        bus = rhs.bus;
        main_path = rhs.main_path;
        configuration_json = rhs.configuration_json;
        v8_isolate = rhs.v8_isolate;
        on_module_start = rhs.on_module_start;
        this->module_id = module_id;

        if (v8_isolate != nullptr && rhs.module_object.IsEmpty() == false)
        {
            module_object.Reset(v8_isolate, rhs.module_object);
        }
    }

    MESSAGE_BUS_HANDLE          bus;
    std::string                 main_path;
    std::string                 configuration_json;
    v8::Isolate                 *v8_isolate;
    v8::Persistent<v8::Object>  module_object;
    size_t                      module_id;
    PFNMODULE_START             on_module_start;
};

#endif /*NODEJS_COMMON_H*/
