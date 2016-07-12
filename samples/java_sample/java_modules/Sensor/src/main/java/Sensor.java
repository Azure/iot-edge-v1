/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

import com.microsoft.azure.gateway.core.GatewayModule;
import com.microsoft.azure.gateway.core.MessageBus;
import com.microsoft.azure.gateway.messaging.Message;

import java.io.IOException;
import java.util.HashMap;

public class Sensor extends GatewayModule {

    private boolean threadStop;

    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link MessageBus}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * @param address       The address of the native module pointer
     * @param bus           The {@link MessageBus} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public Sensor(long address, MessageBus bus, String configuration) {
        super(address, bus, configuration);
        this.threadStop = false;

        new Thread(() -> {
            while(!this.threadStop) {
                try {
                    HashMap<String, String> map = new HashMap<>();
                    map.put("Source", new Long(this.hashCode()).toString());

                    //Get "sensor" reading
                    Double sensorReading = Math.random() * 50;

                    //Construct message
                    Message m = new Message(sensorReading.toString().getBytes(), map);

                    //Publish message
                    this.publish(m);

                    //Sleep
                    Thread.sleep(500);
                } catch (IOException e) {
                    e.printStackTrace();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }

    @Override
    public void receive(Message message) {
        //Ignores incoming messages
    }

    @Override
    public void destroy() {
        //Causes the publishing thread to stop
        this.threadStop = true;
    }
}
