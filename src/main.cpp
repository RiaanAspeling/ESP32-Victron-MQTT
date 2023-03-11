#include "Arduino.h"
#include "WiFi.h"
#include <WiFiClient.h>
#include "U8g2lib.h"
#include "Arduino_GFX_Library.h"
#include "PubSubClient.h"
#include "ArduinoJSON.h"
#include <SPIFFS.h>
#include "config.h"
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "extensions.cpp"

// INIT GFX
Arduino_DataBus *bus = new Arduino_ESP32LCD8(PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_WR, PIN_LCD_RD, PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
Arduino_GFX *gfx = new Arduino_ST7789(bus, PIN_LCD_RES, 
                                      3 /* rotation */, 
                                      true /* IPS */,
                                      LCD_V_RES, LCD_H_RES,
                                      35 /* col offset 1 */, 0 /* row offset 1 */,
                                      35 /* col offset 2 */, 0 /* row offset 2 */);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

AsyncWebServer server(80);

ulong lastMQTTResponse = 0;

int zoomBox = 0;

// Track both buttons states for debounce
int lastButtonState[] = {LOW, LOW};
int lastButtonFlickerableState[] = {LOW, LOW};
int currentButtonState[] = {LOW, LOW};
unsigned long lastButtonDebounceTime[] = {0, 0};

//flag for saving data
bool shouldSaveConfig = false;

String fixLengthStringRightAlign(String text, int length)
{
  String rtnVal;
  for (int i = 0; i < length; i++)
    rtnVal += " ";
  rtnVal += text;
  return rtnVal.substring(rtnVal.length() - length, rtnVal.length());
}

String fixLengthStringLeftAlign(String text, int length, bool removePoint = false)
{
  String rtnVal = text;
  for (int i = 0; i < length; i++)
    rtnVal += " ";
  rtnVal = rtnVal.substring(0, length);
  if (removePoint && rtnVal.endsWith(".")) rtnVal = rtnVal.substring(0, rtnVal.length() -1);
  return rtnVal;
}

void printText(int16_t x, int16_t y, String text, uint8_t size, const uint8_t *font) {
    gfx->setTextSize(size);
    gfx->setFont(font);
    gfx->setCursor(x, y);
    gfx->print(text);
}

void drawBlock1(float value) // Battery
{
  uint16_t colours[] { BLACK, WHITE };
  if (value > float(MSOC) * 1.1) {                                        // Battery is charged more than 10% above the minimum state of charge
    colours[0] = DARKGREEN;
    colours[1] = WHITE;
  } else if ( value >= float(MSOC) / 1.1 && value < float(MSOC) * 1.1) {  // Battery is less than 10% but more than -10% of minimum state of charge
    colours[0] = YELLOW;       
    colours[1] = BLACK;
  } else {                                                                // Battery is less than -10% of minimum state of charge
    colours[0] = RED;
    colours[1] = WHITE;
  }

  gfx->setTextColor(colours[1]);

  if (zoomBox == 0)
  {
    gfx->fillRoundRect(5, 5, 95, 60, 5, colours[0]);
    printText( 34, 30, fixLengthStringLeftAlign(String(value), 4, true) + "%", 1, u8g2_font_10x20_mr);
    printText( 30, 50, "BATTERY", 1, u8g2_font_7x13_mr);
  } else if (zoomBox == 1) {
    gfx->fillRoundRect(5, 5, LCD_H_RES - 10, LCD_V_RES - 10, 5, colours[0]);
    printText( 100, 90, fixLengthStringLeftAlign(String(value), 4, true) + "%", 3, u8g2_font_10x20_mr);
    printText( 110, 130, "BATTERY", 2, u8g2_font_7x13_mr);
  }
}
void drawBlock2(int value, bool hasFailed)  // Non-critical loads
{
  uint16_t colours[] { BLACK, WHITE };
  if ((value >= 3000 && value > TOPICVALUES[3]) || hasFailed) {
    colours[0] = RED;
    colours[1] = WHITE;
  } else if (value < 3000 && value >= 1000 && value > TOPICVALUES[3]) {
    colours[0] = YELLOW;
    colours[1] = BLACK;
  } else {
    colours[0] = DARKGREEN;
    colours[1] = WHITE;
  }

  gfx->setTextColor(colours[1]);

  if (zoomBox == 0) {
    gfx->fillRoundRect(112, 5, 95, 60, 5, colours[0]);
    printText(130, 30, (hasFailed ? fixLengthStringRightAlign("------", 6) : fixLengthStringRightAlign(String(value) + "W", 6)), 1, u8g2_font_10x20_mr);
    printText(122, 50, "NONCRITICAL", 1, u8g2_font_7x13_mr);
  } else if (zoomBox == 2) {
    gfx->fillRoundRect(5, 5, LCD_H_RES - 10, LCD_V_RES - 10, 5, colours[0]);
    printText(70, 90, (hasFailed ? fixLengthStringRightAlign("------", 6) : fixLengthStringRightAlign(String(value) + "W", 6)), 3, u8g2_font_10x20_mr);
    printText(85, 130, "NONCRITICAL", 2, u8g2_font_7x13_mr);
  }
}
void drawBlock3(int value, bool hasFailed)  // Grid power
{
  uint16_t colours[] { BLACK, WHITE };
  if (value >= 3000 || hasFailed) {
    colours[0] = RED;
    colours[1] = WHITE;
  } else if (value < 3000 && value >= 1000) {
    colours[0] = YELLOW;
    colours[1] = BLACK;
  } else {
    colours[0] = DARKGREEN;
    colours[1] = WHITE;
  }

  gfx->setTextColor(colours[1]);

  if (zoomBox == 0) {
    gfx->fillRoundRect(219, 5, 95, 60, 5, colours[0]);
    printText(230, 30, (hasFailed ? fixLengthStringRightAlign("-------", 7) : fixLengthStringRightAlign(String(value) + "W", 7)), 1, u8g2_font_10x20_mr);
    printText(234, 50, "GRID POWER", 1, u8g2_font_7x13_mr);
  } else if (zoomBox == 3) {
    gfx->fillRoundRect(5, 5, LCD_H_RES - 10, LCD_V_RES - 10, 5, colours[0]);
    printText(40, 90, (hasFailed ? fixLengthStringRightAlign("-------", 7) : fixLengthStringRightAlign(String(value) + "W", 7)), 3, u8g2_font_10x20_mr);
    printText(90, 130, "GRID POWER", 2, u8g2_font_7x13_mr);
  }
}
void drawBlock4(int value)  // Solar power
{
  uint16_t colours[] { BLACK, WHITE };
  if (value > float(TOPICVALUES[4] / 1.1)) {            // Solar is generating more than -10% of critical loads
    colours[0] = DARKGREEN;
    colours[1] = WHITE;
  } else if (value > float(TOPICVALUES[4]) / 1.3) {     // Solar is generating less than -10% of critical loads but more than -30% of load
    colours[0] = YELLOW;
    colours[1] = BLACK;
  } else {
    colours[0] = RED;
    colours[1] = WHITE;
  }

  gfx->setTextColor(colours[1]);

  if (zoomBox == 0) {
    gfx->fillRoundRect(5, 77, 149, 88, 5, colours[0]);
    printText(20, 125, fixLengthStringRightAlign(String(value) + "W", 6), 2, u8g2_font_10x20_mr);
    printText(35, 150, "SOLAR CHARGER", 1, u8g2_font_7x13_mr);
  } else if (zoomBox == 4) {
    gfx->fillRoundRect(5, 5, LCD_H_RES - 10, LCD_V_RES - 10, 5, colours[0]);
    printText(70, 90, fixLengthStringRightAlign(String(value) + "W", 6), 3, u8g2_font_10x20_mr);
    printText(70, 130, "SOLAR CHARGER", 2, u8g2_font_7x13_mr);
  }
}
void drawBlock5(int value)  // Critical loads
{
  uint16_t colours[] { BLACK, WHITE };
  if (value < TOPICVALUES[3] || value < float(VICTRON_WATT) * 0.5) {                                                          // Critical load is less than solar power or less than 50% of what the inverter kan handle
    colours[0] = DARKGREEN;
    colours[1] = WHITE;
  } else if (value >= float(VICTRON_WATT) * 0.5 && value < float(VICTRON_WATT) * 0.8 && TOPICVALUES[0] > float(MSOC) * 1.1) { // Critical load is more than 50% but less than 80% of what invertor kan handle and battery charge is more than 10% of minimum state of charge
    colours[0] = YELLOW;
    colours[1] = BLACK;
  } else {
    colours[0] = RED;
    colours[1] = WHITE;
  }

  gfx->setTextColor(colours[1]);

  if (zoomBox == 0) {
    gfx->fillRoundRect(166, 77, 149, 88, 5, colours[0]);
    printText(183, 125, fixLengthStringRightAlign(String(value) + "W", 6), 2, u8g2_font_10x20_mr);
    printText(191, 150, "CRITICAL LOADS", 1, u8g2_font_7x13_mr);
  } else if (zoomBox == 5) {
    gfx->fillRoundRect(5, 5, LCD_H_RES - 10, LCD_V_RES - 10, 5, colours[0]);
    printText(70, 90, fixLengthStringRightAlign(String(value) + "W", 6), 3, u8g2_font_10x20_mr);
    printText(63, 130, "CRITICAL LOADS", 2, u8g2_font_7x13_mr);
  }
}
void drawOTA()
{
    gfx->fillRoundRect(5, 5, LCD_H_RES - 10, LCD_V_RES - 10, 5, YELLOW);
    gfx->setTextColor(ORANGE);
    printText( 30, 50, "UPDATE", 2, u8g2_font_10x20_mr);
    gfx->setTextColor(BLACK);
    printText( 30, 80, "Please connect to the address", 1, u8g2_font_9x15_mr);
    printText( 30, 100, "below to upload new firmware.", 1, u8g2_font_9x15_mr);
    printText( 30, 130, "http://" + WiFi.localIP().toString(), 1, u8g2_font_10x20_mr);
}
void drawBackground(int doBox)
{
  gfx->fillScreen(BLACK);
  if (doBox == 0) {
      gfx->fillRoundRect(0, 0, 105, 70, 5, LIGHTGREY);    gfx->fillRoundRect(5, 5, 95, 60, 5, DARKGREY);
      gfx->fillRoundRect(107, 0, 105, 70, 5, LIGHTGREY);  gfx->fillRoundRect(112, 5, 95, 60, 5, DARKGREY);
      gfx->fillRoundRect(214, 0, 105, 70, 5, LIGHTGREY);  gfx->fillRoundRect(219, 5, 95, 60, 5, DARKGREY);
      gfx->fillRoundRect(0, 72, 159, 98, 5, LIGHTGREY);   gfx->fillRoundRect(5, 77, 149, 88, 5, DARKGREY);
      gfx->fillRoundRect(161, 72, 159, 98, 5, LIGHTGREY); gfx->fillRoundRect(166, 77, 149, 88, 5, DARKGREY);
  } else {
      gfx->fillRoundRect(0, 0, LCD_H_RES, LCD_V_RES, 5, LIGHTGREY);    gfx->fillRoundRect(5, 5, LCD_H_RES - 10, LCD_V_RES - 10, 5, DARKGREY);
      switch (doBox) {
        case 1:
          drawBlock1(TOPICVALUES[0]);
          break;
        case 2:
          drawBlock2(TOPICVALUES[1], false);
          break;
        case 3:
          drawBlock3(TOPICVALUES[2], false);
          break;
        case 4:
          drawBlock4(TOPICVALUES[3]);
          break;
        case 5:
          drawBlock5(TOPICVALUES[4]);
          break;
        case 6:
          drawOTA();
          break;
      }
  }
  gfx->setTextColor(BLACK);
  printText(10, 170, WiFi.localIP().toString(), 1, u8g2_font_4x6_mr);
  printText(176, 170, WiFi.getHostname(), 1, u8g2_font_4x6_mr);
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  lastMQTTResponse = millis();
  if (length < 2) return;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message, length);
  if (String(topic).endsWith(TOPICBLOCK1)) {        // Battery
    float bat = doc["value"];
    TOPICVALUES[0] = bat;
    drawBlock1(bat);
  } else if (String(topic).endsWith(TOPICBLOCK2)) { // Non-critical loads
    int load = doc["value"];
    TOPICVALUES[1] = load;
    drawBlock2(load, doc["value"].isNull());
  } else if (String(topic).endsWith(TOPICBLOCK3)) { // Grid power
    int grid = doc["value"];
    TOPICVALUES[2] = grid;
    drawBlock3(grid, doc["value"].isNull());
  } else if (String(topic).endsWith(TOPICBLOCK4)) { // Solar power
    int pv = doc["value"];
    TOPICVALUES[3] = pv;
    drawBlock4(pv);
  } else if (String(topic).endsWith(TOPICBLOCK5)) { // Critical loads
    int ac = doc["value"];
    TOPICVALUES[4] = ac;
    drawBlock5(ac);
  } else if (String(topic).endsWith(TOPICMSOC)) {  // Minimum state of charge
    MSOC = doc["value"];
  }
}

