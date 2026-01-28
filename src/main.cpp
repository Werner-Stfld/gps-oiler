#include <Arduino.h>
#include <ArduinoJson.h>

#include "globals.h"
#include "prefs.h"
#include "userVar.h"
#include "WebController.h"
#include "Timer.h"
#include "GpsController.h"
#include "RainController.h"
#include "DisplayController.h"
#include "PumpController.h"
#include "TankController.h"
#include "DistanceController.h"
#include "VoltageController.h"

// User variables
// Can partially be modified by web
// May be persisted as preferences
IntVar emergencySpeed = IntVar(60,PrefKeys::emergency_speed); // speed to use if emergency mode is detected
IntVar initFromPreferences = IntVar(0,PrefKeys::init_from_preferences);              // Controls preference storage 


JsonEndpoint *endpoints();  // The endpoints for the web-server

// The controllers
TankController tankController = TankController();
DistanceController distanceController = DistanceController();
RainController rainController = RainController();
GpsController gpsController = GpsController(RXPin, GPSBaud);
PumpController pumpController = PumpController([]() {
  tankController.getOil(1);
  displayController.triggerShowOiling();
});
WebController webController = WebController(endpoints());
DisplayController displayController = DisplayController();
VoltageController voltageController = VoltageController();

void tankReset() {
  Serial.println("Tank Reset");
  tankController.reset();
}

void wiFiReset() {
  webController.resetWiFi();
}

void startWiFi() {
  webController.startWiFi();
}

// setup the oiler scetch

void setup()
{
  delay(500); // required by c3, if CDC is on
  Serial.begin(115200);
  delay(300);

  initFromPreferences.restore();
  if (initFromPreferences.get()==0) // Default after flush of memory. May also be forced by web UI.
  { // flush all vars with initial values of the userVars
    initFromPreferences.set(610118, SetMode::flush);
    gpsController.flush();
    rainController.flush();
    distanceController.flush();
    displayController.flush();
    pumpController.flush();
    tankController.flush();
    emergencySpeed.flush();
    webController.flush();
    delay(100);
  }
  // setups are loading the userVars from the preference storage
  webController.setup();
  gpsController.setup();
  rainController.setup();
  distanceController.setup();
  displayController.setup();
  pumpController.setup();
  tankController.setup();
  voltageController.setup();

  emergencySpeed.restore();

  delay(20);

  displayController.OnTankReset(tankReset);
  displayController.OnResetWiFi(wiFiReset);
  displayController.OnStartWiFi(startWiFi);

  pinMode(BUTTON_PIN, INPUT);
}

static bool edgeSignalled = true;

// standStillEdgeDetected detects transition from speed > 3,0 km/h to <= 2.0. Triggers saving the vars modified so far.
bool standStillEdgeDetected(float speed) {
  if (speed > 3.0)
  {
    edgeSignalled = false; // prepare detection of new edge
    return false;
  }
  if (edgeSignalled) 
    return false; 
  if (speed < 2.0) {
    edgeSignalled = true; // signal edge only once
    return true; 
  }
  return false;
}

Timer secondTimer = Timer(300);
Timer minuteTimer = Timer(60000);

float getOilingSpeed() {
  float oilingSpeed;
  if (gpsController.validData()) {
    oilingSpeed = gpsController.speed();
  } else {
    oilingSpeed = voltageController.assumeMotorOn()? emergencySpeed.get(): 0.0;
  }
  return oilingSpeed;
}

