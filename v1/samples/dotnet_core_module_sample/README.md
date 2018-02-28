.NET Core Sample
================

Overview
--------

This sample showcases how to build .NET Core modules for IoT Edge.

The sample contains:

1. A sensor module (C#) that sends randomly-generated temperature values.
2. A printer module (C#) that receives sensor data and prints it to the console.

Other resources:
- Hello World [sample](../hello_world/README.md)
- [Devbox setup](../../doc/devbox_setup.md)
- [.NET Core Installation Guide](https://www.microsoft.com/net/core)

Prerequisites
--------------
1. Setup your development machine. A guide for doing this can be found [here](../../doc/devbox_setup.md).
2. Make sure you have .NET Core installed. Our current version of the binding was tested and loads modules written in .NET Core v1.1.1.

Building the sample
-------------------
The sample IoT Edge application and modules are built with the following command:
```
tools\build.cmd --enable-dotnet-core-binding
```

The [devbox setup](../../doc/devbox_setup.md) guide has more information on how you can build Azure IoT Edge.

Running the sample (Windows)
----------------------------
1. Navigate to the **v1/build** folder in your local copy of the **iot-edge** repository.
2. Run the following command:
```
samples\dotnet_core_module_sample\Debug\dotnet_core_module_sample.exe ..\samples\dotnet_core_module_sample\src\dotnet_core_module_sample_win.json
```

Running the sample (Linux)
----------------------------
1. Navigate to the **v1/build/samples/dotnet_core_module_sample** folder in your local copy of the **iot-edge** repository.
2. Run the following command:
```
./dotnet_core_module_sample ../../../samples/dotnet_core_module_sample/src/dotnet_core_module_sample_lin.json
```


Creating your own module
------------------------
1. Create a .NET Core project (Class library).
2. Add a reference to the .NET Core binding, Microsoft.Azure.Devices.Gateway.dll.
3. Inherit `IGatewayModule` in your module class, and implement `Create`, `Destroy`, and `Receive`.

   _For examples, see the sample [Printer](modules/PrinterModule/DotNetPrinterModule.cs) and [Sensor](modules/SensorModule/DotNetSensorModule.cs) modules._

4. Optionally inherit `IGatewayModuleStart` in your module class, and implement `Start`. `Start` is called when the broker is ready to send and receive messages.
5. Add your new module to your application's JSON configuration:
```json
{
    "modules" :
    [
        {
            "name": "my_module",
            "loader": {
                "name": "dotnetcore",
                "entrypoint": {
                    "assembly.name": "MyModule",
                    "entry.type": "MyModule.ModuleClass"
                }
            },
            "args": "whatever my module needs, as a UTF-8 string"
        }
    ]
}
```

6. If you want to override the default locations for the .NET Core binding (dotnetcore.dll/libdotnetcore.so) or the .NET Core runtime (coreclr.dll/libcoreclr.so, and path to trusted platform assemblies), add an item to the `loaders` array in your JSON configuration. For example:
```json
{
     "loaders": [
        {
            "type": "dotnetcore",
            "name": "dotnetcore",
            "configuration": {
                "binding.path": "path/to/libdotnetcore.so",
                "binding.coreclrpath": "path/to/libcoreclr.so",
                "binding.trustedplatformassemblieslocation": "path/to/trusted/platform/assemblies"
            }
        }
    ],
}
```
