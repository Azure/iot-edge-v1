// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "azure_c_shared_utility/xlogging.h"
#include <unistd.h>
#include <string>
#include "dotnetcore_common.h"
#include "dotnetcore_utils.h"
#include <dirent.h>
#include <set>
#include <sys/stat.h>



void AddFilesFromDirectoryToTpaList(const std::string& directory, std::string& tpaList)
{
    const char * const tpaExtensions[] = {
                ".ni.dll",      // Probe for .ni.dll first so that it's preferred if ni and il coexist in the same dir
                ".dll",
                ".ni.exe",
                ".exe",
                };

    DIR* dir = opendir(directory.c_str());
    if (dir == nullptr)
    {
        return;
    }

    std::set<std::string> addedAssemblies;

    // Walk the directory for each extension separately so that we first get files with .ni.dll extension,
    // then files with .dll extension, etc.
    for (size_t extIndex = 0; extIndex < sizeof(tpaExtensions) / sizeof(tpaExtensions[0]); extIndex++)
    {
        const char* ext = tpaExtensions[extIndex];
        int extLength = strlen(ext);

        struct dirent* entry;

        // For all entries in the directory
        while ((entry = readdir(dir)) != nullptr)
        {
            // We are interested in files only
            switch (entry->d_type)
            {
            case DT_REG:
                break;

            // Handle symlinks and file systems that do not support d_type
            case DT_LNK:
            case DT_UNKNOWN:
                {
                    std::string fullFilename;

                    fullFilename.append(directory);
                    fullFilename.append("/");
                    fullFilename.append(entry->d_name);

                    struct stat sb;
                    if (stat(fullFilename.c_str(), &sb) == -1)
                    {
                        continue;
                    }

                    if (!S_ISREG(sb.st_mode))
                    {
                        continue;
                    }
                }
                break;

            default:
                continue;
            }

            std::string filename(entry->d_name);

            // Check if the extension matches the one we are looking for
            int extPos = filename.length() - extLength;
            if ((extPos <= 0) || (filename.compare(extPos, extLength, ext) != 0))
            {
                continue;
            }

            std::string filenameWithoutExt(filename.substr(0, extPos));

            // Make sure if we have an assembly with multiple extensions present,
            // we insert only one version of it.
            if (addedAssemblies.find(filenameWithoutExt) == addedAssemblies.end())
            {
                addedAssemblies.insert(filenameWithoutExt);

                tpaList.append(directory);
                tpaList.append("/");
                tpaList.append(filename);
                tpaList.append(":");
            }
        }

        // Rewind the directory stream to be able to iterate over it for the next extension
        rewinddir(dir);
    }

    closedir(dir);
}


bool initializeDotNetCoreCLR(coreclr_initialize_ptr coreclrInitialize_ptr, const char* trustedPlatformAssemblyiesLocation, void** hostHandle, unsigned int* domainId)
{
    bool returnResult;

    char executableFullPath[PATH_MAX] = {0};
    char the_p_path[PATH_MAX];

    //Ignoring failures of these calls. coreclrInitialize_ptr will fail if folder is not right. 
    (void)readlink("/proc/self/exe", executableFullPath, sizeof(executableFullPath));
    getcwd(the_p_path, 255);
    strcat(the_p_path, "/");

    // The properties we provide to the runtime
    const char *property_keys[] = {
        "TRUSTED_PLATFORM_ASSEMBLIES",
        "APP_PATHS"
     };

     std::string tpaList;
     AddFilesFromDirectoryToTpaList(trustedPlatformAssemblyiesLocation, tpaList);

     // The concrete values of the properties
     const char* property_values[] = {
         // TRUSTED_PLATFORM_ASSEMBLIES
         tpaList.c_str(),
         // APP_PATHS
         the_p_path
    };

    int status = -1;

    try
    {
        status = coreclrInitialize_ptr(
            executableFullPath,
            "DotNetCoreModuleHost",
            sizeof(property_keys) / sizeof(property_keys[0]),
            property_keys,
            property_values,
            hostHandle,
            domainId
        );
    }
    catch (const std::exception&)
    {
        status = -1;
    }


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
