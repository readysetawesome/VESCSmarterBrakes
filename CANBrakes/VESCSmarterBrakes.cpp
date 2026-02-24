// Proxy — Arduino IDE only compiles .cpp files within the sketch folder.
// USE_SOFTPWM is defined here so SetDimmerPower uses SoftPWMSet instead of
// analogWrite — required because pins 0/1 have no hardware PWM.
#define USE_SOFTPWM
#include "../VESCSmarterBrakes.cpp"
