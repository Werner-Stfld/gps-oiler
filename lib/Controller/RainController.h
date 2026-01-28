#pragma once

#include <Arduino.h>
#include "globals.h"
#include "userVar.h"

class RainController: public VarContainer {

  int _rainAverage = 0;
  unsigned long tmo = 0;
  int _remainingPulsesAfterRain;
  int _initAverageLoop;



  // lmiit the rainMultiplier between 1 and 5
  static float checkBoundsRainMulti (float v) {
    if (v < 1) {
      return 1;
    }
    if (v > 5) {
      return 5;
    }
    return v;
  };

  public:
    IntVar pulsesAfterRain = IntVar(5, PrefKeys::pulses_after_rain);
    IntVar thresholdRainOn = IntVar(600, PrefKeys::threshold_rain_on);
    IntVar thresholdRainOff = IntVar(800, PrefKeys::threshold_rain_off);
    FloatVar rainSpeedMultiplier = FloatVar(2.0, PrefKeys::rain_speed_multiplier, &checkBoundsRainMulti);
    IntVar raining = IntVar(0, PrefKeys::raining);

    int rainAverage() {
      return _rainAverage;
    }

  RainController() {
    _initAverageLoop = 10; // loop 10 times before using the average value
    add(&pulsesAfterRain);
    add(&thresholdRainOn);
    add(&thresholdRainOff);
    add(&rainSpeedMultiplier);
    add(&raining);
  }

  bool isRaining() {
    return raining.get() != 0;
  }

  int pulses() {
    unsigned int pulses = _remainingPulsesAfterRain;
    _remainingPulsesAfterRain = 0;
    return pulses;
  }

  float getSpeed(float oilingSpeed) {
    if (isRaining()) {
      return oilingSpeed * rainSpeedMultiplier.get(); // if raining, use rain multiplier    
    }
    return oilingSpeed;
  }

  void setup() {
#ifdef HW_PINS_DEFINED
    pinMode(RAIN_SENSOR_PIN, INPUT);
#endif
    tmo = millis() + 1000;
    restore();
  }

  void loop() {
    if (millis() < tmo)
      return;
    tmo = millis() + 1000;

#ifdef HW_PINS_DEFINED
    _rainAverage = 0.85 * _rainAverage + 0.15 * analogRead(RAIN_SENSOR_PIN);
#else
    _rainAverage = 650;
#endif

    if (_initAverageLoop > 0) { // wait until the average has been build up
      _initAverageLoop--;
      return;
    }

    if (_rainAverage <= thresholdRainOn.get())
    {
      raining.set(1);
    }
    if (_rainAverage >= thresholdRainOff.get() && isRaining())
    {
      raining.set(0);
      _remainingPulsesAfterRain = pulsesAfterRain.get();
    }
  }
};

extern RainController rainController;