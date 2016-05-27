#Azure IoT Gateway SDK - Getting Started

This document provides a detailed overview of the Hello World sample [code](../samples/hello_world) which uses the fundamental components of the Azure IoT Gateway SDK architecture to log a hello world message to a file every 5 seconds.

#The walkthrough covers

1.	**Dev box setup** - steps necessary to build and run the sample
2.	**Concepts** - conceptual overview of the components that compose any gateway created with the SDK  
3.	**Hello World sample architecture** - describes how the concepts apply to the sample and how the components fit together
4.	**Detailed architecture** - detailed view of the Hello World sample's architecture
5.  **How to build the sample**
6.  **How to run the sample** 
7.  **Typical output** - example of typical output when the hello world sample is run on Linux or Windows
8.	**Code snippets** - code snippets which show how and where the Hello World sample implements key gateway components 

##Dev box setup 

A dev box configured with the SDK and necessary libraries is necessary to complete this walkthrough. Please complete the [dev box setup](./devbox_setup.md) before continuing.

##Concepts

###Modules

Modules are the brains of a gateway built with the Azure IoT Gateway SDK. Modules exchange data with each other via messages. A module receives a message, performs some action on it, might transform it into a new message, and subsequently publishes it to other modules. There might be modules that only produce new messages and never process incoming messages. A chain of modules can be thought of as a data processing pipeline with a module being nothing more than a chunk of code performing a transformation on the data at one point in that pipeline.

![](./media/modules.png) 
 
The SDK contains the following:

- prewritten modules which perform common gateway functions
- the interfaces needed for a developer to write his own custom modules
- the infrastructure necessary to deploy and run a set of modules

The SDK abstracts away  operating system dependencies via an abstraction layer in order to allow for gateways to be built on a variety of platforms.

![](./media/modules_2.png) 

###Messages

Thinking about modules passing each other messages is a convenient way of conceptualizing how a gateway functions, but it does not accurately reflect what's happening under the hood. Modules actually use a message bus to communicate with each other. They publish messages to the bus and then let the bus broadcast the message to all modules connected to the message bus.

A module publishes a message to the message bus via the `MessageBus_Publish` function. The message bus delivers messages to a module by invoking a callback on the module and passing that function the message that is being delivered. A message consists of a set of key/value properties and content passed as a block of memory.

![](./media/messages_1.png)
 
At this time, the responsibility of filtering messages falls on each module since the message bus uses a broadcast mechanism (i.e. the message bus delivers each message to all the modules that are connected to the message bus). A module should only act upon a message if the message is intended for it. This message filtering is what effectively creates a message pipeline. This filtering can typically be performed by inspecting the properties on the message that a module receives to determine whether it is of interest to the module or not.

This is enough background to actually start discussing the Hello World sample.

##Hello World sample architecture

Before diving into the details of filtering messages based on properties, first think of the Hello World sample simply in terms of modules. The sample is made up of a pipeline of two modules:
-	A "hello world" module
-	A logger module

The "hello world" module creates a message every 5 seconds and passes it to the logger module. The logger module simply writes the message to a file. 

![](./media/high_level_architecture.png) 

##Detailed architecture

Based on the gateway architecture, we know that the "hello world" module is not directly passing messages to the logger module every 5 seconds. Instead, it is actually publishing a message to the message bus every 5 seconds.

The logger module is then receiving the message from the message bus and inspecting it to determine if it should act upon it before writing the contents of the message to a file.

The logger module only consumes messages (by logger them to a file) and never publishes new messages on the bus.
 
 ![](./media/detailed_architecture.png)
 
The figure above shows an accurate representation of the Hello World sample's architecture as well as the relative paths to the source files that implement different portions of the sample. Feel free to explore the code on your own, or use the code snippets below as a guide.

##How to build the sample
Linux

1. Open a shell
2. Navigate to `azure-iot-gateway-sdk/tools/`
3. Run `./build.sh`

