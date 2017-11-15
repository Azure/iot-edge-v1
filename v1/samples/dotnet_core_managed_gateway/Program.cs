using System;
using System.IO;
using Microsoft.Azure.Devices.Gateway;

namespace dotnet_core_managed_gateway
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 1 || !File.Exists(args[0]))
            {
                Console.Error.WriteLine("Usage: managed-gateway <<path to json>>");
            }
            else
            {
                var json_path = args[0];
                var gateway = GatewayInterop.CreateFromJson(json_path);

                Console.WriteLine(".NET Core Gateway is running. Press return to quit.");
                Console.ReadLine();

                GatewayInterop.Destroy(gateway);
            }
        }
    }
}