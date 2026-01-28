#include <Arduino.h>
#include <wire.h>
#include "prefs.h"

namespace PrefKeys {
  PrefKey Noeeprom = nullptr;
  PrefKey init_from_preferences = "init_prefs";
  PrefKey anzahl_Pump = "nmb_pump";
  PrefKey tank_volume = "tank_vol";
  PrefKey pumps_ml = "pumps_per_ml";
  PrefKey tankinhalt_ml_akt = "tank_cur";
  PrefKey pump_distance = "pump_distance";
  PrefKey minimum_oiling_speed = "min_speed";
  PrefKey oilsymbol_Zeit = "oil_sym_tmo";
  PrefKey emergency_speed = "em_speed";
  PrefKey zeit_bis_notbetrieb = "em_tmo";
  PrefKey pump_on_timeout = "pump_on_tmo";
  PrefKey pump_off_timeout = "pump_off_tmo";
  PrefKey threshold_rain_on = "rain_thr_on";
  PrefKey threshold_rain_off = "rain_thr_off";
  PrefKey raining = "rain_mode";
  PrefKey rain_speed_multiplier = "rain_multiplier";
  PrefKey TimeAPout = "ap_tmo";
  PrefKey init_pump_anzahl = "init_pump_cnt";
  PrefKey oiling_distance = "distance";
  PrefKey brightness = "brightness";
  PrefKey currentScreen = "currScreen";
  PrefKey pulses_after_rain = "rain_off_cnt";
  PrefKey ssid_ap = "ssid";
  PrefKey password_ap = "pw";
  PrefKey timezone = "timezone";
  PrefKey tank_content = "tank_cur";
  PrefKey pending_pulses = "pump_pend";
  PrefKey voltageScale = "volt_scale";
}

PrefsNamespace prefs = PrefsNamespace("prefs");

void read_string(PrefKey key, char *dst, size_t len)
{
  size_t l = prefs.getString(key, dst, len);
  if (l == 0) {
    Serial.print("prefs.getString failed for key: ");
    Serial.println(key);
  }
}

void write_string(PrefKey key, const char *value)
{
  size_t n = prefs.putString(key, value);
  if (n == 0) {
    Serial.print("prefs.putString(): key: ");
    Serial.print(key);
    Serial.println(", failed!");
  }
}

int read_int(PrefKey key)
{
  return prefs.getInt(key);
}

void write_int(PrefKey key, int value)
{
  prefs.putInt(key, value);
}

float read_float(PrefKey key)
{
  return prefs.getFloat(key);
}

void write_float(PrefKey key, float value)
{
  prefs.putFloat(key, value);
}

IntPrefVar::IntPrefVar(PrefKey a) : key(a),dirty(true) {}
FloatPrefVar::FloatPrefVar(PrefKey a) : key(a),dirty(true) {}
StringPrefVar::StringPrefVar(PrefKey a) : key(a),dirty(true) {}

int IntPrefVar::get() {
  return cache;
}

void IntPrefVar::set(int v) {
  if (cache != v) {
    dirty = true;
    cache = v;
  }
}

void IntPrefVar::restore() {
  if (dirty) {
    cache = read_int(key);
    dirty = false;
  }
}

void IntPrefVar::flush() {
  if (dirty) {
    write_int(key, cache);
    dirty=false;
  }
}


float FloatPrefVar::get() {
  return cache;
}

void FloatPrefVar::set(float v) {
  if (cache != v) {
    dirty = true;
    cache = v;
  }
}

void FloatPrefVar::restore() {
  if (dirty) {
    cache = read_float(key);
    dirty = false;
  }
}

void FloatPrefVar::flush() {
  if (dirty) {
    write_float(key, cache);
    dirty=false;
  }
}

// Sets the cache to value.
// if value == nullptr or len of value exceeds maxLen, "" will be written to the var.
void StringPrefVar::set(const char *value){
  if (value == nullptr) 
    value="";
  size_t l = strlen(value);
  if (l > maxLen) {
    l=0;
    value = "";
  }
  for (size_t i = 0; i <= l; i++){ // "<="" also copies terminating 0;
    if (cache[i] != value[i]) {
      cache[i] = value[i];
      dirty = true;
    }
  }
}

// get() returns the reference to the internal char array.
// Note: This will generate the side effect, that a new value will get active, without invoking get().
const char *StringPrefVar::get(){
  return cache; 
}

void StringPrefVar::restore() {
  if (dirty) {
    ::read_string(key, cache, maxLen);
    dirty = false;
  }
}

void StringPrefVar::flush() {
  if (dirty) {
    ::write_string(key, cache);
    dirty=false;
  }
}
