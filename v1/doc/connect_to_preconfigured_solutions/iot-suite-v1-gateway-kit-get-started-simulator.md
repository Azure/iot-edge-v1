---
title: Connect a gateway to Azure IoT Suite using an Intel NUC | Microsoft Docs
description: Use the Microsoft IoT Commercial Gateway Kit and the remote monitoring preconfigured solution. Use the Azure IoT Edge gateway to connect to the remote monitoring solution, send simulated telemetry to the cloud, and respond to methods invoked from the solution dashboard.
services: ''
suite: iot-suite
documentationcenter: ''
author: dominicbetts
manager: timlt
editor: ''

ms.service: iot-suite
ms.devlang: c
ms.topic: article
ms.tgt_pltfrm: na
ms.workload: na
ms.date: 11/02/2017
ms.author: dobett

---
# Connect your Azure IoT Edge gateway to the remote monitoring preconfigured solution and send simulated telemetry

This tutorial shows you how to use Azure IoT Edge to simulate temperature and humidity data to send to the remote monitoring preconfigured solution. The tutorial uses:

- Azure IoT Edge to implement a sample gateway.
- The IoT Suite remote monitoring preconfigured solution as the cloud-based back end.

## Overview

In this tutorial, you complete the following steps:

- Deploy an instance of the remote monitoring preconfigured solution to your Azure subscription. This step automatically deploys and configures multiple Azure services.
- Set up your Intel NUC gateway device to communicate with your computer and the remote monitoring solution.
- Configure the IoT Edge gateway to send simulated telemetry that you can view on the solution dashboard.

## Prerequisites

To complete this tutorial, you need an active Azure subscription.

> [!NOTE]
> If you don’t have an account, you can create a free trial account in just a couple of minutes. For details, see [Azure Free Trial][lnk-free-trial].

### Required software

You need SSH client on your desktop machine to enable you to remotely access the command line on the Intel NUC.