void subscribeMessage(String message) {
    char buf0[255];
    message.toCharArray(buf0, message.length() + 1);
    mqttClient.subscribe(buf0); 
}

void subscribe() {
    // Battery (Block1)
    subscribeMessage("N/" + VICTRON_ID + "/" + TOPICBLOCK1 + "/#");
    // Non-critical Loads (Block2)
    subscribeMessage("N/" + VICTRON_ID + "/" + TOPICBLOCK2 + "/#");
    // Grid power (Block3)
    subscribeMessage("N/" + VICTRON_ID + "/" + TOPICBLOCK3 + "/#");
    // Solar power (Block4)
    subscribeMessage("N/" + VICTRON_ID + "/" + TOPICBLOCK4 + "/#");
    // Critical Loads (Block5)
    subscribeMessage("N/" + VICTRON_ID + "/" + TOPICBLOCK5 + "/#");
    // Minimum State of Charge
    subscribeMessage("N/" + VICTRON_ID + "/" + TOPICMSOC + "/#");
    Serial.println("Subscribed!");
}

void reconnect() {
  Serial.println("Reconnecting...");
  while (!mqttClient.connected()) {
    lastMQTTResponse = millis();
    // Attempt to connect
    if (mqttClient.connect(WiFi.getHostname())) {
      Serial.println("Connected!");
      subscribe();
    } else {
      Serial.println("Failed to connect .. retry in 5");
      delay(5000);
    }
  }
}

