#include "Secret.h"

//Include the required libraries
#include <ESP8266WiFi.h>
#include <ArduinoHA.h>
#include "DHT.h"

#include "SerialCom.h"
#include "Types.h"

particleSensorState_t state;

//Define DHT sensor pin and type
#define DHTPIN 13
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Define variables
unsigned long lastTemperatureSend = millis();
bool lastInputState = false;

float temperatureValue;
float humidityValue;
float signalstrengthValue;

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
HASensor sensorAirquality("PM25");
HASensor sensorWindspeed("Wind_speed");
HASensor sensorSignalstrength("Signal_strength");
HASensor sensorBattery("Battery_power");

void setup() {
    Serial.begin(9600);
    Serial.println("Starting...");

    SerialCom::setup();

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
    String pm25NameStr = student_id + " Air Quality";
    String windspeedNameStr = student_id + " Wind speed";
    String signalstrengthNameStr = student_id + " Signal Strength";
    String batteryNameStr = student_id + " Battery Power";
    
    //Convert the strings to const char*
    const char* stationName = stationNameStr.c_str();
    const char* ownerName = ownerNameStr.c_str();
    const char* longName = longNameStr.c_str();
    const char* latName = latNameStr.c_str();
    const char* temperatureName = temperatureNameStr.c_str();
    const char* humidityName = humidityNameStr.c_str();
    const char* pm25Name = pm25NameStr.c_str();
    const char* windspeedName = windspeedNameStr.c_str();
    const char* signalstrengthName = signalstrengthNameStr.c_str();
    const char* batteryName = batteryNameStr.c_str();

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

    sensorAirquality.setName(pm25Name);
    sensorAirquality.setDeviceClass("pm25");
    sensorAirquality.setUnitOfMeasurement("μg/m³");

    sensorWindspeed.setName(windspeedName);
    sensorWindspeed.setIcon("mdi:weather-windy");
    sensorWindspeed.setUnitOfMeasurement("km/h");
    
    sensorSignalstrength.setName(signalstrengthName);
    sensorSignalstrength.setDeviceClass("signal_strength");
    sensorSignalstrength.setUnitOfMeasurement("dBm");

    sensorBattery.setName(batteryName);
    sensorBattery.setDeviceClass("battery");
    sensorBattery.setUnitOfMeasurement("%");

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

}

void loop() {
    mqtt.loop();

    SerialCom::handleUart(state);

    humidityValue = dht.readHumidity();
    temperatureValue = dht.readTemperature();
    signalstrengthValue = WiFi.RSSI();

    if (isnan(temperatureValue)) {
      signalstrengthValue = 0;
    }
  
    if (isnan(temperatureValue)) {
      temperatureValue = 0;
    }

    if (((millis() - lastTemperatureSend) > 10000) && (state.measurements[4] != 0)) { // read in 10s interval

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

        sensorAirquality.setValue(state.avgPM25);
        Serial.print("Current air quality is: ");
        Serial.print(state.avgPM25);
        Serial.println("µg/m3");

        sensorBattery.setValue(96);
        Serial.print("Current battery level is: ");
        Serial.print(96);
        Serial.println("%");

        sensorWindspeed.setValue(6);
        Serial.print("Current wind speed is: ");
        Serial.print(6);
        Serial.println("km/h");
      
        lastTemperatureSend = millis();
    
        Serial.println("Going to sleep... zzzzzz...");
        ESP.deepSleep(10 * 60 * 1000 * 1000);

    }
    
}
