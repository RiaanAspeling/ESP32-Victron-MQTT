#include "Arduino.h"
#pragma once

#define JSON_CONFIG_FILE "/config.json"

char      VICTRON_HOST[50] = "192.168.1.1";
int       VICTRON_PORT = 1883;
String    VICTRON_ID = "0aabbccddeef";                                          // Find this in the https://vrm.victronenergy.com/ portal at "Device list" | "Gateway" | "VRM portal ID"
int       VICTRON_MQTT_TIMEOUT = 5;
int       VICTRON_WATT = 5000;                                                  // Size of the invertor. This is used to change colour when limits are reaced.

int       TOPICVALUES[] = {0, 0, 0, 0, 0};
String    TOPICBLOCK1 = "battery/256/Soc";                                      // Battery state of charge. Alternative location = "system/0/Batteries"
String    TOPICBLOCK2 = "system/0/Ac/ConsumptionOnInput/L1/Power";              // Non-critical load
String    TOPICBLOCK3 = "system/0/Ac/Grid/L1/Power";                            // Grid power
String    TOPICBLOCK4 = "system/0/Dc/Pv/Power";                                 // Solar power
String    TOPICBLOCK5 = "system/0/Ac/ConsumptionOnOutput/L1/Power";             // Critical loads

int       MSOC = 0;
String    TOPICMSOC = "settings/0/Settings/CGwacs/BatteryLife/MinimumSocLimit"; // Minimum State of Charge as set by user

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

#define PIN_BUTTON_DEBOUNCE_TIME     100 // the debounce time in millisecond, increase this time if it still chatters