void loop()
{
  gpsController.loop();
  rainController.loop();
  voltageController.loop();
  float oilingSpeed = 0.0; // oilingSpeed: speed to evaluate distance for oiling
  if (secondTimer.retriggered())
  {
    oilingSpeed = getOilingSpeed();
    oilingSpeed = rainController.getSpeed(oilingSpeed); 
    distanceController.update(oilingSpeed);
    pumpController.RequestPulses(distanceController.pulses() + rainController.pulses()); // pass requested pulses to pumpController

    if (minuteTimer.retriggered() || (gpsController.validData() && standStillEdgeDetected(gpsController.speed()))) { 
      // Flush automatically changing values once per minute or when gps is active indicating speed less then 2 km/h
      // remember: all flush() are writing only, if values have been modified 
      tankController.tankContent.flush();
      distanceController.oilingDistance.flush();
      rainController.raining.flush();
      pumpController.pendingPulses.flush();
    }
  }

  pumpController.loop(oilingSpeed);                     // process pump requests
  displayController.loop(digitalRead(BUTTON_PIN)==LOW); // display and input button handling
  webController.loop();                                 // process web requests
  prefs.AssertClosed();                                 // Assert that Pref storage is closed
}

// WEB-API-Endpoints

void getDisplay(JsonObject doc) {
  doc["brightness"] = displayController.brightness.get();;
  doc["currScreen"] = displayController.currScreen.get();
  doc["timeZone"] = displayController.timeZone.get();
}

void putDisplay(JsonObject doc) {
  displayController.brightness.set(doc["brightness"], SetMode::flush);
  displayController.timeZone.set(doc["timeZone"], SetMode::flush);
}

void getRain(JsonObject doc) {
  doc["onThreshold"] = rainController.thresholdRainOn.get();;
  doc["offThreshold"] = rainController.thresholdRainOff.get();
  doc["distanceMultiplier"] = rainController.rainSpeedMultiplier.get();
  doc["afterRainOilingPulses"] = rainController.pulsesAfterRain.get();
}

void putRain(JsonObject doc) {
  rainController.thresholdRainOn.set( doc["onThreshold"], SetMode::flush);
  rainController.thresholdRainOff.set( doc["offThreshold"], SetMode::flush);
  rainController.rainSpeedMultiplier.set( doc["distanceMultiplier"], SetMode::flush);
  rainController.pulsesAfterRain.set( doc["afterRainOilingPulses"], SetMode::flush);
}

void getPump(JsonObject doc) {
  doc["pulsesPerMl"] = tankController.pumpsMl.get();
  doc["pulseOn"] = pumpController.pumpOnTimeout.get();
  doc["pulseOff"] = pumpController.pumpPauseTimeout.get();
}

void putPump(JsonObject doc) {
  tankController.pumpsMl.set(doc["pulsesPerMl"], SetMode::flush);
  pumpController.pumpOnTimeout.set( doc["pulseOn"] , SetMode::flush);
  pumpController.pumpPauseTimeout.set(doc["pulseOff"], SetMode::flush);
}

void getWifi(JsonObject doc) {
  doc["name"] = webController.ssid_ap.get();
  doc["password"] = webController.password_ap.get();
}

void putWifi(JsonObject doc) {
  const char *str=doc["name"];
  int len = strlen(str);
  if (len > 0) 
    webController.ssid_ap.set(str, SetMode::flush);
  str=doc["password"];
  webController.password_ap.set(str, SetMode::flush);
}

void getTank(JsonObject doc) {
  doc["content"] = tankController.tankContent.get();
  doc["capacity"] = tankController.tankVolume.get();
}

void putTank(JsonObject doc) {
  tankController.tankVolume.set(doc["capacity"], SetMode::flush);
}

void getSystem(JsonObject doc) {
  String rev = Rev_OILER;
  rev += ": ";
  rev += firmware_Vers;
  doc["rev"] = rev;
  doc["init"] = (bool) initFromPreferences.get();
}

void getStates(JsonObject doc) {
  doc["oiling"] = pumpController.isOiling();
  doc["extraOiling"] = distanceController.extraOilen;
  doc["raining"] = rainController.isRaining();
  doc["wifi"] = true; // ToDo:
  doc["washing"] = pumpController.getWashing();
  doc["pumpDistance"] = distanceController.pumpDistance.get();
}

