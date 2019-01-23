using Newtonsoft.Json;
using System.ComponentModel;
using System.Net.Http;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using Xamarin.Forms;

namespace FanController
{
    public class MainPageViewModel : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        bool Set<T>(ref T field, T value, [CallerMemberName]string propertyName = null)
        {
            if (Equals(field, value))
                return false;

            field = value;
            RaisePropertyChanged(propertyName);
            return true;
        }

        void RaisePropertyChanged(string propertyName) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

        readonly Timer timer;
        HttpClient httpClient = new HttpClient();
        const string urlRoot = "<your url goes here>";
        static readonly string tempUrl = $"https://{urlRoot}/api/temperature/fan-controller";
        static readonly string tempThresholdUrl = $"https://{urlRoot}/api/temp-threshold/fan-controller";

        async Task TimerCallback()
        {
            var value = await httpClient.GetAsync(tempUrl);
            var content = await value.Content.ReadAsStringAsync();
            var temperatureItem = JsonConvert.DeserializeObject<TemperatureItem>(content);
            if (Set(ref temperature, temperatureItem.Temperature, nameof(Temperature)))
                RaisePropertyChanged(nameof(TemperatureColor));
        }

        public MainPageViewModel()
        {
            timer = new Timer(async o => await TimerCallback(), null, 0, 5000);
            SetThresholdCommand = new Command(async () => await UpdateThreshold());
        }

        private double temperature;

        public string Temperature
        {
            get => $"{temperature:N1}°C";
        }

        public Color TemperatureColor
        {
            get => temperature > TemperatureThreshold ? Color.Red : Color.DarkBlue;
        }

        private int temperatureThreshold = 20;
        public int TemperatureThreshold
        {
            get => temperatureThreshold;
            set
            {
                if (Set(ref temperatureThreshold, value))
                {
                    RaisePropertyChanged(nameof(TemperatureThresholdText));
                    RaisePropertyChanged(nameof(TemperatureColor));
                }
            }
        }

        async Task UpdateThreshold()
        {
            var data = new
            {
                threshold = temperatureThreshold
            };
            var serialized = JsonConvert.SerializeObject(data);
            var content = new StringContent(serialized, Encoding.ASCII, "application/json");
            await httpClient.PostAsync(tempThresholdUrl, content);
        }

        public string TemperatureThresholdText
        {
            get => $"{temperatureThreshold:N1}°C";
        }

        public ICommand SetThresholdCommand { get; }
    }
}
