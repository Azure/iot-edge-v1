// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "gb_library.h"
void *gb_dlopen(const char *__file, int __mode)
{
    return dlopen(__file, __mode);
}

int gb_dlclose(void *__handle)
{
    return dlclose(__handle);
}

void *gb_dlsym(void * __handle, const char * __name)
{
    return dlsym(__handle, __name);
}