- Windows does not include an SSH client. We recommend using [PuTTY](http://www.putty.org/).
- Most Linux distributions and Mac OS include the command-line SSH utility.

### Required hardware

A desktop computer to enable you to connect remotely to the command line on the Intel NUC.

[IoT Commercial Gateway Kit][lnk-starter-kits]. This tutorial uses the following items from the kit:

- Intel® NUC Kit DE3815TYKE with 4G Memory and Bluetooth expansion card
- Power adaptor

## Provision the solution

If you haven't already provisioned the remote monitoring preconfigured solution in your account:

1. Log on to [azureiotsuite.com][lnk-azureiotsuite] using your Azure account credentials, and click **+** to create a solution.
2. Click **Select** on the **Remote monitoring** tile.
3. Enter a **Solution name** for your remote monitoring preconfigured solution.
4. Select the **Region** and **Subscription** you want to use to provision the solution.
5. Click **Create Solution** to begin the provisioning process. This process typically takes several minutes to run.

### Wait for the provisioning process to complete
1. Click the tile for your solution with **Provisioning** status.
2. Notice the **Provisioning states** as Azure services are deployed in your Azure subscription.
3. Once provisioning completes, the status changes to **Ready**.
4. Click the tile to see the details of your solution in the right-hand pane.

> [!NOTE]
> If you are encountering issues deploying the pre-configured solution, review [Permissions on the azureiotsuite.com site][lnk-permissions] and the [FAQ][lnk-faq]. If the issues persist, create a service ticket on the [portal][lnk-portal].
> 
> 

Are there details you'd expect to see that aren't listed for your solution? Give us feature suggestions on [User Voice](https://feedback.azure.com/forums/321918-azure-iot).

> [!WARNING]
> The remote monitoring solution provisions a set of Azure services in your Azure subscription. The deployment reflects a real enterprise architecture. To avoid unnecessary Azure consumption charges, delete your instance of the preconfigured solution at azureiotsuite.com when you have finished with it. If you need the preconfigured solution again, you can easily recreate it. For more information about reducing consumption while the remote monitoring solution runs, see [Configuring Azure IoT Suite preconfigured solutions for demo purposes][lnk-demo-config].

## View the solution dashboard

The solution dashboard enables you to manage the deployed solution. For example, you can view telemetry, add devices, and invoke methods.

1. When the provisioning is complete and the tile for your preconfigured solution indicates **Ready**, choose **Launch** to open your remote monitoring solution portal in a new tab.

    ![Launch the preconfigured solution][img-launch-solution]

1. By default, the solution portal shows the *dashboard*. Navigate to other areas of the solution portal using the menu on the left-hand side of the page.

    ![Remote monitoring preconfigured solution dashboard][img-menu]

## Add a device

For a device to connect to the preconfigured solution, it must identify itself to IoT Hub using valid credentials. You can retrieve the device credentials from the solution dashboard. You include the device credentials in your client application later in this tutorial.

To add a device to your remote monitoring solution, complete the following steps in the solution dashboard:

1. In the lower left-hand corner of the dashboard, click **Add a device**.

   ![Add a device][1]

1. In the **Custom Device** panel, click **Add new**.

   ![Add a custom device][2]

1. Choose **Let me define my own Device ID**. Enter a Device ID such as **device01**, click **Check ID** to verify you haven't already used the name in your solution, and then click **Create** to provision the device.

   ![Add device ID][3]

1. Make a note the device credentials (**Device ID**, **IoT Hub Hostname**, and **Device Key**). Your client application on the Intel NUC needs these values to connect to the remote monitoring solution. Then click **Done**.

    ![View device credentials][4]

1. Select your device in the device list in the solution dashboard. Then, in the **Device Details** panel, click **Enable Device**. The status of your device is now **Running**. The remote monitoring solution can now receive telemetry from your device and invoke methods on the device.
]

Repeat the previous steps to add a second device using a Device ID such as **device02**. The sample sends data from two simulated devices in the gateway to the remote monitoring solution.

## Prepare your Intel NUC

To complete the hardware setup, you need to:

- Connect your Intel NUC to the power supply included in the kit.
- Connect your Intel NUC to your network using an Ethernet cable.

You have now completed the hardware setup of your Intel NUC gateway device.

### Sign in and access the terminal

You have two options to access a terminal environment on your Intel NUC:

- If you have a keyboard and monitor connected to your Intel NUC, you can access the shell directly. The default credentials are username **root** and password **root**.

- Access the shell on your Intel NUC using SSH from your desktop machine.

#### Sign in with SSH

To sign in with SSH, you need the IP address of your Intel NUC. If you have a keyboard and monitor connected to your Intel NUC, use the `ifconfig` command to find the IP address. Alternatively, connect to your router to list the addresses of devices on your network.

Sign in with username **root** and password **root**.

#### Optional: Share a folder on your Intel NUC

Optionally, you may want to share a folder on your Intel NUC with your desktop environment. Sharing a folder enables you to use your preferred desktop text editor (such as [Visual Studio Code](https://code.visualstudio.com/) or [Sublime Text](http://www.sublimetext.com/)) to edit files on your Intel NUC instead of using `nano` or `vi`.

To share a folder with Windows, configure a Samba server on the Intel NUC. Alternatively, use the SFTP server on the Intel NUC with an SFTP client on your desktop machine.


## Build IoT Edge

This tutorial uses custom IoT Edge modules to communicate with the remote monitoring preconfigured solution. Therefore, you need to build the IoT Edge modules from custom source code. The following sections describe how to install IoT Edge and build the custom IoT Edge module.

### Install IoT Edge

The following steps describe how to install the pre-compiled IoT Edge software on the Intel NUC:

1. Configure the required smart package repositories by running the following commands on the Intel NUC:

    ```bash
    smart channel --add IoT_Cloud type=rpm-md name="IoT_Cloud" baseurl=http://iotdk.intel.com/repos/iot-cloud/wrlinux7/rcpl13/ -y
    smart channel --add WR_Repo type=rpm-md baseurl=https://distro.windriver.com/release/idp-3-xt/public_feeds/WR-IDP-3-XT-Intel-Baytrail-public-repo/RCPL13/corei7_64/
    ```

    Enter `y` when the command prompts you to **Include this channel?**.

1. Update the smart package manager by running the following command:

    ```bash
    smart update
    ```

1. Install the Azure IoT Edge package by running the following command:

    ```bash
    smart config --set rpm-check-signatures=false
    smart install packagegroup-cloud-azure -y
    ```

1. Verify the installation by running the "Hello world" sample. This sample writes a hello world message to the log.txT file every five seconds. The following commands run the "Hello world" sample:

    ```bash
    cd /usr/share/azureiotgatewaysdk/samples/hello_world/
    ./hello_world hello_world.json
    ```

    Ignore any **invalid argument** messages when you stop the sample.

    Use the following command to view the contents of the log file:

    ```bash
    cat log.txt | more
    ```

### Troubleshooting

If you receive the error "No package provides util-linux-dev", try rebooting the Intel NUC.

## Build the custom IoT Edge module

You can now build the custom IoT Edge module that enables the gateway to send messages to the remote monitoring solution. For more information about configuring a gateway and IoT Edge modules, see [Azure IoT Edge concepts][lnk-gateway-concepts].

Download the source code for the custom IoT Edge modules from GitHub using the following commands:

```bash
cd ~
git clone https://github.com/Azure-Samples/iot-remote-monitoring-c-intel-nuc-gateway-getting-started.git
```

Build the custom IoT Edge module using the following commands:

```bash
cd ~/iot-remote-monitoring-c-intel-nuc-gateway-getting-started/simulator
chmod u+x build.sh
sed -i -e 's/\r$//' build.sh
./build.sh
```

The build script places the libsimulator.so custom IoT Edge module in the build folder.

## Configure and run the IoT Edge gateway

You can now configure the IoT Edge gateway to send simulated telemetry to your remote monitoring dashboard. For more information about configuring a gateway and IoT Edge modules, see [Azure IoT Edge concepts][lnk-gateway-concepts].

> [!TIP]
> In this tutorial, you use the standard `vi` text editor on the Intel NUC. If you have not used `vi` before, you should complete an introductory tutorial, such as [Unix - The vi Editor Tutorial][lnk-vi-tutorial] to familiarize yourself with this editor. Alternatively, you can install the more user-friendly [nano](https://www.nano-editor.org/) editor using the command `smart install nano -y`.

Open the sample configuration file in the **vi** editor using the following command:

```bash
vi ~/iot-remote-monitoring-c-intel-nuc-gateway-getting-started/simulator/remote_monitoring.json
```

Locate the following lines in the configuration for the IoTHub module:

```json
"args": {
  "IoTHubName": "<<Azure IoT Hub Name>>",
  "IoTHubSuffix": "<<Azure IoT Hub Suffix>>",
  "Transport": "http"
}
```

Replace the placeholder values with the IoT Hub information you created and saved at the start of this tutorial. The value for IoTHubName looks like **yourrmsolution37e08**, and the value for IoTSuffix is typically **azure-devices.net**.

Locate the following lines in the configuration for the mapping module:

```json
args": [
  {
    "macAddress": "AA:BB:CC:DD:EE:FF",
    "deviceId": "<<Azure IoT Hub Device ID>>",
    "deviceKey": "<<Azure IoT Hub Device Key>>"
  },
  {
    "macAddress": "AA:BB:CC:DD:EE:FF",
    "deviceId": "<<Azure IoT Hub Device ID>>",
    "deviceKey": "<<Azure IoT Hub Device Key>>"
  }
]
```

Replace the **deviceID** and **deviceKey** placeholders with the IDs and keys for the two devices you created in the remote monitoring solution previously.

Save your changes.

You can now run the IoT Edge gateway using the following commands:

```bash
cd ~/iot-remote-monitoring-c-intel-nuc-gateway-getting-started/simulator
/usr/share/azureiotgatewaysdk/samples/simulated_device_cloud_upload/simulated_device_cloud_upload remote_monitoring.json
```

The gateway starts on the Intel NUC and sends simulated telemetry to the remote monitoring solution:

![IoT Edge gateway generates simulated telemetry][img-simulated telemetry]

Press **Ctrl-C** to exit the program at any time.

## View the telemetry

The IoT Edge gateway is now sending simulated telemetry to the remote monitoring solution. You can view the telemetry on the solution dashboard.

- Navigate to the solution dashboard.
- Select one of the two devices you configured in the gateway in the **Device to View** dropdown.
- The telemetry from the gateway devices displays on the dashboard.

![Display telemetry from the simulated gateway devices][img-telemetry-display]

> [!WARNING]
> If you leave the remote monitoring solution running in your Azure account, you are billed for the time it runs. For more information about reducing consumption while the remote monitoring solution runs, see [Configuring Azure IoT Suite preconfigured solutions for demo purposes][lnk-demo-config]. Delete the preconfigured solution from your Azure account when you have finished using it.

## Next steps

Visit the [Azure IoT Dev Center](https://azure.microsoft.com/develop/iot/) for more samples and documentation on Azure IoT.

[img-simulated telemetry]: ./../media/iot-suite-v1-gateway-kit-get-started-simulator/appoutput.png
[img-telemetry-display]: ./../media/iot-suite-v1-gateway-kit-get-started-simulator/telemetry.png
[img-launch-solution]: ./../media/iot-suite-v1-gateway-kit-view-solution/launch.png
[img-menu]: ./../media/iot-suite-v1-gateway-kit-view-solution/menu.png
[1]: ./../media/iot-suite-v1-gateway-kit-view-solution/suite0.png
[2]: ./../media/iot-suite-v1-gateway-kit-view-solution/suite1.png
[3]: ./../media/iot-suite-v1-gateway-kit-view-solution/suite2.png
[4]: ./../media/iot-suite-v1-gateway-kit-view-solution/suite3.png

[lnk-demo-config]: https://github.com/Azure/azure-iot-remote-monitoring/blob/master/Docs/configure-preconfigured-demo.md
[lnk-vi-tutorial]: http://www.tutorialspoint.com/unix/unix-vi-editor.htm
[lnk-gateway-concepts]: https://docs.microsoft.com/azure/iot-hub/iot-hub-linux-iot-edge-get-started
[lnk-starter-kits]: https://azure.microsoft.com/develop/iot/starter-kits/
[lnk-free-trial]: http://azure.microsoft.com/pricing/free-trial/
[lnk-azureiotsuite]: https://www.azureiotsuite.com
[lnk-permissions]: https://docs.microsoft.com/en-us/azure/iot-suite/iot-suite-v1-permissions
[lnk-portal]: http://portal.azure.com/
[lnk-faq]: https://docs.microsoft.com/en-us/azure/iot-suite/iot-suite-v1-faq