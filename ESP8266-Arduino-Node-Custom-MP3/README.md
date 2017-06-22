## Arduino with [ESP8266](http://www.wemos.cc/Products/d1_mini.html)

- X PWM 3W Led
- 1 Hall effect sensor
- [DFRobot Mp3 Player](https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299)

Rename `WifiPass.h.config` to `WifiPass.h.` and update 

```
const char* ssid = "WIFI_SSID_HERE";
const char* pass = "WIFI_PASSWORD_HERE";

const char* serverIP = "192.168.1.102";
```

with SSID, Wifi Password and the server IP


### Libraries

- [DFRobotDFPlayerMini](https://github.com/DFRobot/DFRobotDFPlayerMini)
- [SoftwareSerial for ESP8266](https://github.com/plerup/espsoftwareserial)

