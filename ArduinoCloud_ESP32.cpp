#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth nicht aktiviert!
#endif

BluetoothSerial SerialBT;

#define PIN_MOON 25
#define PIN_SUN  26

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
  
  // Hardware PWM Setup
  ledcAttach(PIN_MOON, 5000, 16);
  ledcAttach(PIN_SUN, 5000, 16);
  
  ledcWriteTone(PIN_MOON, 0);
  ledcWriteTone(PIN_SUN, 0);
  
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
    if (moonOn) ledcWriteTone(PIN_MOON, freqMoon);
    else ledcWriteTone(PIN_MOON, 0);
    
    if (sunOn) ledcWriteTone(PIN_SUN, freqSun);
    else ledcWriteTone(PIN_SUN, 0);
  }
  
  delay(1); // 1ms Stability
}

void parseCommand(String c) {
  c.toUpperCase();
  
  if (c == "MON") { moonOn = true; SerialBT.println("OK Mond AN"); }
  else if (c == "MOFF") { moonOn = false; ledcWriteTone(PIN_MOON, 0); SerialBT.println("OK Mond AUS"); }
  else if (c == "SON") { sunOn = true; SerialBT.println("OK Sonne AN"); }
  else if (c == "SOFF") { sunOn = false; ledcWriteTone(PIN_SUN, 0); SerialBT.println("OK Sonne AUS"); }
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
        playMoon = moonOn && (beatCounter == 0 || beatCounter == 1 || beatCounter == 2); // Alle 3, aber 1. lauter
        playSun = sunOn && (beatCounter == 0 || beatCounter == 1 || beatCounter == 2);
        // 50% Duty Cycle im Takt
        if (beatCounter == 1 || beatCounter == 2) {
          // Leiser/Pause in den Nachschlägen (optional)
        }
        break;
        
      case 3: // 6/8: (X-x-x-X-x-x) - Zwei Hauptbeats
        // Schläge 0 und 3 sind betont (die "großen" Beats)
        playMoon = moonOn && (beatCounter == 0 || beatCounter == 1 || beatCounter == 2 || 
                             beatCounter == 3 || beatCounter == 4 || beatCounter == 5);
        playSun = sunOn && (beatCounter == 0 || beatCounter == 3); // Nur Hauptbeats für Sonne?
        // Oder beides spielen - je nach Geschmack
        playMoon = moonOn;
        playSun = sunOn;
        break;
        
      case 4: // 2/4: Marsch (X-X)
        playMoon = moonOn;
        playSun = sunOn;
        break;
    }
    
    // Ausführung: Immer kurze Pulse statt Dauerton im Takt
    if (playMoon) {
      ledcWriteTone(PIN_MOON, freqMoon);
      delay(50); // 50ms Puls (oder beatMs/2 für halbe Note)
      ledcWriteTone(PIN_MOON, 0);
    }
    if (playSun) {
      ledcWriteTone(PIN_SUN, freqSun);
      delay(50);
      ledcWriteTone(PIN_SUN, 0);
    }
  }
}