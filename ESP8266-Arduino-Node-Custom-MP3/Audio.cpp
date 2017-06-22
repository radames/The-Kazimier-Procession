#include "Arduino.h"
#include "Audio.h"

Audio::Audio()
{
  _value = 0;
}

void Audio::update(int value)
{
  _value = value;
}
