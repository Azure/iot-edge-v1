# Azure IoT Edge - Simulated device-to-cloud sample

This sample uses Azure IoT Edge to build a simple gateway that uses simulated devices to send telemetry to an Azure IoT hub. The associated tutorials provide a detailed walkthrough of the [Simulated Device Cloud Upload sample code](src/main.c) and JSON configuration files to illustrate the fundamental components of the Azure IoT Edge architecture:

* [Use Azure IoT Edge to send device-to-cloud messages with a simulated device on Linux](./iot-hub-linux-iot-edge-simulated-device.md)
* [Use Azure IoT Edge to send device-to-cloud messages with a simulated device on Windows](./iot-hub-windows-iot-edge-simulated-device.md)

## Build the sample for Yocto Linux on an Intel Edison

In addition to the tutorials for Ubuntu Linux and Windows, you can build this sample for an Intel Edison device.

Use the Azure recipes provided by Intel available in the [meta-iot-cloud](https://github.com/intel-iot-devkit/meta-iot-cloud) repository. Follow the instructions in the [README](https://github.com/intel-iot-devkit/meta-iot-cloud#configuration) to build an Edison image with the iot-edge libraries included.

The Edison does not support all of the Azure recipes available in the "meta-iot-cloud" repository. To get the Edison image to build, you must limit the recipes included in the image.

* Remove the **azure-iot-gateway-sdk-java** target.

    The recipe for **azure-iot-gateway-sdk** defines three targets, **azure-iot-gateway-sdk**, **azure-iot-gateway-sdk-bluetooth**, and **azure-iot-gateway-sdk-java**. The dependencies for Java require libraries not included in the Edison BSP source package. To avoid building the Java 
    target, add the following line to your **conf/local.conf** file. This line will limit the packages to the **azure-iot-gateway-sdk** and **azure-iot-gateway-sdk-bluetooth** targets.

    ```
    PACKAGECONFIG_pn-azure-iot-gateway-sdk = "bluetooth"
    ```
    
* Add the **azure-iot-gateway-sdk** recipe to the target.

    Edit the Edison image recipe to include the **azure-iot-gateway-sdk** target. For example:

    ```
    meta-intel-edison/meta-intel-edison-distro/recipes-core/images/edison-image.bb:IMAGE_INSTALL += "azure-iot-gateway-sdk"
    ```
