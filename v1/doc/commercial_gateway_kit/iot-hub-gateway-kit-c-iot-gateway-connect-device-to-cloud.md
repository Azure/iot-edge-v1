---
title: Use an IoT gateway to connect a device to Azure IoT Hub | Microsoft Docs
description: Learn how to use Intel NUC as an IoT gateway to connect a TI SensorTag and send sensor data to Azure IoT Hub in the cloud.
services: iot-hub
documentationcenter: ''
author: shizn
manager: timlt
tags: ''
keywords: 'iot gateway connect device to cloud'

ms.assetid: cb851648-018c-4a7e-860f-b62ed3b493a5
ms.service: iot-hub
ms.devlang: c
ms.topic: article
ms.tgt_pltfrm: na
ms.workload: na
ms.date: 06/25/2017
ms.author: xshi

---
# Use IoT gateway to connect things to the cloud - SensorTag to Azure IoT Hub

> [!NOTE]
> Before you start this tutorial, make sure you’ve completed [Set up Intel NUC as an IoT gateway](iot-hub-gateway-kit-c-lesson1-set-up-nuc.md). In [Set up Intel NUC as an IoT gateway](iot-hub-gateway-kit-c-lesson1-set-up-nuc.md), you set up the Intel NUC device as an IoT gateway.

## What you will learn

You learn how to use an IoT gateway to connect a Texas Instruments SensorTag (CC2650STK) to Azure IoT Hub. The IoT gateway sends temperature and humidity data collected from the SensorTag to Azure IoT Hub.

## What you will do

- Create an IoT hub.
- Register a device in the IoT hub for the SensorTag.
- Enable the connection between the IoT gateway and the SensorTag.
- Run a BLE sample application to send SensorTag data to your IoT hub.

## What you need

- Tutorial [Set up Intel NUC as an IoT gateway](iot-hub-gateway-kit-c-lesson1-set-up-nuc.md) completed in which you set up Intel NUC as an IoT gateway.
- * An active Azure subscription. If you don't have an Azure account, [create a free Azure trial account](https://azure.microsoft.com/free/) in just a few minutes.
- An SSH client that runs on your host computer. PuTTY is recommended on Windows. Linux and macOS already come with an SSH client.
- The IP address and the username and password to access the gateway from the SSH client.
- An Internet connection.

## Create an IoT hub

1. Sign in to the [Azure portal][lnk-portal].
1. Select **New** > **Internet of Things** > **IoT Hub**.
   
    ![Azure portal Jumpbar][1]

1. In the **IoT hub** pane, enter the following information for your IoT hub:

   * **Name**: Create a name for your IoT hub. If the name you enter is valid, a green check mark appears.

> [!IMPORTANT]
> The IoT hub will be publicly discoverable as a DNS endpoint, so make sure to avoid any sensitive information while naming it.

   * **Pricing and scale tier**: For this tutorial, select the **F1 - Free** tier. For more information, see the [Pricing and scale tier][lnk-pricing].

   * **Resource group**: Create a resource group to host the IoT hub or use an existing one. For more information, see [Use resource groups to manage your Azure resources][lnk-resource-groups]

   * **Location**: Select the closest location to you.

   * **Pin to dashboard**: Check this option for easy access to your IoT hub from the dashboard.

    ![IoT hub window][2]

1. Click **Create**. Your IoT hub might take a few minutes to create. You can monitor the progress in the **Notifications** pane.

Now that you have created an IoT hub, locate the important information that you use to connect devices and applications to your IoT hub. 

1. After your IoT hub is created, click it on the dashboard. Make a note of the **Hostname**, and then click **Shared access policies**.

   ![Get the hostname of your IoT hub](./../media/iot-hub-create-hub-and-device/4_get-azure-iot-hub-hostname-portal.png)

1. In the **Shared access policies** pane, click the **iothubowner** policy, and then copy and make a note of the **Connection string** of your IoT hub. For more information, see [Control access to IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-security).

> [!NOTE] 
You will not need this iothubowner connection string for this set-up tutorial. However, you may need it for some of the tutorials on different IoT scenarios after you complete this set-up.

   ![Get your IoT hub connection string](./../media/iot-hub-create-hub-and-device/5_get-azure-iot-hub-connection-string-portal.png)

## Register a device in the IoT hub for your device

