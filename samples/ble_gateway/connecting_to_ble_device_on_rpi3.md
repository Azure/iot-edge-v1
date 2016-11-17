Azure IoT Gateway - Connecting to a Bluetooth Low Energy device on the Raspberry Pi 3 
===================================================================================

Overview
--------

As it stands today, the Bluetooth LE (BLE) module that the Azure IoT Gateway SDK
ships with does not support *discovery* of BLE devices. This is however on the
product roadmap. In the meantime, the BLE module is perfectly usable for
prototyping solutions by performing some preparatory work as documented here.

Prerequisites
--------------

### Prepare your hardware
This tutorial assumes you are using a [Texas Instruments SensorTag](http://www.ti.com/ww/en/wireless_connectivity/sensortag2015/index.html) 
device connected to a Raspberry Pi 3 running Raspbian.

### Install BlueZ 5.37
The BLE modules talk to the Bluetooth hardware on the Raspberry Pi 3 via the 
BlueZ stack. You need version 5.37 of BlueZ for the modules to work correctly. 
These instructions make sure the correct version of BlueZ is installed.

1. Stop the current bluetooth daemon:
   
    ```
    sudo systemctl stop bluetooth
    ```
2. Install dbus, a BlueZ dependency. 
   
    ```
    sudo apt-get install libdbus-1-dev
    ```
3. Install ical, a BlueZ dependency. 
   
    ```
    sudo apt-get install libical-dev
    ```
4. Install readline, a BlueZ dependency. 
   
    ```
    sudo apt-get install libreadline-dev
    ```
5. Download the BlueZ source code from bluez.org. 
   
    ```
    wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.37.tar.xz
    ```
6. Unzip the source code.
   
    ```
    tar -xvf bluez-5.37.tar.xz
    ```
7. Change directories to the newly created folder.
   
    ```
    cd bluez-5.37
    ```
8. Build BlueZ.
   
    ```
    make
    ```
9. Install BlueZ once it is done building.
   
    ```
    sudo make install
    ```
10. Change systemd service configuration for bluetooth so it points to the new bluetooth daemon in the file `/lib/systemd/system/bluetooth.service`. Replace the 'ExecStart' line with the following text: 
    
    ```
    ExecStart=/usr/local/libexec/bluetooth/bluetoothd -E
    ```

Connecting your BLE device
--------------------------

Before running the sample, you need to verify that your Raspberry Pi 3 can connect to the SensorTag device.

1. Unblock bluetooth on the Raspberry Pi 3 and check that the version number is **5.37**.
   
    ```
    sudo rfkill unblock bluetooth
    bluetoothctl --version
    ```
2. Execute the **bluetoothctl** command. You are now in an interactive bluetooth shell. 
3. Enter the command **power on** to power up the bluetooth controller. You should see output similar to:
   
    ```
    [NEW] Controller 98:4F:EE:04:1F:DF C3 raspberrypi [default]
    ```
4. While still in the interactive bluetooth shell, enter the command **scan on** to scan for bluetooth devices. You should see output similar to:
   
    ```
    Discovery started
    [CHG] Controller 98:4F:EE:04:1F:DF Discovering: yes
    ```
5. Make the SensorTag device discoverable by pressing the small button (the green LED should flash). The Raspberry Pi 3 should discover the SensorTag device:
   
    ```
    [NEW] Device A0:E6:F8:B5:F6:00 CC2650 SensorTag
    [CHG] Device A0:E6:F8:B5:F6:00 TxPower: 0
    [CHG] Device A0:E6:F8:B5:F6:00 RSSI: -43
    ```
   
    In this example, you can see that the MAC address of the SensorTag device is **A0:E6:F8:B5:F6:00**.
6. Turn off scanning by entering the **scan off** command.
   
    ```
    [CHG] Controller 98:4F:EE:04:1F:DF Discovering: no
    Discovery stopped
    ```
7. Connect to your SensorTag device using its MAC address by entering **connect \<MAC address>**. Note that the sample output below is abbreviated:
   
    ```
    Attempting to connect to A0:E6:F8:B5:F6:00
    [CHG] Device A0:E6:F8:B5:F6:00 Connected: yes
    Connection successful
    [CHG] Device A0:E6:F8:B5:F6:00 UUIDs: 00001800-0000-1000-8000-00805f9b34fb
    ...
    [NEW] Primary Service
            /org/bluez/hci0/dev_A0_E6_F8_B5_F6_00/service000c
            Device Information
    ...
    [CHG] Device A0:E6:F8:B5:F6:00 GattServices: /org/bluez/hci0/dev_A0_E6_F8_B5_F6_00/service000c
    ...
    [CHG] Device A0:E6:F8:B5:F6:00 Name: SensorTag 2.0
    [CHG] Device A0:E6:F8:B5:F6:00 Alias: SensorTag 2.0
    [CHG] Device A0:E6:F8:B5:F6:00 Modalias: bluetooth:v000Dp0000d0110
    ```
   
    Note: You can list the GATT characteristics of the device again using the **list-attributes** command.
8. You can now disconnect from the device using the **disconnect** command and then exit from the bluetooth shell using the **quit** command:
   
    ```
    Attempting to disconnect from A0:E6:F8:B5:F6:00
    Successful disconnected
    [CHG] Device A0:E6:F8:B5:F6:00 Connected: no
    ```

You're now ready to run the BLE Gateway sample on your Raspberry Pi 3.