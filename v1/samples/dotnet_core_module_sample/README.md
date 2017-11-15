.NET Core Sample
================

Overview
--------

This sample showcases how one might build modules for IoT gateway in .NET Core.

The sample contains:

1. A printer module (C#) that interprets telemetry from sensor and prints its content and properties into Console.
2. A sensor module (C#) that publishes random data to the gateway.
3. A logger module for producing message broker diagnostics.

Other resources:
- Hello World [sample](../hello_world/README.md)
- [Devbox setup](../../doc/devbox_setup.md)
- [.NET Core Installation Guide](https://www.microsoft.com/net/core)

Prerequisites
--------------
1. Setup your development machine. A guide for doing this can be found [here](../../doc/devbox_setup.md).
2. Make sure you have .NET Core Framework installed. Our current version of the binding was tested and loads modules written in .NET **Core** v1.1.1.

Building the sample
-------------------
The sample gateway gets built when you build Azure IoT Edge by first running
```
tools\build.cmd --enable-dotnet-core-binding
```

The [devbox setup](../../doc/devbox_setup.md) guide has more information on how you can build Azure IoT Edge.

The `build_dotnet_core.cmd` script builds the .NET Core binding, and the sample sensor and printer modules.
Building the solution you will have the following binaries: 
1. Microsoft.Azure.Devices.Gateway.dll
2. SensorModule.dll
3. PrinterModule.dll

Running the sample
------------------
1. Open azure_iot_gateway_sdk solution and configure project `dotnet_core_module_sample` as a Startup Sample.
2. Go to the Project Properties and change `Command Arguments` to point to dotnet_core_module_sample.json.
3. Run.




Json Configuration
------------------
```json
{
    "modules": [
        {
            "name": "logger",
            "loader": {
                "name": "native",
                "entrypoint": {
                    "module.path": "..\\..\\..\\modules\\logger\\Debug\\logger.dll"
                }
            },
            "args": { "filename": "C:\\Temp\\Log.txt" }
        },
        {
            "name": "dotnet_sensor_module",
            "loader": {
                "name": "dotnetcore",
                "entrypoint": {

                    "assembly.name": "SensorModule",
                    "entry.type": "SensorModule.DotNetSensorModule"
                }
            },
            "args": "module configuration"
        },
        {
            "name": "dotnet_printer_module",
            "loader": {
                "name": "dotnetcore",
                "entrypoint": {
                    "assembly.name": "PrinterModule",
                    "entry.type": "PrinterModule.DotNetPrinterModule"
                }
            },
            "args": "module configuration"
        }
    ],
    "links": [
      {"source": "dotnet_sensor_module", "sink": "dotnet_printer_module" }
    ]
}
```

Creating your own module
------------------------
1. Create a .NET Core project (DLL) (Class library).
2. Add Reference to Microsoft.Azure.Devices.Gateway DLL.
3. On your class you shall implement `IGatewayModule`.
   See the Printer module as example:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ C#
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
            if(received_message.Properties["source"] == "sensor")
            {
                Console.WriteLine("Printer Module received message from Sensor. Content: " + System.Text.Encoding.UTF8.GetString(received_message.Content, 0, received_message.Content.Length));
            }
        }
    }
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Modules may also implement the `IGatewayModuleStart` interface.  The Start method is called when the gateway's Broker is ready for the module to send and receive messages. 

4. Add your new module on Json configuration:
```json
{
    "modules" :
    [
        {
            "name": "dotnet_printer_module", ==> Your new module name. 
            "loader": {
                "name": "dotnetcore",
                "entrypoint": {

                    "assembly.name": "SensorModule",==> This is the name of your module dll. On this sample it is SensorModule.dll
                    "entry.type": "SensorModule.DotNetSensorModule" ==> This is the name of your Class (Namespace.ClassName) that implements IGatewayModule.
                }
            },
            "args": "module configuration"  ==> This is any configuration you want to use on your sample. It will be passed to you as a byte[] that should be converted to an UTF-8 Encoded String, you can add a JSON configuration in it.
        }
    ]
}
```
5. If you want to load .NET Core CLR from a different location you can add a `loader` item on your JSON Configuration. Sample with CLR path:
```json
{
     "loaders": [
        {
            "type": "dotnetcore",
            "name": "dotnetcore",
            "configuration": {
                "binding.path": "..\\..\\..\\bindings\\dotnetcore\\Debug\\dotnetcore.dll",
                "binding.coreclrpath": "<Path to your .NET Core Path.>",
                "binding.trustedplatformassemblieslocation": "<Path to find trusted platform assemblies, used by CLR."
            }
        }
    ],
    "modules" :
    [
        {
            "name": "dotnet_printer_module", ==> Your new module name. 
            "loader": {
                "name": "dotnetcore",
                "entrypoint": {

                    "assembly.name": "SensorModule",==> This is the name of your module dll. On this sample it is SensorModule.dll
                    "entry.type": "SensorModule.DotNetSensorModule" ==> This is the name of your Class (Namespace.ClassName) that implements IGatewayModule.
                }
            },
            "args": "module configuration"  ==> This is any configuration you want to use on your sample. It will be passed to you as a byte[] that should be converted to an UTF-8 Encoded String, you can add a JSON configuration in it.
        }
    ]
}
```
