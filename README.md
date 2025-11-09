Smart Cane (Arduino Uno) — Obstacle, Ice-Risk & Fall Alert

An accessibility-focused Arduino project that turns a walking stick into a smart assistant. It gives proximity beeps for obstacles ahead, warns about slip risk (cold ground ≈ possible ice), and raises a fall alarm if the cane stays sideways too long—with a button to cancel.

Features

Obstacle beeper (HC-SR04)
Distance-based beep patterns:

Close (≤ ~10 cm): fast beeps

Medium (~10–30 cm): medium tempo

Far (~30–50 cm): slower beeps
(Thresholds/patterns are adjustable in code.)

Ice-risk warning (Thermistor on A0)
Estimates ground temperature via a voltage divider. If below a configurable threshold, plays a distinct ice-tone pattern.

Fall detection (tilt switch on D13)
If the cane remains sideways for ~3 s, a continuous alarm starts.
Press the cancel button (D3) to stop the alarm.

Debounce & state machine
Stable handling of tilt/button inputs and clear states: IDLE → SIDE_PENDING → ALARM.

Hardware

Arduino Uno (5 V)

HC-SR04 ultrasonic distance sensor

Thermistor (10 kΩ NTC, β≈3950) + fixed 10 kΩ resistor

Tilt switch (ball/mercury type)

Momentary push-button (cancel)

Piezo buzzer (active buzzer recommended)

Wires, breadboard, stick/cane mount

You can power the Uno from a USB power bank.

Pinout & Wiring
Function	Arduino Pin	Notes
Buzzer (+)	D2	Buzzer (–) → GND. Uses tone() for patterns.
Cancel Button	D3	One side → D3, other → GND. Pin is INPUT_PULLUP.
Tilt Switch	D13	One side → D13, other → GND. Pin is INPUT_PULLUP.
HC-SR04 TRIG	D9	Standard ultrasonic trigger.
HC-SR04 ECHO	D10	Standard ultrasonic echo (5 V-safe on Uno).
Thermistor Divider	A0	Junction → A0. Thermistor → 5 V. Fixed 10 kΩ → GND.
