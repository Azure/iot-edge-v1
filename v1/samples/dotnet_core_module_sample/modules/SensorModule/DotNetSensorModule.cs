// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.Devices.Gateway;
using System.Threading;

namespace SensorModule
{
    public class DotNetSensorModule : IGatewayModule, IGatewayModuleStart
    {
        private Broker broker;
        private string configuration;

        private Thread oThread;

        private bool quitThread = false;

        public void Create(Broker broker, byte[] configuration)
        {

            this.broker = broker;
            this.configuration = System.Text.Encoding.UTF8.GetString(configuration);

        }

        public void Start()
        {
            oThread = new Thread(new ThreadStart(this.threadBody));
            // Start the thread
            oThread.Start();
        }

        public void Destroy()
        {
            quitThread = true;
            oThread.Join();
        }

        public void Receive(Message received_message)
        {
            //Just Ignore the Message. Sensor doesn't need to print.
        }

        public void threadBody()
        {
            Random r = new Random();
            int n = r.Next();

            while (!quitThread)
            {
                Dictionary<string, string> thisIsMyProperty = new Dictionary<string, string>();
                thisIsMyProperty.Add("source", "sensor");

                Message messageToPublish = new Message("SensorData: " + n, thisIsMyProperty);

                this.broker.Publish(messageToPublish);

                //Publish a message every 5 seconds. 
                Thread.Sleep(5000);
                n = r.Next();
            }
        }
    }
}
