#include <SoftPWM_timer.h>
#include <SoftPWM.h>
#include <SPI.h>
#include <mcp_can.h>
#include <VESCSmarterBrakes.h>
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
// true = use SoftPWM, required because pins 0/1 have no hardware PWM
VESCSmarterBrakes Brakes(DIMMER_PIN, BUTTON_PIN, true);

// Default inpVoltage to a safe value so low-voltage protection
// doesn't fire before the first Status 5 frame arrives.
VescCanData vescData = {0, 0.0f, 48.0f};

void setup() {
    pinMode(LED_PIN, OUTPUT);

    // SoftPWM must be initialised before Brakes.TurnOn() calls SetDimmerPower()
    SoftPWMBegin();

    // Init CAN at 500 kbps — standard VESC CAN rate.
    while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK) {
        delay(200);
    }
    CAN.setMode(MCP_NORMAL);
    digitalWrite(LED_PIN, HIGH);  // LED on = CAN init OK

    Brakes.TurnOn();
}

void loop() {
    bool newData = false;

    if (CAN.checkReceive() == CAN_MSGAVAIL) {
        unsigned long id;
        unsigned char len = 0;
        unsigned char buf[8];
        CAN.readMsgBuf(&id, &len, buf);
        newData = parseVescFrame(id, buf, vescData);
    }

    Brakes.DoLoop(vescData.rpm, vescData.avgMotorCurrent, vescData.inpVoltage, newData);
}
