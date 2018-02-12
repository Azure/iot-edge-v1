// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Newtonsoft.Json;

namespace AzureIotEdgeSimulatedTemperatureSensor
{
    public class MessageBody
    {
        [JsonProperty("machine")]
        public Machine Machine { get; set; }
        [JsonProperty("ambient")]
        public Ambient Ambient { get; set; }
        [JsonProperty("timeCreated")]
        public string TimeCreated { get; set; }
    }

    [JsonObject("machine")]
    public class Machine
    {
        [JsonProperty("temperature")]
        public double Temperature { get; set; }
        [JsonProperty("pressure")]
        public double Pressure { get; set; }
    }

    [JsonObject("ambient")]
    public class Ambient
    {
        [JsonProperty("temperature")]
        public double Temperature { get; set; }
        [JsonProperty("humidity")]
        public int Humidity { get; set; }
    }
}
