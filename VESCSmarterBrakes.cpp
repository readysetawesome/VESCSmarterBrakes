#include "Arduino.h"
#include <EEPROM.h>
#include "VESCSmarterBrakes.h"

VESCSmarterBrakes::VESCSmarterBrakes(int dimmerPin, int buttonPin) {
  pinMode(dimmerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  _dimmerPin = dimmerPin;
  _buttonPin = buttonPin;

  _mode = MODE_STEADY;
  _idling = false;
  _brakeActive = false;
  _startupSplashRate = 1;
  _startupSplashDelay = 2;
  _lastDebounceTime = 0;
}

void VESCSmarterBrakes::TurnOn() {
  // pulse animation on startup
  TransitionBrightness(HIGH_POWER, OFF);
  TransitionBrightness(OFF, MEDIUM_POWER);
  TransitionBrightness(MEDIUM_POWER, OFF);
  TransitionBrightness(OFF, MEDIUM_POWER);
  TransitionBrightness(MEDIUM_POWER, IDLE_POWER);

  // apply saved mode from EEPROM
  int savedMode = EEPROM.read(MODE_EEPROM_ADDRESS);
  if (savedMode != NULL && savedMode <= LAST_MODE) {
    _mode = savedMode;
    ApplyMode();
  }
  _lightOff = false;
}

void VESCSmarterBrakes::TransitionBrightness(int dStart, int dStop) {
  int factor;
  int loopStart = dStart / _startupSplashRate;
  int loopEnd   = dStop  / _startupSplashRate;

  if (dStart > dStop) {
    factor = -1;
  } else {
    factor = 1;
  }

  for (int i = loopStart; i != loopEnd; i += factor) {
    SetDimmerPower(i * _startupSplashRate);
    delay(_startupSplashDelay);
  }

  SetDimmerPower(dStop);
}

void VESCSmarterBrakes::ReadMode() {
  int sensorValue = digitalRead(_buttonPin);
  if (sensorValue != _lastButtonValue) {
    _lastDebounceTime = millis();
  }
  if (millis() - _lastDebounceTime > 70) {
    if (sensorValue != _buttonValue) {
      _buttonValue = sensorValue;
      if (_buttonValue == LOW) {
        CycleMode();
      }
    }
  }
  _lastButtonValue = sensorValue;
}

void VESCSmarterBrakes::CycleMode() {
  _mode++;
  if (_mode > LAST_MODE) {
    _mode = 0;
  }
  EEPROM.write(MODE_EEPROM_ADDRESS, _mode);
  ApplyMode();
}

void VESCSmarterBrakes::ApplyMode() {
  if (!_brakeActive) {
    if (_mode == MODE_LOW) {
      SetDimmerPower(LOW_POWER);
    } else if (_mode == MODE_STROBE) {
      SetDimmerPower(HIGH_POWER);
    } else if (_mode == MODE_STEADY) {
      SetDimmerPower(MEDIUM_POWER);
    } else if (_mode == MODE_OFF) {
      SetDimmerPower(OFF);
    }
  }
}

void VESCSmarterBrakes::SetDimmerPower(int value) {
  if (_dimmerPower == NULL || _dimmerPower != value) {
    _dimmerPower = value;
#ifdef USE_SOFTPWM
    // SoftPWM: 0=off, 255=full on. Our constants are inverted (HIGH_POWER=0,
    // OFF=255) for N-channel MOSFET control, so invert here.
    SoftPWMSet(_dimmerPin, 255 - value);
#else
    analogWrite(_dimmerPin, value);
#endif
  }
}

void VESCSmarterBrakes::ApplyStrobe() {
  if (_mode == MODE_STROBE && !_brakeActive && !_idling) {
    if (_strobeLastCycledOn == NULL || millis() - _strobeLastCycledOn > 400) {
      _strobeLastCycledOn = millis();
      SetDimmerPower(HIGH_POWER);
    } else if (millis() - _strobeLastCycledOn > 200) {
      SetDimmerPower(IDLE_POWER);
    }
  }
}

void VESCSmarterBrakes::DoLoop(int32_t rpm, float current, float voltage, bool newData) {
  _loopStartMillis = millis();

  // Button and strobe run every iteration for responsiveness
  ReadMode();
  ApplyStrobe();

  // Braking logic only runs when fresh telemetry arrived this loop
  if (_mode != MODE_OFF && newData) {

    if (current < -12 && rpm > 100) {
      // Looks like braking — wait for 4 consecutive confirmations
      _loopsInTarget++;
      _idleSince = NULL;

      if (!_brakeActive && _loopsInTarget > 3) {
        _brakeActive = true;
        SetDimmerPower(HIGH_POWER);
        _brakeReleasingFrom = NULL;
      }
    } else {
      _loopsInTarget = 0;

      if (current == 0 && abs(rpm) < 10) {
        if (_idleSince == NULL) {
          _idleSince = _loopStartMillis;
        }
      } else {
        _idleSince = NULL;
        if (_brakeActive) {
          _brakeActive = false;
          _brakeReleasingFrom = _loopStartMillis;
        }
      }
    }

    if (_brakeReleasingFrom != NULL && (_loopStartMillis - _brakeReleasingFrom > BRAKE_RELEASE_DEBOUNCE)) {
      _brakeReleasingFrom = NULL;
      if (!_brakeActive) {
        ApplyMode();
      }
    }

    if (_idleSince == NULL) {
      if (_idling) {
        _idling = false;
        ApplyMode();
      }
    } else if (!_idling && ((_loopStartMillis - _idleSince) > BRAKE_IDLE_CHILL_TIMER)) {
      _idling = true;
      SetDimmerPower(IDLE_POWER);
    }

    if (voltage > 0 && voltage < 22.5) {
      if (!_lightOff) {
        TransitionBrightness(LOW_POWER, OFF);
        _lightOff = true;
        delay(5000);
      }
    } else if (_lightOff) {
      TurnOn();
    }
  }
}