>Note: `build.sh` does multiple things. It builds the project and places it in the "build" folder in the root of the repo. This folder is deleted and recreated everytime `build.sh` is run. Additionally, `build.sh` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `cmake <path to repo root>`

>run `make -j $(nproc)`

>To run the tests:

>run `ctest -j $(nproc) -C Debug --output-on-failure`

Windows

1. Open a Developer Command Prompt for VS2015
2. Navigate to `azure-iot-gateway-sdk\tools\`
3. Run `build.cmd`. 

>Note: `build.cmd` does multiple things. It builds a solution ('azure_iot_gateway_sdk.sln') and places it in the "build" folder in the root of the repo. This folder is deleted and recreated every time `build.cmd` is run. Additionally, `build.cmd` runs all tests. The project can be build manually by using cmake. To do this:

>create a folder for the build output and navigate to that folder.

>run `cmake <path to repo root>`

>run `msbuild /m /p:Configuration="Debug" /p:Platform="Win32" azure_iot_gateway_sdk.sln`

>To run the tests:

>run `ctest -C Debug -V`

##How to run the sample

Linux

- build.sh produces its outputs in `azure-iot-gateway-sdk/build`. This is where the two modules used in this sample are built. 

> Note: `liblogger_hl.so`'s relative path is `/modules/logger/liblogger_hl.so` and `libhello_world_dl.so` is built in `/modules/hello_world/libhello_world_hl.so`. Use these paths for the "module path" value in the json below.

- Copy the JSON file from azure-iot-gateway-sdk/samples/hello_world/src/hello_world_lin.json to the build folder.

- For the logger_hl module, replace in "args" the "filename" value with the path to the file that will contain the log.
This is an example of a JSON settings file for Linux that will write to `log.txt`, if started from `azure-iot-gateway-sdk/build`. The Hello World sample already has a JSON settings file and you do not need to change it. This example has been provided in case you want to change the output location for the log file.

```json
{
    "modules" :
    [ 
        {
            "module name" : "logger_hl",
            "module path" : "./modules/logger/liblogger_hl.so",
            "args" : 
			{
				"filename":"log.txt"
			}
        },
        {
            "module name" : "hello_world",
            "module path" : "./modules/hello_world/libhello_world_hl.so",
			"args" : null
        }
    ]
}
```

- Navigate to `azure-iot-gateway-sdk/build/`.

-  Run `$ ./samples/hello_world/hello_world_sample <path to JSON config file>`

>Note: The simulated device cloud upload process takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk/samples/hello_world/src/hello_world_lin.json'. 

Windows

- From a Developer Command for VS2015 run `build.cmd`. `build.cmd` produces a folder called `build` in the root repo folder. This is where the two modules used in this sample are built.

> Note: 'logger_hl.dll''s relative path is 'modules\logger\logger_hl.dll' and 'hello_world_hl.dll' is built in 'modules\hello_world\hello_world_hl.dll'. Use these paths for the "module path" value in the json below.

- Copy the JSON file from folder: `samples\hello_world\src\hello_world_win.json` to folder: `build\samples\hello_world\Debug\`.

- For the logger_hl module, replace in `args` the `filename` value with the path to the file that will contain the log.
This is an example of a JSON settings file for Windows. that will write to `log.txt` in your current working directory.
```json
{
    "modules" :
    [ 
        {
            "module name" : "logger_hl",
            "module path" : "..\\..\\..\\modules\\logger\\Debug\\logger_hl.dll",
            "args" : 
			{
				"filename":"log.txt"
			}
        },
        {
            "module name" : "hello_world",
             "module path" : "..\\..\\..\\modules\\hello_world\\Debug\\hello_world_hl.dll",
			"args" : null
        }
    ]
}
```

- Navigate to `build\samples\hello_world\Debug\`.

- Run `.\hello_world_sample.exe <path to JSON config file>`

>Note: The simulated device cloud upload process takes the path to a JSON configuration file as an argument in the command line. An example JSON file has been provided as part of the repo at `azure-iot-gateway-sdk\samples\hello_world\src\hello_world_win.json'.


