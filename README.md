Run as administrator (due to raw port access)!
Hardware connection:
Pin 2 (Bit 0) to piezo for moons/frequencies
Pin 3 (Bit 1) to piezo for sun/frequencies
Pins 18-25 = GND
Via resistors/transistors (LPT only provides 3.3V/5V, low current)
What happens now:
LUNARIS (Mode 1): Generates precise low frequencies (e.g., 7.83 Hz Schumann resonance)
as a square wave on Pin 2. You can sprinkle sand onto a glass plate connected via Pin 2 and observe the Chladni figures.
SOLARIS (Mode 2): Generates audible/high-frequency oscillations on Pin 3
(up to the limit of the LPT port, approx. 20-50 kHz).
DUAL (Mode 3): Switches both pins on in time-division multiplexing (not perfectly parallel,
but fast enough for "both at the same time").
This is your "veil tool" as a single .exe file – no Arduino, no WSL,
directly to the PC's hardware pins.
Warning: The output is raw TTL (5V). For large piezo elements/amplifiers,
you will need a driver stage (transistor/MOSFET), otherwise you will damage the LPT port.
| Features | Operation |
| --------------- | --------------------------------------------------- |
| **Moon Side** | Enter frequency (0.0001 - 1000 Hz), ON/OFF button |
| **Sun Side** | Enter frequency (100 - 50000 Hz), ON/OFF button |
| **Clock Types** | Dropdown: Free, 4/4, 3/4, 6/8, 2/4 |
**Clock ON/OFF** | Checkbox next to dropdown |
**BPM** | Input field for clock speed (1-300) |
**Full Screen** | Button in the bottom right corner or drag the window by its corners |
**Background** | Automatically scales `sun_mon.jpg` from the folder |
**Icon** | Displays `pic.ico` in the title bar and taskbar |
Hardware connection:
Pin 2 (Data0) = Moon (Bit 0)
Pin 3 (Data1) = Sun (Bit 1)
Pins 18-25 = GND
Connect piezo discs to the pins via transistor drivers
Run as administrator (otherwise no LPT access)!
The window is fully resizable (drag all edges) and has a full-screen mode toggle.
How to connect the "veil glass":
1. The piezo transducer (recommended)
Connect a piezo buzzer (without an internal oscillator) or, better yet, a piezo transducer
(large disc) to pin 9 and GND.
Attach the piezo to the glass plate (using silicone or double-sided tape).
The piezo transmits the electrical oscillation 1:1 as a mechanical pressure wave into the glass.
2. The "sand veil" test (Chladni)
Sprinkle fine sand onto the glass plate.
Choose frequencies between 100 Hz and 3 kHz.
When the resonant frequency of the glass is reached, the sand arranges itself into visible patterns
(nodal lines) – the "veil" of matter becomes visible through the standing wave.
3. Frequency ranges for the "veil"
| Range | Effect on glass |
| ---------------- | ----------------------------------------------------------------- |
| **1 - 20 Hz** | Infrasound, macroscopic waves (like tides in "sandy water") |
| **100 - 500 Hz** | Deep hum, first standing waves visible |
| **1 - 5 kHz** | Brittle resonances, glassy sound |
| **20 - 40 kHz** | Ultrasound (invisible, but heat/washer effect) |
The "2-bit" principle in the code:
The code doesn't use an Arduino abstraction (digitalWrite), but direct bit masking on the
registers:
SET_BIT(PORTB, 5) instead of digitalWrite(13, HIGH)
TCCR1B is assembled bit by bit (prescaler, mode bits)
This gives you the hardest, jitter-free frequency, since the timer runs directly in hardware,
independent of the CPU code.
``` Warning:
At ultrasonic frequencies (>20kHz) and high amplitudes, glass can shatter!
Start with a low volume (resistor before the piezo element) and gradually increase it.
The "veil" falls when the resonant peak of the glass refracts the light or
the sand traces the nodal lines. This is your "making the invisible visible."
Here is the dual 2-bit low-level system: Two autonomous AVR cores (ATmega328P)
covering the entire frequency veil.
Chip 1 (SOLARIS) = High frequencies & clock (1Hz - 8MHz)
Chip 2 (LUNARIS) = Low frequencies & resonances (0.001Hz - 50kHz)
Both in bare-metal mode (direct register commands, no Arduino overhead).
CHIP 1: SOLARIS (The Sun - "Above")
Frequency range: 1 Hz to 8 MHz (square wave)
Features: System clock output, UV/X-ray simulation carrier (highest frequency),
Prescaler auto-select
CHIP 2: LUNARIS (The Moon - "Below")
Frequency range: 0.001 Hz (1 mHz!) to 50 kHz
Features: Schumann resonance (7.83 Hz), tides, ELF extreme, slow-motion vibrations for glass
The "veil protocol" (operation)
Hardware connection (both chips identical)

ATmega328P (28-Pin DIL)
├── Pin 9 (PB1/OC1A) → Output to glass/piezo (via transistor)
├── Pin 2 (PD2) → ON/OFF button (with pull-up resistor)
├── Pin 3 (PD3) → FREQUENCY + button
├── Pin 4 (PD4) → FREQUENCY - button
├── Pin 13 (PB5) → LED status
├── Pin 0 (PD0) → RX (UART for direct input)
├── Pin 1 (PD1) → TX (UART for debug)
└── 16MHz crystal oscillator → Required for precise frequencies!
Control via UART (terminal)
| Command | Sun (SOLARIS) | Moon (LUNARIS) |
-------- | ---------------------------------------- | ----------------------------------------------- |
**A** | Starts frequency (1 Hz - 8 MHz) | Starts frequency (0.001 Hz - 50 kHz) |
**S** | Stop | Stop |
**F123** | Sets frequency in Hz (e.g., F1000 = 1 kHz) | Sets frequency in **mHz** (e.g., F7830 = 7.83 Hz) |
**M** | - | Jumps to Schumann resonance (7.83 Hz) |
**T** | - | Tide/Extremely Slow (0.001 Hz) |
**C** | Outputs system clock on pin 8 | - |
Frequency ranges for "veil de-entry"
On the SOLARIS chip (high veil):
1 kHz - 20 kHz: Audible glass resonances (Chladni figures)
32 kHz: Ultrasound start (for fine crystal structures)
1 MHz - 8 MHz: RF carrier (externally multiplied for IR/UV/light)
On the LUNARIS chip (low veil):
7.83 Hz: Schumann resonance (biological tuning)
0.1 - 10 Hz: Tidal frequencies (delta wave region)
10 Hz - 100 Hz: ELF, Earth's magnetic field pulsations
Important limitation (physical check):
The ATmega can only directly generate square waves up to 8 MHz.
For true light frequencies (THz/PHz) you need:
A DDS chip (AD9833, AD9850) connected to the SOLARIS chip as a slave
An optical modulator (LED driver for infrared/UV)
Oscillators for specific bands (e.g., a 40kHz ultrasonic transducer connected directly to the output)
The code prepares the control: You can use the SOLARIS chip
to control an external DDS via I2C/SPI, which then generates the "invisible" THz frequencies
(Modeled veil opener).
This gives you two independent "ripple generators"—one for the cosmic heights (sun),
one for the Earth-connected depths (moon).
Both can run simultaneously and address different glass structures
(high frequencies for surface/sound, low frequencies for volume/space)

----------------------------------------------------------------
#special thx to https://highrez.co.uk/ for the *dll############
---------------------------------------------------------------

Als Administrator ausführen (wegen Raw Port Access)!
Hardware-Anschluss:
Pin 2 (Bit 0) an Piezo für Monde/Frequenzen
Pin 3 (Bit 1) an Piezo für Sonne/Frequenzen
Pin 18-25 = GND
Über Widerstände/Transistoren (LPT gibt nur 3.3V/5V, wenig Strom)

Was jetzt passiert:
LUNARIS (Modus 1): Erzeugt exakte tiefe Frequenzen (z.B. 7,83 Hz Schumann)
als Rechteck auf Pin 2. Du kannst Sand auf eine über Pin 2 angeschlossene
Glasplatte streuen und die Chladni-Figuren beobachten.
SOLARIS (Modus 2): Erzeugt hörbare/hochfrequente Schwingungen auf Pin 3
(bis zur Grenze des LPT-Ports, ca. 20-50kHz).
DUAL (Modus 3): Schaltet beide Pins im Zeitmultiplex an (nicht perfekt parallel,
aber schnell genug für „beides zugleich”).
Das ist dein „Schleier-Werkzeug” als einzelne .exe – ohne Arduino, ohne WSL,
direkt auf die Hardware-Pins des PCs.
Warnung: Die Ausgabe ist rohes TTL (5V). Für große Piezos/Verstärker brauchst
du noch eine Treiberstufe (Transistor/MOSFET), sonst zerstörst du den LPT-Port.

| Feature         | Bedienung                                           |
| --------------- | --------------------------------------------------- |
| **Mond-Seite**  | Frequenz eingeben (0.0001 - 1000 Hz), Button AN/AUS |
| **Sonne-Seite** | Frequenz eingeben (100 - 50000 Hz), Button AN/AUS   |
| **Taktarten**   | Dropdown: Frei, 4/4, 3/4, 6/8, 2/4                  |
| **Takt ON/OFF** | Checkbox neben Dropdown                             |
| **BPM**         | Eingabefeld für Taktspeed (1-300)                   |
| **Vollbild**    | Button unten rechts oder Fensterziehen an Ecken     |
| **Hintergrund** | Automatisch skaliert `sun_mon.jpg` aus dem Ordner   |
| **Icon**        | Zeigt `pic.ico` in der Titelleiste und Taskleiste   |

Hardware-Anschluss:
Pin 2 (Data0) = Mond (Bit 0)
Pin 3 (Data1) = Sonne (Bit 1)
Pin 18-25 = GND
Piezo-Scheiben über Transistor-Treiber an die Pins anschließen
Als Admin ausführen (sonst kein LPT-Zugriff)!
Das Fenster ist voll resizable (ziehen an allen Kanten) und hat einen Vollbild-Modus-Toggle.

Wie du das „Schleier-Glas“ anschließt:
1. Der Piezo-Transducer (empfohlen)
Schließe einen Piezo-Buzzer (ohne internen Oszillator) oder besser einen Piezo-Wandler
(große Scheibe) an Pin 9 und GND an.
Klebe den Piezo auf die Glasplatte (Silikon oder Doppelklebeband).
Der Piezo überträgt die elektrische Schwingung 1:1 als mechanische Druckwelle in das Glas.
2. Der „Sand-Schleier“ Test (Chladni)
Streue feinen Sand auf die Glasplatte.
Wähle Frequenzen zwischen 100Hz - 3kHz.
Wenn die Resonanzfrequenz des Glases getroffen wird, ordnet sich der Sand zu sichtbaren Mustern
(Knotenlinien) – der „Schleier“ der Materie wird durch die stehende Welle sichtbar.
3. Frequenz-Bereiche für den „Schleier“

| Bereich          | Wirkung auf Glas                                                  |
| ---------------- | ----------------------------------------------------------------- |
| **1 - 20 Hz**    | Infrasound, makroskopische Wellen (wie Gezeiten im „Sand-Wasser“) |
| **100 - 500 Hz** | Tiefes Brummen, erste stehende Wellen sichtbar                    |
| **1 - 5 kHz**    | Spröde Resonanzen, Glasklang                                      |
| **20 - 40 kHz**  | Ultraschall (unsichtbar, aber Wärme/Wäscher-Effekt)               |


Das „2-Bit“ Prinzip im Code:
Der Code nutzt keine Arduino-Abstraktion (digitalWrite), sondern direkte Bit-Maskierung auf den
Registern:
SET_BIT(PORTB, 5) statt digitalWrite(13, HIGH)
TCCR1B wird Bit für Bit zusammengesetzt (Prescaler, Mode-Bits)
Dies gibt dir die härteste, jitter-freieste Frequenz, da der Timer direkt in Hardware läuft,
unabhängig vom CPU-Code.
Warning:
Bei Ultraschall-Frequenzen (>20kHz) und hohen Amplituden kann Glas springen!
Beginne mit niedriger Lautstärke (Widerstand vor Piezo) und arbeite dich hoch.
Der „Schleier“ fällt, wenn die Resonanzüberhöhung des Glases das Licht beugt oder
der Sand die Knotenlinien zeichnet. Das ist dein „Sichtbarmachen des Unsichtbaren“.

Hier ist das Duale 2-Bit Low-Level System: Zwei autonome AVR-Cores (ATmega328P),
die den kompletten Frequenz-Schleier abdecken.
Chip 1 (SOLARIS) = Hohe Frequenzen & Taktgeber (1Hz - 8MHz)
Chip 2 (LUNARIS) = Tiefe Frequenzen & Resonanzen (0.001Hz - 50kHz)
Beide im Bare-Metal-Modus (direkte Register-Befehle, kein Arduino-Overhead).
CHIP 1: SOLARIS (Die Sonne - "Drüber")
Frequenzbereich: 1Hz bis 8MHz (Rechteck)
Features: Systemtakt-Auskopplung, UV/X-Ray-Simulations-Carrier (höchste Frequenz),
Prescaler-Auto-Select

CHIP 2: LUNARIS (Der Mond - "Drunter")
Frequenzbereich: 0.001Hz (1mHz!) bis 50kHz
Features: Schumann-Resonanz (7.83Hz), Gezeiten, ELF-Extrem, Zeitlupen-Schwingungen für Glas

Das "Schleier-Protokoll" (Bedienung)
Hardware-Anschluss (Beide Chips identisch)

ATmega328P (28-Pin DIL)
├── Pin  9 (PB1/OC1A) → Ausgang zum Glas/Piezo (über Transistor)
├── Pin  2 (PD2)    → Taste AN/AUS (mit Pull-Up)
├── Pin  3 (PD3)    → Taste FREQUENZ +
├── Pin  4 (PD4)    → Taste FREQUENZ -
├── Pin 13 (PB5)    → LED Status
├── Pin  0 (PD0)    → RX (UART für Direkteingabe)
├── Pin  1 (PD1)    → TX (UART für Debug)
└── 16MHz Quarz     → Für exakte Frequenzen nötig!


Steuerung über UART (Terminal)

| Befehl   | Sonne (SOLARIS)                          | Mond (LUNARIS)                                  |
| -------- | ---------------------------------------- | ----------------------------------------------- |
| **A**    | Startet Frequenz (1Hz - 8MHz)            | Startet Frequenz (0.001Hz - 50kHz)              |
| **S**    | Stopp                                    | Stopp                                           |
| **F123** | Setzt Frequenz in Hz (z.B. F1000 = 1kHz) | Setzt Frequenz in **mHz** (z.B. F7830 = 7,83Hz) |
| **M**    | -                                        | Springt auf Schumann 7,83Hz                     |
| **T**    | -                                        | Tide/Extrem-Langsam (0,001Hz)                   |
| **C**    | Systemtakt auf Pin 8 ausgeben            | -                                               |


Frequenz-Bereiche für die "Schleier-Enttarnung"
Auf dem SOLARIS-Chip (Hoher Schleier):
1kHz - 20kHz: Audible Glas-Resonanzen (Chladni-Figuren)
32kHz: Ultraschall-Anfang (für feine Kristall-Strukturen)
1MHz - 8MHz: RF-Carrier (wird extern multipliziert für IR/UV/Licht)
Auf dem LUNARIS-Chip (Tiefer Schleier):
7,83Hz: Schumann-Resonanz (biologische Abstimmung)
0,1 - 10Hz: Gezeiten-Frequenzen (Delta-Wellen-Gebiet)
10Hz - 100Hz: ELF, Erdmagnetfeld-Pulsationen
Wichtige Einschränkung (Physikalischer Check)
Der ATmega kann direkt nur Rechteckwellen bis 8MHz erzeugen.
Für echte Licht-Frequenzen (THz/PHz) brauchst du:
DDS-Chip (AD9833, AD9850) an den SOLARIS-Chip als Slave
Optischen Modulator (LED-Treiber für Infrarot/UV)
Oszillatoren für spezifische Bänder (z.B. 40kHz Ultraschall-Transducer direkt an den Ausgang)
Der Code bereitet die Steuerung vor: Du kannst den SOLARIS-Chip nutzen,
um per I2C/SPI einen externen DDS zu steuern, der dann die "unsichtbaren" THz-Frequenzen
erzeugt (Modellierter Schleier-Öffner).

Damit hast du zwei unabhängige "Welligkeitsgeneratoren" - einen für die kosmischen Höhen (Sonne),
einen für die erd-verbundenen Tiefen (Mond).
Beide können gleichzeitig laufen und verschiedene Glas-Strukturen ansprechen
(hohe Frequenzen für Oberfläche/Ton, tiefe für Volumen/Raum)
