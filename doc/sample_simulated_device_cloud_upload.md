# Simulated Device Cloud Upload sample for Azure IoT Gateway SDK

This sample contains the following modules:

1. A logger module
2. Two SIMULATED_DEVICE module instances 
3. An identity mapping module
4. An IoTHubHTTP module

How it works: the two simulated device modules will produce messages containing simulated temperature readings and MAC address.
These messages are received by the identity mapping module that will add the device name and the device key to the messages.
Once messages carrying device name, device key and temperature are received by the IotHubHTTP module, they will be 
uploaded to IoTHub on a multiplexed connection.

The logger module logs all the exchanges on the message bus to a file. 

## How to build the sample:
Linux

1. Open a shell
2. Navigate to `azure-iot-gateway-sdk\tools\`
3. Run `build.sh`

>Note: `build.sh` does multiple things. It builds the project and places it in the "build" folder in the root of the repo. This folder is deleted and recreated every time `build.sh` is run. Additionally, `build.sh` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `cmake <path to repo root>`

>run `make -j $(nproc)`

>To run the tests:

>run `ctest -j $(nproc) -C Debug --output-on-failure`

Windows

1. Open a Developer Command for VS2015 
2. Navigate to `azure-iot-gateway-sdk/tools/`
3. Run `build.cmd`

>Note: `build.cmd` does multiple things. It builds a solution ('azure_iot_gateway_sdk.sln') and places it in the "build" folder in the root of the repo. This folder is deleted and recreated every time `build.cmd` is run. Additionally, `build.cmd` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `cmake <path to repo root>`

>run `msbuild /m /p:Configuration="Debug" /p:Platform="Win32" azure_iot_gateway_sdk.sln`

>To run the tests:

>run `ctest -C Debug -V`

## How to run the sample:
Linux

1. Navigate to `azure-iot-gateway-sdk/build`.

2. Run `$ ./samples/simulated_device_cloud_upload/simulated_device_cloud_upload_sample <path to JSON config file>`

>Note: The simulated device cloud upload process takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk/samples/simulated_device_cloud_upload/src/simulated_device_cloud_upload_lin.json'. 

Windows

1. Navigate to `azure-iot-gateway-sdk\build\samples\simulated_device_cloud_upload\Debug`.

2. Run `.\simulated_device_cloud_upload_sample.exe <path to JSON config file>`

>Note: The simulated device cloud upload process takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk\samples\simulated_device_cloud_upload\src\simulated_device_cloud_upload_win.json'.

