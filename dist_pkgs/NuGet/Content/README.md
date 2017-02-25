.NET NuGet Package for Windows Desktop
===============================

Overview
--------

This sample showcases how one might build modules for IoT Gateway in .NET.

The sample contains:

1. A printer module (C#) that interprets telemetry from sensor and prints its content and properties into Console.
2. A sensor module (C#) that publishes random data to the gateway.
3. The .NET Microsoft.Azure.IoT.Gateway interface assembly.
4. A sample gateway executable (gw.exe) file.
5. The native binding layer interfaces for .NET, Java, and NodeJs (C++). Which includes `gateway.dll`, `aziotsharedutil.dll`, `nanomsg.dll`, `dotnet.dll`, `java_module_host.dll`, `node.dll`, `nodejs_binding.dll`.
6. A `module_dev_sample.json` file that is used to configure which module the gateway will use.

Prerequisites
--------------
1. Microsoft Visual Studio 2015.
2. Make sure you have .NET Framework installed. Our current version of the binding was tested and loads modules written in .NET version v4.6.01586.

How does the data flow through the Gateway
------------------------------------------
You can find the diagram for Receiving a message and publishing a message on this flow chart:

![](./images/flow_chart.png)

Json Configuration
------------------
**Json**

	{
    	"modules": [
    	    {
    	        "name": "dotnet_sensor_module",
    	        "loader": {
    	            "name": "dotnet",
    	            "entrypoint": {
    	                "dotnet_module_path": "SensorModule",
    	                "dotnet_module_entry_class": "SensorModule.DotNetSensorModule"
    	            }
    	        },
    	        "args": "module configuration"
    	    },
    	    {
    	        "name": "dotnet_printer_module",
    	        "loader": {
    	            "name": "dotnet",
    	            "entrypoint": {
    	                "dotnet_module_path": "PrinterModule",
    	                "dotnet_module_entry_class": "PrinterModule.DotNetPrinterModule"
    	            }
    	        },
    	        "args": "module configuration"
    	    }
    	],
    	"links": [
    	  {"source": "dotnet_sensor_module",
		   "sink": "dotnet_printer_module"}
    	]
	}

Creating your own module
------------------------
1. Create a .NET project (DLL) (Class library).
2. Install the `Microsoft.Azure.Devices.Gateway.Module.Net` NuGet package.
	- The NuGet Package will add a reference to the Microsoft.Azure.IoT.Gateway DLL.
3. On your class you shall implement `IGatewayModule`.
   See the below Printer module as an example:

**C#**

	using System;
	using System.Collections.Generic;
	using System.Linq;
	using System.Text;
	using System.Threading.Tasks;
	using Microsoft.Azure.IoT.Gateway;

	namespace PrinterModule
	{
	    public class DotNetPrinterModule : IGatewayModule
	    {
	        private string configuration;
	        public void Create(Broker broker, byte[] configuration)
        	{
	            this.configuration = System.Text.Encoding.UTF8.GetString(configuration);
    	    }

	        public void Destroy()
        	{
        	}

	        public void Receive(Message received_message)
        	{
	            if(received_message.Properties["source"] == "sensor")
            	{
	                Console.WriteLine("Printer Module received message from Sensor. Content: {0}", System.Text.Encoding.UTF8.GetString(received_message.Content, 0, received_message.Content.Length));
	            }
	        }
	    }
	}

Modules may also implement the `IGatewayModuleStart` interface. The Start method is called when the Gateway's Broker is ready for the module to send and receive messages. 

4. Update the existing dotnet&#95;printer&#95;module Json configuration section to load your custom module instead:

**Json**

	{
		"name": "dotnet_printer_module",
		"loader": {
					"name": "dotnet",
					"entrypoint": {
						"dotnet_module_path": "<Your Namespace>",
						"dotnet_module_entry_class": "<Your Namespace>.<Your Project Name>"
					}
				},
		"args": "module configuration"
	}

Running the sample
------------------
1. Build your project (Ctrl + Shift + B).
2. Open a command prompt (cmd.exe) and navigate to your projects bin folder.
3. Enter the following command:
	- gw.exe module&#95;dev&#95;sample.json