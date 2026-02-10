#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// --- DEINE HARDWARE PINS ---
#define PIN_MOON 25
#define PIN_SUN  26
#define CHAN_MOON 0
#define CHAN_SUN  1

// --- VARIABLEN ---
float freqMoon = 7.83;
float freqSun = 126.22;
bool moonOn = false;
bool sunOn = false;

// TAKT VARIABLEN
int bpm = 120;
int taktTyp = 0;      // 0=Frei, 1=4/4, 2=3/4, 3=6/8, 4=2/4
int beatCounter = 0;
int maxBeats = 4;
unsigned long lastBeatTime = 0;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SCHLEIER_MASTER");

  // PWM Setup
  ledcSetup(CHAN_MOON, 5000, 12);
  ledcSetup(CHAN_SUN, 5000, 12);
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);
   
  Serial.println("System bereit: 2/4, 3/4, 4/4, 6/8 aktiv.");
}

void loop() {
  // 1. BLUETOOTH BEFEHLE
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.startsWith("M:")) {
      freqMoon = cmd.substring(2).toFloat();
      moonOn = true;
    } 
    else if (cmd.startsWith("S:")) {
      freqSun = cmd.substring(2).toFloat();
      sunOn = true;
    }
    else if (cmd == "MOFF") moonOn = false;
    else if (cmd == "SOFF") sunOn = false;
    else if (cmd.startsWith("B:")) bpm = cmd.substring(2).toInt();
    else if (cmd.startsWith("T:")) {
      taktTyp = cmd.substring(2).toInt();
      // Deine APK Takt-Zuweisung:
      if (taktTyp == 1) maxBeats = 4;      // 4/4
      else if (taktTyp == 2) maxBeats = 3; // 3/4
      else if (taktTyp == 3) maxBeats = 6; // 6/8
      else if (taktTyp == 4) maxBeats = 2; // 2/4
      beatCounter = 0; // Reset bei Umschaltung
    }
  }

  // 2. TAKT LOGIK
  bool pulsAn = true;
  float aktuelleSonne = freqSun;

  if (taktTyp > 0) {
    unsigned long interval = 60000 / bpm;
    unsigned long now = millis();

    if (now - lastBeatTime >= interval) {
      lastBeatTime = now;
      beatCounter++;
      if (beatCounter > maxBeats) beatCounter = 1;
      
      // Monitor-Ausgabe für dich zum Prüfen
      Serial.printf("Takt: %d/%d | Schlag: %d\n", 
                    (taktTyp==1?4:(taktTyp==2?3:(taktTyp==3?6:2))), 
                    (taktTyp==3?8:4), beatCounter);
    }

    // 50% Puls-Dauer
    pulsAn = (now - lastBeatTime < (interval / 2));

    // AKZENT AUF DER 1: Die Sonne klingt auf dem ersten Schlag etwas höher
    if (beatCounter == 1) {
      aktuelleSonne = freqSun + 150.0; // +150Hz für den Akzent-Hörtest
    }
  }

  // 3. AUSGABE
  // MOND
  if (moonOn && pulsAn) {
    ledcWriteTone(CHAN_MOON, freqMoon);
  } else {
    ledcWriteTone(CHAN_MOON, 0);
  }

  // SONNE
  if (sunOn && pulsAn) {
    ledcWriteTone(CHAN_SUN, aktuelleSonne);
  } else {
    ledcWriteTone(CHAN_SUN, 0);
  }

  delay(10); 
}
