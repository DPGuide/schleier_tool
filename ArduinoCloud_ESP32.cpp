#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// --- HARDWARE ---
#define PIN_MOON 25
#define PIN_SUN  26
#define PIN_LED   2    
#define PIN_STROBE 4

#define CHAN_MOON 0
#define CHAN_SUN  1
#define CHAN_STROBE 2
#define RESOLUTION 8  // 8 Bit Auflösung

// --- VARIABLEN ---
float freqMoon = 7.83;
float freqSun = 126.22;
bool moonOn = false;
bool sunOn = false;
bool strobeActive = false; 

int bpm = 120;
int taktTyp = 0; 
int beatCounter = 0;
int maxBeats = 4;
int strobeTeiler = 50;
unsigned long lastBeatTime = 0;

// Speicher für die Memory-Funktion (damit nichts stottert)
float lastMoonFreq = -1;
float lastSunFreq = -1;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SCHLEIER_MASTER");
  SerialBT.setTimeout(0); // WICHTIG: Nicht warten!

  pinMode(PIN_LED, OUTPUT);
  
  ledcSetup(CHAN_MOON, 5000, RESOLUTION);
  ledcSetup(CHAN_SUN, 5000, RESOLUTION);
  ledcSetup(CHAN_STROBE, 5000, RESOLUTION);
  
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);
  ledcAttachPin(PIN_STROBE, CHAN_STROBE);
}

void loop() {
  // --- 1. DATEN EMPFANGEN ---
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      if (cmd.startsWith("M:")) { freqMoon = cmd.substring(2).toFloat(); moonOn = true; } 
      else if (cmd.startsWith("S:")) { freqSun = cmd.substring(2).toFloat(); sunOn = true; }
      else if (cmd == "MOFF") moonOn = false;
      else if (cmd == "SOFF") sunOn = false;
      else if (cmd == "L:1") strobeActive = true;
      else if (cmd == "L:0") strobeActive = false;
      else if (cmd.startsWith("B:")) bpm = cmd.substring(2).toInt();
      // Hier setzen wir die maxBeats für die verschiedenen Takte
      else if (cmd.startsWith("T:")) { 
        taktTyp = cmd.substring(2).toInt(); 
        beatCounter = 0;
        if (taktTyp == 1) maxBeats = 4;      // 4/4 Takt
        else if (taktTyp == 2) maxBeats = 3; // 3/4 Walzer
        else if (taktTyp == 3) maxBeats = 6; // 6/8 Schaukeln
        else if (taktTyp == 4) maxBeats = 2; // 2/4 Marsch
      }
      else if (cmd.startsWith("D:")) strobeTeiler = cmd.substring(2).toInt();
    }
  }

  // --- 2. TAKT BERECHNUNG ---
  unsigned long interval = 60000 / bpm; 
  unsigned long now = millis();

  if (now - lastBeatTime >= interval) {
    lastBeatTime = now;
    beatCounter++;
    if (beatCounter > maxBeats) beatCounter = 1;
  }

  unsigned long zeitSeitSchlag = now - lastBeatTime;
  
  // Puls-Logik (50% an/aus)
  bool soundPuls = (taktTyp == 0) ? true : (zeitSeitSchlag < (interval / 2));
  bool lightPuls = (taktTyp > 0) && strobeActive && (zeitSeitSchlag < (interval / strobeTeiler));

  // --- 3. SOUND BERECHNUNG (Hier kommen die Akzente zurück!) ---
  
  float targetMoon = 0;
  float targetSun = 0;

  // A) Mond Berechnung
  if (moonOn && soundPuls) {
    targetMoon = freqMoon;
  } else {
    targetMoon = 0;
  }

  // B) Sonne Berechnung (MIT AKZENTEN für den "Wumms")
  if (sunOn && soundPuls) {
    targetSun = freqSun;
    
    // Akzente setzen (Das macht den Unterschied zwischen den Takten!)
    if (taktTyp > 0) {
      if (beatCounter == 1) {
        targetSun = freqSun + 150.0; // Die EINS ist viel höher/lauter
      } 
      else if (taktTyp == 3 && beatCounter == 4) {
        targetSun = freqSun + 70.0;  // 6/8 Nebenbetonung
      }
      else if (taktTyp == 1 && beatCounter == 3) {
        targetSun = freqSun + 40.0;  // 4/4 Nebenbetonung
      }
    }
  } else {
    targetSun = 0;
  }

  // --- 4. HARDWARE UPDATE (Nur bei Änderung -> Kein Stottern!) ---

  if (targetMoon != lastMoonFreq) {
    if (targetMoon > 0) ledcWriteTone(CHAN_MOON, targetMoon);
    else ledcWriteTone(CHAN_MOON, 0);
    lastMoonFreq = targetMoon;
  }

  if (targetSun != lastSunFreq) {
    if (targetSun > 0) ledcWriteTone(CHAN_SUN, targetSun);
    else ledcWriteTone(CHAN_SUN, 0);
    lastSunFreq = targetSun;
  }

  // --- 5. LICHT ---
  if (lightPuls) {
    ledcWrite(CHAN_STROBE, 150);
    digitalWrite(PIN_LED, HIGH);
  } else {
    ledcWrite(CHAN_STROBE, 0);
    digitalWrite(PIN_LED, LOW);
  }
}
