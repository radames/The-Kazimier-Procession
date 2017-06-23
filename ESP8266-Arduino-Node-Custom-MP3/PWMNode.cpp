#include "Arduino.h"
#include "PWMNode.h"

PWMNode::PWMNode()
{
  _value = 0;
  _pin = 0;
}

void PWMNode::update(int value)
{
  _value = value;
  analogWrite(_pin, value);
}
void PWMNode::setPin(int pin){
  _pin = pin;
  update(0); //start inverted
}

