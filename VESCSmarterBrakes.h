#ifndef VESCSmarterBrakes_h
#define VESCSmarterBrakes_h

#include "Arduino.h"
#include "VescUart.h"

#define OFF 255
#define IDLE_POWER 195
#define RUN_POWER 115
#define LOW_POWER 80
#define HIGH_POWER 0
#define BRAKE_RELEASE_DEBOUNCE 225
#define BRAKE_IDLE_CHILL_TIMER 30000
#define DEFAULT_COMM_BAUD 250000
#define MODE_STROBE 0
#define MODE_STEADY 1
#define MODE_LOW 2
#define MODE_OFF 3
#define BUTTON_PRESSED 0
#define BUTTON_OFF 1

class VESCSmarterBrakes
{
  public:
    VESCSmarterBrakes(int dimmerPin, int buttonPin);
		void TurnOn();
		void DoLoop();
  private:
		void ReadMode();
		void ApplyStrobe();
		void SetDimmerPower(int value);
		void ChangeMode();
		void TransitionBrightness(int dStart, int dStop);
    int _dimmerPin;
    int _buttonPin;
		int _startupSplashRate;
		unsigned long _startupSplashDelay;
		uint32_t _loopStartMillis;
		int _mode;
		int _currentPower;
		int _lastButtonValue;
		int _buttonValue;
		uint32_t _strobeLastCycledOn;
		uint32_t _loopsInTarget;
		uint32_t _idleSince;
		uint32_t _brakeReleasingFrom;
		uint32_t _lastDebounceTime;
		VescUart UART;
		bool _lightOff;
		bool _idleMode;
		bool _brakeActive;
		bool _buttonUnpressed;
};

#endif
