// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifndef GB_LIBRARY_H
#define GB_LIBRARY_H

/*this file, if included instead of  <dlfcn.h> has the following functionality:
1) if GB_LIBRARY_INTERCEPT is defined then
a) some of the dlfcn.h symbols shall be redefined: dlopen, dlclose and dlsym
b) all "code" using the the above functions will actually (because of the preprocessor) call to gb_<function> equivalent
c) gb_<function> shall blindly call into <function>, thus realizing a passthrough

reason is: unittesting. dl functions come with the C Run Time and cannot be mocked 
(that is, in the global namespace cannot exist a function called dlopen)

2) if GB_LIBRARY_INTERCEPT is not defined then
a) it shall include <dlfcn.h> => no passthrough, just direct linking.
*/

#ifndef GB_LIBRARY_INTERCEPT
#include <dlfcn.h>
#else

/*a)source level intercepting of function calls
This causes the C Compiler in the include below (<dlfcn.h>) to declare
the kernel function as <function>_never_called_never_implemented_always_forgotten
so we are free to define our own for testing */
#define dlopen  dlopen_never_called_never_implemented_always_forgotten
#define dlclose dlclose_never_called_never_implemented_always_forgotten
#define dlsym   dlsym_never_called_never_implemented_always_forgotten

#include <dlfcn.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*b) call to gb_<function> equivalent */
#undef dlopen
#define dlopen gb_dlopen
extern void *gb_dlopen(const char *__file, int __mode);


#undef dlclose
#define dlclose gb_dlclose
extern int gb_dlclose(void *__handle);

#undef dlsym
#define dlsym gb_dlsym
extern void *gb_dlsym(void * __handle, const char * __name);


#ifdef __cplusplus
}
#endif

#endif /* GB_LIBRARY_INTERCEPT*/

#endif /* !GB_LIBRARY_H */
