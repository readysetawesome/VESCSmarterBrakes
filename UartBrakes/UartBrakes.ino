#include <VescUart.h>
#include "../VESCSmarterBrakes.h"

// Dimmer pin, mode button pin
VESCSmarterBrakes Brakes(3, 9);
VescUart UART;

void setup() {
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
  Serial1.begin(115200);
  UART.setSerialPort(&Serial1);
  #elif defined(__AVR_ATmega328P__)
  Serial.begin(115200);
  UART.setSerialPort(&Serial);
  #endif

  Brakes.TurnOn();
}

void loop() {
  static unsigned long lastQuery = 0;
  bool newData = false;

  if (millis() - lastQuery >= 15) {
    lastQuery = millis();
    if (UART.getVescValues()) {
      newData = true;
    }
  }

  Brakes.DoLoop(UART.data.rpm, UART.data.avgMotorCurrent, UART.data.inpVoltage, newData);
}