void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered Conf Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void saveConfigFile()
{
  Serial.println(F("Saving config"));
  StaticJsonDocument<2048> json;
  json["VICTRON_HOST"] = VICTRON_HOST;
  json["VICTRON_PORT"] = VICTRON_PORT;
  json["VICTRON_ID"] = VICTRON_ID;
  json["VICTRON_WATT"] = VICTRON_WATT;
  json["TOPICBLOCK1"] = TOPICBLOCK1;
  json["TOPICBLOCK2"] = TOPICBLOCK2;
  json["TOPICBLOCK3"] = TOPICBLOCK3;
  json["TOPICBLOCK4"] = TOPICBLOCK4;
  json["TOPICBLOCK5"] = TOPICBLOCK5;
  json["TOPICMSOC"] = TOPICMSOC;

  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();
}

bool loadConfigFile()
{
  if (SPIFFS.exists(JSON_CONFIG_FILE))
  {
    //file exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
    if (configFile)
    {
      Serial.println("opened config file");
      StaticJsonDocument<2048> json;
      DeserializationError error = deserializeJson(json, configFile);
      serializeJsonPretty(json, Serial);
      if (!error)
      {
        Serial.println("\nparsed json");

        strcpy(VICTRON_HOST, json["VICTRON_HOST"]);
        VICTRON_PORT = json["VICTRON_PORT"].as<int>();
        VICTRON_ID = json["VICTRON_ID"].as<String>();
        VICTRON_WATT = json["VICTRON_WATT"].as<int>();
        TOPICBLOCK1 = json["TOPICBLOCK1"].as<String>();
        TOPICBLOCK2 = json["TOPICBLOCK2"].as<String>();
        TOPICBLOCK3 = json["TOPICBLOCK3"].as<String>();
        TOPICBLOCK4 = json["TOPICBLOCK4"].as<String>();
        TOPICBLOCK5 = json["TOPICBLOCK5"].as<String>();
        TOPICMSOC = json["TOPICMSOC"].as<String>();
        return true;
      }
      else
      {
        Serial.println("failed to load json config");
      }
    }
  }
  //end read
  return false;
}

