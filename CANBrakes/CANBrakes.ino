#include <SPI.h>
#include <mcp_can.h>
#include <SoftPWM.h>
#include "VESCSmarterBrakes.h"
#include "vesc_can.h"

// ── Pin assignments ─────────────────────────────────────────────────────────
// CS pin confirmed from board firmware source (cmdMode.cpp: MCP_CAN CAN(9))
#define CAN_CS_PIN  9

// Grove connector pins repurposed as GPIO (hardware UART no longer used)
#define DIMMER_PIN  0    // RX connector pin → dimmer output via SoftPWM
#define BUTTON_PIN  1    // TX connector pin → mode button input

#define LED_PIN     3    // Onboard status LED (active HIGH)

// ── Objects ─────────────────────────────────────────────────────────────────
MCP_CAN CAN(CAN_CS_PIN);
VESCSmarterBrakes Brakes(DIMMER_PIN, BUTTON_PIN);

// Default inpVoltage to a safe value so low-voltage protection
// doesn't fire before the first Status 5 frame arrives.
VescCanData vescData = {0, 0.0f, 48.0f};

void setup() {
    pinMode(LED_PIN, OUTPUT);

    // SoftPWM must be initialised before Brakes.TurnOn() calls SetDimmerPower()
    SoftPWMBegin();

    // Init CAN at 500 kbps — standard VESC CAN rate.
    // Blink LED rapidly if init fails (check wiring / CS pin).
    while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(200);
    }
    CAN.setMode(MCP_NORMAL);

    // Solid LED = CAN init OK
    digitalWrite(LED_PIN, HIGH);

    Brakes.TurnOn();
}

void loop() {
    bool newData = false;

    if (CAN.checkReceive() == CAN_MSGAVAIL) {
        unsigned char len = 0;
        unsigned char buf[8];
        CAN.readMsgBuf(&len, buf);
        unsigned long id = CAN.getCanId();
        newData = parseVescFrame(id, buf, vescData);
    }

    Brakes.DoLoop(vescData.rpm, vescData.avgMotorCurrent, vescData.inpVoltage, newData);
}
