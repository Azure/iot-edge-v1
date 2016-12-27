# How-To (Node JS)
This document describes how to prepare your development environment to use the *Microsoft Azure IoT Gateway SDK* for Node JS module development.

- [Node JS](https://nodejs.org/)
- [Google V8](https://developers.google.com/v8/)
- [libuv](http://libuv.org/)

## Prerequisites
- Install [Python 2.7](https://www.python.org/downloads/release/python-2712/)
- Clone https://github.com/Azure/azure-iot-gateway-sdk.git

## Building
### Windows
Open a Developer Command Prompt for VS and: 

- cd <\<your azure repo>>\tools
- Run build_nodejs.cmd ==> build_nodejs.cmd downloads and builds Node JS from source as runtime linked modules.
- Set 2 environment variables : `NODE_INCLUDE=<your azure-iot-gateway_root>\build_nodejs\dist\inc` and `NODE_LIB=<your aziore-iot-gateway_root>\%build-root%\build_nodejs\dist\lib`. These envioronment variables are a path to node.dll library and header files used by our Node JS binding. 
- run build.cmd --enable-nodejs-binding


### Linux
Open a terminal window and run the following commands:

- cd <\<your azure gateway repo>>\tools
- build_nodejs.sh ==> build_nodejs.sh downloads and builds Node JS from source as runtime linked modules.
- Copy and paste the export message that shows up on screen so you have the NODE_INCLUDE and NODE_LIB environment variables set.
- build.sh --enable-nodejs-binding


## Running simple sample
### Windows
In order to run a gateway with a Node.js module do the following:

- Edit file under <\<your azure gateway repo>>\samples\nodejs_simple_sample\src\gateway_sample.json
- Copy the configuration values from the sample given below.
- On folder <\<your azure gateway repo>>\build\samples\nodejs_simple_sample\Debug run the following command: 
- nodejs_simple_sample ..\..\..\..\samples\nodejs_simple_sample\src\gateway_sample_win.json

```
{
    "loaders": [
        {
            "type": "node",
            "name": "node",
            "configuration": {
                "binding.path": "..\\..\\..\\bindings\\nodejs\\Debug\\nodejs_binding.dll"
            }
        }
    ],
    "modules": [
        {
            "name": "node_printer",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/printer.js"
                }
            },
            "args": null
        },
        {
            "name": "node_sensor",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/sensor.js"
                }
            },
            "args": null
        },
        {
            "name": "iothub_writer",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/iothub_writer.js"
                }
            },
            "args": {
                "connection_string": "<<IoT Hub Device Connection String>>"
            }
        },
        {
            "name": "Logger",
            "loader": {
                "name": "native",
                "entrypoint": {
                    "module.path": "..\\..\\..\\modules\\logger\\Debug\\logger.dll"
                }
            },
            "args": {
                "filename": "log.txt"
            }
        }
    ],
    "links": [
        {
            "source": "*",
            "sink": "Logger"
        },
        {
            "source": "node_sensor",
            "sink": "iothub_writer"
        },
        {
            "source": "node_sensor",
            "sink": "node_printer"
        }
    ]
}
```


### Linux
On a terminal windows follow these steps:
- Assuming that your azure-iot-gateway-sdk was cloned at ~/azure-iot-gateway-sdk/
- Edit file under ~/azure-iot-gateway-sdk/samples/nodejs_simple_sample/src/gateway_sample.json
- Copy the configuration values from the sample given below.
- On folder ~/azure-iot-gateway-sdk/build/samples/nodejs_simple_sample run the following command: 
- ./nodejs_simple_sample ../../../samples/nodejs_simple_sample/src/gateway_sample_lin.json

Here is a sample of the gateway_sample.json file filled:
```
{
"loaders": [
        {
            "type": "node",
            "name": "node",
            "configuration": {
                "binding.path": "../../../build/bindings/nodejs/libnodejs_binding.so"
            }
        }
    ],

    "modules": [
        {
            "name": "node_printer",
            "loader": {
                "name": "node",
                "entrypoint": {
                "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/printer.js"
				}
           },
            "args": null
        },
        {
            "name": "node_sensor",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/sensor.js"
                }
            },
            "args": null
        },
        {
            "name": "iothub_writer",
            "loader": {
                "name": "node",
                "entrypoint": {
                    "main.path": "../../../samples/nodejs_simple_sample/nodejs_modules/iothub_writer.js"
                }
            },
            "args": {
                "connection_string": "<<IoT Hub Device Connection String>>"
            }
        },
        {
            "name": "Logger",
            "loader": {
                "name": "native",
                "entrypoint": {
                    "module.path": "../../../build/modules/logger/liblogger.so"
                }
            },
            "args": {
                "filename": "<<path to log file>>"
            }
        }
    ],
    "links": [
        {
            "source": "*",
            "sink": "Logger"
        },
        {
            "source": "node_sensor",
            "sink": "iothub_writer"
        },
        {
            "source": "node_sensor",
            "sink": "node_printer"
        }
    ]
}
```

On successful run you should see **sample** output like this

`````
Gateway is running. Press return to quit.
printer.receive - 47, 33
printer.receive - 11, 12
printer.receive - 39, 30
printer.receive - 30, 21
printer.receive - 12, 0

Gateway is quitting
printer.destroy
sensor.destroy
iothub_writer.destroy
`````
