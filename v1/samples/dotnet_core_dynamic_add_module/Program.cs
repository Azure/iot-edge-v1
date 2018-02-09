using System;
using System.IO;
using Microsoft.Azure.Devices.Gateway;

namespace dotnet_core_dynamic_add_module
{
    class Program
    {

        static void Assert(bool val, string err_msg)
        {
            if (val == false)
            {
                throw new Exception(err_msg);
            }
        }

        static void Main(string[] args)
        {
            if (args.Length < 3 || !File.Exists(args[0])  || !File.Exists(args[1]) || !File.Exists(args[2]) )
            {
                Console.Error.WriteLine("Usage: managed-gateway <module1 json> <module2 json> <links json>");
            }
            else
            {
                // Create a gateway without configuration.
                IntPtr gateway = GatewayInterop.Create();
                Assert(gateway != IntPtr.Zero, "Null gateway returned");

                // Fill up the module1 configuration
                int ret = GatewayInterop.UpdateFromJson(gateway, args[0]);
                Assert(ret == 0, "Module 1 update gateway error");
                // Module 2 configuraiton
                ret = GatewayInterop.UpdateFromJson(gateway, args[1]);
                Assert(ret == 0, "Module 2 update gateway error");

                ret = GatewayInterop.Start(gateway);
                Assert(ret == 0, "Start gateway error");
                Console.WriteLine("Started gateway. Press return to link the module");
                Console.ReadLine();

                // Update the link
                ret = GatewayInterop.UpdateFromJson(gateway, args[2]);
                Assert(ret == 0, "Links update gateway error");

                Console.WriteLine(".NET Core Gateway is running. Press return to quit.");
                Console.ReadLine();

                GatewayInterop.Destroy(gateway);
            }
        }
    }
}