1. In the [Azure portal](https://portal.azure.com/), open your IoT hub.

2. Click **Device Explorer**.
3. In the Device Explorer pane, click **Add** to add a device to your IoT hub. Then do the following:

   **Device ID**: Enter the ID of the new device. Device IDs are case sensitive.

   **Authentication Type**: Select **Symmetric Key**.

   **Auto Generate Keys**: Select this check box.

   **Connect device to IoT Hub**: Click **Enable**.

   ![Add a device in the Device Explorer of your IoT hub](./../media/iot-hub-create-hub-and-device/6_add-device-in-azure-iot-hub-device-explorer-portal.png)

> [!IMPORTANT]
> The device ID may be visible in the logs collected for customer support and troubleshooting, so make sure to avoid any sensitive information while naming it.

4. Click **Save**.
5. After the device is created, open the device in the **Device Explorer** pane.
6. Make a note of the primary key of the connection string.

   ![Get the device connection string](./../media/iot-hub-create-hub-and-device/7_get-device-connection-string-in-device-explorer-portal.png)

> [!NOTE]
> Here you register this new device for your SensorTag

## Enable the connection between the IoT gateway and the SensorTag

In this section, you perform the following tasks:

- Get the MAC address of the SensorTag for Bluetooth connection.
- Initiate a Bluetooth connection from the IoT gateway to the SensorTag.

### Get the MAC address of the SensorTag for Bluetooth connection

1. On the host computer, run the SSH client and connect to the IoT gateway.
1. Unblock Bluetooth by running the following command:

   ```bash
   sudo rfkill unblock bluetooth
   ```

1. Start the Bluetooth service on the IoT gateway and enter a Bluetooth shell to configure Bluetooth by running the following commands:

   ```bash
   sudo systemctl start bluetooth
   bluetoothctl
   ```

1. Power on the Bluetooth controller by running the following command at the Bluetooth shell:

   ```bash
   power on
   ```

   ![power on the Bluetooth controller on the IoT gateway with bluetoothctl](./../media/iot-hub-iot-gateway-connect-device-to-cloud/8_power-on-bluetooth-controller-at-bluetooth-shell-bluetoothctl.png)

1. Start scanning for nearby Bluetooth devices by running the following command:

   ```bash
   scan on
   ```

   ![Scan nearby Bluetooth devices with bluetoothctl](./../media/iot-hub-iot-gateway-connect-device-to-cloud/9_start-scan-nearby-bluetooth-devices-at-bluetooth-shell-bluetoothctl.png)

1. Press the pairing button on the SensorTag. The green LED on the SensorTag flashes.
1. At the Bluetooth shell, you should see the SensorTag is found. Make a note of the MAC address of the SensorTag. In this example, the MAC address of the SensorTag is `24:71:89:C0:7F:82`.
1. Turn off the scan by running the following command:

   ```bash
   scan off
   ```

   ![Stop scanning nearby Bluetooth devices with bluetoothctl](./../media/iot-hub-iot-gateway-connect-device-to-cloud/10_stop-scanning-nearby-bluetooth-devices-at-bluetooth-shell-bluetoothctl.png)

### Initiate a Bluetooth connection from the IoT gateway to the SensorTag

1. Connect to the SensorTag by running the following command:

   ```bash
   connect <MAC address>
   ```

   ![Connect to the SensorTag with bluetoothctl](./../media/iot-hub-iot-gateway-connect-device-to-cloud/11_connect-to-sensortag-at-bluetooth-shell-bluetoothctl.png)

1. Disconnect from the SensorTag and exit the Bluetooth shell by running the following commands:

   ```bash
   disconnect
   exit
   ```

   ![Disconnect from the SensorTag with bluetoothctl](./../media/iot-hub-iot-gateway-connect-device-to-cloud/12_disconnect-from-sensortag-at-bluetooth-shell-bluetoothctl.png)

You've successfully enabled the connection between the SensorTag and the IoT gateway.

## Run a BLE sample application to send SensorTag data to your IoT hub

The Bluetooth Low Energy (BLE) sample application is provided by Azure IoT Edge. The sample application collects data from BLE connection and send the data to you IoT hub. To run the sample application, you need to:

1. Configure the sample application.
1. Run the sample application on the IoT gateway.

### Configure the sample application

1. Go to the folder of the sample application by running the following command:

   ```bash
   cd /usr/share/azureiotgatewaysdk/samples/ble_gateway
   ```

1. Open the configuration file by running the following command:

   ```bash
   vi ble_gateway.json
   ```

1. In the configuration file, fill in the following values:

   **IoTHubName**: The name of your IoT hub.

   **IoTHubSuffix**: Get IoTHubSuffix from the primary key of the device connection string that you noted down. Ensure that you get the primary key of the device connection string, not the primary key of your IoT hub connection string. The primary key of the device connection string is in the format of `HostName=IOTHUBNAME.IOTHUBSUFFIX;DeviceId=DEVICEID;SharedAccessKey=SHAREDACCESSKEY`.

   **Transport**: The default value is `amqp`. This value shows the protocol during transpotation. It could be `http`, `amqp`, or `mqtt`.

   **macAddress**: The MAC address of the SensorTag that you noted down.

   **deviceID**: ID of the device that you created in your IoT hub.

   **deviceKey**: The primary key of the device connection string.

   ![Complete the config file of the BLE sample application](./../media/iot-hub-iot-gateway-connect-device-to-cloud/13_edit-config-file-of-ble-sample.png)

1. Press `ESC` and type `:wq` to save the file.

### Run the sample application

1. Make sure the SensorTag is powered on.
1. Run the following command:

   ```bash
   ./ble_gateway ble_gateway.json
   ```

## Next steps

[Use IoT gateway for sensor data transformation with Azure IoT Edge](iot-hub-gateway-kit-c-use-iot-gateway-for-data-conversion.md)

<!-- Images -->
[1]: ./../media/iot-hub-get-started-create-hub/create-iot-hub1.png
[2]: ./../media/iot-hub-get-started-create-hub/create-iot-hub2.png
<!-- Links -->
[lnk-portal]: https://portal.azure.com/
[lnk-pricing]: https://azure.microsoft.com/pricing/details/iot-hub/
[lnk-resource-groups]: https://docs.microsoft.com/en-us/azure/azure-resource-manager/resource-group-create-service-principal-portal