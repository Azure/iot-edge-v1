// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

//Windows Service not yet supported. 
int configureAsAService(void)
{
    return 0;
}


void waitForUserInput(void)
{
    (void)printf("Press return to exit the application. \r\n");
    (void)getchar();
}
