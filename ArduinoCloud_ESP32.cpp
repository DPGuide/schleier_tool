#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// --- HARDWARE ---
#define PIN_MOON 25
#define PIN_SUN  26
#define PIN_LED   2

#define CHAN_MOON 0
#define CHAN_SUN  1
#define RESOLUTION 12

// --- VARIABLEN ---
float freqMoon = 7.83;
float freqSun = 126.22;
bool moonOn = false;
bool sunOn = false;

int bpm = 120;
int taktTyp = 0; // 0 = Dauer, 1=4/4, 2=3/4, 3=6/8, 4=2/4
int beatCounter = 0;
int maxBeats = 4;
unsigned long lastBeatTime = 0;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SCHLEIER_MASTER");
  pinMode(PIN_LED, OUTPUT);

  ledcSetup(CHAN_MOON, 5000, RESOLUTION);
  ledcSetup(CHAN_SUN, 5000, RESOLUTION);
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);
   
  Serial.println(">>> DEBUG-MODUS AKTIV <<<");
  Serial.println("Warte auf Befehle vom Tablet...");
}

void loop() {
  // 1. BLUETOOTH ÜBERWACHUNG
  if (SerialBT.available()) {
    String rawCmd = SerialBT.readStringUntil('\n');
    rawCmd.trim();
    String cmd = rawCmd;
    cmd.toUpperCase();

    // DAS HIER ZEIGT DIR GENAU WAS ANKOMMT:
    Serial.print("Empfangen: ["); Serial.print(rawCmd); Serial.println("]");

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
    
    // VERBESSERTE BPM-ERKENNUNG (sucht nach B oder BPM)
    else if (cmd.indexOf("B") >= 0) {
      // Sucht die Zahl im String, egal wo sie steht
      String val = "";
      for(int i=0; i<cmd.length(); i++) {
        if(isDigit(cmd[i])) val += cmd[i];
      }
      if(val.length() > 0) {
        bpm = val.toInt();
        if(bpm < 1) bpm = 1;
        Serial.printf("=> INTERNER TAKT GEÄNDERT AUF: %d BPM\n", bpm);
      }
    }
    // VERBESSERTE TAKT-ERKENNUNG
    else if (cmd.indexOf("T") >= 0) {
      String val = "";
      for(int i=0; i<cmd.length(); i++) {
        if(isDigit(cmd[i])) val += cmd[i];
      }
      if(val.length() > 0) {
        taktTyp = val.toInt();
        if (taktTyp == 1) maxBeats = 4;
        else if (taktTyp == 2) maxBeats = 3;
        else if (taktTyp == 3) maxBeats = 6;
        else if (taktTyp == 4) maxBeats = 2;
        beatCounter = 0;
        Serial.printf("=> TAKT-MODUS: %d (%d Schläge)\n", taktTyp, maxBeats);
      }
    }
  }

  // 2. TAKT-RECHNUNG
  bool pulsAn = true;
  float aktuelleSonne = freqSun;

  if (taktTyp > 0) {
    // Hier passiert die Zeit-Magie:
    unsigned long interval = 60000 / bpm; 
    unsigned long now = millis();

    if (now - lastBeatTime >= interval) {
      lastBeatTime = now;
      beatCounter++;
      if (beatCounter > maxBeats) beatCounter = 1;
      // Kleiner Punkt im Monitor für jeden Schlag
      Serial.print("."); 
      if(beatCounter == 1) Serial.println(" (1)"); 
    }

    pulsAn = (now - lastBeatTime < (interval / 2));

    // Akzente
    if (beatCounter == 1) aktuelleSonne = freqSun + 150.0;
    else if (taktTyp == 3 && beatCounter == 4) aktuelleSonne = freqSun + 70.0;
    
    digitalWrite(PIN_LED, pulsAn ? HIGH : LOW);
  } else {
    pulsAn = true; // Dauerton
    digitalWrite(PIN_LED, HIGH);
  }

  // 3. SOUND-AUSGABE
  if (moonOn && pulsAn && freqMoon > 0) ledcWriteTone(CHAN_MOON, freqMoon);
  else ledcWriteTone(CHAN_MOON, 0);

  if (sunOn && pulsAn && freqSun > 0) ledcWriteTone(CHAN_SUN, aktuelleSonne);
  else ledcWriteTone(CHAN_SUN, 0);

  delay(5); // Schnelle Reaktionszeit
}