void putStates(JsonObject doc) {
  distanceController.extraOilen = doc["extraOiling"];
  pumpController.setWashing(doc["washing"]);
  distanceController.pumpDistance.set(doc["pumpDistance"], SetMode::flush);
}

void getEmergency(JsonObject doc) {
  doc["timeout"] = gpsController.zeit_bis_notbetrieb.get();
  doc["speed"] = emergencySpeed.get();
}

void putEmergency(JsonObject doc) {
  gpsController.zeit_bis_notbetrieb.set(doc["timeout"], SetMode::flush);
  emergencySpeed.set(doc["speed"], SetMode::flush);
}

// Refill tank
void putTankReset(JsonObject doc) {
  tankController.reset();
}

// Force system defaults during next startup
void putSystemDefaults(JsonObject doc) {
  if (doc["init"].is<JsonVariant>())
    initFromPreferences.set(doc["init"], SetMode::flush); // need 0 value to force reinitialization
}

void getBackup(JsonObject doc) {
  getDisplay(doc["display"].to<JsonObject>());
  getEmergency(doc["emergency"].to<JsonObject>());
  getPump(doc["pump"].to<JsonObject>());
  getRain(doc["rain"].to<JsonObject>());
  getStates(doc["states"].to<JsonObject>());
  getSystem(doc["system"].to<JsonObject>());
  getTank(doc["tank"].to<JsonObject>());
  getWifi(doc["wifi"].to<JsonObject>());
}

void restoreItem(JsonObject doc, const char *item, void (*f)(JsonObject)) {
  JsonObject o;
  o = doc[item].as<JsonObject>();
  if (o != nullptr) {
    Serial.print("restore ");
    Serial.println(item);
    f(o);
  } else {
    Serial.print("restore: no info for ");
    Serial.println(item);
  }
}
// Restore
void restore(JsonObject doc) { 
  restoreItem(doc, "display", putDisplay);
  restoreItem(doc, "emergency", putEmergency);
  restoreItem(doc, "pump", putPump);
  restoreItem(doc, "rain", putRain);
  restoreItem(doc, "states", putStates);
  restoreItem(doc, "tank", putTank);
  restoreItem(doc, "wifi", putWifi);
}

JsonEndpoint *endpoints() {
  static JsonEndpoint ep[20] = {
    JsonEndpoint("/api/system",HTTP_GET, getSystem),
    JsonEndpoint("/api/display",HTTP_GET, getDisplay),
    JsonEndpoint("/api/display",HTTP_PUT, putDisplay),
    JsonEndpoint("/api/rain",HTTP_GET, getRain),
    JsonEndpoint("/api/rain",HTTP_PUT, putRain),
    JsonEndpoint("/api/pump",HTTP_GET, getPump),
    JsonEndpoint("/api/pump",HTTP_PUT, putPump),
    JsonEndpoint("/api/wifi",HTTP_GET, getWifi),
    JsonEndpoint("/api/wifi",HTTP_PUT, putWifi),
    JsonEndpoint("/api/tank",HTTP_GET, getTank),
    JsonEndpoint("/api/tank",HTTP_PUT, putTank),
    JsonEndpoint("/api/tank/reset",HTTP_PUT, putTankReset),
    JsonEndpoint("/api/states",HTTP_GET, getStates),
    JsonEndpoint("/api/states",HTTP_PUT, putStates),
    JsonEndpoint("/api/emergency",HTTP_GET, getEmergency),
    JsonEndpoint("/api/emergency",HTTP_PUT, putEmergency),
    JsonEndpoint("/api/system/defaults",HTTP_PUT, putSystemDefaults),
    JsonEndpoint("/api/backup",HTTP_GET, getBackup),
    JsonEndpoint("/api/backup",HTTP_POST, restore),
    JsonEndpoint(nullptr,HTTP_GET, nullptr)
  };
  return ep;
};
