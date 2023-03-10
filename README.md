# ESP32-Victron-MQTT

It is possible to add your settings as a json file by creating a data folder:
```
\.pio
\.vscode
\data      <- This folder
\include
\lib
\src
\test
```
In the data folder create a file called config.json Add the following content to the file and change settings as required:
```
{
    "VICTRON_HOST" : "192.168.1.1",
    "VICTRON_PORT" : 1883,
    "VICTRON_ID" : "0aabbccddeef",
    "VICTRON_WATT" : 5000,
    "TOPICBLOCK1" : "battery/256/Soc",
    "TOPICBLOCK2" : "system/0/Ac/ConsumptionOnInput/L1/Power",
    "TOPICBLOCK3" : "system/0/Ac/Grid/L1/Power",
    "TOPICBLOCK4" : "system/0/Dc/Pv/Power",
    "TOPICBLOCK5" : "system/0/Ac/ConsumptionOnOutput/L1/Power",
    "TOPICMSOC" : "settings/0/Settings/CGwacs/BatteryLife/MinimumSocLimit"
}
```
You can then open PlatformIO pain and select "PROJECT TASKS | esp32s3box | Platform | Build Filesystem image". Once done you can upload this settings by selecting "Upload Filesystem Image".
