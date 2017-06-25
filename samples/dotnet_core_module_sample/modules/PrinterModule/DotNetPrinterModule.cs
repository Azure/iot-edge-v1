// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.Devices.Gateway;


namespace PrinterModule
{
    public class DotNetPrinterModule : IGatewayModule
    {
        private string configuration;
        public void Create(Broker broker, byte[] configuration)
        {
            this.configuration = System.Text.Encoding.UTF8.GetString(configuration);
        }

        public void Destroy()
        {
        }

        public void Receive(Message received_message)
        {
            if (received_message != null)
            {
                string messageData = System.Text.Encoding.UTF8.GetString(received_message.Content, 0, received_message.Content.Length);
                Console.WriteLine("{0}> Printer module received message: {1}", DateTime.Now.ToLocalTime(), messageData);
 
                int propCount = 0;
                foreach (var prop in received_message.Properties)
                {
                    Console.WriteLine("\tProperty[{0}]> Key={1} : Value={2}", propCount++, prop.Key, prop.Value);
                }
            }
        }
    }
}
