#ifndef VESC_CAN_H
#define VESC_CAN_H

#include <stdint.h>

// ── VESC configuration ──────────────────────────────────────────────────────
// Match this to your VESC's CAN ID (VESC Tool: App Config → General → Controller ID)
#define VESC_CONTROLLER_ID  0

// ── VESC CAN packet types ───────────────────────────────────────────────────
#define CAN_PACKET_STATUS    0x09   // ERpm, Current x10, Duty
#define CAN_PACKET_STATUS_5  0x1B   // Tacho, Input voltage x10

// ── Data struct ─────────────────────────────────────────────────────────────
struct VescCanData {
    int32_t rpm;
    float   avgMotorCurrent;
    float   inpVoltage;
};

// Returns true only when Status 1 (braking-relevant) data was updated.
// Status 5 (voltage) is parsed silently as a side effect.
inline bool parseVescFrame(unsigned long id, unsigned char *buf, VescCanData &out) {
    // VESC CAN ID format: controller_id | (packet_type << 8)
    unsigned char ctrlId  = id & 0xFF;
    unsigned char pktType = (id >> 8) & 0xFF;

    if (ctrlId != VESC_CONTROLLER_ID) return false;

    if (pktType == CAN_PACKET_STATUS) {
        out.rpm = (int32_t)(
            (uint32_t)buf[0] << 24 |
            (uint32_t)buf[1] << 16 |
            (uint32_t)buf[2] << 8  |
            (uint32_t)buf[3]
        );
        out.avgMotorCurrent = (int16_t)((buf[4] << 8) | buf[5]) / 10.0f;
        return true;
    }

    if (pktType == CAN_PACKET_STATUS_5) {
        out.inpVoltage = (int16_t)((buf[4] << 8) | buf[5]) / 10.0f;
    }

    return false;
}

#endif
