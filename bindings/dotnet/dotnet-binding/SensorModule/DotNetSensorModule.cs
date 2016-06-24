// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.IoT.Gateway;
using System.Threading;

namespace SensorModule
{
    public class DotNetSensorModule : IGatewayModule
    {
        private MessageBus busToPublish;
        private string configuration;

        public void Create(MessageBus bus, string configuration)
        {
            this.busToPublish = bus;
            this.configuration = configuration;

            Thread oThread = new Thread(new ThreadStart(this.threadBody));
            // Start the thread
            oThread.Start();
        }

        public void Destroy()
        {
            Console.WriteLine("This is C# Sensor Module Destroy!");
        }

        public void Receive(Message received_message)
        {
            //Just Ignore the Message. Sensor doesn't need to print.
        }

        public void threadBody()
        {
            Random r = new Random();
            int n = r.Next();

            while (true)
            {
                Dictionary<string, string> thisIsMyProperty = new Dictionary<string, string>();
                thisIsMyProperty.Add("source", "sensor");

                Message messageToPublish = new Message("SensorData: " + n, thisIsMyProperty);

                this.busToPublish.Publish(messageToPublish);

                //Publish a message every 5 seconds. 
                Thread.Sleep(5000);
                n = r.Next();
            }
        }
    }
}
