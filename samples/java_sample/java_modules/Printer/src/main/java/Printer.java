/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

import com.microsoft.azure.gateway.core.GatewayModule;
import com.microsoft.azure.gateway.core.MessageBus;
import com.microsoft.azure.gateway.messaging.Message;

public class Printer extends GatewayModule {
    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link MessageBus}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * @param address       The address of the native module pointer
     * @param bus           The {@link MessageBus} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public Printer(long address, MessageBus bus, String configuration) {
        super(address, bus, configuration);
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
