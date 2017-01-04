Azure Functions Sample for Azure IoT Gateway SDK
================================================

Overview
--------

This sample showcases how one can build an IoT Gateway that interacts with Azure Functions using the Azure IoT Gateway SDK. 
Details about Azure Functions can be found on [Introduction to Azure Functions](https://azure.microsoft.com/en-us/blog/introducing-azure-functions/).

The sample contains the following modules:

  1. Hello World module. A module that sends random data on our gateway bus.
  2. Azure Functions module. A module that receives this data and sends to the cloud. More details on this module here [Azure Functions Module](../../modules/azure_functions/devdoc/azure_functions.md)
  
How does the data flow through the Gateway
------------------------------------------
Here's the journey that a piece of data takes originating from the hello_world module to Azure Functions.

  1. The hello_world module will send a message every 5 seconds.
  2. The Azure Functions Module will get content from any module that sends data to it, Encode it (Base64 Encode) and send to an Azure Functions url. 
  It will send this data to a HTTP Trigger Azure Function, using HTTP POST. This module will also print result from the POST request to the console.

Building the sample
-------------------

The sample gets built when you build the SDK by running `tools/build.sh` for Linux or `tools\build.cmd` for windows.  The
[devbox setup](../../doc/devbox_setup.md) guide has information on how you can build the
SDK.

Setting up your Azure Functions
-------------------------------

1. Follow the guideline [Introduction to Azure Functions](https://azure.microsoft.com/en-us/blog/introducing-azure-functions/) to create a
http trigger Function. 

2. In your Azure Function's Develop tab, find the Function URL.  (For example, [https://<yourFunctionsHost>.azurewebsites.net/api/<your FunctionName>?code=123456123456) From this URL, you will need 3 pieces of data:
* hostname: For the example above, this would be: "<yourFunctionsHost>.azurewebsites.net"
* relativePath: For the example above, this would be: "api/<your FunctionName>"
* key(optional): For the example above, this would be: "123456123456" (If you chose anonymous when you created your Azure HttpTrigger you would not have this)

Running the sample
------------------

In order to run the sample you'll need to configure the Azure Functions module.
This configuration is provided as a JSON file, which must be encoded either as ASCII or UTF-8. There is a sample JSON file
provided in the repo called `azure_functions_lin.json` for linux or `azure_functions_win.json` for windows.
Edit this file and provice the 3 parameters `hostname`, `relativePath` and `key`(optional).

In order to run the sample you'll run the `azure_functions_sample` binary passing the
path to the configuration JSON file. The following command assumes that you are
running the the executable from the root of the repo.

```
azure_functions_sample (or azure_functions_sample.exe) <path to your json file>
```
>Note: If you are running on windows (Visual Studio) you can set the azure_functions_sample as a startup project and put the path to the Json file under `Properties->Debugging->Command Arguments` and hit Run.

The result will be: 
Info: Request Sent to Function Succesfully. Response from Functions: "Hello myGatewayDevice"

Template configuration JSONs for these 2 modules are given below. 

```json
{
  "modules": [
    {
      "name": "hello_world",
      "loader": {
        "name": "native",
        "entrypoint": {
          "module.path": "..\\..\\..\\modules\\hello_world\\Debug\\hello_world.dll"
        }
      },
      "args": null
    },
    {
      "name": "azure_functions",
      "loader": {
        "name": "native",
        "entrypoint": {
          "module.path": "..\\..\\..\\modules\\azure_functions\\Debug\\azure_functions.dll"
        }
      },
      "args": {
        "hostname": "<YourHostNameHere from Functions Portal>",
        "relativePath": "api/<YourFunctionName>",
        "key": "<your Api Key Here>"
      }
    }
  ],
  "links": [
    {
      "source": "hello_world",
      "sink": "azure_functions"
    }
  ]
}
```