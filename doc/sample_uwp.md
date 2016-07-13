UWP Sample
===============================

Overview
--------

This sample showcases how one might build modules for IoT Gateway for UWP.  It domonstrates 3 types of gateways: a UWP C++ App, a 
UWP C++ Background Application, and a UWP C# Background application.  Each of these gateways can utilize the UWP Sensor and Printer 
modules.

Other resources:
* [getting started doc](getting_started.md)
* [devbox setup](devbox_setup.md)
* [UWP binding High Level Design](../bindings/uwp/devdoc/uwp_hld.md)

Prerequisites
--------------
1. Have your devbox setup (windows) [devbox setup](devbox_setup.md).
2. Make sure you have Visual Studio Tools for Universal Windows Apps installed.

How does the data flow through the Gateway
------------------------------------------
You can find the diagram for Receiving a message and publishing a message on this flow chart:

![](../bindings/uwp/devdoc/flow-chart.png)


Building the sample
-------------------
At this point, creating a UWP gateway containing UWP modules is supported on all UWP platforms (Windows Desktop, Windows Phone, Windows 10 IoT Core). 
To build the UWP Gateway and UWP Modules you should build the solution here (../samples/uwp/UwpSample.sln).
Today the Solution has: 

1. Microsoft.Azure.IoT.Gateway ==> UWP DLL you shall reference from your application and module projects.
2. IotCoreGatewayCpp ==> UWP(C++) Background Application (most useful on Windows 10 Iot Core) gateway project.
3. IotCoreGatewayCs ==> UWP(C#) Background Application (most useful on Windows 10 Iot Core) gateway project
4. IotCoreGatewayCppUwp ==> UWP(C++) App (can be run on all UWP platforms) gateway project.
5. SetOfCppModules ==> UWP(C++) DLL containing Sensor and Printer modules.
6. SetOfCsModules ==> UWP(C#) DLL containing Sensor and Printer modules.

Building the solution you will have the following binaries: 

1. Microsoft.Azure.IoT.Gateway.dll.
2. IotCoreGatewayCpp.dll.
3. IotCoreGatewayCs.dll.
4. IotCoreGatewayCppUwp.exe.
5. SetOfCppModules.dll.
6. SetOfCsModules.winmd.

To deploy these binaries and run your gateway, these files will need to be packagesd in an APPX file or
deployed with Visual Studio (via F5). 

Running the sample
------------------
1. Open UwpSample.slb solution and configure project `IotCoreGatewayCppUwp` as a Startup Sample.
2. Go to the Project Properties and change `Debugging` to point at Local Machine.
3. Change the configuration Debugger Type to Mixed (this way you will be able to set breakpoints on Managed code as well as Native Code).
4. Run.

Creating your own module
------------------------

1. Create a Windows Runtime Component (DLL) project.
2. Add Reference to Microsoft.Azure.IoT.Gateway DLL.
3. On your class you shall implement `IGatewayModule`.

    ```
    using System;
    using System.Collections.Generic;
    using Microsoft.Azure.IoT.Gateway;
    
    namespace MyNewModule
    {
        public sealed class Module : IGatewayModule
        {
            public void Create(MessageBus bus, IReadOnlyDictionary<string, string> configuration)
            {
                System.Diagnostics.Debug.WriteLine("MyNewModule.Module.Create");
            }
    
            public void Destroy()
            {
                System.Diagnostics.Debug.WriteLine("MyNewModule.Module.Destroy");
            }
    
            public void Receive(Message received_message)
            {
                System.Diagnostics.Debug.WriteLine("MyNewModule.Module.Receive");
            }
        }
    }
    ```
4. Add a reference to your new module to IotCoreGatewayCppUwp and add a module instantiation to the module list in MainPage.xaml.cpp:

    ```C#
        modules->Append(ref new MyNewModule::Module());
    ```
