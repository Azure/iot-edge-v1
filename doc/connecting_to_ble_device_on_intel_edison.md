Azure IoT Gateway - Connecting to a Bluetooth Low Energy device on the Intel Edison 
===================================================================================

Overview
--------

As it stands today, the Bluetooth LE (BLE) module that the Azure IoT Gateway SDK
ships with does not support *discovery* of BLE devices. This is however on the
product roadmap. In the meantime, the BLE module is perfectly usable for
prototyping solutions by performing some preparatory work as documented here.

Prerequisites
--------------

Currently, the Azure IoT Gateway SDK supports communicating with BLE devices
only on Linux.  Here's what you'll need to do get your Intel Edison board setup:

  1. Setup your Intel Edison board using instructions as documented for the
     [Azure IoT Hub Device SDK](https://github.com/Azure/azure-iot-sdks/blob/master/doc/get_started/yocto-intel-edison-c.md).
     You'll need to follow along only up till the section titled "**Installing
     Git on your Intel Edison**".

  2. Upgrade the installation of BlueZ on your board to version 5.37 by building
     BlueZ from source. Here're the steps to do so:
     
       - Stop the currently running bluetooth daemon.
       
             systemctl stop bluetooth

       - Download and extract the [source code](http://www.kernel.org/pub/linux/bluetooth/bluez-5.37.tar.xz)
         for BlueZ version 5.37.
         
             wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.37.tar.xz
             tar -xvf bluez-5.37.tar.xz
             cd bluez-5.37

       - Build and install BlueZ.
             
             ./configure --disable-udev --disable-systemd --enable-experimental
             make
             make install
             
       - Change the *systemd* service configuration for bluetooth so that it
         points to the new bluetooth daemon by editing the file
         `/lib/systemd/system/bluetooth.service`. Replace the value of the
         `ExecStart` attribute so that it looks like this:

             ExecStart=/usr/local/libexec/bluetooth/bluetoothd -E

  3. Reboot your Edison.

  4. Unblock bluetooth.

         rfkill unblock bluetooth

  5. Check that the version of BlueZ is now 5.37. Running the following command
     should cause it to print the string `5.37` to the terminal.

         bluetoothctl --version

Connecting your BLE device
--------------------------

Here're the steps to be performed in order to *discover* and connect to a BLE
device:

  1. Run `bluetoothctl`. You should see output that looks like this:
  
         root@edison:~# bluetoothctl
         [NEW] Controller 98:4F:EE:03:BC:6B edison [default]
         [bluetooth]# _
  
  2. You should find yourself inside the interactive bluetooth shell. Start
     scanning for BLE devices:
     
         [bluetooth]# scan on
         Discovery started
         [CHG] Controller 98:4F:EE:03:BC:6B Discovering: yes

  2. Now cause your device to enter the mode that will cause it to broadcast
     BLE advertisement packets. Eventually you should see your device being
     listed by `bluetoothctl`.
     
         [NEW] Device B0:B4:48:B9:27:82 CC2650 SensorTag
         [CHG] Device B0:B4:48:B9:27:82 TxPower: 0

  3. Turn off scanning for bluetooth devices:
  
         [bluetooth]# scan off
         [CHG] Device B0:B4:48:B9:27:82 TxPower is nil
         [CHG] Device B0:B4:48:B9:27:82 RSSI is nil
         [CHG] Controller 98:4F:EE:03:BC:6B Discovering: no
         Discovery stopped
         [bluetooth]# _

  4. Connect to your BLE device:
  
         [bluetooth]# connect B0:B4:48:B9:27:82
         Attempting to connect to B0:B4:48:B9:27:82
         [CHG] Device B0:B4:48:B9:27:82 Connected: yes
         Connection successful
         [CC2650 SensorTag]# _

  5. After connecting, `bluetoothctl` might proceed to list the GATT
     characteristics and attributes supported by your BLE device. Inspect the
     characteristics it discovered by running `list-attributes`:

         [CC2650 SensorTag]# list-attributes
         Primary Service
                 /org/bluez/hci0/dev_B0_B4_48_B9_27_82/service000c
                 Device Information
         Characteristic
                 /org/bluez/hci0/dev_B0_B4_48_B9_27_82/service000c/char000d
                 System ID
         Characteristic
                 /org/bluez/hci0/dev_B0_B4_48_B9_27_82/service000c/char000f
                 Model Number String

     The full output has not been shown above - the output has been truncated
     after the enumeration of the first 3 GATT attribute rows.

  6. Now you can proceed to disconnect from the device:
  
         [CC2650 SensorTag]# disconnect B0:B4:48:B9:27:82
         Attempting to disconnect from B0:B4:48:B9:27:82
         Successful disconnected
         [CHG] Device B0:B4:48:B9:27:82 Connected: no
         [bluetooth]# _

That's it. Now you're fully setup to build and run the BLE gateway module with
your BLE device.