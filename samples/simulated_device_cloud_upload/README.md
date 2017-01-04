# Simulated Device Cloud Upload sample for Azure IoT Gateway SDK

These are step by step instructions on how to build and run this sample. A more in depth walkthrough, for 
both [Linux](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-linux-gateway-sdk-simulated-device) and
[Windows](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-windows-gateway-sdk-simulated-device), with 
concept explanations and code snippets can be found on [docs.microsoft.com](https://docs.microsoft.com/en-us/azure/iot-hub/).

This sample contains the following modules:

1. A logger module
2. Two SIMULATED_DEVICE module instances 
3. An identity mapping module
4. An IoT Hub module

How it works: the two simulated device modules will produce messages containing simulated temperature readings and MAC address.
These messages are received by the identity mapping module, which will add the device ID and key to the messages. Once messages
carrying device ID, key, and temperature are received by the IotHub module, they will be sent to IoT Hub.

The logger module logs all the exchanges on the message broker to a file. 

## How to build the sample:
Linux

1. Open a shell
2. Navigate to `azure-iot-gateway-sdk\tools\`
3. Run `build.sh`

>Note: `build.sh` does multiple things. It builds the project and places it in the "build" folder in the root of the repo. This folder is deleted and recreated every time `build.sh` is run. Additionally, `build.sh` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `sudo cmake <path to repo root>`

>Note: The cmake scripts will check for installed cmake dependencies on your machine before it attempts to fetch, build, and install them for you.

>run `make -j $(nproc)`

>To run the tests:

>run `ctest -j $(nproc) -C Debug --output-on-failure`

Windows

1. Open a Developer Command for VS2015 as an Administrator
2. Navigate to `azure-iot-gateway-sdk/tools/`
3. Run `build.cmd`

>Note: `build.cmd` does multiple things. It builds a solution ('azure_iot_gateway_sdk.sln') and places it in the "build" folder in the root of the repo. This folder is deleted and recreated every time `build.cmd` is run. Additionally, `build.cmd` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `cmake <path to repo root>`

>Note: The cmake scripts will check for installed cmake dependencies on your machine before it attempts to fetch, build, and install them for you.

>run `msbuild /m /p:Configuration="Debug" /p:Platform="Win32" azure_iot_gateway_sdk.sln`

>To run the tests:

>run `ctest -C Debug -V`

## How to run the sample:
Linux

- In a text editor, open the file `samples/simulated_device_cloud_upload/src/simulated_device_cloud_upload_lin.json` in your local copy
of the azure-iot-gateway-sdk repository. This file configures the modules in the sample gateway:

>Note: You will need connection information for two devices in IoT Hub before continuing. Refer to the sample [walkthrough](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-linux-gateway-sdk-simulated-device)
if you need help getting this information. 

>The `IoTHub` module connects to your IoT hub. You must configure it to send data to your IoT hub. Specifically, set the `IoTHubName`
value to the name of your IoT hub and set the value of `IoTHubSuffix` to `azure-devices.net`.

>The `mapping` module maps the MAC addresses of your simulated devices to your IoT Hub device ids. Make sure that `deviceId` values
match the ids of the two devices you added to your IoT hub, and that the `deviceKey` values contain the keys of your two devices.

>The `BLE1` and `BLE2` modules are the simulated devices. Note how their MAC addresses match those in the `mapping` module.

>The `Logger` module logs your gateway activity to a file.

>The `module path` values shown below assume that you will run the sample from the root of your local copy of the `azure-iot-gateway-sdk` repository.

- Navigate to `azure-iot-gateway-sdk/build`.

- Run `$ ./samples/simulated_device_cloud_upload/simulated_device_cloud_upload_sample <path to JSON config file>`

>Note: The simulated device cloud upload process takes the path to a JSON configuration file as an argument in the command line. The file must be encoded either as ASCII or UTF-8. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk/samples/simulated_device_cloud_upload/src/simulated_device_cloud_upload_lin.json'. 

Windows

- In a text editor, open the file `samples/simulated_device_cloud_upload/src/simulated_device_cloud_upload_win.json` in your local copy
of the azure-iot-gateway-sdk repository. This file configures the modules in the sample gateway:

>Note: You will need connection information for two devices in IoT Hub before continuing. Refer to the sample [walkthrough](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-windows-gateway-sdk-simulated-device)
if you need help getting this information. 

>The `IoTHub` module connects to your IoT hub. You must configure it to send data to your IoT hub. Specifically, set the `IoTHubName`
value to the name of your IoT hub and set the value of `IoTHubSuffix` to `azure-devices.net`.

>The `mapping` module maps the MAC addresses of your simulated devices to your IoT Hub device ids. Make sure that `deviceId` values
match the ids of the two devices you added to your IoT hub, and that the `deviceKey` values contain the keys of your two devices.

>The `BLE1` and `BLE2` modules are the simulated devices. Note how their MAC addresses match those in the `mapping` module.

>The `Logger` module logs your gateway activity to a file.

>The `module path` values shown below assume that you will run the sample from the root of your local copy of the `azure-iot-gateway-sdk` repository.

- Navigate to `azure-iot-gateway-sdk\build\samples\simulated_device_cloud_upload\Debug`.

- Run `.\simulated_device_cloud_upload_sample.exe <path to JSON config file>`

>Note: The simulated device cloud upload process takes the path to a JSON configuration file as an argument in the command line. The file must be encoded either as ASCII or UTF-8. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk\samples\simulated_device_cloud_upload\src\simulated_device_cloud_upload_win.json'.

