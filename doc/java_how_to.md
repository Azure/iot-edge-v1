# How-To (Java)

- [Setup Dev Box](java_devbox_setup.md)
- [Create Project](#createproject)
- [Sample Java Gateway](#sampleguide)

<a name="createproject">
## Create Your Project

As mentioned in the Dev Box Setup guide, we recommend you create your modules as a Maven project. Doing this will make it easy to manage all dependencies and package your jar for easy inclusion in your gateway.

Creating a Java module is easy:
- **Include the Java binding project in your Maven project (Described in the Dev Box Setup guide)**
- **Implement the methods**
  
  In order to create a proper module that can be recognized by the gateway, it must extend the ```GatewayModule``` abstract class.
  After extending the ```GatewayModule``` abstract class, you must override the ```public void receive(Message message)``` and ```publich void destroy()``` methods and provide a constructor that calls the super constructor.
  
  ```java
  public class YourModule extends GatewayModule {
    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link MessageBus}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * The {@code address} parameter must be passed to the super constructor but can be ignored by the module-implementor when writing a module implementation.
     * 
     * @param address       The address of the native module pointer
     * @param bus           The {@link MessageBus} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public YourModule(long address, MessageBus bus, String configuration) {
        super(address, bus, configuration);
    }

    @Override
    public void receive(Message message) {
    }

    @Override
    public void destroy() {
    }
  }
  ```
  
  To see details on each method, you may see the GatewayModule.java and IGatewayModule.java sources.
  
- **Publish messages**

  Each module has its own ```MessageBus``` object. Calling the ```Module```s ```int publish(Message message)``` method will publish any message onto this ```MessageBus```.
  
- **JSON Configuration**
  
  A gateway is configured using a JSON configuration file. As a reference, you can see the JSON configuration file located [here](../samples/java_sample/src/java_sample_win.json).
  
  ```json
  {
      "module name": "<<Friendly name for your module>>",
      "module path": "<<path/to/java_module_host_hl.[so|dll]>>",
      "args": {
        "class_path": "<<Complete java class path containing all .jar and .class files necessary>>",
        "library_path": "<<path/to/dir/containing/java_module_host.[so|dll]>>",
        "class_name": "<<Name of your module class>>",
        "args": <<User-defined JSON configuration for your Java module>>,
        "jvm_options": {
          "version": <<Version number>>,
          "debug": [true | false],
          "debug_port": <<Remote debugging port>>,
          "verbose": [true | false],
          "additional_options": [
            "<<Any additional options>>"
          ]
        }
      }
    }
  ```
  
  According to [Java documentation](https://docs.oracle.com/javase/tutorial/essential/environment/paths.html) one may also set the CLASSPATH environment
  variable rather than including it in this configuration.
  
  **Note:** The "jvm_options" section is not necessary. If ommitted, a default configuration will be used. If included and multiple Java modules
  will be loaded, all configurations MUST be the same. If multiple "jvm_options" configurations are not the same, creation will fail.
  The default configuration is:

  ```json
  {
      "jvm_options": {
          "version": 4,
          "debug": false,
          "debug_port": 9876,
          "verbose": false,
          "additional_options": null
        }
  }
  ```
  
  **Note:** Since the JVM is only loaded once, the full classpath must be set and be the same across all module in a configuration. Similar to the
  "jvm_options" section, if the classpath differs across configuration, creation will fail.


<a name="sampleguide">
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
module that generates random sensor data and publishes it on the bus.

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
  - If using msbuild on Windows: ```msbuild /m /p:Configuration=[Debug | Release] /p:Platform=Win32 azure_iot_gateway_sdk.sln```
  - If using make on Linux: ```make```
  
###Running the samples
On windows:
  - ```cd {project_root}\build\samples\java_sample```
  - ```Debug\java_sample.exe ..\..\..\samlpes\java_sample\src\java_sample_win.json```

On linux:
  - ```cd {project_root}/build/samples/java_sample```
  - ```./java_sample ../../../samples/java_sample/src/java_sample_lin.json```