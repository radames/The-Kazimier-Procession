#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include "WifiPass.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

#define HALLSENSOR 5 //GPIO5
#define PIN 4  //GPIO4
#define PWMPIN 0  //GPIO0
#define NUMPIXELS 48 //48 ws2812 RGB Pixels

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
//

// A UDP instance to let us send and receive packets over UDP

WiFiUDP Udp;
const IPAddress outIp(192, 168, 20, 3);  // remote IP (not needed for receive) kronosServer static IP


const unsigned int inPort = 8888;
const unsigned int outPort = 9999;

OSCErrorCode error;
unsigned int ledState = HIGH;              // LOW means led is *on*

//node s
enum NodeState { CONNECT, WAIT, DISCONNECT };

NodeState nState = CONNECT;
long lastMillis = 0;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, ledState);    // turn *on* led

  analogWriteRange(255);
  analogWriteFreq(200);

  analogWrite(PWMPIN, 255); //inverted PWM, starting OFF 255

  pinMode(HALLSENSOR, INPUT_PULLUP);
  strip.begin();

  Serial.begin(115200);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  int ledCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    ledCount = (ledCount + 1) % strip.numPixels();
    if (ledCount == 0) {
      strip.clear();
      strip.show();
    }
    strip.setPixelColor(ledCount, strip.Color(255, 0, 0));
    strip.show(); // Initialize all pixels to 'off'
    delay(100);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  Udp.begin(inPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());


  strip.clear();
  strip.show(); // Initialize all pixels to 'off'


}

void loop() {
  OSCMessage oscMessage;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) {
      oscMessage.fill(Udp.read());
    }
    if (oscMessage.fullMatch("/connected")) {
      boolean response = oscMessage.getBoolean(0);
      if (response) {
        nState = WAIT;
        digitalWrite(BUILTIN_LED, 0); //ON
      }
    } else if (oscMessage.fullMatch("/disconnect")) {
      boolean response = oscMessage.getBoolean(0);
      if (response) {
        nState = DISCONNECT;
        digitalWrite(BUILTIN_LED, 1); //OFF
        strip.clear();
        strip.show(); // Initialize all pixels to 'off'
        analogWrite(PWMPIN, 255); //inverted PWM, starting OFF 255
      }
    } else if (oscMessage.fullMatch("/RGB")) { //

      int pixelByte = 0;
      //first 24 pixels with the first 72 bytes
      while (pixelByte < 24 * 3) {
        int r = oscMessage.getInt(pixelByte);
        int g = oscMessage.getInt(pixelByte + 1);
        int b = oscMessage.getInt(pixelByte + 2);
        strip.setPixelColor(pixelByte / 3, strip.Color(r, g, b));
        pixelByte += 3;
      }
      //next 3 bytes define the LED ring color
      int r = oscMessage.getInt(pixelByte);
      int g = oscMessage.getInt(pixelByte + 1);
      int b = oscMessage.getInt(pixelByte + 2);

      //set all 24 ws2812 pixels to the a sigle color
      for(int i = 0; i < 24; i++){
        strip.setPixelColor(24 + i, strip.Color(r, g, b));
      }
      strip.show(); // Initialize all pixels to 'off'
      //last byte address number 75
      int PWM = oscMessage.getInt(pixelByte + 3); //last BYTE is PWM
      analogWrite(PWMPIN, 255 - PWM);
    }

  }

  //sampling every 1s
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    switch (nState) {
      case CONNECT: //send IP to connect and wait for income message

        if (digitalRead(HALLSENSOR) == 0) {
          Serial.println("Trying to connect...");
          sendMessage("/connect", WiFi.localIP().toString());
          Serial.println("PRESSED");
          uint16_t i, j;

          for (j = 0; j < 256; j++) {
            for (i = 0; i < strip.numPixels(); i++) {
              strip.setPixelColor(i, Wheel((i + j) & 255));
            }
            strip.show();
            delay(10);
          }
          strip.clear();
          strip.show(); // Initialize all pixels to 'off'

        }
        break;
      case WAIT:
        Serial.println("Waiting...");
        break;
      case DISCONNECT:
        Serial.println("Disconnecting...");
        //sendMessage("/disconnect", WiFi.localIP().toString());
        nState = CONNECT; //back to wait
        break;
    }
  }
}


void sendMessage(String address, String data) {
  OSCMessage msg(address.c_str());
  msg.add(data.c_str());
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

