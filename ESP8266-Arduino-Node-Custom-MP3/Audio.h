#ifndef Audio_h
#define Audio_h

#include "Audio.h"

class Audio
{
  public:
    Audio();
    void update(int value);
    void setVolume(int value);
    void start(DFRobotDFPlayerMini &DFPlayer);
  private:
    static int _id;
    int _trackid;
    DFRobotDFPlayerMini* _DFPlayer;
    int _value;
    int _volume;
    bool _isPlaying;
};

#endif
