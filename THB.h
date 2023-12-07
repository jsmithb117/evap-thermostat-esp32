#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

class THB {
private:
  Adafruit_AHTX0 aht20;
  Adafruit_BMP280 bmp280;
  float hum;
  float temp;
  float setTemp;
public:
  THB() {}

  void init(float* temperatureF, float* humidity, float* pressure, float* setTempF) {
    Serial.println("Initializing THB");
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
    bool isOverride = false;
    readAll(temperatureF, humidity, pressure, setTempF, &isOverride);
    setTemp = *setTempF;
  }

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

  float readSetTempF(bool* isOverride) {
    // Read set temperature from potentiometer
    float mappedSetTemp = map(analogRead(POT_PIN), 0, 4095, 650, 850) / 10.0F; // map potentiometer value to 65.0-85.0

    // when server issues an UPDATE_SET_TEMP command, isOverride prevents the potentiometer from updating setTemp
    // User removes the override by turning the potentiometer to its maximum value then setting the desired setTemp
    if (*isOverride && mappedSetTemp != 85.0F) {
      return setTemp;
    }
    *isOverride = false;
    setTemp = mappedSetTemp;
    return mappedSetTemp;
  }

  void readAll(float* temperatureF, float* humidity, float* pressure, float* setTempF, bool* isOverride) {
    // Read all sensor data

    // if not overriden, update setTemp
    if (isOverride) {
      setTemp = *setTempF;
    }
    *temperatureF = readTemperatureF();
    *humidity = readHumidity();
    *pressure = readPressure();
    *setTempF = readSetTempF(isOverride);
  }
};