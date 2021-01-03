#include "Arduino.h"
#include "VescUart.h"
#include "VESCSmarterBrakes.h"



VESCSmarterBrakes::VESCSmarterBrakes(int dimmerPin, int buttonPin) {
  pinMode(dimmerPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  _dimmerPin = dimmerPin;
  _buttonPin = buttonPin;
  SetDimmerPower(IDLE_POWER);
  /** Setup UART port */
  Serial.begin(DEFAULT_COMM_BAUD);

  /** Define which ports to use as UART */
  UART.setSerialPort(&Serial);
  _mode = MODE_STROBE;
  _lightOff = false;
  _idleMode = false;
  _brakeActive = false;
  _buttonUnpressed = true;
  _startupSplashRate = 1;
  _startupSplashDelay = 2;
  _lastDebounceTime = 0;
}

void VESCSmarterBrakes::TurnOn() {
  _lightOff = false;
  
  // pulse animation on startup
  TransitionBrightness(HIGH_POWER, OFF);
  TransitionBrightness(OFF, LOW_POWER);
  TransitionBrightness(LOW_POWER, OFF);
  TransitionBrightness(OFF, LOW_POWER);
  TransitionBrightness(LOW_POWER, IDLE_POWER); 
  TransitionBrightness(IDLE_POWER, RUN_POWER);   
}

void VESCSmarterBrakes::TransitionBrightness(int dStart, int dStop) {
  int factor;
  int loopStart = dStart/_startupSplashRate;
  int loopEnd = dStop/_startupSplashRate;

  if (dStart > dStop) {
    factor = -1;
  } else {
    factor = 1;
  }
  
  for (int i=loopStart; i != loopEnd; i+=factor ) {
    SetDimmerPower(i*_startupSplashRate);
    delay(_startupSplashDelay);
  }

  SetDimmerPower(dStop);
}


void VESCSmarterBrakes::ReadMode() {
  int sensorValue = digitalRead(_buttonPin);
  if (sensorValue != _lastButtonValue) {
    _lastDebounceTime = millis();
  }
  if (millis() - _lastDebounceTime > 50) {
    if (sensorValue != _buttonValue) {
      _buttonValue = sensorValue;
      if (_buttonValue == LOW) {
        ChangeMode();
      }
    }
  }
  _lastButtonValue = sensorValue;
}


void VESCSmarterBrakes::ChangeMode() {
  if (_mode == MODE_STROBE && !_brakeActive) {
    SetDimmerPower(LOW_POWER);
  }
  _mode++;
  if (_mode > MODE_OFF) {
    _mode = MODE_STROBE;
  }
}

void VESCSmarterBrakes::SetDimmerPower(int value) {
  if (_currentPower == NULL || _currentPower != value) {
    _currentPower = value;
    analogWrite(_dimmerPin, value);
  }
}

void VESCSmarterBrakes::ApplyStrobe() {
  if (_mode == MODE_STROBE && !_brakeActive) {
    if (_strobeLastCycledOn == NULL || millis() - _strobeLastCycledOn > 400) {
      _strobeLastCycledOn = millis();
      SetDimmerPower(LOW_POWER);
    } else if (millis() - _strobeLastCycledOn > 200) {
      SetDimmerPower(IDLE_POWER);
    }
  }
}

void VESCSmarterBrakes::DoLoop() {
  int commReqDelayTime;
  float systemVoltage;
  
  if (_loopStartMillis != NULL) {
    commReqDelayTime = 15-(millis()-_loopStartMillis);
  }
  
  ReadMode();
  ApplyStrobe();

  if (_mode == MODE_OFF) {
    SetDimmerPower(OFF);
  } else if (commReqDelayTime <= 0 && UART.getVescValues()) {
    _loopStartMillis = millis();
    systemVoltage = UART.data.inpVoltage;

    int32_t RPM;
    float I;
  
    RPM = UART.data.rpm;
    I = UART.data.avgMotorCurrent;

    if (I < -12 && RPM > 100 ) {
      // This looks like brakes. wait for 4 consecutive confirmations within the target ranges
      _loopsInTarget++;
      _idleSince = NULL;
    
      if (!_brakeActive && _loopsInTarget > 3) {
        _brakeActive = true;
        SetDimmerPower(HIGH_POWER);
        _brakeReleasingFrom = NULL;
      }
    } else {
      // Not in braking ranges, reset the counter
      _loopsInTarget = 0;
    
      if (I == 0 && abs(RPM) < 10) {
        // Very low/zero rpm with zero current is idle state, mark time at the beginning of idle period
        if (_idleSince == NULL) {
          _idleSince = _loopStartMillis;
        }
      } else {
        // Not braking but not idling either. reset idle timer and start releasing if active
        _idleSince = NULL;
        if (_brakeActive) {
          _brakeActive = false;
          _brakeReleasingFrom = _loopStartMillis;
        }
      }
    }  
  
    if (_brakeReleasingFrom != NULL && (_loopStartMillis - _brakeReleasingFrom > BRAKE_RELEASE_DEBOUNCE)) {
      // the brake release debounce period has passed, reset it
      _brakeReleasingFrom = NULL;

      // honor the debounce by first making sure subsequent event didn't reactivate brake
      if (!_brakeActive) {
        // More than debounce ms have passed since ANY braking event, return to run power
        SetDimmerPower(RUN_POWER);
      }
    }
    if (_idleSince == NULL) {
      if (_idleMode) {
        _idleMode = false;
        SetDimmerPower(RUN_POWER);
      }
    } else if (!_idleMode && ((_loopStartMillis - _idleSince) > BRAKE_IDLE_CHILL_TIMER)) {
      _idleMode = true;
      SetDimmerPower(IDLE_POWER);
    }
    if (systemVoltage != NULL) {
      if (systemVoltage < 22.5) {
        if (!_lightOff) {
          TransitionBrightness(RUN_POWER, OFF);
          _lightOff= true;
        }
      } else if (_lightOff) {
        TurnOn();
      }
    }
  }
}