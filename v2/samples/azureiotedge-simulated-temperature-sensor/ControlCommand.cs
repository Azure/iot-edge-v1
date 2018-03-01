// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Newtonsoft.Json;

namespace AzureIotEdgeSimulatedTemperatureSensor
{
    public enum ControlCommandEnum
    {
         Reset = 0,
         Noop = 1
    };

    public class ControlCommand
    {
        [JsonProperty("command")]
        public ControlCommandEnum Command { get; set; }
    }
}

