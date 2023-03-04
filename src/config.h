#include "Arduino.h"
#pragma once

#define JSON_CONFIG_FILE "/config.json"

char      VICTRON_HOST[50] = "";
int       VICTRON_PORT = 0;
String    VICTRON_ID = "";              // Find this in the https://vrm.victronenergy.com/ portal at "Device list" | "Gateway" | "VRM portal ID"
int       VICTRON_MQTT_TIMEOUT = 5;
int       VICTRON_WATT = 5000;

int       TOPICVALUES[] = {0, 0, 0, 0, 0};
String    TOPICBLOCK1 = "";             // Battery state of charge. Alternative location = "system/0/Batteries"
String    TOPICBLOCK2 = "";             // Non-critical load
String    TOPICBLOCK3 = "";             // Grid power
String    TOPICBLOCK4 = "";             // Solar power
String    TOPICBLOCK5 = "";             // Critical loads

int       MSOC = 0;
String    TOPICMSOC = "";               // Minimum State of Charge as set by user

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
