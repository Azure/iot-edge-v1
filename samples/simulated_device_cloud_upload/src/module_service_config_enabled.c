// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/xlogging.h"
#include <unistd.h>

int configureAsAService(void)
{
    int returnValue;
    int functionReturn;
    
    returnValue = chdir("/");
    
    if (returnValue != 0)
    {
        LogError("Could not change working dir. Error Code: %d.", returnValue);
        functionReturn = __LINE__;
    }
    else
    {
        returnValue = daemon(0, 0);
        if (returnValue != 0)
        {
            LogError("Could not detach program from terminal. Error Code: %d.", returnValue);
            functionReturn = __LINE__;
        }
        else
        {
            functionReturn = 0;
        }
    }

	return functionReturn;
}

void waitForUserInput(void)
{
    while(1)
    {
       sleep(1000); //Stays here till the service is stopped. 
    }       
}
