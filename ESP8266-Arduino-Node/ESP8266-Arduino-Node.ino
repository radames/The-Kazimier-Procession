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

#define HALLSENSOR 5 //GPIO5 - PIN D1
#define LEDRINGPIN 4  //GPIO4 - PIN D2
#define PWMPIN 0  //GPIO0 -  PIN D3
#define NUMPIXELS 48 //48 ws2812 RGB Pixels

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, LEDRINGPIN, NEO_GRB + NEO_KHZ800);
//

// A UDP instance to let us send and receive packets over UDP

WiFiUDP Udp;
const IPAddress outIp(192, 168, 20, 3);  // remote IP (not needed for receive) kronosServer static IP

const unsigned int inPort = 8888;
const unsigned int outPort = 9999;

OSCErrorCode error;
unsigned int ledState = HIGH;              // LOW means led is *on*
unsigned int pwmValue = 255;
//node s
enum NodeState { CONNECT, WAIT, DISCONNECT };

NodeState nState = CONNECT;
long lastMillis = 0;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, ledState);    // turn *on* led

  analogWriteRange(255);
  analogWriteFreq(200);

  analogWrite(PWMPIN, pwmValue); //inverted PWM, starting OFF 255

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

  while (WiFi.status() != WL_CONNECTED) {
    ledPatternMode(false);
    delay(10);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  Serial.println("Starting UDP");
  Udp.begin(inPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());

  turnOffLights();

}

void loop() {
  //if not WIFI present, wait with signal
  if (WiFi.status() != WL_CONNECTED) {

    ledPatternMode(false);
    delay(10);

  } else {

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
          turnOffLights();
        }
      } else if (oscMessage.fullMatch("/isAlive")) {
        //Respond alive if get this message
        //change the state to wait
        sendMessage("/alive", WiFi.macAddress());
        nState = WAIT;
        digitalWrite(BUILTIN_LED, 0); //ON LED back to ON
        turnOffLights();

      } else if (oscMessage.fullMatch("/RGB")) {
        //overrides Magnet state, in case of the assignement is already made and kept on server
        if (nState != WAIT) {
          nState = WAIT;
        }

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
        for (int i = 0; i < 24; i++) {
          strip.setPixelColor(24 + i, strip.Color(r, g, b));
        }
        strip.show(); // Initialize all pixels to 'off'
        //last byte address number 75
        int PWM = oscMessage.getInt(pixelByte + 3); //last BYTE is PWM
        pwmValue = 255 - PWM;
        analogWrite(PWMPIN, pwmValue);
      }

    }

    //sampling every 1s

    switch (nState) {
      case CONNECT: //send IP to connect and wait for income message

        if (digitalRead(HALLSENSOR) == 0) {
          //if magnet is present, send OSC every 500 until is connected
          if (millis() - lastMillis > 500) {
            lastMillis = millis();

            Serial.println("Trying to connect...");
            sendMessage("/connect", WiFi.macAddress());
            Serial.println("MAGNET DETECTED");
            uint16_t i, j;
            //magnet detected all leds to RED for 1s
            for (i = 0; i < strip.numPixels(); i++) {
              strip.setPixelColor(i, strip.Color(255, 0, 0));
            }
            strip.show();
            delay(500);

            strip.clear();
            strip.show(); // Initialize all pixels to 'off'
          }

        } else {
          //if magnet is not present, led pattern mode wifi wifi true
          ledPatternMode(true);
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

void turnOffLights() {
  strip.clear();
  strip.show(); // Initialize all pixels to 'off'
  pwmValue = 255;
  analogWrite(PWMPIN, pwmValue); //inverted PWM, starting OFF 255
}


void sendMessage(String address, String data) {
  OSCMessage msg(address.c_str());
  msg.add(data.c_str());
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void ledPatternMode(boolean wifi) {
  //rotating,pulsing yellowish fire color
  int green = abs(sin(millis() / 20 * PI / 180)) * 30;
  int whiteP = int(millis() / 90) % 24;

  //first 24 leds top LED ring
  for (int i = 0; i < 24; i++) {
    if (!wifi) {
      strip.setPixelColor(i % 24, strip.Color(255, green, 0)); //first ring
      strip.setPixelColor(24 + i, strip.Color(255, green, 0)); //second ring is mirrored
    } else {
      strip.setPixelColor(i, strip.Color(50 + green*6, 0, 0)); //first ring
      strip.setPixelColor(24 + i, strip.Color(50 + green*6, 0, 0)); //second ring
    }
  }
  if (!wifi) {
    for (int i = 0; i < 2; i++) {
      strip.setPixelColor((whiteP + 12 * i) % 24, strip.Color(green * 8, green * 8, green * 8)); //first ring
      strip.setPixelColor(24 + (whiteP + 12 * i) % 24, strip.Color(green * 8, green * 8, green * 8)); //second ring
    }
  }


  //turn off PWM pin only once
  if (pwmValue != 255) {
    pwmValue = 255;
    analogWrite(PWMPIN, pwmValue); //inverted PWM, starting OFF 255
  }

  strip.show();//update leds
}

