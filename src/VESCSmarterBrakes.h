#ifndef VESCSmarterBrakes_h
#define VESCSmarterBrakes_h

#include "Arduino.h"

#define OFF 255
#define IDLE_POWER 230
#define LOW_POWER 190
#define MEDIUM_POWER 95
#define HIGH_POWER 0
#define BRAKE_RELEASE_DEBOUNCE 225
#define BRAKE_IDLE_CHILL_TIMER 30000
#define DEFAULT_COMM_BAUD 250000
#define MODE_STROBE 0
#define MODE_STEADY 1
#define MODE_LOW 2
#define MODE_OFF 3
#define LAST_MODE 3
#define BUTTON_PRESSED 0
#define BUTTON_OFF 1
#define MODE_EEPROM_ADDRESS 128

class VESCSmarterBrakes
{
  public:
    VESCSmarterBrakes(int dimmerPin, int buttonPin);
    void TurnOn();
    void DoLoop();
  private:
    void ReadMode();
    void ApplyMode();
    void ApplyStrobe();
    void SetDimmerPower(int value);
    void CycleMode();
    void TransitionBrightness(int dStart, int dStop);
    int _dimmerPin;
    int _buttonPin;
    int _startupSplashRate;
    unsigned long _startupSplashDelay;
    unsigned long _loopStartMillis;
    int _mode;
    int _dimmerPower;
    int _lastButtonValue;
    int _buttonValue;
    unsigned long _strobeLastCycledOn;
    uint32_t _loopsInTarget;
    unsigned long _idleSince;
    unsigned long _brakeReleasingFrom;
    unsigned long _lastDebounceTime;

    bool _lightOff;
    bool _idling;
    bool _brakeActive;
};

#endif
