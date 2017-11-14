// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <filesystem>
#include "dotnetcore_utils.h"
#include "dotnetcore_common.h"
#include "azure_c_shared_utility/xlogging.h"

#ifndef UNICODE
#define UNICODE
#define UNICODE_WAS_UNDEFINED
#endif

#include "windows.h"

#ifdef UNICODE_WAS_UNDEFINED
#undef UNICODE
#endif

#if !defined(MAX_LONGPATH)
#define MAX_LONGPATH   260 /* max. length of full pathname */
#endif

#include <stdio.h>
#include <vector>
#include <string>

namespace fs = std::experimental::filesystem;

void AddFilesFromDirectoryToTpaList(const std::string& directory, std::string& tpaList) {
    for (auto& dirent : fs::directory_iterator(directory)) {
        auto entpath = dirent.path();
        if (entpath.has_extension() &&
            entpath.extension() == fs::path(".dll")) {
            tpaList += entpath.string() + ";";
        }
    }
}

bool initializeDotNetCoreCLR(coreclr_initialize_ptr coreclrInitialize_ptr, const char* trustedPlatformAssemblyiesLocation, void** hostHandle, unsigned int* domainId)
{
    bool returnResult;
    wchar_t appPath[MAX_LONGPATH];
    char appPath_char[MAX_LONGPATH];
    char splittedPath_char[MAX_LONGPATH];
    char driver_char[MAX_LONGPATH];
    
    //This method can't really fail, maximum will happen is to Truncate the String if it's greater than MAX_LONGPATH.
    GetModuleFileName(NULL, appPath, MAX_LONGPATH);
    wcstombs(appPath_char, appPath, wcslen(appPath) + 1);
    _splitpath(appPath_char, driver_char, splittedPath_char, NULL, NULL);
    strcat(driver_char, splittedPath_char);
      
    // The properties we provide to the runtime
    const char *property_keys[] = {
        "APPBASE",
        "TRUSTED_PLATFORM_ASSEMBLIES",
        "APP_PATHS"
    };

    std::string tpaList;
    AddFilesFromDirectoryToTpaList(trustedPlatformAssemblyiesLocation, tpaList);

    const char* property_values[] = {
        // APPBASE (just for windows?)
        driver_char,
        // TRUSTED_PLATFORM_ASSEMBLIES
        tpaList.c_str(),
        // APP_PATHS
        driver_char,
    };

    int status = -1;

    status = coreclrInitialize_ptr(
        NULL,
        "DotNetCoreModuleHost",
        sizeof(property_keys) / sizeof(property_keys[0]),
        property_keys,
        property_values,
        hostHandle,
        domainId
    );

    if (status < 0) {
        LogError("ERROR! coreclr_initialize status: %d\r\n", status);
        returnResult = false;
    }
    else
    {
        returnResult = true;
    }

    return returnResult;
}