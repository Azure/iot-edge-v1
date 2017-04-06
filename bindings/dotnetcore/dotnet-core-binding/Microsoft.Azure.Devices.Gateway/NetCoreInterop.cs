using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace Microsoft.Azure.Devices.Gateway
{
    internal class DotNetCoreReflectionLayer
    {
        virtual public Type GetType(string assemblyName, string entryType)
        {
            AssemblyName assemblyNameToLoad = new AssemblyName(assemblyName);
            Assembly moduleAssembly = Assembly.Load(assemblyNameToLoad);

            Type dotNetCoreModuleEntryType = moduleAssembly.GetType(entryType);
            return dotNetCoreModuleEntryType;
        }

        virtual public IGatewayModule CreateInstance(Type type)
        {
            return (IGatewayModule)Activator.CreateInstance(type);
        }

        virtual public MethodInfo GetMethod(Type classType, string name, Type[] parameters)
        {
            return classType.GetRuntimeMethod(name, parameters);
        }

        virtual public Object InvokeMethod(IGatewayModule module, MethodInfo method, Object[] args)
        {
            return method.Invoke(module, args);
        }
    }

    internal class NetCoreInteropInstance
    {
        private IDictionary<uint, DotNetCoreModuleInstance> loadedModules = null;
        private uint moduleIDCounter = 0;
        private DotNetCoreReflectionLayer _reflectionLayer;

        private NetCoreInteropInstance(DotNetCoreReflectionLayer interop)
        {
            _reflectionLayer = interop;
        }

        public static NetCoreInteropInstance GetInstance()
        {
            return new NetCoreInteropInstance(new DotNetCoreReflectionLayer());
        }

        public static NetCoreInteropInstance GetInstance(DotNetCoreReflectionLayer dotnetReflectionLayer)
        {
            return new NetCoreInteropInstance(dotnetReflectionLayer);
        }

        public uint Create(IntPtr broker, IntPtr module, string assemblyName, string entryType, string configuration)
        {
            if (broker == IntPtr.Zero)
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
                throw new ArgumentNullException("broker", "broker cannot be null");
            }
            else if (module == IntPtr.Zero)
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
                throw new ArgumentNullException("module", "module cannot be null");
            }
            else if (String.IsNullOrEmpty(assemblyName))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
                throw new ArgumentNullException("assemblyName", "assemblyName cannot be null or empty");
            }
            else if (String.IsNullOrEmpty(entryType))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
                throw new ArgumentNullException("entryType", "entryType cannot be null or empty");
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_002: [ If the set of modules is not initialized, it shall be. ] */
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_003: [ If any underlying api fails, this method shall raise an Exception. ] */
                if (loadedModules == null)
                {
                    loadedModules = new Dictionary<uint, DotNetCoreModuleInstance>();
                }

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_005: [ Create shall load client library identified by assemblyName. ] */
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_006: [ Create shall get types from loaded assembly and check if .NET Core module has implemented IGatewayModule. ] */
                Type type = _reflectionLayer.GetType(assemblyName, entryType);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_004: [ Create shall create an instance of DotNetCoreModuleInstance to store module information; ] */
                DotNetCoreModuleInstance moduleInstance = new DotNetCoreModuleInstance();

                //Checks if entrytype implements IGatewayModule.
                moduleInstance.gatewayModule = _reflectionLayer.CreateInstance(type);

                MethodInfo createMethod = _reflectionLayer.GetMethod(
                    type,
                    "Create",
                    new Type[] { System.Type.GetType("Microsoft.Azure.Devices.Gateway.Broker"), System.Type.GetType("System.Byte[]") }
                    );

                var brokerObject = new Broker(broker, module, new BrokerInterop());

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_009: [ Create shall call Create method on client module. ] */
                byte[] moduleConfiguration = configuration == null ? Encoding.UTF8.GetBytes("") : Encoding.UTF8.GetBytes(configuration);
                _reflectionLayer.InvokeMethod(moduleInstance.gatewayModule, createMethod, new Object[] { brokerObject, moduleConfiguration });

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_007: [ Create shall save delegates to Receive, Destroy and Start(if implemented) to DotNetCoreModuleInstance. ] */
                moduleInstance.receiveMethodInfo = _reflectionLayer.GetMethod(type, "Receive", new Type[] { System.Type.GetType("Microsoft.Azure.Devices.Gateway.Message") });
                moduleInstance.destroyMethodInfo = _reflectionLayer.GetMethod(type, "Destroy", new Type[] { });
                moduleInstance.startMethodInfo = _reflectionLayer.GetMethod(type, "Start", new Type[] { });

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_024: [ Create shall verify if the module contains the type describe by entryType and check of methods Create, Destroy, Receive and Start were implemented on that type. ] */
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_008: [ If client .NET Core module doesn't implement one of the required methods Create, Receive or Destroy, Create shall raise an exception. ] */
                if (moduleInstance.destroyMethodInfo == null || moduleInstance.receiveMethodInfo == null)
                {
                    throw new Exception("Missing implementation of Create, Destroy or Receive.");
                }

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_010: [ If Create module on client module succeeds return moduleID, otherwise raise an Exception ] */
                loadedModules.Add(++moduleIDCounter, moduleInstance);
            }

            return moduleIDCounter;
        }

        public void Receive(byte[] messageAsArray, UInt32 size, uint moduleID)
        {
            DotNetCoreModuleInstance moduleDetails;

            /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_011: [ Receive shall get the DotNetCoreModuleInstance based on moduleID ] */
            if (loadedModules.TryGetValue(moduleID, out moduleDetails))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_012: [ Receive shall create a Message object based on messageAsArray ] */
                Message messageReceived = new Message(messageAsArray);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_013: [Receive shall invoke.NET Core client method Receive. ] */
                Object[] parameters = { messageReceived };
                _reflectionLayer.InvokeMethod(moduleDetails.gatewayModule, moduleDetails.receiveMethodInfo, new Object[] { messageReceived });
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_021: [ Receive shall raise an Exception if module can't be found. ] */
                throw new Exception("Module " + moduleID + " can't be found.");
            }
        }

        public void Destroy(uint moduleID)
        {
            DotNetCoreModuleInstance moduleDetails;

            /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_014: [ Destroy shall get the DotNetCoreModuleInstance based on moduleID ] */
            if (loadedModules.TryGetValue(moduleID, out moduleDetails))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_020: [ Destroy shall remove module from the set of modules. ] */
                loadedModules.Remove(moduleID);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_015: [ Destroy shall invoke .NET Core client method Destroy. ] */
                _reflectionLayer.InvokeMethod(moduleDetails.gatewayModule, moduleDetails.destroyMethodInfo, null);
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_022: [ Destroy shall raise an Exception if module can't be found. ] */
                throw new Exception("Module " + moduleID + " can't be found.");
            }

        }

        public void Start(uint moduleID)
        {
            DotNetCoreModuleInstance moduleDetails;

            /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_016: [ Start shall get the DotNetCoreModuleInstance based on moduleID ] */
            if (loadedModules.TryGetValue(moduleID, out moduleDetails))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_017: [ If .NET Core client module did not implement start, Start shall do nothing. ] */
                if (moduleDetails.startMethodInfo != null)
                {
                    /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_018: [ Start shall invoke .NET Core client method Start. ] */
                    _reflectionLayer.InvokeMethod(moduleDetails.gatewayModule, moduleDetails.startMethodInfo, null);
                }
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_023: [ Start shall raise an Exception if module can't be found. ] */
                throw new Exception("Module " + moduleID + " can't be found.");
            }
        }
    }

    /// <summary>
    ///    This class holds the static methods that are going to be called by the native .NET Core binding in order to create a module, receive message, destroy and start modules. 
    ///     It will use reflection to call the.NET Core managed module.
    /// </summary>
    internal class NetCoreInterop
    {
        private delegate uint CreateDelegate(IntPtr broker, IntPtr module, string assemblyName, string entryType, string configuration);

        private delegate void ReceiveDelegate([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] messageAsArray, UInt32 size, uint moduleID);

        private delegate void DestroyDelegate(uint moduleID);

        private delegate void StartDelegate(uint moduleID);

        [DllImport("dotnetcore",
            CharSet = CharSet.Ansi,
            EntryPoint = "Module_DotNetCoreHost_SetBindingDelegates",
            CallingConvention = CallingConvention.Cdecl
        )]
        private static extern void InitializeDelegatesOnNative(IntPtr createAddress, IntPtr receiveAddress, IntPtr destroyAddress, IntPtr startAddress);


        private static NetCoreInteropInstance _netCoreInteropInstance = NetCoreInteropInstance.GetInstance();


        public static void replaceReflectionLayer(DotNetCoreReflectionLayer dotNetCoreReflectionLayer)
        {
            _netCoreInteropInstance = NetCoreInteropInstance.GetInstance(dotNetCoreReflectionLayer);
        }

        /// <summary>
        ///    Loads .NET Core module and call the Create method. This method is not thread safe, since gateway serializes calls to Create.
        /// </summary>
        /// <param name="broker">Reference to the gateway broker.</param>
        /// <param name="module">Reference to the gateway module.</param>
        /// <param name="assemblyName">Assembly name for .NET Core Module.</param>
        /// <param name="entryType">Class that implements IGatewayModule.</param>
        /// <param name="configuration">.NET Core module configuration.</param>
        /// <returns>Module ID.</returns>
        public static uint Create(IntPtr broker, IntPtr module, string assemblyName, string entryType, string configuration)
        {
            return _netCoreInteropInstance.Create(broker, module, assemblyName, entryType, configuration);
        }

        /// <summary>
        ///     Calls Receive method on .NET Core module.
        /// </summary>
        /// <param name="messageAsArray">Reference to the message serialized as byte array.</param>
        /// <param name="size">Size of the message byte array.</param>
        /// <param name="moduleID">Gateway module ID.</param>
        public static void Receive([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] messageAsArray, UInt32 size, uint moduleID)
        {
            _netCoreInteropInstance.Receive(messageAsArray, size, moduleID);
        }

        /// <summary>
        ///     Calls Destroy method on .NET Core module. This method is not thread safe, since gateway serializes calls to Destroy
        /// </summary>
        /// <param name="moduleID">Gateway module ID.</param>
        public static void Destroy(uint moduleID)
        {
            _netCoreInteropInstance.Destroy(moduleID);
        }

        /// <summary>
        ///     Calls Start method on .NET Core module (If implemented).
        /// </summary>
        /// <param name="moduleID">Gateway module ID.</param>
        public static void Start(uint moduleID)
        {
            _netCoreInteropInstance.Start(moduleID);
        }

        private static CreateDelegate delCreate = null;
        private static ReceiveDelegate delReceive = null;
        private static DestroyDelegate delDestroy = null;
        private static StartDelegate delStart = null;

        public static void InitializeDelegates()
        {
            
            delCreate = Create;
            delReceive = Receive;
            delDestroy = Destroy;
            delStart = Start;

            InitializeDelegatesOnNative(Marshal.GetFunctionPointerForDelegate(delCreate),
                                        Marshal.GetFunctionPointerForDelegate(delReceive),
                                        Marshal.GetFunctionPointerForDelegate(delDestroy),
                                        Marshal.GetFunctionPointerForDelegate(delStart));
        }
    }
}
