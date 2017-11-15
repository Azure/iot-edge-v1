/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.remote;

import com.microsoft.azure.gateway.core.Broker;
import com.microsoft.azure.gateway.core.IGatewayModule;

class TestModuleImplementsInterface implements IGatewayModule {

    public TestModuleImplementsInterface() {
    }
    
    @Override
    public void create(long moduleAddr, Broker broker, String configuration) {
    }

    @Override
    public void start() {
    }

    @Override
    public void receive(byte[] source) {
    }

    @Override
    public void destroy() {
    }
    
}

