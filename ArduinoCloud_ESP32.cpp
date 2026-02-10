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
  // --- 1. BLUETOOTH ÜBERWACHUNG ---
  if (SerialBT.available()) {
    String rawCmd = SerialBT.readStringUntil('\n');
    rawCmd.trim();
    String cmd = rawCmd;
    cmd.toUpperCase();

    // Kurzes Debugging
    // Serial.print("Cmd: "); Serial.println(cmd);

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
      }
    }
  }

  // --- 2. DIE LOGIK-ZENTRALE (Kombiniert) ---
  bool soundPuls = true;   // Für den Ton (lang)
  bool lightPuls = true;   // Für die Brille (kurz)
  float aktuelleSonne = freqSun;

  if (taktTyp > 0) {
    // --- TAKT MODUS ---
    unsigned long interval = 60000 / bpm; 
    unsigned long now = millis();

    // Zähler weiterschalten
    if (now - lastBeatTime >= interval) {
      lastBeatTime = now;
      beatCounter++;
      if (beatCounter > maxBeats) beatCounter = 1;
    }

    unsigned long zeitSeitSchlag = now - lastBeatTime;

    // A) SOUND-LOGIK: Der Ton ist 50% der Zeit an (angenehmer Rhythmus)
    soundPuls = (zeitSeitSchlag < (interval / 2));

    // B) BRILLEN-LOGIK: Das Licht blitzt nur ganz kurz (1/50stel der Zeit)
    // Das ist der "Reveal"-Effekt für Plexiglas/Rauch
    lightPuls = (zeitSeitSchlag < (interval / 50)); 

    // C) TON-AKZENTE (Die Melodie im Rhythmus)
    if (beatCounter == 1) {
      aktuelleSonne = freqSun + 150.0; // Die "Eins" ist hoch
    } 
    else if (taktTyp == 3 && beatCounter == 4) {
      aktuelleSonne = freqSun + 70.0;  // 6/8 Akzent
    }
    else if (taktTyp == 1 && beatCounter == 3) {
      aktuelleSonne = freqSun + 40.0;  // 4/4 Akzent
    }

    // D) STROBOSKOP AUSGABE
    if (lightPuls) {
      // Helligkeit: 20 ist dunkel/scharf, 255 ist hell/unscharf (bei 8-bit)
      // Wir nehmen hier einen kräftigen Blitz für den Effekt
      ledcWrite(CHAN_STROBE, 150); 
      digitalWrite(PIN_LED, HIGH);
    } else {
      ledcWrite(CHAN_STROBE, 0);
      digitalWrite(PIN_LED, LOW);
    }

  } else {
    // --- DAUERTON MODUS ---
    soundPuls = true;
    lightPuls = true;
    aktuelleSonne = freqSun;
    
    // Im Dauermodus kein Stroboskop (oder Dauer-An)
    ledcWrite(CHAN_STROBE, 0); 
    digitalWrite(PIN_LED, HIGH);
  }

  // --- 3. SOUND-AUSGABE ---
  
  // Mond-Kanal
  if (moonOn && soundPuls && freqMoon > 0) {
    ledcWriteTone(CHAN_MOON, freqMoon);
  } else {
    ledcWriteTone(CHAN_MOON, 0);
  }

  // Sonnen-Kanal (mit Akzenten)
  if (sunOn && soundPuls && freqSun > 0) {
    ledcWriteTone(CHAN_SUN, aktuelleSonne);
  } else {
    ledcWriteTone(CHAN_SUN, 0);
  }

  delay(1); // Kurze Pause für Stabilität
}
