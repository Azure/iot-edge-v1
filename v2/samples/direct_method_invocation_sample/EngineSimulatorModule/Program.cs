using System.Globalization;

namespace EngineSimulatorModule
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Loader;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Devices.Client;
    using Microsoft.Azure.Devices.Client.Transport.Mqtt;

    class Program
    {
        static void Main(string[] args)
        {
            var engine = new Engine();
            // The Edge runtime gives us the connection string we need -- it is injected as an environment variable
            string connectionString = Environment.GetEnvironmentVariable("EdgeHubConnectionString");

            // Cert verification is not yet fully functional when using Windows OS for the container
            bool bypassCertVerification = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
            if (!bypassCertVerification)
            {
                InstallCert();
            }

            DeviceClient deviceClient = Init(connectionString, engine, bypassCertVerification).Result;

            // Wait until the app unloads or is cancelled
            var cts = new CancellationTokenSource();
            AssemblyLoadContext.Default.Unloading += (ctx) => cts.Cancel();
            Console.CancelKeyPress += (sender, cpe) => cts.Cancel();

            // Begin sending engine data. 
            SendMessages(deviceClient, engine, cts.Token).Wait();

            WhenCancelled(cts.Token).Wait();
        }

        /// <summary>
        /// Handles cleanup operations when app is cancelled or unloads
        /// </summary>
        public static Task WhenCancelled(CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<bool>();
            cancellationToken.Register(s => ((TaskCompletionSource<bool>)s).SetResult(true), tcs);
            return tcs.Task;
        }

        /// <summary>
        /// Add certificate in local cert store for use by client for secure connection to IoT Edge runtime
        /// </summary>
        static void InstallCert()
        {
            string certPath = Environment.GetEnvironmentVariable("EdgeModuleCACertificateFile");
            if (string.IsNullOrWhiteSpace(certPath))
            {
                // We cannot proceed further without a proper cert file
                Console.WriteLine($"Missing path to certificate collection file: {certPath}");
                throw new InvalidOperationException("Missing path to certificate file.");
            }
            else if (!File.Exists(certPath))
            {
                // We cannot proceed further without a proper cert file
                Console.WriteLine($"Missing path to certificate collection file: {certPath}");
                throw new InvalidOperationException("Missing certificate file.");
            }
            X509Store store = new X509Store(StoreName.Root, StoreLocation.CurrentUser);
            store.Open(OpenFlags.ReadWrite);
            store.Add(new X509Certificate2(X509Certificate2.CreateFromCertFile(certPath)));
            Console.WriteLine("Added Cert: " + certPath);
            store.Close();
        }


        /// <summary>
        /// Initializes the DeviceClient and sets up the callback to receive
        /// messages containing temperature information
        /// </summary>
        static async Task<DeviceClient> Init(string connectionString, Engine engine, bool bypassCertVerification)
        {
            Console.WriteLine("Connection String {0}", connectionString);

            MqttTransportSettings mqttSetting = new MqttTransportSettings(TransportType.Mqtt_Tcp_Only);
            // During dev you might want to bypass the cert verification. It is highly recommended to verify certs systematically in production
            if (bypassCertVerification)
            {
                mqttSetting.RemoteCertificateValidationCallback = (sender, certificate, chain, sslPolicyErrors) => true;
            }
            ITransportSettings[] settings = { mqttSetting };

            // Open a connection to the Edge runtime
            DeviceClient ioTHubModuleClient = DeviceClient.CreateFromConnectionString(connectionString, settings);
            await ioTHubModuleClient.OpenAsync();

            // Setup a handler for the Reset method.
            await ioTHubModuleClient.SetMethodHandlerAsync("Reset", ResetMethodHandler, engine);
            Console.WriteLine("IoT Hub module client initialized.");

            return ioTHubModuleClient;
        }

        // This method is called if the Reset method is invoked on this module.
        // It resets the engine RPM value. 
        private static Task<MethodResponse> ResetMethodHandler(MethodRequest methodrequest, object usercontext)
        {
            Console.WriteLine($"Received reset method call... resetting engine");
            try
            {
                if (usercontext is Engine engine)
                {
                    engine.Reset();
                }
                return Task.FromResult(new MethodResponse(200));
            }
            catch (Exception ex)
            {
                return Task.FromResult(new MethodResponse(Encoding.UTF8.GetBytes(ex.Message), 500));
            }
        }

        // Sends messages containing simulated Engine data at a fixed interval 
        static async Task SendMessages(DeviceClient ioTHubModuleClient, Engine engine, CancellationToken cancellationToken)
        {
            Console.WriteLine("Sending engine data...");
            long counter = 0;
            while (!cancellationToken.IsCancellationRequested)
            {
                string data = engine.GetCurrentRpm().ToString(CultureInfo.InvariantCulture);
                var message = new Message(Encoding.UTF8.GetBytes(data));
                message.MessageSchema = "engineData";
                message.MessageId = counter.ToString();
                await ioTHubModuleClient.SendEventAsync(message);
                Console.WriteLine($"Sent message {counter} with engine RPM value = {data}");
                await Task.Delay(TimeSpan.FromMilliseconds(300));
                counter++;
            }
        }
    }
}
