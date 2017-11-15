// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "gateway.h"

int main(int argc, char** argv)
{
    GATEWAY_HANDLE gateway;
    if (argc != 2)
    {
        printf("usage: proxy_sample configFile\n");
        printf("where configFile is the name of the file that contains the Gateway configuration\n");
    }
    else
    {
        if ((gateway = Gateway_CreateFromJson(argv[1])) == NULL)
        {
            printf("failed to create the gateway from JSON\n");
        }
        else
        {
            printf("gateway successfully created from JSON\n");
            printf("gateway shall run until ENTER is pressed\n");
            // Since the std descriptors are shared between the out processes in launch case
            // EOF is generated when the child process dies. Make sure the getchar
            // can survive this.
            int c;
            while((c = getchar()) == EOF)
            {
                printf("\nGot %d from getchar. Continuing to wait for ENTER\n", c);
            }
            Gateway_Destroy(gateway);
        }
    }
    return 0;
}