## Typical Output
Below is an example of typical output that is written to the log file when the Hello World sample is run on Linux or Windows. The output below has been formatted for readability purposes.

```json
[{
	"time": "Mon Apr 11 13:48:07 2016",
	"content": "Log started"
}, {
	"time": "Mon Apr 11 13:48:48 2016",
	"properties": {
		"helloWorld": "from Azure IoT Gateway SDK simple sample!"
	},
	"content": "aGVsbG8gd29ybGQ="
}, {
	"time": "Mon Apr 11 13:48:55 2016",
	"properties": {
		"helloWorld": "from Azure IoT Gateway SDK simple sample!"
	},
	"content": "aGVsbG8gd29ybGQ="
}, {
	"time": "Mon Apr 11 13:49:01 2016",
	"properties": {
		"helloWorld": "from Azure IoT Gateway SDK simple sample!"
	},
	"content": "aGVsbG8gd29ybGQ="
}, {
	"time": "Mon Apr 11 13:49:04 2016",
	"content": "Log stopped"
}]
```

##Code snippets

###Gateway creation

The gateway process needs to be written by the developer. This a program which creates internal infrastructure (e.g. the message bus), loads the correct modules, and sets everything up to function correctly. The SDK provides the `Gateway_Create_From_JSON` function which allows developers to bootstrap a gateway from a JSON file.

`Gateway_Create_FromJSON` deals with creating internal infrastructure (e.g. the message bus), loading modules, and setting everything up to function correctly. All the developer needs to do is provide this function with the path to a JSON file specifying what modules they want loaded. 

The code for the Hello World sample's gateway process is contained in [`samples/hello_world/main.c`](../samples/hello_world/src/main.c) A slightly abbreviated version of that code is copied below. This very short program just creates a gateway and then waits for the ENTER key to be pressed before it tears down the gateway. 

```c
int main(int argc, char** argv)
{
    GATEWAY_HANDLE gateway;
    if ((gateway = Gateway_Create_From_JSON(argv[1])) == NULL)
    {
        printf("failed to create the gateway from JSON\n");
    }
    else
    {
        printf("gateway successfully created from JSON\n");
        printf("gateway shall run until ENTER is pressed\n");
        (void)getchar();
        Gateway_LL_Destroy(gateway);
    }
	return 0;
}
```
 
The JSON file specifying the modules to be loaded is quite simple. It contains a list of modules to load. Each module must specify a:
- `module name` – a unique name for the module
- `module path` – the path to the library containing the module. For Linux this will be a .so while on Windows this will be a .dll file
- `args` – any arguments/configuration the module needs. Specifically, they are really another json value which is passed (as string) to the Module's `_Create` function.

