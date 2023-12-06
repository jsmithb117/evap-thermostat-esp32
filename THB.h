#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

class THB {
private:
  Adafruit_AHTX0 aht20;
  Adafruit_BMP280 bmp280;
  float hum;
  float temp;
public:
  THB() {}

  void init(float* temperatureF, float* humidity, float* pressure, float* setTempF) {
    // Initialize AHT20 sensor
    if (!aht20.begin()) {
      Serial.println("Could not find AHT20 sensor, check wiring!");
      while (1);
    }

    // Initialize BMP280 sensor
    if (!bmp280.begin()) {
      Serial.println("Could not find BMP280 sensor, check wiring!");
      while (1);
    }
    readAll(temperatureF, humidity, pressure, setTempF);
  }

  // float readTemperatureC() {
  //   // Read temperature in Celsius from AHT20 sensor
  //   return aht20.readTemperature();
  // }

  float readTemperatureF() {
    sensors_event_t humidity, temperature;
    aht20.getEvent(&humidity, &temperature); // populate local objects with fresh data
    // update class state if it differs from local object by more than .1
    if (abs(temperature.temperature - temp) > .1) {
      temp = temperature.temperature * 9 / 5 + 32; // convert to fahrenheit
    }
    return round(temp * 10) / 10; // return rounded class state regardless of update
  }

  int readHumidity() {
    // Read humidity from AHT20 sensor
    sensors_event_t humidity, temperature;
    aht20.getEvent(&humidity, &temperature);// populate temp and humidity objects with fresh data
    return round(humidity.relative_humidity * 10) / 10; // round to 1 decimal place
  }

  float readPressure() {
    // Read pressure from BMP280 sensor
    return round(bmp280.readPressure() / 100.0F * 10) / 10;
  }

  float readSetTempF() {
    // Read set temperature from potentiometer
    float mappedSetTemp = map(analogRead(POT_PIN), 0, 4095, 650, 850) / 10.0F; // map potentiometer value to 65.0-85.0
    return mappedSetTemp;
  }

  void readAll(float* temperatureF, float* humidity, float* pressure, float* setTempF) {
    // Read all sensor data
    // temperatureC = readTemperatureC();
    sensors_event_t hum, temp;
    aht20.getEvent(&hum, &temp);// populate temp and humidity objects with fresh data

    *temperatureF = readTemperatureF();
    *humidity = readHumidity();
    *pressure = readPressure();
    *setTempF = readSetTempF();
  }
};