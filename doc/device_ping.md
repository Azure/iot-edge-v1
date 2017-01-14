# Device Ping tool for Azure IoT Gateway SDK

These are step by step instructions on how to build and run this tool.

This tool contains the following modules:

1. A logger module
2. A IoT Hub device ping module

How it works:
The IoT Hub device ping module will create a thread to send a message to the destination device previously provisioned in IoT Hub. Upon successful delivery, the message will be sinked to logger module for logging. The message delivery was done through IoT Hub API e.g. IoTHubClient_LL_SendEventAsync whose callback contains logic to poll the backend event hub to see if the message has arrived or delivery is timed out. The polling thread will simutaneously launch x number of worker threads to carry out the work for x number of event hub partitions. Whichever thread finds the wanted message will stop the rest from running.

The logger module logs all the activities on the message broker to a file.

## How to run the tool:
Linux

- In a text editor, open the file `samples/device_ping/src/device_ping_lin.json` in your local copy
of the azure-iot-gateway-sdk repository. You need to fill the following information:

>`DeviceConnectionString`: Go to your IoT hub 'Device Details' page and set the value to 'Connection string—primary key'.

>`EH_HOST`: Go to your IoT hub 'Messaging' page and set the value to 'Event Hub-compatible endpoint'.

>`EH_KEY_NAME`: Go to your IoT hub 'Shared access policies' page and set the value to the policy name that has 'registry write', 'service connect' and 'device connect' permissions e.g. iothubowner

>`EH_KEY`: The 'Primary key' of `EH_KEY_NAME`.

>`EH_COMP_NAME`: Go to your IoT hub 'Messaging' page and set the value to 'Event Hub-compatible name'.

>`EH_PARTITION_NUM`: Go to your IoT hub 'Messaging' page and set the value to 'Partitions'.

>`DataProtocol`: Three types of data protocol are supported: `HTTP`, `AMQP` and `MQTT`.

- Navigate to `build/`.

- Run `$ ./samples/device_ping/device_ping <path to JSON config file>`

>Note: The device ping tool takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `samples/device_ping/src/device_ping_lin.json'.

Windows

- In a text editor, open the file `samples/device_ping/src/device_ping_lin.json` in your local copy
of the azure-iot-gateway-sdk repository. You need to fill the following information:

>`DeviceConnectionString`: Go to your IoT hub 'Device Details' page and set the value to 'Connection string—primary key'.

>`EH_HOST`: Go to your IoT hub 'Messaging' page and set the value to 'Event Hub-compatible endpoint'.

>`EH_KEY_NAME`: Go to your IoT hub 'Shared access policies' page and set the value to the policy name that has 'registry write', 'service connect' and 'device connect' permissions e.g. iothubowner

>`EH_KEY`: The 'Primary key' of `EH_KEY_NAME`.

>`EH_COMP_NAME`: Go to your IoT hub 'Messaging' page and set the value to 'Event Hub-compatible name'.

>`EH_PARTITION_NUM`: Go to your IoT hub 'Messaging' page and set the value to 'Partitions'.

>`DataProtocol`: Three types of data protocol are supported: `HTTP`, `AMQP` and `MQTT`.

- Navigate to `build\samples\device_ping\Debug`.

- Run `.\device_ping.exe <path to JSON config file>`

>Note: The device ping tool takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `samples\device_ping\src\device_ping_win.json'.

