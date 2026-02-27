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
#define DIMMER_PIN  1    // TX connector pin → dimmer output via SoftPWM
#define BUTTON_PIN  0    // RX connector pin → mode button input

#define LED_PIN     3    // Onboard status LED (active HIGH)

// ── Objects ─────────────────────────────────────────────────────────────────
MCP_CAN CAN(CAN_CS_PIN);
// true = use SoftPWM, required because pins 0/1 have no hardware PWM
VESCSmarterBrakes Brakes(DIMMER_PIN, BUTTON_PIN, true);

// Default inpVoltage to a safe value so low-voltage protection
// doesn't fire before the first Status 5 frame arrives.
VescCanData vescData = {0, 0.0f, 48.0f};

void setup() {
    // Optiboot leaves UART enabled (RXEN0/TXEN0 set in UCSR0B), which overrides
    // GPIO function on pins 0 and 1. Disable UART before using those pins.
    UCSR0B = 0;

    pinMode(LED_PIN, OUTPUT);

    // Init CAN at 500 kbps — standard VESC CAN rate.
    while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK) {
        delay(200);
    }
    CAN.setMode(MCP_NORMAL);
    digitalWrite(LED_PIN, LOW);   // LED off = CAN init OK, waiting for VESC data

    // SoftPWM must be initialised before Brakes.TurnOn() calls SetDimmerPower()
    SoftPWMBegin();
    // SoftPWM frequency set to 120Hz in SoftPWM.cpp (SOFTPWM_FREQ 120UL).
    // Default 60Hz was visibly flickery; 120Hz is above threshold with acceptable overhead.

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
        if (newData) digitalWrite(LED_PIN, HIGH);
    }

    Brakes.DoLoop(vescData.rpm, vescData.avgMotorCurrent, vescData.inpVoltage, newData);
}
