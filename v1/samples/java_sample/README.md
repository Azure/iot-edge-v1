# How-To (Java)

- [Setup Dev Box](java_devbox_setup.md)
- [Create Project](#createproject)
- [Sample Java Gateway](#sampleguide)
- [Sample out of process module](#oopsampleguide)

<a name="createproject"/>

## Create Your Project

As mentioned in the Dev Box Setup guide, we recommend you create your modules as a Maven project. Doing this will make it easy to manage all dependencies and package your jar for easy inclusion in your gateway.

Creating a Java module is easy:
- **Include the Java binding project in your Maven project (Described in the Dev Box Setup guide)**
- **Implement the methods**

  In order to create a proper module that can be recognized by the gateway, it must extend the ```GatewayModule``` abstract class or implement ```IGatewayModule``` interface.
  
  ### Extend GatewayModule abstract class

  After extending the ```GatewayModule``` abstract class, you must override the ```public void receive(Message message)``` and ```public void destroy()``` methods and provide a constructor that calls the super constructor.
  Optionally, you may override the ```public void start()``` method which is called to notify modules that it is safe to start processing and sending messages.

  ```java
  public class YourModule extends GatewayModule {
    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link Broker}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * The {@code address} parameter must be passed to the super constructor but can be ignored by the module-implementor when writing a module implementation.
     *
     * @param address       The address of the native module pointer
     * @param broker        The {@link Broker} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public YourModule(long address, Broker broker, String configuration) {
        super(address, broker, configuration);
    }

    @Override
    public void receive(Message message) {
    }

    @Override
    public void destroy() {
    }

    /**
     * Optional start function.
     */
    @Override
    public void start(){

    }
  }
  ```

  ### Implement IGatewayModule interface

  ```java
  public class YourModule implements IGatewayModule {
    
    public void create(long moduleAddr, Broker broker, String configuration) {
		  // Implement method and store moduleAddr, broker and configuration into class fields if they are required later
      // moduleAddr and broker are required to publish messages
	  }

	  public void start() {
		  // Add implementation here
	  }

	  public void receive(byte[] source) {	
		  // Add implementation here
	  }

	  public void destroy() {
		  // Add implementation here
	  }
  }
  ```

  To see details on each method, you may see the GatewayModule.java and IGatewayModule.java sources.

- **Publish messages**

  Each module has its own ```Broker``` object. Calling the ```Module```s ```int publish(Message message)``` method will publish any message to this ```Broker```.

- **JSON Configuration**

  A gateway is configured using a JSON configuration file. The file must be encoded either as ASCII or UTF-8. As a reference, you can see the JSON configuration file located [here](./src/java_sample_win.json).

  ```json
  {
      "loaders": [
        {
          "type": "java",
          "name": "java",
          "configuration":{
            "jvm.options":{
                "library.path": "<<path/to/dir/containing/java_module_host.[so|dll]>>",
                "version": <<Version number>>,
                "debug": [true | false],
                "debug.port": <<Remote debugging port>>,
                "verbose": [true | false],
                "additional.options": [
                  "<<Any additional options>>"
                ]
              }
          }

        }
      ],
      "modules": [
        {
          "name": "<<Friendly name for your module>>",
          "loader":{
            "name": "java",
          "entrypoint": {
              "class.name": "<<Name of your module class>>",
              "class.path": "<<Complete java class path containing all .jar and .class files necessary>>"
            }
          },
          "args": <<User-defined JSON configuration for your Java module>>
        }
  }
  ```
  **Note** Name of your module class is a fully-qualified class name. For example, the fully-qualified class name for com.microsoft.azure.gateway.sample.YourModule class is: "com/microsoft/azure/gateway/sample/YourModule"
 
  **Note:** The "jvm.options" section is not necessary. If ommitted, a default configuration will be used. If included and multiple Java modules
  will be loaded, all configurations MUST be the same. If multiple "jvm.options" configurations are not the same, creation will fail.
  The default configuration is:

  ```json
  {
    ...
    "jvm.options": {
      "library.path": "<<Default search paths (see below)>>",
      "version": 5,
      "debug": false,
      "debug.port": 9876,
      "verbose": false,
      "additional.options": null
    }
    ...
  }
  ```

  **Note:** The default search locations are as follows:

  On Windows: 

      %PROGRAMFILES%\azure_iot_gateway_sdk-{version}\lib\modules\java_module_host.dll
      %PROGRAMFILES(x86)%\azure_iot_gateway_sdk-{version}\lib\modules\java_module_host.dll

  On Linux:

      /usr/local/lib/modules/java_module_host.so

  **Note:** Since the JVM is only loaded once, the full classpath must be set and be the same across all module in a configuration. Similar to the
  "jvm_options" section, if the classpath differs across configuration, creation will fail.


<a name="sampleguide"/>

## Java Module Sample Gateway

Running the sample gateway containing Java modules only requires a few steps:

1. Build the Java binding (if not already done)
2. Build the Java modules
3. Compiling the project
4. Running the sample

### Building the Java binding & Java modules
Navigate to /bindings/java/gateway-java-binding and run: ```mvn clean install```. This will build the Java binding project pulling in any necessary dependencies.

### Building the Java modules
There are 2 sample Java modules in this sample. The first is a Printer modules that prints any incoming messages to the console. The second is a simulated Sensor
module that generates random sensor data and publishes it to the broker.

Navigate to /samples/java_sample/java_modules/Printer and run: ```mvn clean install```

Navigate to /samples/java_sample/java_modules/Sensor and run: ```mvn clean install```

###Compiling the project
If the project is not already built, either run the build script with the ```--enable-java-binding``` flag, or use cmake and your platform build commands separately.

If using the build script:

  - ```tools/build.[sh|cmd] --enable-java-binding```

If using cmake and msbuild or make:
  - Create a new directory from the root of the repository called 'build'
  - ```cd build```
  - ```cmake -Denable_java_binding=ON ../```
  - If your JDK arch is different to ***amd64*** (e.g. i386) you can specify an arch using the ```JDK_ARCH``` variable in your ```cmake``` command:
  - ```-DJDK_ARCH={jdk_arch}```
  - If using msbuild on Windows: ```msbuild /m /p:Configuration=[Debug | Release] /p:Platform=Win32 azure_iot_gateway_sdk.sln```
  - If using make on Linux: ```make```

###Running the samples
On windows:
  - ```cd {project_root}\build\samples\java_sample```
  - ```Debug\java_sample.exe ..\..\..\samples\java_sample\src\java_sample_win.json```

On linux:
  - ```cd {project_root}/build/samples/java_sample```
  - ```./java_sample ../../../samples/java_sample/src/java_sample_lin.json```

<a name="oopsampleguide"/>

## Java Out of Process Module Sample

Running the out of process sample gateway containing a Java module only requires a few steps:

1. Building the Java module
2. Compiling the project
3. Running the sample

### Building the Java modules

To build the out of process module first build the Printer module and then the RemotePrinter:

Navigate to {project_root}/samples/java_sample/java_modules/Printer and run: ```mvn clean install```

Navigate to {project_root}/samples/java_sample/java_modules/RemotePrinter and run: ```mvn clean install```

### Compiling the project
If the project is not already built, either run the build script with the ```--enable-java-remote-modules``` flag, or use cmake and your platform build commands separately.

If using the build script:

  - ```tools/build.[sh|cmd]  --enable-java-remote-modules```

If using cmake and msbuild or make:
  - Create a new directory from the root of the repository called 'build'
  - ```cd build```
  - ```cmake -Denable_java_remote_modules=ON ../```
  - If your JDK arch is different to ***amd64*** (e.g. i386) you can specify an arch using the ```JDK_ARCH``` variable in your ```cmake``` command:
  - ```-DJDK_ARCH={jdk_arch}```
  - If using msbuild on Windows: ```msbuild /m /p:Configuration=[Debug | Release] /p:Platform=Win32 azure_iot_gateway_sdk.sln```
  - If using make on Linux: ```make```

### Running the samples

#### Running the Gateway:

To run the out of process module sample, two processes have to be started: the gateway and the module sample. Follow the steps on starting the proxy_sample to start the gateway that communicates with the out of process module: [Start gateway](../proxy_sample/README.md)

#### Running the out of process module:

The communication between the out of process module and the gateway is done through nanomsg. Java out of process module uses a JNI binding to the nanomsg native library. That's why is necessary to set `java.library.path` to java_nanomsg.dll | libjava_nanomsg.so (which contains the JNI binding) and nanomsg.dll | libnanomsg.so.

On windows:

  - ```cd {project_root}\samples\java_sample\java_modules\RemotePrinter\```
  - ```java -cp  target\*;target\lib\* -Djava.library.path=..\..\..\..\install-deps\bin;..\..\..\..\build\proxy\gateway\java\Debug com.microsoft.azure.gateway.sample.RemotePrinterSample outprocess_module_control```
  
Alternatively, the path to native libraries can be added to PATH environment variable.

On linux:

  - ```cd {project_root}/build/samples/proxy_sample```
  - ```cp ../../../samples/java_sample/java_modules/Remo tePrinter/target/sample-printer-module-remote-1.1.0.jar . ```
  - ```java -cp  sample-printer-module-remote-1.1.0.jar:../../../samples/java_sample/java_modules/RemotePrinter/target/lib/* -Djava.library.path=../../proxy/gateway/java:../../../deps/nanomsg/build/ com.microsoft.azure.gateway.sample.RemotePrinterSample outprocess_module_control```
  
Alternatively, the path to native libraries can be added to LD_LIBRARY_PATH environment variable.

**Note**: On Linux, the gateway and the out of process module have to be started from the same working directory, that's why the second command is to copy the sample-printer-module-remote-1.1.0.jar to the proxy_sample folder