void connectSPIFFS() {
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Connected!");
  } else {
    Serial.println("SPIFFS FAILED to mount!");
    delay(5000);
    ESP.restart();
  }
}

void connectWifi(bool forceConfig)
{
    // Setup wifi and manager
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setBreakAfterConfig(true);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);

  // Create custom data for configuration
  WiFiManagerParameter wc_victron_address("VICTRON_HOST", "Victron MQTT Address/Host", VICTRON_HOST, 50);   wm.addParameter(&wc_victron_address);
  IntParameter wc_victron_port("VICTRON_PORT", "Victron MQTT Port", VICTRON_PORT);                          wm.addParameter(&wc_victron_port);
  StringParameter wc_victron_id("VICTRON_ID", "Victron Id", VICTRON_ID);                                    wm.addParameter(&wc_victron_id);
  IntParameter wc_victron_watt("VICTRON_WATT", "Victron Inverter Wh", VICTRON_WATT);                        wm.addParameter(&wc_victron_watt);
  StringParameter wc_victron_topicblock1("TOPICBLOCK1", "Battery state of charge topic", TOPICBLOCK1);      wm.addParameter(&wc_victron_topicblock1);
  StringParameter wc_victron_topicblock2("TOPICBLOCK2", "Non-critical load topic", TOPICBLOCK2);            wm.addParameter(&wc_victron_topicblock2);
  StringParameter wc_victron_topicblock3("TOPICBLOCK3", "Grid power topic", TOPICBLOCK3);                   wm.addParameter(&wc_victron_topicblock3);
  StringParameter wc_victron_topicblock4("TOPICBLOCK4", "Solar power topic", TOPICBLOCK4);                  wm.addParameter(&wc_victron_topicblock4);
  StringParameter wc_victron_topicblock5("TOPICBLOCK5", "Critical loads topic", TOPICBLOCK5);               wm.addParameter(&wc_victron_topicblock5);
  StringParameter wc_victron_topicmsoc("TOPICMSOC", "Minimum State of Charge topic", TOPICMSOC);            wm.addParameter(&wc_victron_topicmsoc);

  if (forceConfig) {
    gfx->setTextColor(RED);
    gfx->println("Forced config mode!");
    gfx->setTextColor(YELLOW);
    gfx->println("SSID to configure:");
    gfx->setTextColor(GREEN);
    gfx->println(wm.getDefaultAPName());
    if (!wm.startConfigPortal())
    {
      gfx->setTextColor(WHITE);
      gfx->println("Failed to connect, restarting!");
      Serial.println("failed to connect and hit timeout");
      delay(5000);
      ESP.restart();
    }
  }
  else {
    gfx->setTextColor(GREEN);
    if (wm.getWiFiIsSaved())
      gfx->println("Connecting to " + wm.getWiFiSSID());
    else
    {
      gfx->println("Connect to configure:");
      gfx->setTextColor(GREEN);
      gfx->println(wm.getDefaultAPName());
    }
    bool canConnect = wm.autoConnect();
    
    if (!canConnect)
    {
      gfx->setTextColor(WHITE);
      gfx->println("Failed to connect, restarting!");
      Serial.println("Failed to connect and hit timeout");
      delay(5000);
      ESP.restart();
    }
  }
  // Allow to reconnect to WiFi if signal is lost
  WiFi.setAutoReconnect(true);
  // Would be set with the callback saveConfigCallback
  if (shouldSaveConfig) {
    gfx->setTextColor(YELLOW);
    gfx->println("Saving settings ...");
    strncpy(VICTRON_HOST, wc_victron_address.getValue(), sizeof(VICTRON_HOST));
    VICTRON_PORT = wc_victron_port.getValue();
    VICTRON_ID = wc_victron_id.getValue();
    VICTRON_WATT = wc_victron_watt.getValue();
    TOPICBLOCK1 = wc_victron_topicblock1.getValue();
    TOPICBLOCK2 = wc_victron_topicblock2.getValue();
    TOPICBLOCK3 = wc_victron_topicblock3.getValue();
    TOPICBLOCK4 = wc_victron_topicblock4.getValue();
    TOPICBLOCK5 = wc_victron_topicblock5.getValue();
    TOPICMSOC = wc_victron_topicmsoc.getValue();
    saveConfigFile();
  }
}

