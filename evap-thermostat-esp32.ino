#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "DynamicQueue.h"
#include "constants.h"
#include "secrets.h"
#include "THB.h"
#include "RTCManager.h"

// TODO: render setTempOverride as a box around the setTemp?
// TODO: render pressure? With trend indicator (arrows?)
// TODO: render datetime?
// TODO: append datetime to each outgoing message?
WiFiClient espClient;
RTCManager rtcClient;
PubSubClient mqttClient(MQTT_SERVER, MQTT_PORT, espClient);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_ADDRESS);
THB thb;
DynamicQueue messageQueue; // stores mqtt pub-sub messages
State state; // stores device state

void renderPressure() {
  // render pressure to the right of the "P" and to the left of the pumpDelayTimer
  static float oldPressure;
  if (state.pressure != oldPressure) {
    oldPressure = state.pressure;
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(SCREEN_WIDTH - 65, SCREEN_HEIGHT - 10);
    display.println(state.pressure);
  }
}

void renderWifiSignalStrengthIndicator() {
  // Renders the WiFi signal strength indicator

  static int oldMappedSignalStrength;
  if (state.mappedSignalStrength != oldMappedSignalStrength) { // assign colors for signal strength indicator
    uint16_t segment1Color =
        state.mappedSignalStrength >= 1 ? SSD1306_WHITE : SSD1306_BLACK;
    uint16_t segment2Color =
        state.mappedSignalStrength >= 2 ? SSD1306_WHITE : SSD1306_BLACK;
    uint16_t segment3Color =
        state.mappedSignalStrength >= 3 ? SSD1306_WHITE : SSD1306_BLACK;
    uint16_t segment4Color =
        state.mappedSignalStrength >= 4 ? SSD1306_WHITE : SSD1306_BLACK;
    // render WiFi bitmap segments
    display.drawBitmap(SCREEN_WIDTH - (WIDEST_BITMAP) - (WIDE1 / 2),
                       HEIGHT1 + HEIGHT2 + HEIGHT3 + 1, BITMAP_WIFI1, WIDE1,
                       HEIGHT1, SSD1306_BLACK, segment1Color);
    display.drawBitmap(SCREEN_WIDTH - (WIDEST_BITMAP) - (WIDE2 / 2),
                       HEIGHT2 + HEIGHT3 + 1, BITMAP_WIFI2, WIDE2, HEIGHT2,
                       SSD1306_BLACK, segment2Color);
    display.drawBitmap(SCREEN_WIDTH - (WIDEST_BITMAP) - (WIDE3 / 2), HEIGHT3,
                       BITMAP_WIFI3, WIDE3, HEIGHT3, SSD1306_BLACK,
                       segment3Color);
    display.drawBitmap(SCREEN_WIDTH - (WIDEST_BITMAP) - (WIDE4 / 2), 0,
                       BITMAP_WIFI4, WIDE4, HEIGHT4, SSD1306_BLACK,
                       segment4Color);
    oldMappedSignalStrength = state.mappedSignalStrength;
  }
}

String formatTemp(float value) {
  // Formats the temperature string to be a consistent width
  String val = String(value, 1);
  const int width = 5;  // set the desired width
  while (val.length() < width) {
    val = " " + val;
  }
  return val;
}
String formatTimer(int value, int width) {
  String val = String(value);
  while (val.length() < width) {
    val = " " + val;
  }
  return val;
}

void renderTemp() {
  // Renders the temperature in the center of the display
  static float oldTempF;
  if (state.tempF != oldTempF) {
    oldTempF = state.tempF;
    display.setTextSize(3);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(0, 16);
    // format the temperature string to be a consistent width
    String renderTemp = formatTemp(state.tempF);
    display.println(renderTemp);
  }
}

void renderHumidity() {
  // Renders humidity in the upper left corner of the display
  static float oldHumidity;
  if (state.humidity != oldHumidity) {
    oldHumidity = state.humidity;
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(0, 0);
    String humidityStringInteger = String(state.humidity > 99 ? 99 : state.humidity, 0);
    display.println(humidityStringInteger + "%");
  }
}

