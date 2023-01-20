#include "Arduino.h"
#include "WiFi.h"
#include "U8g2lib.h"
#include "Arduino_GFX_Library.h"
#include "PubSubClient.h"
#include "ArduinoJSON.h"
#include "main.h"
#include "config.h"

// INIT GFX
Arduino_DataBus *bus = new Arduino_ESP32LCD8(PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_WR, PIN_LCD_RD, PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
Arduino_GFX *gfx = new Arduino_ST7789(bus, PIN_LCD_RES, 
                                      0 /* rotation */, true /* IPS */,
                                      LCD_V_RES, LCD_H_RES,
                                      35 /* col offset 1 */, 0 /* row offset 1 */,
                                      35 /* col offset 2 */, 0 /* row offset 2 */);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

ulong lastMQTTResponse = 0;

void setup() {

  Serial.begin(9600);

  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  ledcSetup(0, 10000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, LCD_BRIGHTNESS);

  gfx->begin();
  gfx->setRotation(3);
  gfx->fillScreen(BLACK);
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_10x20_mr);
  gfx->setTextColor(GREEN);
  gfx->setCursor(0, 12);
  gfx->println("Starting...");

  Serial.println("Starting...");

  gfx->setTextColor(WHITE);
  gfx->println("SSID: " + String(WIFI_SSID));

  Serial.println("SSID: " + String(WIFI_SSID));

  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  gfx->setTextColor(RED);
  while (WiFi.status() != WL_CONNECTED) 
  {
    gfx->printf(".");
    Serial.print(".");
    delay(1000);
  }
  gfx->setTextColor(GREEN);
  gfx->println("\nConnected!");
  Serial.println("\nConnected!");
  gfx->setTextColor(WHITE);
  gfx->println("IP: " + WiFi.localIP().toString());
  Serial.println("IP: " + WiFi.localIP().toString());

  delay(2000);

  mqttClient.setServer(VICTRON_IP, VICTRON_PORT);
  mqttClient.setCallback(mqttCallback);

  drawBackground();
}

