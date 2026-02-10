#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// --- HARDWARE KONFIGURATION ---
#define PIN_MOON 25    // Ausgang für Mond-Frequenz
#define PIN_SUN  26    // Ausgang für Sonnen-Frequenz
#define PIN_LED   2    // Eingebaute LED für Status-Blinken

#define CHAN_MOON 0
#define CHAN_SUN  1
#define RESOLUTION 12  // 12-bit Auflösung für saubere Schwingungen

// --- VARIABLEN ---
float freqMoon = 7.83;
float freqSun = 126.22;
bool moonOn = false;
bool sunOn = false;

// TAKT & RHYTHMUS
int bpm = 120;
int taktTyp = 0;      // 0=Dauerton, 1=4/4, 2=3/4, 3=6/8, 4=2/4
int beatCounter = 0;
int maxBeats = 4;
unsigned long lastBeatTime = 0;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SCHLEIER_MASTER"); // Name für dein Tablet/Handy

  pinMode(PIN_LED, OUTPUT);

  // PWM Setup für den ESP32 (Klassik-Modus)
  ledcSetup(CHAN_MOON, 5000, RESOLUTION);
  ledcSetup(CHAN_SUN, 5000, RESOLUTION);
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);
   
  Serial.println(">>> SOLARIS MASTER BEREIT <<<");
  Serial.println("Warte auf Bluetooth-Befehle...");
}

void loop() {
  // 1. BLUETOOTH BEFEHLE VERARBEITEN
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    // Frequenzen einstellen & Auto-An
    if (cmd.startsWith("M:")) {
      freqMoon = cmd.substring(2).toFloat();
      moonOn = true;
      Serial.printf("Mond: %.2f Hz\n", freqMoon);
    } 
    else if (cmd.startsWith("S:")) {
      freqSun = cmd.substring(2).toFloat();
      sunOn = true;
      Serial.printf("Sonne: %.2f Hz\n", freqSun);
    }
    // Manuelles Aus
    else if (cmd == "MOFF") moonOn = false;
    else if (cmd == "SOFF") sunOn = false;
    
    // BPM & Takt-Typ
    else if (cmd.startsWith("B:")) {
      bpm = cmd.substring(2).toInt();
      if (bpm < 1) bpm = 1;
      Serial.printf("BPM eingestellt: %d\n", bpm);
    }
    else if (cmd.startsWith("T:")) {
      taktTyp = cmd.substring(2).toInt();
      // Mapping für deine APK
      if (taktTyp == 1) maxBeats = 4;      // 4/4
      else if (taktTyp == 2) maxBeats = 3; // 3/4
      else if (taktTyp == 3) maxBeats = 6; // 6/8
      else if (taktTyp == 4) maxBeats = 2; // 2/4
      beatCounter = 0; 
      Serial.printf("Takt-Modus: %d (%d Schläge)\n", taktTyp, maxBeats);
    }
  }

  // 2. TAKT-LOGIK & AKZENTE
  bool pulsAn = true;
  float aktuelleSonne = freqSun;

  if (taktTyp > 0) {
    unsigned long interval = 60000 / bpm;
    unsigned long now = millis();

    // Neuer Schlag?
    if (now - lastBeatTime >= interval) {
      lastBeatTime = now;
      beatCounter++;
      if (beatCounter > maxBeats) beatCounter = 1;
      
      // Kurze Info in die Konsole
      Serial.print("Beat: "); Serial.println(beatCounter);
    }

    // 50% Puls-Dauer (Takt-Gefühl)
    pulsAn = (now - lastBeatTime < (interval / 2));

    // AKZENTE SETZEN (Hörbare Betonung)
    if (beatCounter == 1) {
      aktuelleSonne = freqSun + 150.0; // Starke Betonung der EINS
    } 
    else if (taktTyp == 3 && beatCounter == 4) {
      aktuelleSonne = freqSun + 70.0;  // 6/8 Schaukel-Betonung
    }
    else if (taktTyp == 1 && beatCounter == 3) {
      aktuelleSonne = freqSun + 40.0;  // Leichte Betonung im 4/4
    }
    
    // Status LED blinkt im Takt
    digitalWrite(PIN_LED, pulsAn ? HIGH : LOW);

  } else {
    // Dauerton-Modus
    pulsAn = true;
    aktuelleSonne = freqSun;
    digitalWrite(PIN_LED, HIGH);
  }

  // 3. FINALE AUSGABE AN DIE PINS
  
  // Mond-Ausgabe
  if (moonOn && pulsAn && freqMoon > 0) {
    ledcWriteTone(CHAN_MOON, freqMoon);
  } else {
    ledcWriteTone(CHAN_MOON, 0);
  }

  // Sonnen-Ausgabe
  if (sunOn && pulsAn && freqSun > 0) {
    ledcWriteTone(CHAN_SUN, aktuelleSonne);
  } else {
    ledcWriteTone(CHAN_SUN, 0);
  }

  delay(10); // System-Stabilität
}
