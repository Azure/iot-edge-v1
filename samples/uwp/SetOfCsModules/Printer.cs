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
    public sealed class Printer : IGatewayModule
    {
        public void Create(MessageBus bus, IReadOnlyDictionary<string, string> configuration)
        {
            System.Diagnostics.Debug.WriteLine("SetOfCsModules.Printer.Create");
        }

        public void Destroy()
        {
            System.Diagnostics.Debug.WriteLine("SetOfCsModules.Printer.Destroy");
        }

        public void Receive(Message received_message)
        {
            var contentBytes = received_message.GetContent();
            StringBuilder content = new StringBuilder();
            for (int i = 0; i < contentBytes.Count; i++)
            {
                char ch = (char)contentBytes[i];
                if (ch != '\0')
                {
                    content.Append(ch);
                }
            }

            var props = received_message.GetProperties();
            StringBuilder propsString = new StringBuilder();
            foreach (var key in props.Keys)
            {
                propsString.Append(key);
                propsString.Append("=");
                propsString.Append(props[key] as string);
                propsString.Append(", ");
            }

            StringBuilder sb = new StringBuilder();
            sb.Append("SetOfCsModules.Printer.Receive: Content=" + content.ToString() + "\r\n");
            sb.Append("SetOfCsModules.Printer.Receive: Properties={" + propsString.ToString() + "}\r\n");
            System.Diagnostics.Debug.Write(sb.ToString());
        }
    }
}
