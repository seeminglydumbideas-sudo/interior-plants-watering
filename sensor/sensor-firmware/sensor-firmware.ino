#include <Arduino.h>
#include "Secrets.h"
#include "PowerManager.h"
#include "Moisture.h"
#include "Blues.h"
#include "Sensor.h"
#include "BootButton.h"

/*

Code written for esp32:esp32:XIAO_ESP32C3.

See README.adoc and deploy.sh for details.
*/

// globbal variables
PowerManager power;
Moisture moisture;
BlueS bluetooth;
Sensor sensor(bluetooth, moisture, power);
BootButton bootButton(moisture);

// =========================
// Setup (run once on boot)
// =========================
void setup(void) {  
  // init Serial for debug output
  Serial.begin(115200);
  //Serial.setDebugOutput(true);  

  // modules
  power.init();
  moisture.init();
  bluetooth.init();
  sensor.init();
  bootButton.init();
}

// =========================
// Main loop
// =========================
void loop(void) {
  power.loop();
  moisture.loop();
  bluetooth.loop();
  sensor.loop();
  bootButton.loop();
  delay(500);
}

