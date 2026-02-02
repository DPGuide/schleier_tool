#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth nicht aktiviert!
#endif

BluetoothSerial SerialBT;

#define PIN_MOON 25
#define PIN_SUN  26

// WICHTIG für ESP32 V2.x: Wir müssen PWM-Kanäle (0-15) definieren
#define CHAN_MOON 0
#define CHAN_SUN  1

float freqMoon = 7.83;
float freqSun = 1000.0;
bool moonOn = false;
bool sunOn = false;
int taktMode = 0; // 0=Frei, 1=4/4, 2=3/4, 3=6/8, 4=2/4
int bpm = 120;

unsigned long lastBeat = 0;
int beatCounter = 0; // Für komplexe Takte (6/8 etc.)

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SCHLEIER_MASTER");
  
  pinMode(PIN_MOON, OUTPUT);
  pinMode(PIN_SUN, OUTPUT);
  
  // --- Hardware PWM Setup für ESP32 V2.x ---
  // 1. Kanal konfigurieren (Kanal, Frequenz, Auflösung)
  ledcSetup(CHAN_MOON, 5000, 16);
  ledcSetup(CHAN_SUN, 5000, 16);
  
  // 2. Pin an Kanal binden
  ledcAttachPin(PIN_MOON, CHAN_MOON);
  ledcAttachPin(PIN_SUN, CHAN_SUN);
  
  // Initial stumm schalten (Wichtig: Kanal nutzen, nicht Pin!)
  ledcWriteTone(CHAN_MOON, 0);
  ledcWriteTone(CHAN_SUN, 0);
  
  Serial.println("Schleier Controller bereit");
}

void loop() {
  // Bluetooth Empfang
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) parseCommand(cmd);
  }

  // Takt-Verarbeitung
  if (taktMode > 0 && (moonOn || sunOn)) {
    handleRhythm(); // Rhythmisches Pulsieren
  } else {
    // Freilauf: Dauerhafter Ton
    // WICHTIG: Hier CHAN_... statt PIN_... verwenden
    if (moonOn) ledcWriteTone(CHAN_MOON, freqMoon);
    else ledcWriteTone(CHAN_MOON, 0);
    
    if (sunOn) ledcWriteTone(CHAN_SUN, freqSun);
    else ledcWriteTone(CHAN_SUN, 0);
  }
  
  delay(1); // 1ms Stability
}

void parseCommand(String c) {
  c.toUpperCase();
  
  // WICHTIG: Auch hier CHAN_... statt PIN_... verwenden
  if (c == "MON") { moonOn = true; SerialBT.println("OK Mond AN"); }
  else if (c == "MOFF") { moonOn = false; ledcWriteTone(CHAN_MOON, 0); SerialBT.println("OK Mond AUS"); }
  else if (c == "SON") { sunOn = true; SerialBT.println("OK Sonne AN"); }
  else if (c == "SOFF") { sunOn = false; ledcWriteTone(CHAN_SUN, 0); SerialBT.println("OK Sonne AUS"); }
  else if (c.startsWith("M:")) { 
    freqMoon = c.substring(2).toFloat(); 
    SerialBT.printf("OK Mond=%.2fHz\n", freqMoon);
  }
  else if (c.startsWith("S:")) { 
    freqSun = c.substring(2).toFloat(); 
    SerialBT.printf("OK Sonne=%.2fHz\n", freqSun);
  }
  else if (c.startsWith("T:")) { 
    taktMode = c.substring(2).toInt(); 
    beatCounter = 0; // Reset
    SerialBT.printf("OK Takt Mode=%d\n", taktMode);
  }
  else if (c.startsWith("B:")) { 
    bpm = c.substring(2).toInt(); 
    if(bpm < 1) bpm = 1;
    if(bpm > 300) bpm = 300;
    SerialBT.printf("OK BPM=%d\n", bpm);
  }
  else if (c == "STATUS") {
    SerialBT.printf("M:%s %.1f S:%s %.1f T:%d BPM:%d\n", 
      moonOn?"ON":"OFF", freqMoon, 
      sunOn?"ON":"OFF", freqSun,
      taktMode, bpm);
  }
}

void handleRhythm() {
  // Berechne Beat-Dauer
  unsigned long beatMs = 60000UL / bpm;
  unsigned long now = millis();
  
  if (now - lastBeat >= beatMs) {
    lastBeat = now;
    beatCounter++;
    
    int beatsPerBar = (taktMode == 1) ? 4 : (taktMode == 2) ? 3 : (taktMode == 3) ? 6 : (taktMode == 4) ? 2 : 4;
    if (beatCounter >= beatsPerBar) beatCounter = 0;
    
    // Logik für verschiedene Takte
    bool playMoon = false;
    bool playSun = false;
    
    switch(taktMode) {
      case 1: // 4/4: Alle 4 Schläge gleich (X-X-X-X)
        playMoon = moonOn;
        playSun = sunOn;
        break;
        
      case 2: // 3/4: Walzer (X-xx-X-xx) - Betonung auf 1
        playMoon = moonOn && (beatCounter == 0 || beatCounter == 1 || beatCounter == 2); 
        playSun = sunOn && (beatCounter == 0 || beatCounter == 1 || beatCounter == 2);
        break;
        
      case 3: // 6/8: (X-x-x-X-x-x)
        playMoon = moonOn && (beatCounter == 0 || beatCounter == 1 || beatCounter == 2 || 
                             beatCounter == 3 || beatCounter == 4 || beatCounter == 5);
        playSun = sunOn && (beatCounter == 0 || beatCounter == 3); 
        break;
        
      case 4: // 2/4: Marsch (X-X)
        playMoon = moonOn;
        playSun = sunOn;
        break;
    }
    
    // Ausführung: WICHTIG -> CHAN_... verwenden
    if (playMoon) {
      ledcWriteTone(CHAN_MOON, freqMoon);
      delay(50); // Kurzer Impuls
      ledcWriteTone(CHAN_MOON, 0);
    }
    if (playSun) {
      ledcWriteTone(CHAN_SUN, freqSun);
      delay(50);
      ledcWriteTone(CHAN_SUN, 0);
    }
  }
}
