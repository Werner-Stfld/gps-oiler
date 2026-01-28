#pragma once

#include <arduino.h>
#include "globals.h"
#include "userVar.h"

class PumpController: public VarContainer {

  enum {
    pulseIdle,
    pulseOn,
    pulseOff
  } state = pulseIdle;
  unsigned long tmo = 0;
  bool washing = false;
  void (*notifyPulse)();

  public:
  IntVar pumpOnTimeout = IntVar(50, PrefKeys::pump_on_timeout); // Zeit in ms wie lange die Pumpe eineschaltet ist
  IntVar pumpPauseTimeout = IntVar(500, PrefKeys::pump_off_timeout); // Zeit zwischen den einzelnen Pumpimpulsen
  IntVar minimumOilingSpeed = IntVar(5, PrefKeys::minimum_oiling_speed); // Mindestgeschwindigkeit zum Ölen
  IntVar pendingPulses =  IntVar(0, PrefKeys::pending_pulses); // Noch zu erledigende Pulse

  PumpController(void (*onPulse)()=[]() {}) : notifyPulse(onPulse) {
    add(&pumpOnTimeout);
    add(&pumpPauseTimeout);
    add(&minimumOilingSpeed);
    add(&pendingPulses);
  }

  void setWashing(bool v) {
    washing = v;
  }

  bool getWashing() {
    return washing;
  }

  void RequestPulses(unsigned int n) {
    pendingPulses.set(pendingPulses.get() + n);
  }

  bool isOiling() {
    return state != pulseIdle;
  }

  void setPin(int v) {
#ifdef HW_PINS_DEFINED
    digitalWrite(OIL_PIN, v);
#endif
  }

  void setup() {

#ifdef HW_PINS_DEFINED
    pinMode(OIL_PIN, OUTPUT);
#endif
    setPin(HIGH);
    state = pulseIdle;
    restore();
  }

  void loop(float speed) {
    if (state == pulseIdle) {
      if (washing) {
        state = pulseOn;
      } else {
        if (pendingPulses.get() > 0 && (speed >= minimumOilingSpeed.get())) { // Don't oil in standstill
          state = pulseOn;
        }
      }
      if (state == pulseOn) {
        setPin(LOW);
        tmo = millis() + pumpOnTimeout.get();
      }
      return;
    } 

    if (millis() < tmo)
      return; // wait a while

    if (state == pulseOn) {
      state = pulseOff;
      setPin(HIGH);
      tmo = millis() + pumpPauseTimeout.get();
      return;
    }
    if (state == pulseOff) {
      state = pulseIdle;
      notifyPulse();
      int pp =pendingPulses.get(); 
      if (pp > 0 && !washing) 
        pendingPulses.set(pp-1);
      return;
    }
  }
};

extern PumpController pumpController;
