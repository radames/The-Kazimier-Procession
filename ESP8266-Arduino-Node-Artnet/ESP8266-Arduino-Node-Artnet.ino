#include <Adafruit_NeoPixel.h>
#include <Artnet.h>
#include "WifiPass.h"

#define HALLSENSOR 5 //GPIO5 - PIN D1
#define LEDRINGPIN 4 //GPIO4 - PIN D2
#define PWMPIN 0     //GPIO0 -  PIN D3
#define NUMPIXELS 48 //48 ws2812 RGB Pixels
#define CHANNELS 3 // channels
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, LEDRINGPIN, NEO_GRB + NEO_KHZ800);

ArtnetReceiver artnet;
uint32_t universe1 = 0;
uint32_t universe2 = 1;

unsigned int ledState = HIGH; // LOW means led is *on*
unsigned int pwmValue = 255;

void updateLeds(uint8_t *data, uint16_t size)
{
  // you can also use pre-defined callbacks

  for (int led = 0; led < NUMPIXELS; led++)
  {
    strip.setPixelColor(led, data[led * CHANNELS], data[led * CHANNELS + 1], data[led * CHANNELS + 2]);
  }
  strip.show();
}
void updatePWM(uint8_t *data, uint16_t size)
{
  analogWrite(PWMPIN, data[0]);
}

void turnOffLights()
{
  strip.clear();
  strip.show(); // Initialize all pixels to 'off'
  pwmValue = 255;
  analogWrite(PWMPIN, pwmValue); //inverted PWM, starting OFF 255
}

void ledPatternMode(boolean wifi)
{
  //rotating,pulsing yellowish fire color
  int green = abs(sin(millis() / 20 * PI / 180)) * 30;
  int whiteP = int(millis() / 90) % 24;

  //first 24 leds top LED ring
  for (int i = 0; i < 24; i++)
  {
    if (!wifi)
    {
      strip.setPixelColor(i % 24, strip.Color(255, green, 0)); //first ring
      strip.setPixelColor(24 + i, strip.Color(255, green, 0)); //second ring is mirrored
    }
    else
    {
      strip.setPixelColor(i, strip.Color(50 + green * 6, 0, 0));      //first ring
      strip.setPixelColor(24 + i, strip.Color(50 + green * 6, 0, 0)); //second ring
    }
  }
  if (!wifi)
  {
    for (int i = 0; i < 2; i++)
    {
      strip.setPixelColor((whiteP + 12 * i) % 24, strip.Color(green * 8, green * 8, green * 8));      //first ring
      strip.setPixelColor(24 + (whiteP + 12 * i) % 24, strip.Color(green * 8, green * 8, green * 8)); //second ring
    }
  }

  //turn off PWM pin only once
  if (pwmValue != 255)
  {
    pwmValue = 255;
    analogWrite(PWMPIN, pwmValue); //inverted PWM, starting OFF 255
  }

  strip.show(); //update leds
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, ledState); // turn *on* led

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
  // WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    ledPatternMode(false);
    delay(10);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  artnet.begin();

  // if Artnet packet comes to this universe, this function (lambda) is called
  artnet.subscribe(universe1, updateLeds);
  // you can also use pre-defined callbacks
  artnet.subscribe(universe2, updatePWM);
  turnOffLights();
}

void loop()
{
  //if not WIFI present, wait with signal
  if (WiFi.status() != WL_CONNECTED)
  {

    ledPatternMode(false);
    delay(10);
  }
  else
  {

    artnet.parse(); // check if artnet packet has come and execute callback
  }
}
