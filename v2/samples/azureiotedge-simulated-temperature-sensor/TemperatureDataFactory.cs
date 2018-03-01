// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;

namespace AzureIotEdgeSimulatedTemperatureSensor
{
    public class TemperatureDataFactory
    {
        private static readonly Random rand = new Random();
        private static double CurrentMachineTemperature;

        public static MessageBody CreateTemperatureData(int counter, DataGenerationPolicy policy, bool reset = false)
        {
            if(reset)
            {
                TemperatureDataFactory.CurrentMachineTemperature = policy.CalculateMachineTemperature();
            }
            else
            {
                TemperatureDataFactory.CurrentMachineTemperature =
                    policy.CalculateMachineTemperature(TemperatureDataFactory.CurrentMachineTemperature);
            }

            var machinePressure = policy.CalculatePressure(TemperatureDataFactory.CurrentMachineTemperature);
            var ambientTemperature = policy.CalculateAmbientTemperature();
            var ambientHumidity = policy.CalculateHumidity();

            var messageBody = new MessageBody
            {
                Machine = new Machine
                {
                    Temperature = TemperatureDataFactory.CurrentMachineTemperature,
                    Pressure =  machinePressure
                },
                Ambient = new Ambient
                {
                    Temperature = ambientTemperature,
                    Humidity = ambientHumidity
                },
                TimeCreated = string.Format("{0:O}", DateTime.Now)
            };

            return messageBody;
        }
    }
}
