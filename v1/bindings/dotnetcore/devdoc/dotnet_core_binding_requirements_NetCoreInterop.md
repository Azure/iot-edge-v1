.NET Core binding for Azure IoT Gateway Modules - NetCoreInterop Class
======================================================================

Overview
--------

This document specifies the requirements for the .NET `NetCoreInterop` class which is part of `Microsoft.Azure.Devices.Gateway` namespace. 

This class holds the static methods that are going to be called by the native .NET Core binding in order to create a module, receive messages, destroy and start modules. 
It will use reflection to call the .NET Core managed module.

This class is not thread safe, since gateway serializes calls to `Create`.

More details can be found in the [high level design](./dotnet_core_binding_hld.md).

Class interface
---------------
```C#
namespace Microsoft.Azure.Devices.Gateway
{
    /// <summary>
    ///    This class holds the static methods that are going to be called by the native .NET Core binding in order to create a module, receive message, destroy and start modules. 
    ///     It will use reflection to call the.NET Core managed module.
    /// </summary>
    public class NetCoreInterop
    {
        /// <summary>
        ///    Loads .NET Core module and call the Create method. This method is not thread safe, since gateway serializes calls to Create.
        /// </summary>
        /// <param name="broker">Reference to the gateway broker.</param>
        /// <param name="module">Reference to the gateway module.</param>
        /// <param name="assemblyName">Assembly name for .NET Core Module.</param>
        /// <param name="entryType">Class that implements IGatewayModule.</param>
        /// <param name="configuration">.NET Core module configuration.</param>
        /// <returns>Module ID.</returns>
        public static uint Create(IntPtr broker, IntPtr module, string assemblyName, string entryType, string configuration);

        /// <summary>
        ///     Calls Receive method on .NET Core module.
        /// </summary>
        /// <param name="messageAsArray">Reference to the message serialized as byte array.</param>
        /// <param name="size">Size of the message byte array.</param>
        /// <param name="moduleID">Gateway module ID.</param>
        public static void Receive([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] messageAsArray, ulong size, uint moduleID);

        /// <summary>
        ///     Calls Destroy method on .NET Core module. This method is not thread safe, since gateway serializes calls to Destroy.
        /// </summary>
        /// <param name="moduleID">Gateway module id.</param>
        public static void Destroy(uint moduleID);

        /// <summary>
        ///     Calls Start method on .NET Core module (if implemented).
        /// </summary>
        /// <param name="moduleID">Gateway module id.</param>
        public static void Start(uint moduleID);
    }
}
```

Create
------
```c#
public static int Create(IntPtr broker, IntPtr module, string assemblyName, string entryType, string configuration)
```

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [** If any parameter (except configuration) is null, constructor shall throw a `ArgumentNullException`. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_002: [** If the set of modules is not initialized, it shall be. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [** If any underlying api fails, this method shall raise an `Exception`. **]**.

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_004: [** `Create` shall create an instance of `DotNetCoreModuleInstance` to store module information; **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_005: [** `Create` shall load client library identified by `assemblyName`. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_006: [** `Create` shall get types from loaded assembly and check if .NET Core module has implemented `IGatewayModule`. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_007: [** `Create` shall save delegates to `Receive`, `Destroy` and `Start`(if implemented) to `DotNetCoreModuleInstance`. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_008: [** If client .NET Core module doesn't implement one of the required methods `Create`, `Receive` or `Destroy`, `Create` shall raise an exception. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_009: [** `Create` shall call `Create` method on client module.  **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_010: [** If `Create` module on client module succeeds return `moduleID`, otherwise raise an `Exception` **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_024: [** `Create` shall verify if the module contains the type describe by `entryType` and check of methods `Create`, `Destroy`, `Receive` and `Start` were implemented on that type. **]**

Receive
-------
```c#
public static void Receive([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 2)] byte[] messageAsArray, long size, int moduleID)
```

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_011: [** `Receive` shall get the `DotNetCoreModuleInstance` based on `moduleID` **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_019: [** `Receive` shall raise an `Exception` if module can not be found based on `moduleID` **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_012: [** `Receive` shall create a `Message` object based on `messageAsArray` **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_013: [** `Receive` shall invoke .NET Core client method `Receive`. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_021: [** `Receive` shall raise an `Exception` if module can't be found. **]**


Destroy
-------
```c#
public static void Destroy(int moduleID)
```

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_014: [** `Destroy` shall get the `DotNetCoreModuleInstance` based on `moduleID` **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_015: [** `Destroy` shall invoke .NET Core client method `Destroy`. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_020: [** `Destroy` shall remove module from the set of modules. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_022: [** `Destroy` shall raise an `Exception` if module can't be found. **]**

Start
-----
```c#
public static void Start(int moduleID)
```

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_016: [** `Start` shall get the `DotNetCoreModuleInstance` based on `moduleID` **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_017: [** If .NET Core client module did not implement start, `Start` shall do nothing. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_018: [** `Start` shall invoke .NET Core client method `Start`. **]**

**SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_023: [** `Start` shall raise an `Exception` if module can't be found. **]**