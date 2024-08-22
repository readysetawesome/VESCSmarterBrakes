#include <VESCSmarterBrakes.h>

// Pass in dimmer pin, mode switch pin
VESCSmarterBrakes Brakes(3, 9);

void setup() {
  Serial.begin(115200);
  while (!Serial) {;}
  Brakes.SetSerial(&Serial)
  Brakes.TurnOn();
}

void loop() {
  Brakes.DoLoop();
  delay(10);
}
