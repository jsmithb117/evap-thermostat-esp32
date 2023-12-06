#ifndef CONSTANTS_H
#define CONSTANTS_H
// Display client
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

struct State {
  float setTempF;
  float tempF;
  float humidity;
  float pressure;
  int rawSignalStrength;
  int mappedSignalStrength;
  bool lowFanIsOn;
  bool highFanIsOn;
  bool pumpIsOn;
  bool lowFanIsSelected;
  bool pumpIsSelected;
};



// pin assignments
const int POT_PIN = 36;
const int PUMP_PIN = 14;
const int LOW_FAN_PIN = 12;
const int HIGH_FAN_PIN = 13;
const int PUMP_SELECT_PIN = 19;
const int FAN_SELECT_PIN = 18;

// topic schema:
// $building/$room/$domain/$device_name/$topic_class/$data_type/$unit_of_measure(optional)

// Topics this device can publish to:
const char *SET_TEMP_TOPIC = "home/living/hvac/evap-thermostat/input/set-temp/fahrenheit";
const char *TEMP_TOPIC = "home/living/hvac/evap-thermostat/sensor/temp/fahrenheit";
const char *HUMIDITY_TOPIC = "home/living/hvac/evap-thermostat/sensor/humidity/rh";
const char *SIGNAL_STRENGTH_TOPIC = "home/living/hvac/evap-thermostat/sensor/wifi-signal-strength/dbm";
const char *STATUS_TOPIC = "home/living/hvac/evap-thermostat/device/logged-in/boolean";
const char *PRESSURE_TOPIC = "home/living/hvac/evap-thermostat/sensor/pressure/hpa";

// Topics this device subscribes to:
const char *UPDATE_SET_TEMP = "home/living/hvac/evap-thermostat/controller/set-temp/fahrenheit";
// TODO add capabilies list to be published on login
// capabilites will explain the topics this device is capable of pub/sub
const char *SUBSCRIBE_TOPICS[] = {
  UPDATE_SET_TEMP,
};

// Optimized WiFi bitmaps
const unsigned char BITMAP_WIFI1[] PROGMEM = {
    0x90, 0x00, 0x90
  };
const unsigned char BITMAP_WIFI2[] PROGMEM = {
    0xc3, 0x00, 0xbd
  };
const unsigned char BITMAP_WIFI3[] PROGMEM = {
    0xe0, 0x70, 0x80, 0x10, 0x1f, 0x80, 0xbf, 0xd0};
const unsigned char BITMAP_WIFI4[] PROGMEM = {
    0xf0, 0x0f, 0xc0, 0x03, 0x8f, 0xf1, 0x3f, 0xfc
  };
// WiFi Signal Strength Indicator constants
const int WIDEST_BITMAP = 8;
const int WIDE1 = 4;
const int WIDE2 = 8;
const int WIDE3 = 12;
const int WIDE4 = 16;
const int HEIGHT1 = 3;
const int HEIGHT2 = 3;
const int HEIGHT3 = 4;
const int HEIGHT4 = 4;

// other display constants
const int L_X = 0; // X coordinate of the "L" low fan indicator
const int H_X = 25; // X coordinate of the "H" high fan indicator
const int P_X = 50; // X coordinate of the "P" pump indicator
const int LETTER_Y = 40; // Y coordinate of the indicators
const bool DELETE = true;

// HVAC constants
const int HIGH_HUMIDITY_THRESHOLD = 70;

struct DateTime {
  int year;
  int month;
  int dayOfMonth;
  int dayOfWeek;
  int hour;
  int minute;
  int second;
};

#endif