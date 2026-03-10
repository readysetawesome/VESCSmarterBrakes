#include "Arduino.h"
#include <EEPROM.h>
#include "VESCSmarterBrakes.h"
#ifdef USE_SOFTPWM
#include <SoftPWM.h>
#endif

VESCSmarterBrakes::VESCSmarterBrakes(int dimmerPin, int buttonPin, bool useSoftPWM) {
  pinMode(dimmerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  _dimmerPin = dimmerPin;
  _buttonPin = buttonPin;
  _useSoftPWM = useSoftPWM;

  _mode = MODE_STEADY;
  _idling = false;
  _brakeActive = false;
  _startupSplashRate = 1;
  _startupSplashDelay = 10;
  _lastDebounceTime = 0;
}

void VESCSmarterBrakes::TurnOn() {
  if (_useSoftPWM) {
    // Register pin with SoftPWM and configure fade times for all transitions.
    // Subsequent SoftPWMSet calls (via SetDimmerPower) fade automatically.
    // Brake/strobe activation bypasses the fade via hardset (immediate=true).
    SetDimmerPower(HIGH_POWER, true);
#ifdef USE_SOFTPWM
    SoftPWMSetPolarity(_dimmerPin, SOFTPWM_INVERTED);  // checkval=0 → pin HIGH → perfect off
    SoftPWMSetFadeTime(_dimmerPin, 300, 300);
#endif
    delay(150);
    SetDimmerPower(OFF);          delay(350);
    SetDimmerPower(MEDIUM_POWER); delay(350);
    SetDimmerPower(OFF);          delay(350);
    SetDimmerPower(MEDIUM_POWER); delay(350);
    SetDimmerPower(IDLE_POWER);   delay(350);
#ifdef USE_SOFTPWM
    SoftPWMSetFadeTime(_dimmerPin, 0, 0);  // restore instant response after animation
#endif
  } else {
    TransitionBrightness(HIGH_POWER, OFF);
    TransitionBrightness(OFF, MEDIUM_POWER);
    TransitionBrightness(MEDIUM_POWER, OFF);
    TransitionBrightness(OFF, MEDIUM_POWER);
    TransitionBrightness(MEDIUM_POWER, IDLE_POWER);
  }

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
    factor = -5;
  } else {
    factor = 5;
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

void VESCSmarterBrakes::SetDimmerPower(int value, bool immediate) {
  if (_dimmerPower == NULL || _dimmerPower != value) {
    _dimmerPower = value;
    if (_useSoftPWM) {
      // SOFTPWM_INVERTED polarity + 255-value inversion:
      //   OFF=255   → checkval=0   → pin always HIGH → perfect off (no glow)
      //   HIGH_POWER=0 → checkval=255 → pin ~100% LOW → full brightness
      // immediate=true bypasses SoftPWMSetFadeTime for instant response (brakes, strobe).
#ifdef USE_SOFTPWM
      SoftPWMSet(_dimmerPin, 255 - value, immediate ? 1 : 0);
#endif
    } else {
      analogWrite(_dimmerPin, value);
    }
  }
}

void VESCSmarterBrakes::ApplyStrobe() {
  if (_mode == MODE_STROBE && !_brakeActive && !_idling) {
    if (_strobeLastCycledOn == NULL || millis() - _strobeLastCycledOn > 400) {
      _strobeLastCycledOn = millis();
      SetDimmerPower(HIGH_POWER, true);   // immediate — strobe must snap, not fade
    } else if (millis() - _strobeLastCycledOn > 200) {
      SetDimmerPower(IDLE_POWER, true);   // immediate
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
        SetDimmerPower(HIGH_POWER, true);  // immediate — brake response must be instant
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
        if (_useSoftPWM) {
          SetDimmerPower(OFF);  // fades via SoftPWMSetFadeTime
        } else {
          TransitionBrightness(LOW_POWER, OFF);
        }
        _lightOff = true;
        delay(5000);
      }
    } else if (_lightOff) {
      TurnOn();
    }
  }
}