void renderSetTemp() {
  // Renders setTemp in the top center of the display
  static float oldSetTemp;
  if (state.setTempF != oldSetTemp) {
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(35, 0);
    display.println(formatTemp(state.setTempF));
  }
}

void renderLetter(char letter,
                  int16_t x,
                  int16_t y,
                  bool deleteLetter = false) {
  // Renders a single letter at the specified x and y coordinates
  if (deleteLetter) {
    display.setTextColor(SSD1306_BLACK, SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  }
  display.setTextSize(2);
  display.setCursor(x, y);
  display.print(letter);
}

void renderBar(char character, bool deleteBar = false) {
  // Renders a bar under the specified character to denote if it is selected
  int barWidth = 10;  // adjust the width of the bar as needed
  int barHeight = 3;  // adjust the height of the bar as needed

  int16_t barX;
  int16_t barY = 56;

  uint16_t color = deleteBar ? SSD1306_BLACK : SSD1306_WHITE;

  if (character == 'L') {
    barX = 0;
  } else if (character == 'H') {
    barX = 25;
  } else if (character == 'P') {
    barX = 50;
  }

  display.fillRect(barX, barY, barWidth, barHeight, color);
}
void renderL(bool deleteLetter = false) {
  // Renders an L in the lower left corner of the display
  renderLetter('L', L_X, LETTER_Y, deleteLetter);
}

void renderH(bool deleteLetter = false) {
  // Renders an H in the lower center of the display
  renderLetter('H', H_X, LETTER_Y, deleteLetter);
}

void renderP(bool deleteLetter = false) {
  // Renders a P in the lower right corner of the display
  renderLetter('P', P_X, LETTER_Y, deleteLetter);
}
void renderAllMemoized(bool force = false) {
  static float lastTempF;
  static int lastHumidity;
  static float lastSetTemp;
  static bool lastLowFanIsOn;
  static bool lastHighFanIsOn;
  static bool lastPumpIsOn;
  static bool lastLowFanIsSelected;
  static bool lastPumpIsSelected;
  static bool lastSetTempOverride;
  static int lastMappedSignalStrength;
  static float lastPressure;
  // for each static variable
  //    if value is new
  //      publish (or enqueue) message
  //      render value to display
  //      set static variable to new value

  if (force || state.tempF != lastTempF) {
    static long lastTempPublishTime;

    renderTemp();
    lastTempF = state.tempF;

    if (millis() - lastTempPublishTime < 60000) { // ensure temp is published no more often than once per minute
      return;
    }

    if (!mqttClient.publish(TEMP_TOPIC, String(state.tempF).c_str())) {
      Serial.println("Failed to publish temp.  Adding message to queue.");
      messageQueue.enqueue(TEMP_TOPIC, String(state.tempF).c_str());
    }
    lastTempPublishTime = millis();
  }
  if (force || state.humidity != lastHumidity) {
    static long lastHumidityPublishTime;

    renderHumidity();
    lastHumidity = state.humidity;

    if (millis() - lastHumidityPublishTime < 60000) { // ensure humidity is published no more often than once per minute
      return;
    }
    if (!mqttClient.publish(HUMIDITY_TOPIC, String(state.humidity).c_str())) {
      Serial.println("Failed to publish humidity.  Adding message to queue.");
      messageQueue.enqueue(HUMIDITY_TOPIC, String(state.humidity).c_str());
    }
    lastHumidityPublishTime = millis();
  }
  if (force || state.setTempF != lastSetTemp) {
    static long lastSetTempPublishTime;

    renderSetTemp();
    lastSetTemp = state.setTempF;

    if (millis() - lastSetTempPublishTime < 60000) { // ensure setTemp is published no more often than once per minute
      return;
    }

    if (!mqttClient.publish(SET_TEMP_TOPIC, String(state.setTempF).c_str())) {
      Serial.println("Failed to publish setTemp.  Adding message to queue.");
      messageQueue.enqueue(SET_TEMP_TOPIC, String(state.setTempF).c_str());
    }

    lastSetTempPublishTime = millis();
  }
  if (force || state.lowFanIsOn != lastLowFanIsOn) {
    renderBar('L', !state.lowFanIsOn);
    lastLowFanIsOn = state.lowFanIsOn;
  }
  if (force || state.highFanIsOn != lastHighFanIsOn) {
    renderBar('H', !state.highFanIsOn);
    lastHighFanIsOn = state.highFanIsOn;
  }
  if (force || state.pumpIsOn != lastPumpIsOn) {
    renderBar('P', !state.pumpIsOn);
    lastPumpIsOn = state.pumpIsOn;
  }
  if (force || state.lowFanIsSelected != lastLowFanIsSelected) {
    renderL(!state.lowFanIsSelected);
    renderH(state.lowFanIsSelected);
    lastLowFanIsSelected = state.lowFanIsSelected;
  }
  if (force || state.pumpIsSelected != lastPumpIsSelected) {
    renderP(!state.pumpIsSelected);
    lastPumpIsSelected = state.pumpIsSelected;
  }
  if (force || state.mappedSignalStrength != lastMappedSignalStrength && state.mappedSignalStrength <= 4) {
    static long lastSignalStrengthPublishTime;

    renderWifiSignalStrengthIndicator();
    lastMappedSignalStrength = state.mappedSignalStrength;

    if (millis() - lastSignalStrengthPublishTime < 60000) { // ensure signalStrength is published no more often than once per minute
      return;
    }
    if (!mqttClient.publish(SIGNAL_STRENGTH_TOPIC,
                            String(state.rawSignalStrength).c_str())) {
      Serial.println(
          "Failed to publish signalStrength.  Adding message to queue.");
      messageQueue.enqueue(SIGNAL_STRENGTH_TOPIC,
                           String(state.rawSignalStrength).c_str());
    }
  }
  if (state.pressure != lastPressure) {
    static long lastPressurePublishTime;

    lastPressure = state.pressure;
    renderPressure();
    if (millis() - lastPressurePublishTime < 60000) { // ensure pressure is published no more often than once per minute
      return;
    }
    if (!mqttClient.publish(PRESSURE_TOPIC, String(state.pressure).c_str())) {
      Serial.println("Failed to publish pressure.  Adding message to queue.");
      messageQueue.enqueue(PRESSURE_TOPIC, String(state.pressure).c_str());
    }

    lastPressurePublishTime = millis();
  }
}

void thermostatInit() {
  pinMode(PUMP_SELECT_PIN, INPUT_PULLUP);
  pinMode(FAN_SELECT_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LOW_FAN_PIN, OUTPUT);
  pinMode(HIGH_FAN_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(LOW_FAN_PIN, LOW);
  digitalWrite(HIGH_FAN_PIN, LOW);
  state.setTempF = 72.0F;
  state.lowFanIsOn = false;
  state.highFanIsOn = false;
  state.pumpIsOn = false;
  state.lowFanIsSelected = true;
  state.pumpIsSelected = true;
  state.setTempOverride = false;
}

void turnLowFanOff() {
  digitalWrite(LOW_FAN_PIN, LOW);
  state.lowFanIsOn = false;
}
void turnHighFanOff() {
  digitalWrite(HIGH_FAN_PIN, LOW);
  state.highFanIsOn = false;
}
void turnLowFanOn() {
  turnHighFanOff(); // ensure only one fan is on at a time
  digitalWrite(LOW_FAN_PIN, HIGH);
  state.lowFanIsOn = true;
}
void turnHighFanOn() {
  turnLowFanOff(); // ensure only one fan is on at a time
  digitalWrite(HIGH_FAN_PIN, HIGH);
  state.highFanIsOn = true;
}
void turnFanOff() {
  turnLowFanOff();
  turnHighFanOff();
  renderFanDelay(-1); // clear fan delay
}
void turnFanOn() {
  // turns fan on if it's been at least 5 minutes since last fan on command
  static long lastFanOnTime = -300000; // set as 5 minutes prior to startup so fan can engage immediately
  renderFanDelay(-1); // clear fan delay

  if (millis() - lastFanOnTime < 300000) {
    if (!state.lowFanIsOn && !state.highFanIsOn) {
      renderFanDelay(300000 - (millis() - lastFanOnTime));
    }
    return;
  }
  lastFanOnTime = millis();

  // passed guards, turn fan on
  if (state.lowFanIsSelected) {
    turnLowFanOn();
  } else {
    turnHighFanOn();
  }
}
void renderFanDelay(int remainingMs) {
  // Renders the fan delay in the lower right corner of the display, just above the pump delay
  static int oldRemainingMs;

  if (remainingMs != oldRemainingMs) {
    oldRemainingMs = remainingMs;
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20);

    if (remainingMs > 0) {
      display.println(formatTimer(remainingMs / 1000, 3));
    } else {
      display.println("   ");
    }
  }
}
void turnPumpOff() {
  digitalWrite(PUMP_PIN, LOW);
  state.pumpIsOn = false;
  renderPumpDelay(-1); // clear pump delay
}
void renderPumpDelay(int remainingMs) {
  // Renders the pump delay in the lower right corner of the display, just below the fan delay
  static int oldRemainingMs;

  if (remainingMs != oldRemainingMs) {
    oldRemainingMs = remainingMs;
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(SCREEN_WIDTH - 20, SCREEN_HEIGHT - 10);

    if (remainingMs > 0) {
      display.println(formatTimer((remainingMs / 1000), 2));
    } else {
      display.println("  ");
    }
  }

}
void turnPumpOn() {
  static long lastPumpOnTime = -60000; // set as 1 minute prior to startup so pump can engage immediately
  renderPumpDelay(-1); // clear pump delay

  if (state.humidity >= HIGH_HUMIDITY_THRESHOLD || !state.pumpIsSelected) { // ensure it doesn't get too humid indoors
    turnPumpOff();
    return;
  }
  if (millis() - lastPumpOnTime < 60000) { // ensure pump turns on no more often than once per minute
    if (!state.pumpIsOn) {
      renderPumpDelay(60000 - (millis() - lastPumpOnTime));
    }
    return;
  }

  // passed guards, turn pump on
  if (state.pumpIsSelected && !state.pumpIsOn) {
    digitalWrite(PUMP_PIN, HIGH);
    state.pumpIsOn = true;
    lastPumpOnTime = millis();
  }
}
void turnCoolerOn() {
  turnFanOn();
  turnPumpOn();
}
void turnVentOn() {
  turnFanOn();
  turnPumpOff();
}
void turnCoolerOff() {
  turnFanOff();
  turnPumpOff();
}
void thermostatLoop() {
  // if it's hot, turn cool on
  if (state.tempF > state.setTempF + 2) {
    turnCoolerOn();
    return;
  }
  // if it's warm, turn vent on
  if (state.tempF >= state.setTempF + 1) {
    turnVentOn();
    return;
  }
  // if it's cold, turn cooler off
  if (state.tempF <= state.setTempF) {
    turnCoolerOff();
  }
}
void displayInit() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Display allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  display.clearDisplay();
  renderWifiSignalStrengthIndicator();
  displayLoop();
}
void displayLoop() {
  renderAllMemoized();
  display.display();
}
void wifiLoop() {
  // Update signalStrength vars and connect to WiFi if disconnected
  rssiLoop();
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, WIFI_PW);
  while (!WiFi.isConnected()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // handles subscribed mqtt messages
  Serial.print("Message arrived from topic: ");
  Serial.println(topic);
  if (strcmp(topic, UPDATE_SET_TEMP) == 0) {
    // convert payload to float and assign to setTempF
    char *temp = (char *)payload;
    float newSetTempF = atof(temp);


    // if newSetTempF is valid, set setTempOverride to true and assign newSetTempF to setTempF
    // else, set setTempOverride to false and assign setTempF to thb.readSetTempF()
    if (newSetTempF < 65.0 || newSetTempF > 85.0) {
      Serial.print("Invalid setTemp received from MQTT.  Using current setTemp: ");
      Serial.println(state.setTempF);
      state.setTempOverride = false;
      state.setTempF = thb.readSetTempF(&state.setTempOverride);
    } else {
      Serial.print("Valid setTemp received from MQTT.  Using new setTemp: ");
      Serial.println(newSetTempF);
      state.setTempOverride = true;
      state.setTempF = newSetTempF;
    }
    state.setTempF = newSetTempF;
    renderSetTemp();
    display.display();
  }
}
void mqttInit() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttConnect();
}
void mqttConnect() {
  Serial.println("Connecting to MQTT");

  static long lastMqttConnectAttempt = -5000; // set as 5 seconds prior to startup so mqtt can connect immediately
  const char* constClientId = finalClientId;

  // connect to mqtt, set last will and testament
  if (lastMqttConnectAttempt + 5000 < millis() && mqttClient.connect(constClientId, MQTT_USER, MQTT_PW, STATUS_TOPIC, 0, 1, "0")) {
    Serial.println("MQTT connected");
    lastMqttConnectAttempt = millis();
    // resubscribe to topics
    for (int i = 0; i < sizeof(SUBSCRIBE_TOPICS) / sizeof(SUBSCRIBE_TOPICS[0]); i++) {
      Serial.print("Subscribing to topic: ");
      Serial.println(SUBSCRIBE_TOPICS[i]);
      mqttClient.subscribe(SUBSCRIBE_TOPICS[i]);
    }
    // add status message to queue, signaling that this device is online
    messageQueue.enqueue(STATUS_TOPIC, "1");
    // publish messages from queue
    while (!messageQueue.isEmpty()) {
      Message message = messageQueue.dequeue();
      if (!mqttClient.publish(message.topic, message.payload)) {
        // publish failed, re-enqueue message and break to allow reconnect on next loop
        Serial.println("Failed to publish message from queue");
        messageQueue.enqueue(message.topic, message.payload);
        break;
      } else {
        Serial.println("Published a queue message");
      }
    }
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.println(mqttClient.state());
    Serial.println("Retrying in 5 seconds");
  }
}
void mqttLoop() {
  // Connect to MQTT if disconnected
  if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();
}
void rssiLoop() {
  // Update signal strengths
  state.rawSignalStrength = WiFi.RSSI(); // reported to mqtt when mapped value changes
  state.mappedSignalStrength = map(state.rawSignalStrength, -90, -40, 0, 4); // used to render signal strength indicator
}

