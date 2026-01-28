#pragma once

#include <arduino.h>
#include "userVar.h"

class TankController: public VarContainer {

  public:
  IntVar pumpsMl = IntVar(50, PrefKeys::pumps_ml);                   
  FloatVar tankVolume = FloatVar(150, PrefKeys::tank_volume);         
  FloatVar tankContent =  FloatVar(150, PrefKeys::tank_content);      

  TankController() {
    add(&pumpsMl);
    add(&tankVolume);
    add(&tankContent);
  }

  void setup() {
    restore();
  };

  unsigned int fillGradeInPercent() {
    return tankContent.get()/tankVolume.get() * 100;
  }

  void getOil(int pumps) {
    tankContent.set(tankContent.get() - ((float)pumps / pumpsMl.get()));
  }

  void reset() {
    tankContent.set(tankVolume.get(), SetMode::flush);
    //tankinhalt_Aktuell.set(15, SetMode::flush);
  }
};

extern TankController tankController;