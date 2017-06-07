# Remote module sample (Node.js)

This sample demonstrates how a Node.js app can act as an Azure IoT Edge module by using the `azure-iot-proxy-gateway` package. By default, IoT Edge loads modules from shared libraries and runs them in-process, but this package allows any Node.js application to communicate with an IoT Edge application running on the same device.

The sample defines a simple IoT Edge module called "forward" that forwards any message it receives. It meets the definition of an IoT Edge module by implementing the `create`, `start`, `receive`, and `destroy` functions. For more module samples, see the [nodejs_modules folder of nodejs_simple_sample](https://github.com/Azure/iot-edge/tree/master/samples/nodejs_simple_sample/nodejs_modules).

The IoT Edge application--a native application that runs in another process--links the input and output of this module to other IoT Edge modules.

This Node.js application uses the `azure-iot-proxy-gateway` package to act as an IoT Edge module. It creates a new instance of the ProxyGateway class and gives it an instance of the "forward" module. Then it uses the `attach` and `detach` methods on the class to establish communication with the IoT Edge application.

## Instructions

To run this sample:

1. Install [Node.js](https://nodejs.org/en/download/) version 6.0 or later.

1. Build the IoT Edge code with the Node.js out-of-process modules feature enabled. If you're using a build script (`tools/build.sh` or `tools\build.cmd`), pass the `--enable-nodejs-remote-modules` argument. If you're using CMake directly, pass the `enable_nodejs_remote_modules` option (e.g., `cmake -Denable_nodejs_remote_modules=ON ..`).

1. Follow the instructions in the [proxy gateway sample](../proxy_sample/README.md) to build and configure an IoT Edge application that communicates with an out-of-process module.

1. In the `proxy/gateway/nodejs` folder, run `npm install` to install the package's dependencies.

1. In the `samples/nodejs_remote_sample` directory, run `npm install` to install the sample's dependencies.

1. Start the `proxy_gateway` app from one terminal like this:

    ### Linux/Mac
    ```
    cd build/samples/proxy_sample
    ./proxy_sample ../../../samples/proxy_sample/src/proxy_sample_lin.json
    ```
    
    ### Windows
    ```
    cd build\samples\proxy_sample
    Debug\proxy_sample.exe ..\..\..\samples\proxy_sample\src\proxy_sample_win.json
    ```

1. Start the `nodejs_remote_sample` Node.js app from _another_ terminal like this:

    ### Linux/Mac
    ```
    cd samples/nodejs_remote_sample
    node . /tmp/proxy_sample.control
    ```
    
    ### Windows
    ```
    cd build\samples\proxy_sample
    node . outprocess_module_control
    ```

When both applications are running, you'll see this in the `proxy_sample` terminal:
```
$ ./proxy_sample ~/azure-iot-gateway-sdk/samples/proxy_sample/src/proxy_sample_lin.json
gateway successfully created from JSON
gateway shall run until ENTER is pressed
```

...and this in the `nodejs_remote_sample` terminal:
```
$ node . /tmp/proxy_sample.control
forward.receive - hello world
forward.receive - hello world
...
```