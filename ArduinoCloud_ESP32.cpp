#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// --- HARDWARE ---
#define PIN_MOON 25
#define PIN_SUN  26
#define PIN_LED   2    // Nur einmal definiert
#define PIN_STROBE 4

#define CHAN_MOON 0
#define CHAN_SUN  1
#define CHAN_STROBE 2
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
  ledcSetup(CHAN_STROBE, 5000, RESOLUTION);
  
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);
  ledcAttachPin(PIN_STROBE, CHAN_STROBE);
   
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
    
    // BPM
    else if (cmd.indexOf("B") >= 0) {
      String val = "";
      for(int i=0; i<cmd.length(); i++) {
        if(isDigit(cmd[i])) val += cmd[i];
      }
      if(val.length() > 0) {
        bpm = val.toInt();
        if(bpm < 1) bpm = 1;
        Serial.printf("=> BPM: %d\n", bpm);
      }
    }
    // TAKT
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
        Serial.printf("=> TAKT: %d (%d Schläge)\n", taktTyp, maxBeats);
      }
    }
  }

  // 2. TAKT-RECHNUNG & LOGIK
  bool pulsAn = true;
  float aktuelleSonne = freqSun;

  if (taktTyp > 0) {
    // --- TAKT MODUS ---
    unsigned long interval = 60000 / bpm; 
    unsigned long now = millis();

    if (now - lastBeatTime >= interval) {
      lastBeatTime = now;
      beatCounter++;
      if (beatCounter > maxBeats) beatCounter = 1;
      Serial.print("."); 
      if(beatCounter == 1) Serial.println(" (1)"); 
    }

    // 50% Puls-Dauer
    pulsAn = (now - lastBeatTime < (interval / 2));

    // TON-AKZENTE (6/8 Schaukel & Co)
    if (beatCounter == 1) {
      aktuelleSonne = freqSun + 150.0; 
    } 
    else if (taktTyp == 3 && beatCounter == 4) {
      aktuelleSonne = freqSun + 70.0;
    }
    else if (taktTyp == 1 && beatCounter == 3) {
      aktuelleSonne = freqSun + 40.0;
    }

    // STROBOSKOP (Nur kurz blitzen)
    if (pulsAn) {
      ledcWrite(CHAN_STROBE, 20); // Wert anpassen für Helligkeit (0-4095)
      digitalWrite(PIN_LED, HIGH);
    } else {
      ledcWrite(CHAN_STROBE, 0);
      digitalWrite(PIN_LED, LOW);
    }

  } else {
    // --- DAUERTON MODUS ---
    pulsAn = true;
    aktuelleSonne = freqSun;
    
    // Kein Strobe im Dauermodus (oder Dauer-An, wenn du willst)
    ledcWrite(CHAN_STROBE, 0); 
    digitalWrite(PIN_LED, HIGH);
  }

  // 3. SOUND-AUSGABE
  if (moonOn && pulsAn && freqMoon > 0) {
    ledcWriteTone(CHAN_MOON, freqMoon);
  } else {
    ledcWriteTone(CHAN_MOON, 0);
  }

  if (sunOn && pulsAn && freqSun > 0) {
    ledcWriteTone(CHAN_SUN, aktuelleSonne);
  } else {
    ledcWriteTone(CHAN_SUN, 0);
  }

  delay(5);
}
