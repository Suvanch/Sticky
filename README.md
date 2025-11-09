SMART CANE (ARDUINO UNO) — OBSTACLE, SLIP‑RISK & FALL ALERT
=========================================================

An accessibility‑focused Arduino project that turns a walking stick into a smart assistant.
It gives proximity beeps for obstacles ahead, warns about slip risk (cold ground ≈ possible ice),
and raises a fall alarm if the cane stays sideways too long—with a button to cancel.

Files
-----
- final_code.ino   The Arduino sketch for the Smart Cane.

Features
--------
- Obstacle beeper (HC‑SR04):
  • Distance‑based beep patterns (close / medium / far), thresholds are configurable in code.

- Slip‑risk warning (thermistor on A0):
  • Estimates ground temperature via a voltage divider.
  • If below a configurable threshold, plays a distinct “ice‑risk” tone pattern.

- Fall detection (tilt switch on D13) + Cancel button (D3):
  • If the cane remains sideways for ~3 seconds, a fall alarm starts.
  • Press the cancel button to stop the alarm immediately.

- Debounce & state machine:
  • Stable handling of tilt/button inputs with clear states (IDLE → SIDE_PENDING → ALARM).

Hardware
--------
- Arduino Uno (5 V)
- HC‑SR04 ultrasonic distance sensor
- Thermistor (10 kΩ NTC, β≈3950) + fixed 10 kΩ resistor (voltage divider)
- Tilt switch (ball/mercury type)
- Momentary push‑button (cancel/“I’m OK”)
- Piezo buzzer (active recommended) or vibration motor (if used, drive with transistor + flyback diode)
- Wires, breadboard or proto board, cane/stick mounts
- Optional: USB power bank for portable power

Pinout & Wiring
---------------
- Buzzer (+)        → D2          (Buzzer − → GND; uses tone() for patterns)
- Cancel Button     → D3          (One side to pin, other side to GND; pin uses INPUT_PULLUP)
- Tilt Switch       → D13         (One side to pin, other side to GND; pin uses INPUT_PULLUP)
- HC‑SR04 TRIG      → D9
- HC‑SR04 ECHO      → D10         (5 V‑safe on Uno)
- Thermistor Divider→ A0          (Junction to A0; Thermistor → 5 V; Fixed 10 kΩ → GND)

Thermistor divider:
  5V ---[ Thermistor (NTC) ]---o---[ 10kΩ ]--- GND
                               |
                               A0

Setup
-----
1) Open final_code.ino in Arduino IDE (Board: “Arduino Uno”).
2) Select the correct Port (Tools → Port → your /dev/cu.* or COM*).
3) Click Verify, then Upload.
4) (Optional) Open Serial Monitor at 9600 baud for debug messages.

Key Configuration (tune in code)
--------------------------------
- Pins:
  TILT_PIN=13, BTN_PIN=3, BUZZER_PIN=2, TRIG_PIN=9, ECHO_PIN=10, THERM_PIN=A0

- Thermistor model:
  SERIES_RESISTOR=10000.0 (10 kΩ fixed)
  NOMINAL_RESISTANCE=10000.0 (10 kΩ @ 25°C)
  NOMINAL_TEMPERATURE=25.0 (°C)
  B_COEFFICIENT=3950.0
  ICE_THRESHOLD_C=18.0 (adjust after testing; lower for stricter “near‑freezing only”)

- Fall timing & debounce:
  TILT_DEBOUNCE_MS=80
  FALL_HOLD_MS=3000      (sideways ≥ 3 s → alarm)
  BTN_DEBOUNCE_MS=60

- Proximity beep patterns (example):
  PAT_CLOSE  = {on 120 ms, off 120 ms, freq 2200 Hz}   (≤ ~10 cm)
  PAT_MED    = {on 150 ms, off 200 ms, freq 2000 Hz}   (~10–30 cm)
  PAT_FAR    = {on 200 ms, off 400 ms, freq 1800 Hz}   (~30–50 cm)
  (Feel free to tweak on/off durations and frequencies.)

How It Works
------------
- Ultrasonic distance is sampled and mapped to beep patterns.
- Thermistor reading is converted to temperature (B‑coefficient method).
- If temperature is below ICE_THRESHOLD_C, the “ice‑risk” tone overrides proximity beeps.
- Tilt switch triggers a fall countdown. If still sideways when the timer expires, alarm sounds.
- Press the button anytime during the countdown or alarm to cancel.
- When upright again and stable, the system returns to IDLE and stops alerts.

Testing & Calibration
---------------------
- Ultrasonic: try targets at known distances; adjust thresholds/patterns to taste.
- Thermistor: compare reported °C against a reference (room temp, ice pack) and adjust
  ICE_THRESHOLD_C for your environment.
- Tilt/Alarm: place cane sideways—alarm should start ~3 seconds later. Press the button to cancel.

Power Tips
----------
- A USB power bank works well. Keep wiring secure and strain‑relieved at the cane tip.
- If you switch to a vibration motor for quiet haptics:
  • Drive through an NPN transistor (e.g., 2N2222) and add a flyback diode across the motor.

Troubleshooting
---------------
- No serial port in IDE: try a data‑capable USB cable/adapter; unplug/replug; restart IDE.
- Ultrasonic reads −1 or constant: check TRIG/ECHO wiring and common ground; avoid soft targets/angles.
- Thermistor nonsense values: verify the divider (A0 at the junction); confirm resistor values.
- Alarm never triggers: ensure tilt switch actually closes to GND when sideways (INPUT_PULLUP logic → LOW is active).
- Button doesn’t cancel: confirm wiring to GND and that pin is set to INPUT_PULLUP.

Roadmap (Nice‑to‑Have)
----------------------
- Add a water/puddle sensor near the tip; combine with temperature for better slip‑risk scoring.
- Bluetooth (HC‑05 / BLE) to send fall alerts to a phone.
- Low‑power mode, Li‑ion pack, and enclosure.

License
-------
MIT (recommended). Add a LICENSE file if you plan to publish.

Acknowledgments
---------------
Built for accessibility—multi‑modal sensing with distinct haptic/audio cues to reduce slips,
avoid obstacles, and escalate help on falls.
