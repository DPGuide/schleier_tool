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
int taktMode = 0;
int bpm = 120;

unsigned long lastBeat = 0;
bool beatPhase = false;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("SCHLEIER_MASTER");
  
  // Für alle ESP32-Versionen kompatibel
  pinMode(PIN_MOON, OUTPUT);
  pinMode(PIN_SUN, OUTPUT);
  
  ledcAttach(PIN_MOON, 5000, 16);
  ledcAttach(PIN_SUN, 5000, 16);
  
  ledcWrite(PIN_MOON, 0);
  ledcWrite(PIN_SUN, 0);
  
  Serial.println("Bereit. Warte auf BT...");
}

void loop() {
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) parseCommand(cmd);
  }

  if (taktMode > 0 && (moonOn || sunOn)) {
    handleTakt();
  } else {
    if (moonOn) ledcWriteTone(PIN_MOON, freqMoon);
    else ledcWriteTone(PIN_MOON, 0);
    
    if (sunOn) ledcWriteTone(PIN_SUN, freqSun);
    else ledcWriteTone(PIN_SUN, 0);
  }
  delay(1);
}

void parseCommand(String c) {
  c.toUpperCase();
  if (c == "MON") { moonOn = true; SerialBT.println("OK Mond AN"); }
  else if (c == "MOFF") { moonOn = false; ledcWriteTone(PIN_MOON, 0); SerialBT.println("OK Mond AUS"); }
  else if (c == "SON") { sunOn = true; SerialBT.println("OK Sonne AN"); }
  else if (c == "SOFF") { sunOn = false; ledcWriteTone(PIN_SUN, 0); SerialBT.println("OK Sonne AUS"); }
  else if (c.startsWith("M:")) { freqMoon = c.substring(2).toFloat(); SerialBT.println("OK"); }
  else if (c.startsWith("S:")) { freqSun = c.substring(2).toFloat(); SerialBT.println("OK"); }
  else if (c.startsWith("T:")) { taktMode = c.substring(2).toInt(); lastBeat = millis(); SerialBT.println("OK Takt"); }
  else if (c.startsWith("B:")) { bpm = c.substring(2).toInt(); SerialBT.println("OK BPM"); }
  else if (c == "STATUS") { 
    SerialBT.printf("M:%s %.1f S:%s %.1f T:%d\n", 
      moonOn?"ON":"OFF", freqMoon, sunOn?"ON":"OFF", freqSun, taktMode); 
  }
}

void handleTakt() {
  unsigned long bd = 60000UL / bpm;
  if (millis() - lastBeat >= bd) {
    lastBeat = millis();
    beatPhase = !beatPhase;
    if (moonOn) ledcWriteTone(PIN_MOON, beatPhase ? freqMoon : 0);
    if (sunOn) ledcWriteTone(PIN_SUN, beatPhase ? freqSun : 0);
  }
}