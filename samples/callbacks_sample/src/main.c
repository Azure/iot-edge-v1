// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "gateway_ll.h"

void my_callback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx)
{
    printf("Gateway is being destroyed, first callback called.\n");
}

void second_callback(GATEWAY_HANDLE gw, GATEWAY_EVENT event_type, GATEWAY_EVENT_CTX ctx)
{
	printf("Gateway is being destroyed, second callback called.\n");
}

int main(int argc, char** argv)
{
    GATEWAY_HANDLE gw = Gateway_LL_Create(NULL);
    Gateway_LL_AddEventCallback(gw, GATEWAY_DESTROYED, my_callback);
	Gateway_LL_AddEventCallback(gw, GATEWAY_DESTROYED, second_callback);
    printf("Gateway is running.\n");
    Gateway_LL_Destroy(gw);
	printf("Gateway was destroyed, exiting main function.\n");
}