#include "Arduino.h"
#include "PWMNode.h"

PWMNode::PWMNode()
{
  _value = 0;
  _pin = 0;
  _invpwm = false;
}
void PWMNode::update(int value)
{
  _value = value;
  if(_invpwm == true){
    value =  255 - value;
  }
  analogWrite(_pin, value);
}
void PWMNode::setPin(int pin){
  _pin = pin;
  update(0); //start inverted
}
void PWMNode::setPWM(bool invpwm){
  _invpwm = invpwm;
}

