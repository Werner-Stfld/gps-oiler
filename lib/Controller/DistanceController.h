#pragma once

#include <Arduino.h>
#include "userVar.h"


class DistanceController : public VarContainer {
  unsigned long start = 0;

  // Limit pumpDistance to be not less than 500 m
  static int checkPumpDistanz(int i) {
    return i > 500?i:500;
  }
  public:
  
  IntVar pumpDistance = IntVar(2000, PrefKeys::pump_distance, checkPumpDistanz);              // Abstand in m zwischen den einzelnen Ölungen
  FloatVar oilingDistance = FloatVar(0, PrefKeys::oiling_distance);
  boolean extraOilen = false;       

  DistanceController() {
    add(&pumpDistance);
    add(&oilingDistance);
  }

  void setup() {
    restore();

    start = millis();
  }

  void update (float oilingSpeed) {
    unsigned long now = millis();
    float secondsToIntegrate = (now - start);
    start = now;
    secondsToIntegrate = secondsToIntegrate/1000;
    float distance = oilingDistance.get();
    distance += (oilingSpeed / 3600) * secondsToIntegrate * (extraOilen?1.5:1);
    oilingDistance.set(distance);
  }

  int oilingDistanceInPercent() {
    float d = pumpDistance.get();
    d = d / 1000;
    return oilingDistance.get() / d *100;
  }

  unsigned int pulses() {
    float d = pumpDistance.get();
    d = d / 1000;
    unsigned int pulses = oilingDistance.get() / d;
    float distance = oilingDistance.get();
    distance -= pulses * d;
    oilingDistance.set(distance); // Keep rest to accumulate further distances.
    return pulses;
  }
};

extern DistanceController distanceController;