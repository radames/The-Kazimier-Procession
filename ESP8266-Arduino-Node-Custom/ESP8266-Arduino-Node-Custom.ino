#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include "WifiPass.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

#define HALLSENSOR 14 //GPIO14 D5

//pwm pins, d1,d2,d3,d4
//esp8266, 5,4,0  
#define NUM_PWMS 3
const unsigned int pwm_pins[NUM_PWMS] = {5, 4, 0};
unsigned int pwmValue[NUM_PWMS] = {0, 0, 0};

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

  //seting up pwm pins
  for (int i = 0; i < NUM_PWMS; i++) {
    analogWrite(pwm_pins[i], 0);
    pwmValue[i] = 0;
  }
  pinMode(HALLSENSOR, INPUT_PULLUP);


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

      } else if (oscMessage.fullMatch("/PWMS")) {
        //overrides Magnet state, in case of the assignement is already made and kept on server
        if (nState != WAIT) {
          nState = WAIT;
        }
        //get pwm bytes
        for (int i = 0; i < NUM_PWMS; i++) {
          analogWrite(pwm_pins[i], oscMessage.getInt(i));
        }
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
            sendMessage("/connectPWM", WiFi.macAddress());
            Serial.println("MAGNET DETECTED");
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
  //turn off all PWMS
  for (int i = 0; i < NUM_PWMS; i++) {
    analogWrite(pwm_pins[i], 0);
    pwmValue[i] = 0;
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

void ledPatternMode(boolean wifi) {
  int power;
  if (!wifi) {
    power = abs(sin(millis() / 10 * PI / 180)) * 100;
  } else {
    power = abs(sin(millis() / 20 * PI / 180)) * 255;
  }
  for (int i = 0; i < NUM_PWMS; i++) {
    analogWrite(pwm_pins[i], power);
  }
}

