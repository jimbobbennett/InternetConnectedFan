using Newtonsoft.Json;

namespace FanController
{
    public class TemperatureItem
    {
        public string PartitionKey {get; set;}

        [JsonProperty("id")]
        public string Id {get; set;}
        public double Temperature {get; set;}
    }
}