namespace AnalyzerModule
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.Loader;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Azure.Devices;
    using Microsoft.Azure.Devices.Client;
    using Microsoft.Azure.Devices.Client.Transport.Mqtt;
    using Message = Microsoft.Azure.Devices.Client.Message;
    using TransportType = Microsoft.Azure.Devices.Client.TransportType;

    class Program
    {
        static void Main(string[] args)
        {
            // The Edge runtime gives us the connection string we need -- it is injected as an environment variable
            string connectionString = Environment.GetEnvironmentVariable("EdgeHubConnectionString");

            // Cert verification is not yet fully functional when using Windows OS for the container
            bool bypassCertVerification = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
            Init(connectionString, bypassCertVerification).Wait();

            // Wait until the app unloads or is cancelled
            var cts = new CancellationTokenSource();
            AssemblyLoadContext.Default.Unloading += (ctx) => cts.Cancel();
            Console.CancelKeyPress += (sender, cpe) => cts.Cancel();
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
        /// Initializes the DeviceClient and sets up the callback to receive
        /// messages containing temperature information
        /// </summary>
        static async Task Init(string connectionString, bool bypassCertVerification = false)
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
            Console.WriteLine("IoT Hub module client initialized.");

            ServiceClient serviceClient = ServiceClient.CreateFromConnectionString(connectionString);
            await ioTHubModuleClient.SetInputMessageHandlerAsync("input1", ReceiveMessage, serviceClient);

            Console.WriteLine("Analyzer module initialized.");
        }

        static async Task<MessageResponse> ReceiveMessage(Message message, object userContext)
        {
            var serviceClient = userContext as ServiceClient;
            if (serviceClient == null)
            {
                throw new InvalidOperationException("UserContext doesn't contain ServiceClient");
            }

            // Here we expect that the sender sends messages with a property "randVal" with values ranging between 0 and 100
            // If the value is > 70, we invoke a dummy method Poke
            byte[] messageBytes = message.GetBytes();
            if (message.MessageSchema == "engineData" &&
                double.TryParse(Encoding.UTF8.GetString(messageBytes), out double engineRpm))
            {
                Console.WriteLine($"Received message with Id {message.MessageId} with engine RPM {engineRpm}");
                if (engineRpm > 100)
                {
                    Console.WriteLine($"Engine RPM too high: {engineRpm}, resetting device");
                    var response = await serviceClient.InvokeDeviceMethodAsync(message.ConnectionDeviceId, message.ConnectionModuleId, new CloudToDeviceMethod("Reset"));
                    Console.WriteLine($"Got response status = {response.Status}");
                }
            }
            else
            {
                Console.WriteLine($"Received message with unknown schema or payload.");
            }
            return MessageResponse.Completed;
        }
    }
}
