#ifndef PWMNode_h
#define PWMNode_h

#include "PWMNode.h"

class PWMNode
{
  public:
    PWMNode();
    void update(int value);
    void setPin(int pin);

  private:
    int _value;
    int _pin;
};

#endif
