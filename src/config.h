#include "Arduino.h"
#pragma once

IPAddress VICTRON_IP (10, 0, 2, 90);
int       VICTRON_PORT = 1883;
String    VICTRON_ID = "0cb2b788345a";
int       VICTRON_MQTT_TIMEOUT = 5;

int       TOPICVALUES[] = {0, 0, 0, 0, 0};
//String    TOPICBLOCK1 = "system/0/Batteries";
String    TOPICBLOCK1 = "battery/256/Soc";                          // Battery state of charge
String    TOPICBLOCK2 = "system/0/Ac/ConsumptionOnInput/L1/Power";  // Non-critical load
String    TOPICBLOCK3 = "system/0/Ac/Grid/L1/Power";                // Grid power
String    TOPICBLOCK4 = "system/0/Dc/Pv/Power";                     // Solar power
String    TOPICBLOCK5 = "system/0/Ac/ConsumptionOnOutput/L1/Power"; // Critical loads

#define WIFI_SSID                   "RAADMIN"
#define WIFI_PASSWORD               "69062752060860126585543"

#define LCD_H_RES                   320
#define LCD_V_RES                   170
#define LCD_BUF_SIZE                (LCD_H_RES * LCD_V_RES)
#define LCD_BRIGHTNESS              180

/*ESP32S3*/
#define PIN_LCD_BL                   38
#define PIN_LCD_D0                   39
#define PIN_LCD_D1                   40
#define PIN_LCD_D2                   41
#define PIN_LCD_D3                   42
#define PIN_LCD_D4                   45
#define PIN_LCD_D5                   46
#define PIN_LCD_D6                   47
#define PIN_LCD_D7                   48
#define PIN_LCD_WR                   8
#define PIN_LCD_RD                   9
#define PIN_LCD_DC                   7
#define PIN_LCD_CS                   6
#define PIN_LCD_RES                  5
#define PIN_POWER_ON                 15
#define PIN_BUTTON_1                 0
#define PIN_BUTTON_2                 14
#define PIN_BAT_VOLT                 4
#define PIN_IIC_SCL                  17
#define PIN_IIC_SDA                  18
#define PIN_TOUCH_INT                16
#define PIN_TOUCH_RES                21