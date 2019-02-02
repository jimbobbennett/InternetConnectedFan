#include "Arduino.h"
#include "AZ3166WiFi.h"
#include "DevKitMQTTClient.h"
#include "SystemVersion.h"
#include "Sensor.h"
#include "parson.h"
#include "PinNames.h"

static bool hasWifi = false;

static void initWifi()
{
  Screen.print(2, "Connecting...");

  if (WiFi.begin() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Screen.print(1, ip.get_address());
    hasWifi = true;
    Screen.print(2, "\r\n");
    Screen.print(3, "Running... \r\n");
  }
  else
  {
    hasWifi = false;
    Screen.print(1, "No Wi-Fi\r\n ");
  }
}

DevI2C *ext_i2c;
HTS221Sensor *ht_sensor;
RGB_LED rgbLed;

static float temperature = 50;
static int temperatureThreshold = 100;

void initSensor()
{
  ext_i2c = new DevI2C(D14, D15);
  ht_sensor = new HTS221Sensor(*ext_i2c);
  ht_sensor->init(NULL);
}

void getSensorData()
{
  try
  {
    ht_sensor->enable();
    ht_sensor->getTemperature(&temperature);
    ht_sensor->disable();
    ht_sensor->reset();

    char buff[128];
    sprintf(buff, "Temp:%sC\r\n", f2s(temperature, 1));
    Screen.print(1, buff);

    sprintf(buff, "Threshold:%sC\r\n", f2s(temperatureThreshold, 1));
    Screen.print(2, buff);
  }
  catch(int error)
  {
    Screen.print(1, "Sensor error\r\n ");
  }
}

void parseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message)
{
    JSON_Value *root_value;
    root_value = json_parse_string(message);

    if (json_value_get_type(root_value) != JSONObject)
    {
        if (root_value != NULL)
        {
            json_value_free(root_value);
        }
        LogError("parse %s failed", message);
        return;
    }

    JSON_Object *root_object = json_value_get_object(root_value);

    if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
    {
        JSON_Object *desired_object = json_object_get_object(root_object, "desired");
        if (desired_object != NULL)
        {
          if (json_object_has_value(desired_object, "temperatureThreshold"))
          {
            temperatureThreshold = json_object_get_number(desired_object, "temperatureThreshold");
          }
        }
    }
    else
    {
      if (json_object_has_value(root_object, "temperatureThreshold"))
      {
        temperatureThreshold = json_object_get_number(root_object, "temperatureThreshold");
      }
    }

    json_value_free(root_value);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  parseTwinMessage(updateState, temp);
  free(temp);
}

void setup()
{
  rgbLed.turnOff();
  Screen.init();
  Screen.print(0, "IoT Fan");
  Screen.print(2, "Initializing...");
  Screen.print(3, " > WiFi");

  hasWifi = false;
  initWifi();
  if (!hasWifi)
  {
    return;
  }

  initSensor();

  Screen.print(3, " > IoT Hub");
  DevKitMQTTClient_Init(true);
  DevKitMQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);

  Screen.print(3, " ");

  pinMode(PB_0, OUTPUT);
}

void sendData(const char *data)
{
  time_t t = time(NULL);
  char buf[sizeof "2011-10-08T07:07:09Z"];
  strftime(buf, sizeof buf, "%FT%TZ", gmtime(&t));

  EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(data, MESSAGE);

  DevKitMQTTClient_Event_AddProp(message, "$$CreationTimeUtc", buf);
  DevKitMQTTClient_Event_AddProp(message, "$$MessageSchema", "fan-sensors;v1");
  DevKitMQTTClient_Event_AddProp(message, "$$ContentType", "JSON");
  
  DevKitMQTTClient_SendEventInstance(message);
}

void loop()
{
  getSensorData();

  char sensorData[200];
  sprintf_s(sensorData, sizeof(sensorData), "{\"temperature\":%s,\"temperature_unit\":\"C\"}", f2s(temperature, 1));
  sendData(sensorData);

  if (temperature >= temperatureThreshold)
  {
    rgbLed.setColor(255, 0, 0);
    digitalWrite(PB_0, HIGH);
  }
  else
  {
    rgbLed.setColor(0, 0, 255);
    digitalWrite(PB_0, LOW);
  }

  delay(5000);
}