void debouncedPumpSelectHandler() {
  // handler for pump select button
  bool pumpSelectInput = !digitalRead(PUMP_SELECT_PIN); // invert input because INPUT_PULLUP
  static bool enableStateChange = false;
  if (pumpSelectInput && !enableStateChange) { // button pressed, enable state change
    enableStateChange = true;
  }
  if (!pumpSelectInput && enableStateChange) { // set/reset state on falling signal to prevent multiple state changes per button press
    state.pumpIsSelected = !state.pumpIsSelected;
    enableStateChange = false;
  }
}
void debouncedFanSelectHandler() {
  // handler for fan select button
  bool fanSelectInput = !digitalRead(FAN_SELECT_PIN); // invert input because INPUT_PULLUP
  static bool enableStateChange = false;
  if (fanSelectInput && !enableStateChange) { // button pressed, enable state change
    enableStateChange = true;
  }
  if (!fanSelectInput && enableStateChange) { // set/reset state on falling signal to prevent multiple state changes per button press
    state.lowFanIsSelected = !state.lowFanIsSelected;
    enableStateChange = false;
  }
}
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); };

  // generate clientId
  WiFi.macAddress(mac);
  sprintf(finalClientId, "%s%02X%02X%02X%02X%02X%02X", BASE_CLIENT_ID, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.println("Display init");
  displayInit();

  Serial.println("WiFi init");
  wifiLoop();

  Serial.println("MQTT init");
  mqttInit();

  Serial.println("Thermostat init");
  thermostatInit();

  Serial.println("RTC init");
  rtcClient.init();

  Serial.println("THB init");
  thb.init(&state.tempF, &state.humidity, &state.pressure, &state.setTempF);
}

void loop() {
  thb.readAll(&state.tempF, &state.humidity, &state.pressure, &state.setTempF, &state.setTempOverride);
  displayLoop();
  wifiLoop();
  mqttLoop();
  debouncedPumpSelectHandler();
  debouncedFanSelectHandler();
  thermostatLoop();
  delay(100);
}