#include "Secret.h"

//Include the required libraries
#include <ESP8266WiFi.h>
#include <ArduinoHA.h>
#include "DHT.h"

//Define DHT sensor pin and type
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Define Rain sensor pin
#define rainDigital 2

//Define variables
unsigned long lastTemperatureSend = millis();
bool lastInputState = false;

float temperatureValue;
float humidityValue;
float signalstrengthValue;

int rainDigitalVal = 0;
int rainVal;

int rpm = 0;

int analogInput = A0;
float Vout = 0.00;
float Vin = 0.00;
float R1 = 50.00; // resistance of R1 (10K) 
float R2 = 100.00; // resistance of R2 (1K) 
int val = 0;
int Percentage = 0; 

//Initialize WiFi
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

//Define the sensors and/or devices
//The string must not contain any spaces!!! Otherwise the sensor will not show up in Home Assistant
HASensor sensorOwner("Owner");
HASensor sensorLong("Long");
HASensor sensorLat("Lat");
HASensor sensorTemperature("Temperature");
HASensor sensorHumidity("Humidity");
HASensor sensorWindspeed("Wind_speed");
HASensor sensorSignalstrength("Signal_strength");
HASensor sensorBattery("Battery_power");
HABinarySensor sensorRain("Rain", "moisture", true);

volatile unsigned long lastWindRotation = millis();
volatile int currentRotationIndex = 0;
volatile int timeBetweenRotations [10] = {};

void IRAM_ATTR processWindRotation() {
  timeBetweenRotations[currentRotationIndex] = millis() - lastWindRotation;
  currentRotationIndex++;
  if (currentRotationIndex > 10)
  {
    currentRotationIndex = 0;
  }
  lastWindRotation = millis();
}

void setup() {
    Serial.begin(9600);
    Serial.println("Starting...");

    // Unique ID must be set!
    byte mac[WL_MAC_ADDR_LENGTH];
    WiFi.macAddress(mac);
    device.setUniqueId(mac, sizeof(mac));

    // Connect to wifi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500); // waiting for the connection
    }
    Serial.println();
    Serial.println("Connected to the network");

    // Set sensor and/or device names
    // String conversion for incoming data from Secret.h
    String student_id = STUDENT_ID;
    String student_name = STUDENT_NAME;

    //Add student ID number with sensor name
    String stationNameStr = student_name + "'s Home Station";
    String ownerNameStr = student_id + " Station owner";
    String longNameStr = student_id + " Long";
    String latNameStr = student_id + " Lat";
    String temperatureNameStr = student_id + " Temperature";
    String humidityNameStr = student_id + " Humidity";
    String windspeedNameStr = student_id + " Wind speed";
    String signalstrengthNameStr = student_id + " Signal Strength";
    String batteryNameStr = student_id + " Battery Power";
    String rainNameStr = student_id + " Rain";
    
    //Convert the strings to const char*
    const char* stationName = stationNameStr.c_str();
    const char* ownerName = ownerNameStr.c_str();
    const char* longName = longNameStr.c_str();
    const char* latName = latNameStr.c_str();
    const char* temperatureName = temperatureNameStr.c_str();
    const char* humidityName = humidityNameStr.c_str();
    const char* windspeedName = windspeedNameStr.c_str();
    const char* signalstrengthName = signalstrengthNameStr.c_str();
    const char* batteryName = batteryNameStr.c_str();
    const char* rainName = rainNameStr.c_str();


    //Set main device name
    device.setName(stationName);
    device.setSoftwareVersion(SOFTWARE_VERSION);
    device.setManufacturer(STUDENT_NAME);
    device.setModel(MODEL_TYPE);

    sensorOwner.setName(ownerName);
    sensorOwner.setIcon("mdi:account");

    sensorLong.setName(longName);
    sensorLong.setIcon("mdi:crosshairs-gps");
    sensorLat.setName(latName);
    sensorLat.setIcon("mdi:crosshairs-gps");
    
    sensorTemperature.setName(temperatureName);
    sensorTemperature.setDeviceClass("temperature");
    sensorTemperature.setUnitOfMeasurement("°C");

    sensorHumidity.setName(humidityName);
    sensorHumidity.setDeviceClass("humidity");
    sensorHumidity.setUnitOfMeasurement("%");

    sensorWindspeed.setName(windspeedName);
    sensorWindspeed.setIcon("mdi:weather-windy");
    sensorWindspeed.setUnitOfMeasurement("km/h");
    
    sensorSignalstrength.setName(signalstrengthName);
    sensorSignalstrength.setDeviceClass("signal_strength");
    sensorSignalstrength.setUnitOfMeasurement("dBm");

    sensorBattery.setName(batteryName);
    sensorBattery.setDeviceClass("battery");
    sensorBattery.setUnitOfMeasurement("%");

    sensorRain.setName(rainName);

    mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);

    while (!mqtt.isConnected()) {
        mqtt.loop();
        Serial.print(".");
        delay(500); // waiting for the connection
    }
    
    Serial.println();
    Serial.println("Connected to MQTT broker");

    sensorOwner.setValue(STUDENT_NAME);

    sensorLat.setValue(LAT, (uint8_t)15U);
    sensorLong.setValue(LONG, (uint8_t)15U);
    
    dht.begin();
    attachInterrupt(digitalPinToInterrupt(13), processWindRotation, FALLING);
    pinMode(rainDigital,INPUT);

}

