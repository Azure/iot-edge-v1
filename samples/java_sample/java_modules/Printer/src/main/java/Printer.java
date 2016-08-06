/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

import com.microsoft.azure.gateway.core.GatewayModule;
import com.microsoft.azure.gateway.core.Broker;
import com.microsoft.azure.gateway.messaging.Message;

public class Printer extends GatewayModule {
    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link Broker}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * @param address       The address of the native module pointer
     * @param broker        The {@link Broker} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public Printer(long address, Broker broker, String configuration) {
        super(address, broker, configuration);
    }

    @Override
    public void receive(Message message) {
        System.out.println("Printer Module Received: " + message);
    }

    @Override
    public void destroy() {
        //No cleanup necessary
    }
}
