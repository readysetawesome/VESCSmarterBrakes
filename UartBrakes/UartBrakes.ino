#include <VESCSmarterBrakes.h>

// Pass in dimmer pin, mode switch pin
VESCSmarterBrakes Brakes(3, 9);


void setup() {
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)

  Serial1.begin(115200);
  while (!Serial1) {;}
  Brakes.SetSerial(&Serial1);

  #endif

  #if defined(__AVR_ATmega328P__)

  Serial.begin(115200);
  while (!Serial) {;}
  Brakes.SetSerial(&Serial);

  #endif

  Brakes.TurnOn();
}

void loop() {
  Brakes.DoLoop();
  delay(10);
}
