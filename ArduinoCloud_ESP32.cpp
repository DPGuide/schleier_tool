#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

#define PIN_MOON 25
#define PIN_SUN  26
#define CHAN_MOON 0
#define CHAN_SUN  1

float freqMoon = 7.83;
float freqSun = 126.22;
bool moonOn = false;
bool sunOn = false;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SCHLEIER_MASTER");

  // PWM Setup für den klassischen ESP32
  ledcSetup(CHAN_MOON, 5000, 12);
  ledcSetup(CHAN_SUN, 5000, 12);
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);
  
  Serial.println("System bereit. Warte auf Handy...");
}

void loop() {
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    // Logik: Wenn Frequenz kommt, automatisch einschalten
    if (cmd.startsWith("M:")) {
      freqMoon = cmd.substring(2).toFloat();
      moonOn = true; // Auto-Aktivierung
      Serial.printf("Mond Freq: %.2f Hz\n", freqMoon);
    } 
    else if (cmd.startsWith("S:")) {
      freqSun = cmd.substring(2).toFloat();
      sunOn = true; // Auto-Aktivierung
      Serial.printf("Sonne Freq: %.2f Hz\n", freqSun);
    }
    else if (cmd == "MOFF") moonOn = false;
    else if (cmd == "SOFF") sunOn = false;
  }

  // Ton-Ausgabe ohne Unterbrechung
  if (moonOn && freqMoon > 0) {
    ledcWriteTone(CHAN_MOON, freqMoon);
  } else {
    ledcWriteTone(CHAN_MOON, 0);
  }

  if (sunOn && freqSun > 0) {
    ledcWriteTone(CHAN_SUN, freqSun);
  } else {
    ledcWriteTone(CHAN_SUN, 0);
  }

  delay(20); 
}
