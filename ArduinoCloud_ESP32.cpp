#include "BluetoothSerial.h" // <--- Das hat gefehlt!

BluetoothSerial SerialBT;

// Pins für AZDelivery ESP32 Dev Kit C V4
#define PIN_MOON 25
#define PIN_SUN  26

// PWM-Kanäle (für ESP32 Board Version 2.x)
#define CHAN_MOON 0
#define CHAN_SUN  1

// Variablen
float freqMoon = 7.83;  // Schumann-Resonanz
float freqSun = 126.22; // Sonnen-Frequenz (Beispiel)
bool moonOn = false;
bool sunOn = false;

// Takt-Steuerung
int taktMode = 0; // 0=Frei, 1=4/4, 2=3/4, 3=6/8, 4=2/4
int bpm = 120;
unsigned long lastBeat = 0;
int beatCounter = 0;

void setup() {
  Serial.begin(115200);
  
  // Bluetooth Name, der im Handy erscheint
  SerialBT.begin("SCHLEIER_MASTER"); 
  Serial.println("Bluetooth gestartet! Suche nach 'SCHLEIER_MASTER'");

  // --- Hardware PWM Setup (ESP32 API v2) ---
  // Wir nutzen ledcSetup, um den Timer zu konfigurieren
  // Kanal 0 und 1, 5000 Hz Basis, 12-Bit Auflösung (reicht völlig aus)
  ledcSetup(CHAN_MOON, 5000, 12);
  ledcSetup(CHAN_SUN, 5000, 12);

  // Pins an die Kanäle binden
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);

  // Initial alles aus
  ledcWriteTone(CHAN_MOON, 0);
  ledcWriteTone(CHAN_SUN, 0);
}

// Hilfsfunktion um Befehle zu verarbeiten
void parseCommand(String c) {
  c.toUpperCase(); // Alles in Großbuchstaben umwandeln

  if (c == "MON") { 
    moonOn = true; 
    SerialBT.println("OK: Mond AN"); 
  }
  else if (c == "MOFF") { 
    moonOn = false; 
    ledcWriteTone(CHAN_MOON, 0); 
    SerialBT.println("OK: Mond AUS"); 
  }
  else if (c == "SON") { 
    sunOn = true; 
    SerialBT.println("OK: Sonne AN"); 
  }
  else if (c == "SOFF") { 
    sunOn = false; 
    ledcWriteTone(CHAN_SUN, 0); 
    SerialBT.println("OK: Sonne AUS"); 
  }
  else if (c.startsWith("M:")) { 
    freqMoon = c.substring(2).toFloat(); 
    SerialBT.printf("OK: Mond Freq=%.2f Hz\n", freqMoon);
  }
  else if (c.startsWith("S:")) { 
    freqSun = c.substring(2).toFloat(); 
    SerialBT.printf("OK: Sonne Freq=%.2f Hz\n", freqSun);
  }
  else if (c.startsWith("T:")) { 
    taktMode = c.substring(2).toInt(); 
    beatCounter = 0; // Reset des Zählers bei Taktwechsel
    SerialBT.printf("OK: Takt Modus=%d\n", taktMode);
  }
  else if (c.startsWith("B:")) { 
    bpm = c.substring(2).toInt(); 
    if(bpm < 1) bpm = 1;
    if(bpm > 300) bpm = 300;
    SerialBT.printf("OK: BPM=%d\n", bpm);
  }
  else if (c == "STATUS") {
    SerialBT.printf("STATUS -> M:%s (%.2fHz) | S:%s (%.2fHz) | Takt:%d | BPM:%d\n", 
      moonOn?"ON":"OFF", freqMoon, 
      sunOn?"ON":"OFF", freqSun,
      taktMode, bpm);
  }
}

void handleRhythm() {
  unsigned long beatMs = 60000UL / bpm; // Dauer eines Schlags in ms
  unsigned long now = millis();

  if (now - lastBeat >= beatMs) {
    lastBeat = now;
    
    // Takt-Logik: Wie viele Schläge pro Takt?
    int beatsPerBar = 4;
    if (taktMode == 2) beatsPerBar = 3; // 3/4 Takt
    if (taktMode == 3) beatsPerBar = 6; // 6/8 Takt
    if (taktMode == 4) beatsPerBar = 2; // 2/4 Takt
    
    // Zählerlogik
    if (beatCounter >= beatsPerBar) beatCounter = 0;

    bool playMoon = false;
    bool playSun = false;

    // Rhythmus-Muster definieren
    switch(taktMode) {
      case 1: // 4/4: Jeder Schlag
        playMoon = moonOn;
        playSun = sunOn;
        break;
        
      case 2: // 3/4 Walzer: BUMM-zack-zack
        playMoon = moonOn; // Mond immer
        // Sonne nur auf dem ersten Schlag (die "1") betonen oder andersrum
        playSun = sunOn && (beatCounter == 0); 
        break;
        
      case 3: // 6/8
        playMoon = moonOn;
        // Sonne auf 1 und 4 betonen
        playSun = sunOn && (beatCounter == 0 || beatCounter == 3); 
        break;
        
      case 4: // 2/4 Marsch
        playMoon = moonOn; // Mond auf 1 und 2
        playSun = sunOn && (beatCounter == 0); // Sonne nur auf 1
        break;
    }

    // Töne abspielen (Impuls)
    if (playMoon) {
      ledcWriteTone(CHAN_MOON, freqMoon);
    }
    if (playSun) {
      ledcWriteTone(CHAN_SUN, freqSun);
    }

    // Kurzer Impuls (50ms), dann Stille, damit es rhythmisch wirkt
    if (playMoon || playSun) {
      delay(50); 
      ledcWriteTone(CHAN_MOON, 0);
      ledcWriteTone(CHAN_SUN, 0);
    }
    
    beatCounter++;
  }
}

void loop() {
  // 1. Bluetooth Befehle lesen
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim(); // Leerzeichen entfernen
    if (cmd.length() > 0) {
      Serial.print("Kommando empfangen: ");
      Serial.println(cmd);
      parseCommand(cmd);
    }
  }

  // 2. Audio Ausgabe
  if (taktMode > 0) {
    // Wenn ein Taktmodus aktiv ist (1-4)
    if (moonOn || sunOn) {
      handleRhythm();
    }
  } else {
    // Modus 0: Freilauf (Dauerton)
    if (moonOn) ledcWriteTone(CHAN_MOON, freqMoon);
    else ledcWriteTone(CHAN_MOON, 0);
    
    if (sunOn) ledcWriteTone(CHAN_SUN, freqSun);
    else ledcWriteTone(CHAN_SUN, 0);
  }
  
  delay(1); // Kurze Pause für CPU-Entlastung
}
