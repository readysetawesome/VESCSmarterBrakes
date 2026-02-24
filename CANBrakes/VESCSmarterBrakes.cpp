// Proxy — Arduino IDE compiles .cpp files in the sketch folder in-place,
// so the relative include below resolves correctly to the project root.
// USE_SOFTPWM switches SetDimmerPower to SoftPWMSet (pins 0/1 have no
// hardware PWM on the ATmega168PA).
#define USE_SOFTPWM
#include "../VESCSmarterBrakes.cpp"