Copied below is the code from one of the JSON files (hello_world_lin.json)[(../samples/hello_world/src/hello_world_lin.json) used to configure the Hello World sample. There are a couple things to note:

- This is the Linux version of the file 
- Whether a module takes an argument or not is completely dependent on the module's design. The logger module does take an argument which is the path to the file to be used for output. On the other hand, the "hello world" module does not take any arguments.

```json
{
    "modules" :
    [ 
        {
            "module name" : "logger_hl",
            "module path" : "./modules/logger/liblogger_hl.so",
            "args" : {"filename":"log.txt"}
        },
        {
            "module name" : "hello_world",
            "module path" : "./modules/hello_world/libhello_world_hl.so",
			"args" : null
        }
    ]
}
```

###"hello world" module message publishing

The code used by the "hello world" module to publish messages is in ['hello_world.c'](../modules/hello_world/src/hello_world.c). An amended version has been reproduced below. Comments calling out the more interesting parts of message creation and publishing process has been given below. 
Also, error checks have been omitted for the sake of brevity.

```c
int helloWorldThread(void *param)
{
    // create data structures used in function.
    HELLOWORLD_HANDLE_DATA* handleData = param;
    MESSAGE_CONFIG msgConfig;
    MAP_HANDLE propertiesMap = Map_Create(NULL);
    
    // add a property named "helloWorld" with a value of "from Azure IoT
    // Gateway SDK simple sample!" to a set of message properties that
    // will be appended to the message before publishing it. 
    Map_AddOrUpdate(propertiesMap, "helloWorld", "from Azure IoT Gateway SDK simple sample!")

    // set the content for the message
    msgConfig.size = strlen(HELLOWORLD_MESSAGE);
    msgConfig.source = HELLOWORLD_MESSAGE;

    // set the properties for the message
    msgConfig.sourceProperties = propertiesMap;
    
    // create a message based on the msgConfig structure
    MESSAGE_HANDLE helloWorldMessage = Message_Create(&msgConfig);

    while (1)
    {
        if (handleData->stopThread)
        {
            (void)Unlock(handleData->lockHandle);
            break; /*gets out of the thread*/
        }
        else
        {
            // publish the message to the bus
            (void)MessageBus_Publish(handleData->busHandle, helloWorldMessage);
            (void)Unlock(handleData->lockHandle);
        }

        (void)ThreadAPI_Sleep(5000); /*every 5 seconds*/
    }

    Message_Destroy(helloWorldMessage);

    return 0;
}
```

###"hello world" module message processing

The "hello world" module does not ever have to process any messages since it is not interested in any of the messages that are published to the message bus. This makes implementation of the "hello world" module's message receive callback a no-op function.

```c
static void HelloWorld_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*no action, HelloWorld is not interested in any messages*/
}
```

###Logger module message publishing

The logger module receives messages from the message bus and writes them to a file. It never has to publish messages to the message bus. Therefore, the code of the logger module never calls 'MessageBus_Publish'.

`Logger_Receive`, located in [`logger.c`](../modules/logger/src/logger.c), is the logger module's callback invoked by the message bus when it wants to deliver a message to the logger module. An amended version has been reproduced below. Comments call out parts of message processing have been provided. Again, error checks have been omitted for the sake of brevity.
```c
static void Logger_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{

    time_t temp = time(NULL);
    struct tm* t = localtime(&temp);
    char timetemp[80] = { 0 };

    // get the message properties from the message
    CONSTMAP_HANDLE originalProperties = Message_GetProperties(messageHandle); 
    MAP_HANDLE propertiesAsMap = ConstMap_CloneWriteable(originalProperties);

    // convert the collection of properties into a JSON string
    STRING_HANDLE jsonProperties = Map_ToJSON(propertiesAsMap);

    //  base64 encode the message content
    const CONSTBUFFER * content = Message_GetContent(messageHandle);
    STRING_HANDLE contentAsJSON = Base64_Encode_Bytes(content->buffer, content->size);

    // start the construction of the final string to be logged by adding
    // the timestamp
    STRING_HANDLE jsonToBeAppended = STRING_construct(",{\"time\":\"");
    STRING_concat(jsonToBeAppended, timetemp);

    // add the message properties
    STRING_concat(jsonToBeAppended, "\",\"properties\":"); 
    STRING_concat_with_STRING(jsonToBeAppended, jsonProperties);

    // add the content
    STRING_concat(jsonToBeAppended, ",\"content\":\"");
    STRING_concat_with_STRING(jsonToBeAppended, contentAsJSON);
    STRING_concat(jsonToBeAppended, "\"}]");

    // write the formatted string
    LOGGER_HANDLE_DATA *handleData = (LOGGER_HANDLE_DATA *)moduleHandle;
    addJSONString(handleData->fout, STRING_c_str(jsonToBeAppended);
}
```

