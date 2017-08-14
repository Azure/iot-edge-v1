using System;
using System.Runtime.InteropServices;
using System.IO;

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
            EntryPoint = "Gateway_Create",
            CallingConvention = CallingConvention.Cdecl
        )]
        private static extern IntPtr CreateInternal(IntPtr p);

        [DllImport(
            "gateway",
            CharSet = CharSet.Ansi,
            EntryPoint = "Gateway_CreateFromJson",
            CallingConvention = CallingConvention.Cdecl
        )]
        private static extern IntPtr CreateFromJsonInternal(string file_path);

        [DllImport(
            "gateway",
            CharSet = CharSet.Ansi,
            EntryPoint = "Gateway_UpdateFromJson",
            CallingConvention = CallingConvention.Cdecl
        )]
        private static extern int UpdateFromJsonInternal(IntPtr gw, [MarshalAs(UnmanagedType.LPStr)] string json_string);


        [DllImport(
            "gateway",
            CharSet = CharSet.Ansi,
            EntryPoint = "Gateway_Start",
            CallingConvention = CallingConvention.Cdecl
        )]
        private static extern int StartInternal(IntPtr gw);

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
    	///   Update the gateway with module, link details etc.
	    /// <param name="gw">#GATEWAY_HANDLE to be started</param>
    	/// <param name="file_path">Path to the JSON configuration file to upate from .</param>  
	    /// </summary>	
    	public static int UpdateFromJson(IntPtr gw, string file_path)
	    {
		    var json_string = File.ReadAllText(file_path); 	
    		return UpdateFromJsonInternal(gw, json_string);
		}

    	/// <summary>
	    ///	Create gateway using a NULL property. This forms the template which we
    	///     will fill up with module and link information with subsequent calls to UpdateFromJson
	    ///     and then eventually start the gateway.
    	/// </summary>
        public static IntPtr Create()
        {
            NetCoreInterop.InitializeDelegates();
            return CreateInternal(IntPtr.Zero);
	    }
	

    	/// <summary>
	    ///	Start the gateway server
    	/// </summary>
	    /// <param name="gw">#GATEWAY_HANDLE to be started</param>
    	public static int Start(IntPtr gw)
	    {
	        return StartInternal(gw);
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
