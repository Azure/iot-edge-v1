// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using System.IO;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Azure.Devices.Client;
using Microsoft.Azure.Devices.Client.Transport.Mqtt;
using Microsoft.Azure.Devices.Shared;
using Newtonsoft.Json;
using System.Net;
using AzureIotEdgeSimulatedTemperatureSensor.Simulation;

// disabling async warning as the SendSimulationData is an async method
// but we don't wait for it
#pragma warning disable CS4014

namespace AzureIotEdgeSimulatedTemperatureSensor
{
    class Program
    {
        private static int counter;

        private static volatile DesiredPropertiesData desiredPropertiesData;
        private static DataGenerationPolicy generationPolicy = new DataGenerationPolicy();

        private static volatile bool IsReset = false;

        static async Task Main(string[] args)
        {
            await Init();

            // Wait until the app unloads or is cancelled
            var cts = new CancellationTokenSource();
            AssemblyLoadContext.Default.Unloading += (ctx) => cts.Cancel();
            Console.CancelKeyPress += (sender, cpe) => cts.Cancel();
            await WhenCancelled(cts.Token);
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
        /// Initializes the ModuleClient and sets up the callback to receive
        /// messages containing temperature information
        /// </summary>
        static async Task Init()
        {
            MqttTransportSettings mqttSetting = new MqttTransportSettings(TransportType.Mqtt_Tcp_Only);
            ITransportSettings[] settings = { mqttSetting };

            // Open a connection to the Edge runtime
            var ioTHubModuleClient = await ModuleClient.CreateFromEnvironmentAsync(settings);
            ioTHubModuleClient.OpenAsync();
            Console.WriteLine("IoT Hub module client initialized.");

            // initialize module twin properties
            var moduleTwin = await ioTHubModuleClient.GetTwinAsync();
            var moduleTwinCollection = moduleTwin.Properties.Desired;
            desiredPropertiesData = new DesiredPropertiesData(moduleTwinCollection);

            // callback for updating desired properties through the portal or rest api
            await ioTHubModuleClient.SetDesiredPropertyUpdateCallbackAsync(OnDesiredPropertiesUpdate, null);

            // this direct method will allow to reset the temperature sensor values back to their initial state
            await ioTHubModuleClient.SetMethodHandlerAsync("reset", ResetMethod, null);

            // we don't pass ioTHubModuleClient as we're not sending any messages out to the message bus
            await ioTHubModuleClient.SetInputMessageHandlerAsync("control", ControlMessageHandler, null);

            // as this runs in a loop we do not await
            SendSimulationData(ioTHubModuleClient);
        }

        private static async Task SendSimulationData(ModuleClient moduleClient)
        {
            while(true)
            {
                try
                {
                    if(desiredPropertiesData.SendData)
                    {
                        counter++;
                        if(counter == 1)
                        {
                            // first time execution needs to reset the data factory
                            IsReset = true;
                        }

                        var messageBody = TemperatureDataFactory.CreateTemperatureData(counter, generationPolicy, IsReset);
                        IsReset = false;
                        var messageString = JsonConvert.SerializeObject(messageBody);
                        var messageBytes = Encoding.UTF8.GetBytes(messageString);
                        var message = new Message(messageBytes);
                        message.ContentEncoding = "utf-8"; 
                        message.ContentType = "application/json"; 

                        await moduleClient.SendEventAsync("temperatureOutput", message);
                        Console.WriteLine($"\t{DateTime.UtcNow.ToShortDateString()} {DateTime.UtcNow.ToLongTimeString()}> Sending message: {counter}, Body: {messageString}");

                    }
                    await Task.Delay(TimeSpan.FromSeconds(desiredPropertiesData.SendInterval));
                }
                catch(Exception ex)
                {
                    Console.WriteLine($"[ERROR] Unexpected Exception {ex.Message}" );
                    Console.WriteLine($"\t{ex.ToString()}");
                }
            }

        }

        private static Task OnDesiredPropertiesUpdate(TwinCollection twinCollection, object userContext)
        {
            desiredPropertiesData = new DesiredPropertiesData(twinCollection);
            return Task.CompletedTask;
        }


        private static Task<MethodResponse> ResetMethod(MethodRequest request, object userContext)
        {
            var response = new MethodResponse((int) HttpStatusCode.OK);
            Console.WriteLine("Received reset command via direct method invocation");
            Console.WriteLine("Resetting temperature sensor...");
            IsReset = true;
            return Task.FromResult(response);
        }

        private static Task<MessageResponse> ControlMessageHandler(Message message, object userContext)
        {
            var messageBytes = message.GetBytes();
            var messageString = Encoding.UTF8.GetString(messageBytes);

            Console.WriteLine($"Received message Body: [{messageString}]");

            try
            {
                var messages = JsonConvert.DeserializeObject<ControlCommand[]>(messageString);
                foreach (ControlCommand messageBody in messages)
                {
                    if (messageBody.Command == ControlCommandEnum.Reset)
                    {
                        Console.WriteLine("Resetting temperature sensor..");
                        IsReset = true;
                    }
                    else
                    {
                        //NoOp
                        Console.WriteLine("Received NOOP message");
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Failed to deserialize control command with exception: [{ex.Message}]");
            }

            return Task.FromResult(MessageResponse.Completed);
        }
    }
}
