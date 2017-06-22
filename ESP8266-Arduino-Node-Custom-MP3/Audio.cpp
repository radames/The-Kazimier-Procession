#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include "Audio.h"

int Audio::_id = 1;
Audio::Audio()
{
  _trackid = _id++;
  _value = 0;
  _isPlaying = false;
  _volume = 30;
}
void Audio::start(DFRobotDFPlayerMini &DFPlayer){
  _DFPlayer = &DFPlayer;
}
void Audio::update(int value)
{
  _value = value;
  if(value >= 254){
    _DFPlayer->play(_trackid);
    Serial.println("Playing: " + String(_trackid));
  }
}

void Audio::setVolume(int volume){
  _volume = volume;
  _DFPlayer->volume(volume);
}

