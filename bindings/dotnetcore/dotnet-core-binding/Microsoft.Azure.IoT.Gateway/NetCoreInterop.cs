using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace Microsoft.Azure.IoT.Gateway
{
    /// <summary>
    ///    This class holds the static methods that are going to be called by the native .NET Core binding in order to create a module, receive message, destroy and start modules. 
    ///     It will use reflection to call the.NET Core managed module.
    /// </summary>
    public class NetCoreInterop
    {
        private static IDictionary<uint, DotNetCoreModuleInstance> loadedModules = null;

        private static uint moduleIDCounter = 0;

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
            if (broker == IntPtr.Zero)
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
                throw new ArgumentNullException("broker", "broker cannot be null");
            }
            else if(module == IntPtr.Zero)
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_001: [ If any parameter (except configuration) is null, constructor shall throw a ArgumentNullException. ] */
                throw new ArgumentNullException("module", "module cannot be null");
            }
            else if(String.IsNullOrEmpty(assemblyName))
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
                if(loadedModules == null)
                {
                    loadedModules = new Dictionary<uint, DotNetCoreModuleInstance>();
                }                

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_005: [ Create shall load client library identified by assemblyName. ] */
                AssemblyName assemblyNameToLoad = new AssemblyName(assemblyName);
                Assembly moduleAssembly = Assembly.Load(assemblyNameToLoad);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_006: [ Create shall get types from loaded assembly and check if .NET Core module has implemented IGatewayModule. ] */
                Type dotNetCoreModuleEntryType = moduleAssembly.GetType(entryType);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_004: [ Create shall create an instance of DotNetCoreModuleInstance to store module information; ] */
                DotNetCoreModuleInstance moduleInstance = new DotNetCoreModuleInstance();

                //Checks if entrytype implements IGatewayModule.
                moduleInstance.gatewayModule = (IGatewayModule)Activator.CreateInstance(dotNetCoreModuleEntryType);

                

                Type[] createParameters = { System.Type.GetType("Microsoft.Azure.IoT.Gateway.Broker"), System.Type.GetType("System.Byte[]") };
                MethodInfo createMethod = dotNetCoreModuleEntryType.GetRuntimeMethod("Create", createParameters);
                
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_009: [ Create shall call Create method on client module. ] */
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_024: [ Create shall verify if the module contains the type describe by entryType and check of methods Create, Destroy, Receive and Start were implemented on that type. ] */
                moduleInstance.gatewayModule = (IGatewayModule)Activator.CreateInstance(dotNetCoreModuleEntryType);
                Microsoft.Azure.IoT.Gateway.Broker brokerObject = new Broker(broker, module);
                Object[] parameters = { brokerObject, Encoding.UTF8.GetBytes(configuration) };
                createMethod.Invoke(moduleInstance.gatewayModule, parameters);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_007: [ Create shall save delegates to Receive, Destroy and Start(if implemented) to DotNetCoreModuleInstance. ] */
                Type[] receiveParameters = { System.Type.GetType("Microsoft.Azure.IoT.Gateway.Message") };
                moduleInstance.receiveMethodInfo = dotNetCoreModuleEntryType.GetRuntimeMethod("Receive", receiveParameters);

                Type[] destroyParameters = { };
                moduleInstance.destroyMethodInfo = dotNetCoreModuleEntryType.GetRuntimeMethod("Destroy", destroyParameters);

                Type[] startParameters = { };
                moduleInstance.startMethodInfo = dotNetCoreModuleEntryType.GetRuntimeMethod("Start", startParameters);

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

        /// <summary>
        ///     Calls Receive method on .NET Core module.
        /// </summary>
        /// <param name="messageAsArray">Reference to the message serialized as byte array.</param>
        /// <param name="size">Size of the message byte array.</param>
        /// <param name="moduleID">Gateway module ID.</param>
        public static void Receive([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)] byte[] messageAsArray, ulong size, uint moduleID)
        {
            DotNetCoreModuleInstance moduleDetails;

            /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_011: [ Receive shall get the DotNetCoreModuleInstance based on moduleID ] */
            if (loadedModules.TryGetValue(moduleID, out moduleDetails))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_012: [ Receive shall create a Message object based on messageAsArray ] */
                Message messageReceived = new Message(messageAsArray);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_013: [Receive shall invoke.NET Core client method Receive. ] */
                Object[] parameters = { messageReceived };
                moduleDetails.receiveMethodInfo.Invoke(moduleDetails.gatewayModule, parameters);
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_021: [ Receive shall raise an Exception if module can't be found. ] */
                throw new Exception("Module " + moduleID + " can't be found.");
            }
        }

        /// <summary>
        ///     Calls Destroy method on .NET Core module. This method is not thread safe, since gateway serializes calls to Destroy
        /// </summary>
        /// <param name="moduleID">Gateway module ID.</param>
        public static void Destroy(uint moduleID)
        {
            DotNetCoreModuleInstance moduleDetails;
            
            /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_014: [ Destroy shall get the DotNetCoreModuleInstance based on moduleID ] */
            if (loadedModules.TryGetValue(moduleID, out moduleDetails))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_020: [ Destroy shall remove module from the set of modules. ] */
                loadedModules.Remove(moduleID);

                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_015: [ Destroy shall invoke .NET Core client method Destroy. ] */
                moduleDetails.destroyMethodInfo.Invoke(moduleDetails.gatewayModule, null);
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_022: [ Destroy shall raise an Exception if module can't be found. ] */
                throw new Exception("Module " + moduleID + " can't be found.");
            }
               
        }

        /// <summary>
        ///     Calls Start method on .NET Core module (If implemented).
        /// </summary>
        /// <param name="moduleID">Gateway module ID.</param>
        public static void Start(uint moduleID)
        {
            DotNetCoreModuleInstance moduleDetails;

            /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_016: [ Start shall get the DotNetCoreModuleInstance based on moduleID ] */
            if (loadedModules.TryGetValue(moduleID, out moduleDetails))
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_017: [ If .NET Core client module did not implement start, Start shall do nothing. ] */
                if (moduleDetails.startMethodInfo != null)
                {
                    /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_018: [ Start shall invoke .NET Core client method Start. ] */
                    moduleDetails.startMethodInfo.Invoke(moduleDetails.gatewayModule, null);
                }
            }
            else
            {
                /* Codes_SRS_DOTNET_CORE_NATIVE_MANAGED_GATEWAY_INTEROP_04_023: [ Start shall raise an Exception if module can't be found. ] */
                throw new Exception("Module " + moduleID + " can't be found.");
            }
        }
    }
}
