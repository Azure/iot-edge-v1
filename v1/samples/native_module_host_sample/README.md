# Azure IoT Edge - native module host example

This document provides an overview of the out of process module [code](./src) which allows a native gateway module to run in a separate process.

## Concepts

This is an extension on the [out of process module example](../proxy_sample/README.md), the native module host allows a user to take an existing gateway module (including bindings) and run it in an external process. For details, please read the [outprocess high level design](../../proxy/outprocess/devdoc/outprocess_hld.md).


There are two executables "native\_gateway" and "native\_host\_sample". The "native\_gateway" is a gateway executable with an out of process module, and "native\_host\_sample" is the executable which implements the out of process module.

The two processes must establish a control channel to communicate gateway events to the module, such as module creation and deletion. In this example, the control channel name is given in the sample JSON configuration for "proxy\_sample" is provided on the command line to "proxy\_sample\_remote".  Once communication is established, the modules will sent gateway data over a messaging channel.

![](../../core/devdoc/media/outprocess-gateway-modules.png)

## How to run the sample

### Linux

The easiest way to run the sample is to start two terminal sessions, navigate to the `build/samples/proxy_sample` directory in each and run
"native\_gateway" in one and "native\_host\_sample" in the other.

Terminal 1:
```
cd <build root>
cd build/samples/proxy_sample
./native_gateway ../../../samples/native_host_sample/src/native_host_sample_lin.json
```

Terminal 2:
```
cd <build root>
cd build/samples/proxy_sample
./native_host_sample outprocess_module_control
```

### Windows

Start two command prompts, navigate to the `build\samples\native_host_sample` directory in each and run "native\_gateway" in one and "native\_host\_sample" in the other.

Command prompt 1:
```
cd <build root>
cd build\samples\native_host_sample
Debug\native_gateway.exe ..\..\..\samples\native_host_sample\src\native_host_sample_win.json
```

Command prompt 2:
```
cd <build root>
cd build\samples\native_host_sample
Debug\native_host_sample.exe outprocess_module_control
```

You may also run the executables simultaneously by opening up the solution file, opening the properties for the top level solution, select "Multiple startup projects", and choose "native\_gateway" in one and "native\_host\_sample" to start.

## Example Gateway configuration

This sample gateway has three modules, the "Hello World" module, which periodically generates a message, the "out of process" module which configures the gateway for communication to the "native\_host\_sample" process, and a "logger" module to record any messages returning from the out of process module.

```JSON
{
  "modules": [
    {
      "name": "out of process logger",
      "loader": {
        "name": "outprocess",
        "entrypoint": {
          "activation.type": "none",
          "control.id": "outprocess_module_control"
        }
      },
      "args": {
        "outprocess.loader" : {
          "name": "native",
          "entrypoint": {
            "module.path": "..\\..\\..\\modules\\logger\\Debug\\logger.dll"
          }
        },
        "module.arguments" : {
          "filename": "log.txt"
        }
      }
    },
    {
      "name": "hello_world",
      "loader": {
        "name": "native",
        "entrypoint": {
          "module.path": "..\\..\\..\\modules\\hello_world\\Debug\\hello_world.dll"
        }
      },
      "args": null
    }
  ],
  "links": [
    {
      "source": "hello_world",
      "sink": "out of process logger"
    }
  ]
}
```
