
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID   "Your_Template_ID"


#define DHT_TYPE DHT11

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <Esp.h>
#include <time.h>
#include <TimeLib.h> //https://github.com/PaulStoffregen/Time


#define I2C_SDA 25
#define I2C_SCL 26
#define DHT_PIN 16
#define BAT_ADC 33
#define SALT_PIN 34
#define SOIL_PIN 32
#define BOOT_PIN 0
#define POWER_CTRL 4
#define USER_BUTTON 35
#define DS18B20_PIN 21

#define soil_max 1638
#define soil_min 3285

#define NIGHT_LIGHT_THRESOLD 5
#define LOW_SOIL_MOISTURE_THRESOLD 15
#define HIGH_SOIL_MOISTURE_THRESOLD 95
#define HIGH_TEMP_THRESOLD 45
#define LOW_TEMP_THRESOLD 15

bool dht_found;
float luxRead;
float advice;
float soil;

BH1750 lightMeter(0x23); //0x23
DHT dht(DHT_PIN, DHT_TYPE);

BlynkTimer timer;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "You_Auth_Token";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "SSID";
char pass[] = "PASS";




void setup()
{
  // Debug console
  Serial.begin(115200);

  dht.begin();

  

  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, 1);
  delay(1000);

  bool wireOk = Wire.begin(I2C_SDA, I2C_SCL); // wire can not be initialized at beginng, the bus is busy
  if (wireOk)
  {
    Serial.println(F("Wire ok"));
  }
  else
  {
    Serial.println(F("Wire NOK"));
  }
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    Serial.println(F("BH1750 Advanced begin"));
  }
  else
  {
    Serial.println(F("Error initialising BH1750"));
  }

  // Battery status, and charging status and days.
  float bat = readBattery();
  Serial.println("Battery level");
  Serial.println(bat);

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, send_data);

}

void loop()
{
  Blynk.run();
  timer.run();
}


void send_data()
{

  luxRead = lightMeter.readLightLevel();
  Serial.print("Lux - "); Serial.println(luxRead);
  Blynk.virtualWrite(V2, luxRead);

  uint16_t soil = readSoil();
  Serial.print("Soil Moisture - "); Serial.println(soil);
  Blynk.virtualWrite(V3, soil);

  uint32_t salt = readSalt();
  String advice;
  if (salt < 201)
  {
    advice = "needed";
  }
  else if (salt < 251)
  {
    advice = "low";
  }
  else if (salt < 351)
  {
    advice = "optimal";
  }
  else if (salt > 350)
  {
    advice = "too high";
  }
  Serial.print("Salt Advice - "); Serial.println(advice);
  Blynk.virtualWrite(V4, advice);

  float t = dht.readTemperature(); // Read temperature as Fahrenheit then dht.readTemperature(true)
  Serial.print("Temperature - "); Serial.println(t);
  Blynk.virtualWrite(V0, t);

  float h = dht.readHumidity();
  Serial.print("Humidity - "); Serial.println(h);
  Blynk.virtualWrite(V1, h);

  float batt = readBattery();
  Serial.print("Battery - "); Serial.println(batt);
  Blynk.virtualWrite(V5, batt);

  // Display Respected Image based on Sensor's Data

  if (luxRead <= NIGHT_LIGHT_THRESOLD)
  {
    Blynk.virtualWrite(V6, "1"); // Sleepy
    Serial.println("Feeling Sleepy");
  }
  else if ( soil <= LOW_SOIL_MOISTURE_THRESOLD)
  {
    Blynk.virtualWrite(V6, "2"); // Thirsty
    Blynk.logEvent("low_moisture");
    Serial.println("Feeling Thirsty");
  }
  else if (soil >= HIGH_SOIL_MOISTURE_THRESOLD )
  {
    Blynk.virtualWrite(V6, "3"); // Over Water
    Serial.println("Over Water");
  }
  else if ( t >= HIGH_TEMP_THRESOLD)
  {
    Blynk.virtualWrite(V6, "4"); // High Temperature
    Blynk.logEvent("high_temperature");
    Serial.println("Feeling Hot");
  }
  else if (t <= LOW_TEMP_THRESOLD)
  {
    Blynk.virtualWrite(V6, "5"); // Low Temperature
    Blynk.logEvent("low_temperature");
    Serial.println("Feeling Cold");
  }
  else
  {
    Blynk.virtualWrite(V6, "0"); // Happy
    Serial.println("Feeling Happy");
  }
}

// READ Soil
uint16_t readSoil()
{
  Serial.println(soil_max);
  uint16_t soil = analogRead(SOIL_PIN);
  Serial.print("Soil before map: ");
  Serial.println(soil);
  return map(soil, soil_min, soil_max, 0, 100);
}

// READ Battery
float readBattery()
{
  int vref = 1100;
  uint16_t volt = analogRead(BAT_ADC);
  Serial.print("Volt direct ");
  Serial.println(volt);
  float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;
  Serial.print("Battery Voltage: ");
  Serial.println(battery_voltage);
  battery_voltage = battery_voltage * 100;
  return map(battery_voltage, 416, 290, 100, 0);
}

// READ Salt
// I am not quite sure how to read and use this number. I know that when put in water wich a DH value of 26, it gives a high number, but what it is and how to use ??????
uint32_t readSalt()
{
  uint8_t samples = 120;
  uint32_t humi = 0;
  uint16_t array[120];

  for (int i = 0; i < samples; i++)
  {
    array[i] = analogRead(SALT_PIN);
    //  Serial.print("Read salt pin : ");

    //  Serial.println(array[i]);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 0; i < samples; i++)
  {
    if (i == 0 || i == samples - 1)
      continue;
    humi += array[i];
  }
  humi /= samples - 2;
  return humi;
}
