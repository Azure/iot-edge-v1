// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef GB_LIBRARY_H
#define GB_LIBRARY_H

/*this file, if included instead of   <libloaderapi.h> has the following functionality:
1) if GB_LIBRARY_INTERCEPT is defined then
a) some of the dlfcn.h symbols shall be redefined: LoadLibraryA, FreeLibrary and GetProcAddress
b) all "code" using the the above functions will actually (because of the preprocessor) call to gb_<function> equivalent
c) gb_<function> shall blindly call into <function>, thus realizing a passthrough

reason is: unittesting. DLL functions come with the C Run Time and cannot be mocked 
(that is, in the global namespace cannot exist a function called LoadLibraryA)

2) if GB_LIBRARY_INTERCEPT is not defined then
a) it shall include <libloaderapi.h> => no passthrough, just direct linking.
*/

#ifndef GB_LIBRARY_INTERCEPT
#include <windows.h>
#else

/*a)source level intercepting of function calls
This causes the C Compiler in the include below (<windows.h>) to declare
the kernel function as <function>_never_called_never_implemented_always_forgotten
so we are free to define our own for testing 
*/
#define LoadLibraryA         LoadLibraryA_never_called_never_implemented_always_forgotten
#define FreeLibrary          FreeLibrary_never_called_never_implemented_always_forgotten
#define GetProcAddress       GetProcAddress_never_called_never_implemented_always_forgotten
#define GetLastError         GetLastError_never_called_never_implemented_always_forgotten
#define GetCurrentDirectoryA GetCurrentDirectoryA_never_called_never_implemented_always_forgotten

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*b) call to gb_<function> equivalent */

#undef LoadLibraryA
#define LoadLibraryA gb_LoadLibraryA
extern void*  gb_LoadLibraryA(const char* dynamicLibraryFileName);

#undef FreeLibrary
#define FreeLibrary gb_FreeLibrary
extern int gb_FreeLibrary(void* library);

#undef GetProcAddress
#define GetProcAddress gb_GetProcAddress
extern void* gb_GetProcAddress(void* library, const char* symbolName);

#undef GetLastError
#define GetLastError gb_GetLastError
extern DWORD gb_GetLastError();

#undef GetCurrentDirectoryA
#define GetCurrentDirectoryA gb_GetCurrentDirectoryA
extern DWORD gb_GetCurrentDirectoryA(DWORD  nBufferLength, char* lpBuffer );




#ifdef __cplusplus
}
#endif

#endif /* GB_LIBRARY_INTERCEPT*/

#endif /* !GB_LIBRARY_H */
