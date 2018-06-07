# IoT Edge SDK Version 1 - Modules Remote Update Extension 
This sample is to show how to use module remote update extension.
You can update modules on your edge device from cloud via device twin desired properties. 

## Step 1 
Setup Azure IoT Edge SDK ver 1 on your device by [https://github.com/Azure/iot-edge/blob/master/v1/doc/devbox_setup.md](https://github.com/Azure/iot-edge/blob/master/v1/doc/devbox_setup.md). 

## Step 2 
Regist edge device on IoT Hub and get connection string for deviceId. 
Setup 'connection-string' and 'modules-local-path' in hello_world_remote_edge.json. 
### connection-string 
Replace '<< IoT Hub Connection String >>' part by connection string from IoT Hub. 
### modules-local-path 
Replace '<< local path for remote updated module >>' part by real directory on your edge device. Examples are following...  
- Windows  c:\\azureiot\\modules 
- Linux    /usr/local/azureiot/modules 

## Step 3 
Run samples/hello_world/hello_world using above hello_world_remote_edge.json as argument.

## Step 4 
Store module library into cloud storage and prepare url to download it. 
In the case of Azure blob, you create private container, store the library and get url with SAS token. 
Then create edge-config.json refering edge-config-[lin|win].json. Modify '<< url for xxx >>' by above url. '<< local deploy path >>' should be same of the Step 2.  
The version of each module should be changed as your valid one. 
Then store edge-config.json into cloud storage and prepare url. 

## Step 5 
Update device twin desired properties refering device-twin-cloud.json with modify '<< edge-config-cloud.json url >>' using above edge-config.json on IoT Hub or Device Explorer. 

## Starting! 
Device will receive device twin desired properties change then download configuration and libraries, deploy and execute them. 

Let's enjoy! 
