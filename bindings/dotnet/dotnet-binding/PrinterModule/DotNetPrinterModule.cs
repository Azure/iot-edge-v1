// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.IoT.Gateway;


namespace PrinterModule
{
    public class DotNetPrinterModule : IGatewayModule
    {
        private MessageBus busToPublish;
        private string configuration;
        public void Create(MessageBus bus, string configuration)
        {
            this.busToPublish = bus;
            this.configuration = configuration;
        }

        public void Destroy()
        {
            Console.WriteLine("This is C# Sensor Module Destroy!");
        }

        public void Receive(Message received_message)
        {
            if(received_message.Properties["source"] == "sensor")
            {
                Console.WriteLine("Printer Module received message from Sensor. Content: " + System.Text.Encoding.UTF8.GetString(received_message.Content, 0, received_message.Content.Length));
            }
        }
    }
}
