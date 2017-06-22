#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"
#include "Audio.h"
#include "PWMNode.h"

#include "WifiPass.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

#define HALLSENSOR 4 //GPIO4 D2

//number of audio tracks to be played
#define AUDIO_TRACKS 3
Audio tracks[AUDIO_TRACKS];

//pwm pins, d1
//esp8266 GPIO5
#define NUM_PWMS 1
PWMNode pwmNodes[NUM_PWMS];
const unsigned int pwm_pins[NUM_PWMS] = {5};

// Software Serial communication with MP3 player
SoftwareSerial vSerial(14, 12, false, 256);  // wemos pins D5(RX) and D6(TX)
DFRobotDFPlayerMini myDFPlayer; //DFmp3 player instance

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
IPAddress outIp;  // remote IP (not needed for receive) kronosServer static IP

const unsigned int inPort = 8888;
const unsigned int outPort = 9999;

OSCErrorCode error;
unsigned int ledState = HIGH;              // LOW means led is *on*
//node s
enum NodeState { CONNECT, WAIT, DISCONNECT };

NodeState nState = CONNECT;
long lastMillis = 0;

void setup() {
  outIp.fromString(serverIP); //server ip from config file
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, ledState);    // turn *on* led

  analogWriteRange(255);
  analogWriteFreq(200);

  //seting up pwm pins
  for (int i = 0; i < NUM_PWMS; i++) {
    pwmNodes[i].setPin(pwm_pins[i]);
  }
  pinMode(HALLSENSOR, INPUT_PULLUP);


  Serial.begin(115200);
  vSerial.begin(9600);

   if (!myDFPlayer.begin(vSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  myDFPlayer.volume(30);  //Set volume value. From 0 to 30
    //setting up audio objects
  for (int i = 0; i < AUDIO_TRACKS; i++) {
    tracks[i].start(myDFPlayer);
  }
  
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
        sendMessage("/alive", WiFi.macAddress(), String(NUM_PWMS + AUDIO_TRACKS));
        nState = WAIT;
        digitalWrite(BUILTIN_LED, 0); //ON LED back to ON
        turnOffLights();

      } else if (oscMessage.fullMatch("/PWMS")) {
        //overrides Magnet state, in case of the assignement is already made and kept on server
        if (nState != WAIT) {
          nState = WAIT;
        }
        //get pwm bytes
        int oscCount = 0;
        for (int i = 0; i < NUM_PWMS; i++) {
          pwmNodes[i].update( 255 - oscMessage.getInt(oscCount));
          oscCount++;
        }
        //get MP3 Tracks bytes
        for (int i = 0; i < AUDIO_TRACKS; i++) {
          tracks[i].update(oscMessage.getInt(oscCount)); //Audio track byte
          oscCount++;
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
            sendMessage("/connect", WiFi.macAddress(), String(NUM_PWMS + AUDIO_TRACKS));
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
    pwmNodes[i].update(255);
  }
}


void sendMessage(String address, String data, String info) {
  OSCMessage msg(address.c_str());
  msg.add(data.c_str());
  if(info.length() > 0){
    msg.add(info.c_str());
  }
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void ledPatternMode(boolean wifi) {
  int power;
  if (!wifi) {
    power = abs(sin(millis() / 5 * PI / 180)) * 255;
  } else {
    power = (sin(millis() / 5 * PI / 180)) > 0?255:0;
  }
  for (int i = 0; i < NUM_PWMS; i++) {
    pwmNodes[i].update(power);
  }
}