void setup() {

  pinMode(PIN_BUTTON_1, INPUT);     // Press this button after startup to zoom into each pannel
  pinMode(PIN_BUTTON_2, INPUT);     // Press button two on startup to force config mode

  Serial.begin(115200);
  delay(10);

  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  ledcSetup(0, 10000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, LCD_BRIGHTNESS);

  gfx->begin();
  gfx->setTextWrap(false);
  gfx->fillScreen(BLACK);
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_10x20_mr);
  gfx->setTextColor(GREEN);
  gfx->setCursor(0, 12);
  gfx->println("Starting...");  Serial.println("Starting...");

  connectSPIFFS();

  bool loadedConfig = loadConfigFile();

  gfx->println("SPIFFS connected!");

  if (!digitalRead(PIN_BUTTON_2)) {
    gfx->println("Starting config ...");
    connectWifi(true);
  }
  else
    connectWifi(!loadedConfig); // Force config if config file error

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("/update");
  });

  AsyncElegantOTA.begin(&server, "", ""); // Start AsyncElegantOTA
  server.begin();

  gfx->println("Loaded OTA...");  Serial.println("Loaded OTA...");

  delay(1000);

  mqttClient.setServer(VICTRON_HOST, VICTRON_PORT);
  mqttClient.setCallback(mqttCallback);

  drawBackground(zoomBox);
}

