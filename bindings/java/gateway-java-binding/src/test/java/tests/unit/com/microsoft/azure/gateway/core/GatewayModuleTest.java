/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package tests.unit.com.microsoft.azure.gateway.core;

import com.microsoft.azure.gateway.core.GatewayModule;
import com.microsoft.azure.gateway.core.MessageBus;
import com.microsoft.azure.gateway.messaging.Message;
import mockit.Mocked;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class GatewayModuleTest {

    @Mocked(stubOutClassInitialization = true)
    protected MessageBus mockBus;

    /*Codes_SRS_JAVA_GATEWAY_MODULE_14_001: [ The constructor shall save address, bus, and configuration into class variables. ]*/
    @Test
    public void constructorSavesAllDataSuccess(){
        long address = 0x12345678;
        String configuration = "\"test-configuration\"";

        GatewayModule module = new TestModule(mockBus, configuration);

        MessageBus expectedBus = module.getMessageBus();
        String expectedConfiguration = module.getConfiguration();

        assertEquals(mockBus, expectedBus);
        assertEquals(configuration, expectedConfiguration);
    }

    /*Codes_SRS_JAVA_GATEWAY_MODULE_14_002: [ If address or bus is null the constructor shall throw an IllegalArgumentException. ]*/
    @Test(expected = IllegalArgumentException.class)
    public void constructorThrowsExceptionForNullMessageBus(){
        long address = 0x12345678;

        GatewayModule module = new TestModule(null, null);
    }

    public class TestModule extends GatewayModule{

        /**
         * Constructs a {@link GatewayModule} from the provided address and {@link MessageBus}. A {@link GatewayModule} should always call this super
         * constructor before any module-specific constructor code.
         *
         * @param bus           The {@link MessageBus} to which this module belongs
         * @param configuration The module-specific configuration
         */
        public TestModule(MessageBus bus, String configuration) {
            super(bus, configuration);
        }

        @Override
        public void receive(Message message) {

        }

        @Override
        public void destroy() {

        }
    }
}