void loop() {
  // If WiFi is not connected exit loop and wait for reconnection
  if (!WiFi.isConnected()) {
      Serial.println("WiFi connection lost ...");
      delay(1000);
      drawBackground();
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

void drawBackground()
{
  gfx->fillScreen(BLACK);
  gfx->fillRoundRect(0, 0, 105, 70, 5, LIGHTGREY);        gfx->fillRoundRect(5, 5, 95, 60, 5, DARKGREY);
  gfx->fillRoundRect(107, 0, 105, 70, 5, LIGHTGREY);      gfx->fillRoundRect(112, 5, 95, 60, 5, DARKGREY);
  gfx->fillRoundRect(214, 0, 105, 70, 5, LIGHTGREY);      gfx->fillRoundRect(219, 5, 95, 60, 5, DARKGREY);
  gfx->fillRoundRect(0, 72, 159, 98, 5, LIGHTGREY);      gfx->fillRoundRect(5, 77, 149, 88, 5, DARKGREY);
  gfx->fillRoundRect(161, 72, 159, 98, 5, LIGHTGREY);    gfx->fillRoundRect(166, 77, 149, 88, 5, DARKGREY);
}

void drawBlock1(float value) // Battery
{
  if (value > float(MSOC) * 1.1) {
    // Battery is charged more than 10% above the minimum state of charge
    gfx->fillRoundRect(5, 5, 95, 60, 5, DARKGREEN);
    gfx->setTextColor(WHITE);
  } else if ( value >= float(MSOC) / 1.1 && value < float(MSOC) * 1.1) {
    // Battery is less than 10% but more than -10% of minimum state of charge
    gfx->fillRoundRect(5, 5, 95, 60, 5, YELLOW);
    gfx->setTextColor(BLACK);
  } else {
    // Battery is less than -10% of minimum state of charge
    gfx->fillRoundRect(5, 5, 95, 60, 5, RED);
    gfx->setTextColor(WHITE);
  }
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_10x20_mr);
  gfx->setCursor(30, 30);
  gfx->println(fixLengthStringLeftAlign(String(value), 4) + "%");
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_7x13_mr);
  gfx->setCursor(30, 50);
  gfx->println("BATTERY");
}
void drawBlock2(int value, bool hasFailed)  // Non-critical loads
{
  if ((value >= 3000 && value > TOPICVALUES[3]) || hasFailed) {
    gfx->fillRoundRect(112, 5, 95, 60, 5, RED);
    gfx->setTextColor(WHITE);
  } else if (value < 3000 && value >= 1000 && value > TOPICVALUES[3]) {
    gfx->fillRoundRect(112, 5, 95, 60, 5, YELLOW);
    gfx->setTextColor(BLACK);
  } else {
    gfx->fillRoundRect(112, 5, 95, 60, 5, DARKGREEN);
    gfx->setTextColor(WHITE);
  }
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_10x20_mr);
  gfx->setCursor(137, 30);
  if (hasFailed) 
    gfx->println(fixLengthStringRightAlign("-----", 5));
  else
    gfx->println(fixLengthStringRightAlign(String(value) + "W", 5));
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_7x13_mr);
  gfx->setCursor(122, 50);
  gfx->println("NONCRITICAL");
}
void drawBlock3(int value, bool hasFailed)  // Grid power
{
  if (value >= 3000 || hasFailed) {
    gfx->fillRoundRect(219, 5, 95, 60, 5, RED);
    gfx->setTextColor(WHITE);
  } else if (value < 3000 && value >= 1000) {
    gfx->fillRoundRect(219, 5, 95, 60, 5, YELLOW);
    gfx->setTextColor(BLACK);
  } else {
    gfx->fillRoundRect(219, 5, 95, 60, 5, DARKGREEN);
    gfx->setTextColor(WHITE);
  }
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_10x20_mr);
  gfx->setCursor(244, 30);
  if (hasFailed)
    gfx->println(fixLengthStringRightAlign("-----", 5));
  else
    gfx->println(fixLengthStringRightAlign(String(value) + "W", 5));
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_7x13_mr);
  gfx->setCursor(234, 50);
  gfx->println("GRID POWER");
}
void drawBlock4(int value)  // Solar power
{
  if (value > float(TOPICVALUES[4] / 1.1)) {
    // Solar is generating more than -10% of critical loads
    gfx->fillRoundRect(5, 77, 149, 88, 5, DARKGREEN);
    gfx->setTextColor(WHITE);
  } else if (value > float(TOPICVALUES[4]) / 1.3) {
    // Solar is generating less than -10% of critical loads but more than -30% of load
    gfx->fillRoundRect(5, 77, 149, 88, 5, YELLOW);
    gfx->setTextColor(BLACK);
  } else {
    gfx->fillRoundRect(5, 77, 149, 88, 5, RED);
    gfx->setTextColor(WHITE);
  }
  gfx->setTextSize(2);
  gfx->setFont(u8g2_font_10x20_mr);
  gfx->setCursor(30, 125);
  gfx->println(fixLengthStringRightAlign(String(value) + "W", 5));
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_7x13_mr);
  gfx->setCursor(35, 150);
  gfx->println("SOLAR CHARGER");
}
void drawBlock5(int value)  // Critical loads
{
  if (value < TOPICVALUES[3] || value < float(VICTRON_WATT) * 0.5) {
    // Critical load is less than solar power or less than 50% of what the inverter kan handle
    gfx->fillRoundRect(166, 77, 149, 88, 5, DARKGREEN);
    gfx->setTextColor(WHITE);
  } else if (value >= float(VICTRON_WATT) * 0.5 && value < float(VICTRON_WATT) * 0.8 && TOPICVALUES[0] > float(MSOC) * 1.1) {
    // Critical load is more than 50% but less than 80% of what invertor kan handle and battery charge is more than 10% of minimum state of charge
    gfx->fillRoundRect(166, 77, 149, 88, 5, YELLOW);
    gfx->setTextColor(BLACK);
  } else {
    gfx->fillRoundRect(166, 77, 149, 88, 5, RED);
    gfx->setTextColor(WHITE);
  }
  gfx->setTextSize(2);
  gfx->setFont(u8g2_font_10x20_mr);
  gfx->setCursor(191, 125);
  gfx->println(fixLengthStringRightAlign(String(value) + "W", 5));
  gfx->setTextSize(1);
  gfx->setFont(u8g2_font_7x13_mr);
  gfx->setCursor(191, 150);
  gfx->println("CRITICAL LOADS");

}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  lastMQTTResponse = millis();
  //Serial.println("MQTT Response at " + String(lastMQTTResponse) + " : " + topic);
  if (length < 2) return;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message, length);
  if (String(topic).endsWith(TOPICBLOCK1)) {  // Battery
    //float bat = doc["value"][0]["soc"];
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
    if (mqttClient.connect("ESP32-Victron-MQTT")) {
      Serial.println("Connected!");
      subscribe();
    } else {
      Serial.println("Failed to connect .. retry in 5");
      delay(5000);
    }
  }
}


String fixLengthStringRightAlign(String text, int length)
{
  String rtnVal;
  for (int i = 0; i < length; i++)
    rtnVal += " ";
  rtnVal += text;
  return rtnVal.substring(rtnVal.length() - length, rtnVal.length());
}

String fixLengthStringLeftAlign(String text, int length)
{
  String rtnVal = text;
  for (int i = 0; i < length; i++)
    rtnVal += " ";
  return rtnVal.substring(0, length);
}
