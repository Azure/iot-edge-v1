.NET Core Managed Gateway Sample
================================

Overview
--------

This sample showcases how one might build an IoT gateway as a .NET Core Application and still be able to load .NET Core Modules. 

The sample contains:

1. A printer module (C#) that interprets telemetry from sensor and prints its content and properties into Console.
2. A sensor module (C#) that publishes random data to the gateway.
3. A logger module for producing message broker diagnostics.

Other resources:
- Hello World [sample](../hello_world/README.md)
- .NET Core Sample [dotnetcoresample] (../dotnet_core_module_sample/README.md)
- [Devbox setup](../../doc/devbox_setup.md)
- [.Net Core Framework Installer](https://www.microsoft.com/net/download/core)

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
1. Open azure_iot_gateway_sdk solution and configure project `dotnet_core_managed_gateway` as a Startup Sample.
2. Go to the Project Properties and change `Command Arguments` to point to dotnet_core_managed_gateway_[win or lin].json.
3. Make sure you copy all the dependencies libraries to your working directory (Right now they are: `gateway.dll`, `dotnetcore.dll`, `nanomsg.dll` and `aziotsharedutil.dll` for windows and relative .so for linux); *for Linux copy libdotnetcore.so to the bin folder(where your console app dll is located).
4. Run.




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