#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

// Hardware-Pins (ESP32 DevKit V1)
#define PIN_MOON 25  // GPIO25 - DAC/LED Output für tiefe Frequenzen
#define PIN_SUN  26  // GPIO26 - DAC/LED Output für hohe Frequenzen

// Zustände
volatile float freqMoon = 7.83f;
volatile float freqSun = 1000.0f;
volatile bool moonOn = false;
volatile bool sunOn = false;
volatile int taktMode = 0; // 0=Frei, 1=4/4, 2=3/4, 3=6/8, 4=2/4
volatile int bpm = 120;

// Timing
unsigned long lastBeat = 0;
bool beatPhase = false;

void setup() {
  Serial.begin(115200);
  delay(1000); // Zeit für Serial-Monitor
  
  // Bluetooth starten
  if (!SerialBT.begin("SCHLEIER_MASTER")) {
    Serial.println(">>> FEHLER: Bluetooth konnte nicht gestartet werden!");
    while(1); // Stopp bei HW-Fehler
  }
  
  Serial.println("=== SCHLEIER CONTROLLER v2.0 ===");
  Serial.println("Warte auf Bluetooth-Verbindung...");
  Serial.println("Befehle: MON, MOFF, M:7.83, SON, SOFF, S:1000, T:4, B:120");

  // LEDC/PWM Setup für ESP32 Arduino Core 3.x (aktuell)
  // Für alte Versionen (2.x): Einfach ledcSetup() + ledcAttachPin() verwenden
  if (ledcAttach(PIN_MOON, 5000, 16)) { // Pin, 5kHz default, 16bit
    Serial.printf("Mond-Kanal (Pin %d) initialisiert\n", PIN_MOON);
  }
  if (ledcAttach(PIN_SUN, 5000, 16)) {
    Serial.printf("Sonne-Kanal (Pin %d) initialisiert\n", PIN_SUN);
  }
  
  // Initial alles aus
  ledcWrite(PIN_MOON, 0);
  ledcWrite(PIN_SUN, 0);
}

void loop() {
  // 1. Bluetooth Befehle empfangen (Non-Blocking)
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      Serial.print("Empfangen: ");
      Serial.println(cmd);
      parseCommand(cmd);
    }
  }

  // 2. Takt-Logik oder Freilauf
  if (taktMode > 0 && (moonOn || sunOn)) {
    handleTaktSystem();
  } else {
    // Freilauf: Direkte Frequenzausgabe
    updateOutputs();
  }
  
  // Kleine Pause für Stabilität (1ms)
  delayMicroseconds(1000);
}

void updateOutputs() {
  if (moonOn) {
    ledcWriteTone(PIN_MOON, freqMoon);
  } else {
    ledcWriteTone(PIN_MOON, 0);
  }
  
  if (sunOn) {
    ledcWriteTone(PIN_SUN, freqSun);
  } else {
    ledcWriteTone(PIN_SUN, 0);
  }
}

void parseCommand(String cmd) {
  cmd.toUpperCase(); // Großbuchstaben für einfaches Parsing
  
  // Sicherheitsprüfung: Mindestlänge für Befehle mit Wert
  bool hasValue = (cmd.length() >= 3 && cmd.charAt(1) == ':');
  
  if (cmd.startsWith("M:") && hasValue) {
    float val = cmd.substring(2).toFloat();
    if (val > 0 && val <= 8000000) { // 8MHz Limit ESP32
      freqMoon = val;
      SerialBT.printf("OK Mond=%.2f Hz\n", freqMoon);
    } else {
      SerialBT.println("ERR Freq invalid");
    }
  }
  else if (cmd == "MON") {
    moonOn = true;
    SerialBT.println("OK Mond AN");
  }
  else if (cmd == "MOFF") {
    moonOn = false;
    ledcWriteTone(PIN_MOON, 0);
    SerialBT.println("OK Mond AUS");
  }
  else if (cmd.startsWith("S:") && hasValue) {
    float val = cmd.substring(2).toFloat();
    if (val > 0 && val <= 8000000) {
      freqSun = val;
      SerialBT.printf("OK Sonne=%.2f Hz\n", freqSun);
    } else {
      SerialBT.println("ERR Freq invalid");
    }
  }
  else if (cmd == "SON") {
    sunOn = true;
    SerialBT.println("OK Sonne AN");
  }
  else if (cmd == "SOFF") {
    sunOn = false;
    ledcWriteTone(PIN_SUN, 0);
    SerialBT.println("OK Sonne AUS");
  }
  else if (cmd.startsWith("T:") && hasValue) {
    int val = cmd.substring(2).toInt();
    if (val >= 0 && val <= 4) {
      taktMode = val;
      lastBeat = millis(); // Reset Takt
      SerialBT.printf("OK Takt Mode=%d\n", taktMode);
    }
  }
  else if (cmd.startsWith("B:") && hasValue) {
    int val = cmd.substring(2).toInt();
    if (val >= 1 && val <= 300) {
      bpm = val;
      SerialBT.printf("OK BPM=%d\n", bpm);
    } else {
      SerialBT.println("ERR BPM 1-300");
    }
  }
  else if (cmd == "STATUS") {
    SerialBT.printf("Mond:%s %.2fHz | Sonne:%s %.2fHz | Takt:%d BPM:%d\n",
      moonOn?"ON":"OFF", freqMoon, 
      sunOn?"ON":"OFF", freqSun,
      taktMode, bpm);
  }
  else {
    SerialBT.println("ERR Unknown. Use: MON,MOFF,M:7.83,SON,SOFF,S:1000,T:0-4,B:60-300");
  }
}

void handleTaktSystem() {
  unsigned long currentMillis = millis();
  unsigned long beatDuration = 60000UL / bpm; // ms pro Schlag
  
  // Beat-Wechsel?
  if (currentMillis - lastBeat >= beatDuration) {
    lastBeat = currentMillis;
    beatPhase = !beatPhase;
    
    // Takt-Arten: 1=4/4, 2=3/4, 3=6/8, 4=2/4
    // Wir nutzen beatPhase für ON/OFF im Beat
    // Bei 6/8 könnte man komplexere Patterns machen, hier binary für Einfachheit
    
    bool shouldPlay = beatPhase;
    
    // Bei 6/8: Spielt nur in den starken Schlägen (vereinfacht)
    if (taktMode == 3 && !beatPhase) shouldPlay = false;
    
    if (moonOn) {
      if (shouldPlay) ledcWriteTone(PIN_MOON, freqMoon);
      else ledcWriteTone(PIN_MOON, 0);
    }
    if (sunOn) {
      if (shouldPlay) ledcWriteTone(PIN_SUN, freqSun);
      else ledcWriteTone(PIN_SUN, 0);
    }
  }
}