void loop() {
    mqtt.loop();

    val = analogRead(analogInput);//reads the analog input
    delay(3);
    Vout = (val * 3.30) / 1024.00; // formula for calculating voltage out i.e. V+, here 5.00
    Vin = Vout / (R2/(R1+R2)); // formula for calculating voltage in i.e. GND

    if (Vin < 2.75 ) {
       Percentage = 0;
    } else {
       Percentage = 68.96552 * Vin - 189.6552;
    }

    rainDigitalVal = digitalRead(rainDigital);

    if (rainDigitalVal == 1) {
      rainVal = 0;
    } else {
      rainVal = 1;
    }

    humidityValue = dht.readHumidity();
    temperatureValue = dht.readTemperature();
    signalstrengthValue = WiFi.RSSI();

    if (isnan(humidityValue)) {
      humidityValue = 0;
    }
  
    if (isnan(temperatureValue)) {
      temperatureValue = 0;
    }

    int totalMs = 0;
    for(int i = 0; i < 10; i++)
    {
      totalMs += timeBetweenRotations[i];
    }

    if ((millis() - lastWindRotation) < 5000) {
      int averageMs = totalMs / 10;
      rpm = (1.0 / (float) averageMs * 1000 * 60); // Print rpm
    } else {
      rpm = 0;
    }

    float kmh = 138961.3 + (0.0002109906 - 138961.3)/(1.0 + pow(rpm/355865700.0, 0.6213368));

    if ((millis() - lastTemperatureSend) > 20000) { // read in 30ms interval

        sensorTemperature.setValue(temperatureValue);
        Serial.print("Current temperature is: ");
        Serial.print(temperatureValue);
        Serial.println("°C");
    
        sensorHumidity.setValue(humidityValue);
        Serial.print("Current humidity is: ");
        Serial.print(humidityValue);
        Serial.println("%");
    
        sensorSignalstrength.setValue(signalstrengthValue);
        Serial.print("Current signal strength is: ");
        Serial.print(signalstrengthValue);
        Serial.println("dBm");

       sensorBattery.setValue(Percentage);
       Serial.print("Current battery level is: ");
       Serial.print(Percentage);
       Serial.println("%");

        sensorWindspeed.setValue(kmh);
        Serial.print("Current wind speed is: ");
        Serial.print(kmh);
        Serial.print(rpm);
        Serial.println("km/h");

        sensorRain.setState(rainVal);
        Serial.print("Current rain is: ");
        Serial.println(rainVal);
      
        lastTemperatureSend = millis();
    
//        Serial.println("Going to sleep... zzzzzz...");
//        ESP.deepSleep(10 * 60 * 1000 * 1000);

    }
    
}
