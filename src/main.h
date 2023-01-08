#include "Arduino.h"

void drawBackground();
void drawBlock1(int value);
void drawBlock2(int value);
void drawBlock3(int value);
void drawBlock4(int value);
void drawBlock5(int value);
void mqttCallback(char* topic, byte* message, unsigned int length);
void reconnect();
String fixLengthStringRightAlign(String text, int length);
String fixLengthStringLeftAlign(String text, int length);
// void readMQTTTopic(String mqttTopic);