// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.IoT.Gateway;

namespace SetOfCsModules
{
    public sealed class Sensor : IGatewayModule
    {
        public void Create(MessageBus bus, IReadOnlyDictionary<string, string> configuration)
        {
            System.Diagnostics.Debug.WriteLine("SetOfCsModules.Module1.Create");

            new Task(() => {
                int i = 0;
                while (true)
                {
                    new System.Threading.ManualResetEvent(false).WaitOne(1000);

                    i = (i + 1) % 500;

                    var props = new Dictionary<string, string>();
                    props.Add("ReadingNumber", i.ToString());
                    bus.Publish(new Microsoft.Azure.IoT.Gateway.Message("SetOfCsModules.Sensor reading", props));
                }
            }).Start();
        }

        public void Destroy()
        {
            System.Diagnostics.Debug.WriteLine("SetOfCsModules.Module1.Destroy");
        }

        public void Receive(Message received_message)
        {
            // Sensor does not care about messages
        }
    }
}
