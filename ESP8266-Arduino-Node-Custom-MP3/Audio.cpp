#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include "Audio.h"

int Audio::_id = 1;
Audio::Audio()
{
  _trackid = _id++;
  _value = 0;
  _isPlaying = false;
}
void Audio::start(DFRobotDFPlayerMini &DFPlayer){
  _DFPlayer = &DFPlayer;
}
void Audio::update(int value)
{
  _value = value;
  if(value >= 254){
    _DFPlayer->play(_trackid);

  }else if(value >= 224 && value <= 254){
    _DFPlayer->volume(value - 224);
  
  }else if(value >= 200 && value <= 223){
    _DFPlayer->stop();
  }
}

