# Device Ping tool for Azure IoT Gateway SDK

These are step by step instructions on how to build and run this tool.

This tool contains the following modules:

1. A logger module
2. A iot hub device ping module

How it works:
The iot hub device ping module will create a thread to send a message to the destination device previously provisioned in iot hub. Upon successful delivery, the message will be sinked to logger module for logging. The message delivery was done through iot hub API e.g. IoTHubClient_LL_SendEventAsync whose callback contains logic to poll the backend event hub to see if the message has arrived or delivery is timed out. The polling thread will simutaneously launch x number of worker threads to carry out the work for x number of event hub partitions. Whichever thread finds the wanted message will stop the rest from running.

The logger module logs all the activities on the message broker to a file.

## How to build the tool:
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

## How to run the tool:
Linux

- In a text editor, open the file `samples/device_ping/src/device_ping_lin.json` in your local copy
of the azure-iot-gateway-sdk repository. You need to fill the following information:

>`DeviceConnectionString`: Go to your iot hub 'Device Details' page and set the value to 'Connection string—primary key'.

>`EH_HOST`: Go to your iot hub 'Messaging' page and set the value to 'Event Hub-compatible endpoint'.

>`EH_KEY_NAME`: Go to your iot hub 'Shared access policies' page and set the value to the policy name that has 'registry write', 'service connect' and 'device connect' permissions e.g. iothubowner

>`EH_KEY`: The 'Primary key' of `EH_KEY_NAME`.

>`EH_COMP_NAME`: Go to your iot hub 'Messaging' page and set the value to 'Event Hub-compatible name'.

>`EH_PARTITION_NUM`: Go to your iot hub 'Messaging' page and set the value to 'Partitions'.

- Navigate to `azure-iot-gateway-sdk/build`.

- Run `$ ./samples/device_ping/device_ping <path to JSON config file>`

>Note: The device ping tool takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk/samples/device_ping/src/device_ping_lin.json'.

Windows

- In a text editor, open the file `samples/device_ping/src/device_ping_lin.json` in your local copy
of the azure-iot-gateway-sdk repository. You need to fill the following information:

>`DeviceConnectionString`: Go to your iot hub 'Device Details' page and set the value to 'Connection string—primary key'.

>`EH_HOST`: Go to your iot hub 'Messaging' page and set the value to 'Event Hub-compatible endpoint'.

>`EH_KEY_NAME`: Go to your iot hub 'Shared access policies' page and set the value to the policy name that has 'registry write', 'service connect' and 'device connect' permissions e.g. iothubowner

>`EH_KEY`: The 'Primary key' of `EH_KEY_NAME`.

>`EH_COMP_NAME`: Go to your iot hub 'Messaging' page and set the value to 'Event Hub-compatible name'.

>`EH_PARTITION_NUM`: Go to your iot hub 'Messaging' page and set the value to 'Partitions'.

Note: if you don't have D driver on your host machine, you'll need to change `filename` to other path.

- Navigate to `azure-iot-gateway-sdk\build\samples\device_ping\Debug`.

- Run `.\device_ping.exe <path to JSON config file>`

>Note: The device ping tool takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk\samples\device_ping\src\device_ping_win.json'.

