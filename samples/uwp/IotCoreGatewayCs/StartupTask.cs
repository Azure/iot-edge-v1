// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net.Http;
using Windows.ApplicationModel.Background;

// The Background Application template is documented at http://go.microsoft.com/fwlink/?LinkID=533884&clcid=0x409

namespace IotCoreGatewayCs
{
    public sealed class StartupTask : IBackgroundTask
    {
        BackgroundTaskDeferral deferral;
        Microsoft.Azure.IoT.Gateway.Gateway gateway;
        public void Run(IBackgroundTaskInstance taskInstance)
        {
            deferral = taskInstance.GetDeferral();

            var modules = new List<Microsoft.Azure.IoT.Gateway.IGatewayModule>();
            modules.Add(new SetOfCsModules.Printer());
            modules.Add(new SetOfCsModules.Sensor());

            var properties = new Dictionary<string, string>();
            properties.Add("ConfigProperty", "ConfigValue");
            gateway = new Microsoft.Azure.IoT.Gateway.Gateway(modules, properties);
        }
    }
}