void loop() {

  // If WiFi is not connected exit loop and wait for reconnection
  if (!WiFi.isConnected()) {
      Serial.println("WiFi connection lost ...");
      delay(1000);
      drawBackground(zoomBox);
    return;
  }

  currentButtonState[0] = digitalRead(PIN_BUTTON_1);
  currentButtonState[1] = digitalRead(PIN_BUTTON_2);

  if (currentButtonState[0] != lastButtonFlickerableState[0]) {
    lastButtonDebounceTime[0] = millis();
    lastButtonFlickerableState[0] = currentButtonState[0];
  }
  if (currentButtonState[1] != lastButtonFlickerableState[1]) {
    lastButtonDebounceTime[1] = millis();
    lastButtonFlickerableState[1] = currentButtonState[1];
  }

  // Check button 1 for press and release
  if ((millis() - lastButtonDebounceTime[0]) > PIN_BUTTON_DEBOUNCE_TIME) {
    if(lastButtonState[0] == HIGH && currentButtonState[0] == LOW) {
      Serial.println("The button 1 is released");
      zoomBox++;
      if (zoomBox > 6) zoomBox = 0;
      drawBackground(zoomBox);
      if (zoomBox == 6) drawOTA();
    }
    else if(lastButtonState[0] == LOW && currentButtonState[0] == HIGH) {
      Serial.println("The button 1 is released");
    }
    lastButtonState[0] = currentButtonState[0];
  }

  // Check button 2 for press and release
  if ((millis() - lastButtonDebounceTime[1]) > PIN_BUTTON_DEBOUNCE_TIME) {
    if(lastButtonState[1] == HIGH && currentButtonState[1] == LOW) {
      Serial.println("The button 2 is released");
      zoomBox--;
      if (zoomBox < 0) zoomBox = 6;
      drawBackground(zoomBox);
      if (zoomBox == 6) drawOTA();
    }
    else if(lastButtonState[1] == LOW && currentButtonState[1] == HIGH) {
      Serial.println("The button 2 is released");
    }
    lastButtonState[1] = currentButtonState[1];
  }

  if (zoomBox == 6) // Do not run MQTT below when in OTA mode.
  {
    if (mqttClient.connected()) {
      Serial.println("Disconnect MQTT!");
      mqttClient.disconnect();
    }
    return;
  }

  if (!mqttClient.connected()) {
    Serial.println("Not connected!");
    reconnect();
  }

  mqttClient.loop();

  if (lastMQTTResponse + (VICTRON_MQTT_TIMEOUT * 1000) < millis())
  {
    // Send a keep alive message
    if (mqttClient.connected())
    {
      lastMQTTResponse = millis();
      Serial.println("Sending keep alive.");
      char buf0[25];
      char buf1[255];
      String tempStr = "R/" + VICTRON_ID + "/keepalive";
      tempStr.toCharArray(buf0, tempStr.length() + 1);
      tempStr = "[\"" + TOPICBLOCK1 + "\", \"" + TOPICBLOCK2 + "\", \"" + TOPICBLOCK3 + "\", \"" + TOPICBLOCK4 + "\", \"" + TOPICBLOCK5 + "\", \"" + MSOC + "\"]";
      tempStr.toCharArray(buf1, tempStr.length() + 1);
      mqttClient.publish(buf0, buf1);
    }
  }
}

