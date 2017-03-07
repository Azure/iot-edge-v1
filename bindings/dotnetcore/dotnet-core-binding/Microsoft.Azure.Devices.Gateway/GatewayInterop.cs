using System;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Devices.Gateway
{
    /// <summary>
    ///  Wrapper for Azure IoT Gateway SDK. 
    /// </summary>
    static public class GatewayInterop
    {
        [DllImport(
            "gateway",
            CharSet = CharSet.Ansi,
            EntryPoint = "Gateway_CreateFromJson",
            CallingConvention = CallingConvention.Cdecl
        )]
        private static extern IntPtr CreateFromJsonInternal(string file_path);


        /// <summary>
        ///   Creates a gateway using a JSON configuration file as inputwhich describes each module.
        ///   Each module described in the configuration must support Module_CreateFromJson.
        /// </summary>
        /// <param name="file_path">Path to the JSON configuration file for this gateway.</param>
        /// <returns>A non-NULL #GATEWAY_HANDLE that can be used to manage the gateway or @c NULL on failure.</returns>
        public static IntPtr CreateFromJson(string file_path)
        {
            NetCoreInterop.InitializeDelegates();
            return CreateFromJsonInternal(file_path);
        }

        /// <summary>
        ///     Destroys the gateway and disposes of all associated data.
        /// </summary>
        /// <param name="gateway">#GATEWAY_HANDLE to be destroyed.</param>
        [DllImport(
            "gateway",
            EntryPoint = "Gateway_Destroy",
            CallingConvention = CallingConvention.Cdecl
        )]
        public static extern void Destroy(IntPtr gateway);
    }
}
