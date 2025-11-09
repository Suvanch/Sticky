// -------- Pins --------
const int TILT_PIN   = 13;   // tilt switch to GND (INPUT_PULLUP)
const int BTN_PIN    = 3;    // button to GND (INPUT_PULLUP)
const int BUZZER_PIN = 2;    // buzzer (+) here, (-) to GND

// Ultrasonic
const int TRIG_PIN   = 9;
const int ECHO_PIN   = 10;

// ---- Ice sensor (thermistor) ----
const int THERM_PIN = A0;  // junction between thermistor and 10k

const float SERIES_RESISTOR     = 10000.0;  // 10k fixed resistor
const float NOMINAL_RESISTANCE  = 10000.0;  // 10k at 25°C
const float NOMINAL_TEMPERATURE = 25.0;     // 25 °C
const float B_COEFFICIENT       = 3950.0;   // typical value
const float KELVIN_OFFSET       = 273.15;
const float ICE_THRESHOLD_C     = 18.0;     // adjust after testing

// -------- Timings --------
const unsigned long TILT_DEBOUNCE_MS  = 80;
const unsigned long FALL_HOLD_MS      = 3000;  // side >= 3s
const unsigned long BTN_DEBOUNCE_MS   = 60;

// Proximity beep windows (ms)
struct Pattern { unsigned onMs; unsigned offMs; unsigned freq; };
const Pattern PAT_CLOSE  = {120, 120, 2200};   // <= 10 cm
const Pattern PAT_MED    = {150, 200, 2000};   // 10–30 cm
const Pattern PAT_FAR    = {200, 400, 1800};   // 30–50 cm

// -------- State machine for fall --------
enum State { IDLE, SIDE_PENDING, ALARM };
State state = IDLE;
unsigned long stateStart = 0;

// Tilt debounce
int tiltLastRaw = HIGH, tiltStable = HIGH;
unsigned long tiltLastChange = 0;

// Button debounce (simplified)
int btnLastRaw = HIGH, btnStable = HIGH;
unsigned long btnLastChange = 0;

// Proximity beeper state
unsigned long buzzWindowStart = 0;
bool buzzOnWindow = false;

// Ice warning state
bool  iceRiskGlobal = false;
bool  iceBeepOn     = false;
unsigned long icePhaseStart = 0;

// -------- Helpers --------
void beepOff() {
  noTone(BUZZER_PIN);
}

// -------- Debounce & reads --------
void readTilt() {
  unsigned long now = millis();
  int raw = digitalRead(TILT_PIN);
  if (raw != tiltLastRaw) {
    tiltLastRaw = raw;
    tiltLastChange = now;
  }
  if (now - tiltLastChange >= TILT_DEBOUNCE_MS) {
    tiltStable = raw;
  }
}

bool isSideways() {
  // With INPUT_PULLUP and one leg to GND, many tilt switches read LOW when sideways.
  // If yours is opposite, flip this to (tiltStable == HIGH).
  return tiltStable == HIGH;
}

void readButton() {
  unsigned long now = millis();
  int raw = digitalRead(BTN_PIN);
  if (raw != btnLastRaw) {
    btnLastRaw = raw;
    btnLastChange = now;
  }
  if (now - btnLastChange >= BTN_DEBOUNCE_MS) {
    btnStable = raw;  // debounced
  }
}

bool buttonPressedNow() {
  return (btnStable == LOW);   // active-LOW with INPUT_PULLUP
}

// -------- Fall state transitions --------
void enter(State s) {
  state = s;
  stateStart = millis();
  if (s == ALARM) {
    tone(BUZZER_PIN, 2000);  // continuous alarm
    Serial.println("FALL ALARM ACTIVE!");
  } else {
    // IDLE or SIDE_PENDING
    beepOff();
    buzzOnWindow   = false;
    iceBeepOn      = false;
    icePhaseStart  = millis();
    Serial.println("Alarm stopped / normal mode.");
  }
}

// -------- Ultrasonic distance --------
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return -1; // no echo
  return (duration * 0.0343f) / 2.0f;
}

// -------- Proximity beeper (owns buzzer ONLY when called) --------
void runProximityBeep() {
  static unsigned long lastMeasure = 0;
  static float lastDistance = -1;

  unsigned long now = millis();
  if (now - lastMeasure >= 60) {
    lastMeasure = now;
    lastDistance = readDistanceCm();
  }

  const Pattern* p = nullptr;
  if (lastDistance > 0 && lastDistance <= 10)        p = &PAT_CLOSE;
  else if (lastDistance > 10 && lastDistance <= 30)  p = &PAT_MED;
  else if (lastDistance > 30 && lastDistance <= 50)  p = &PAT_FAR;

  if (!p) {
    buzzOnWindow = false;
    beepOff();
    return;
  }

  if (!buzzOnWindow) {
    buzzOnWindow    = true;
    buzzWindowStart = now;
    tone(BUZZER_PIN, p->freq);
  } else {
    unsigned long elapsed = now - buzzWindowStart;
    if (elapsed >= p->onMs) {
      beepOff();
      if (elapsed >= (unsigned long)(p->onMs + p->offMs)) {
        buzzOnWindow = false;
      }
    }
  }
}

// ---- Ice sensor: read temperature in °C ----
float readThermistorC() {
  int adc = analogRead(THERM_PIN);
  if (adc == 0) return 999.0; // avoid divide-by-zero edge-case

  float resistance = SERIES_RESISTOR * (1023.0 / adc - 1.0);
  float steinhart = resistance / NOMINAL_RESISTANCE;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + KELVIN_OFFSET);
  steinhart = 1.0 / steinhart;
  float tempC = steinhart - KELVIN_OFFSET;
  return tempC;
}

// ---- Ice detection: update global iceRisk ----
void updateIceRisk() {
  static unsigned long lastSample = 0;
  static float lastTempC = 0;

  unsigned long now = millis();
  if (now - lastSample >= 1000) { // once per second
    lastSample = now;
    lastTempC  = readThermistorC();
    iceRiskGlobal = (lastTempC <= ICE_THRESHOLD_C);

    Serial.print("TempC: ");
    Serial.print(lastTempC);
    Serial.print("  Ice risk: ");
    Serial.println(iceRiskGlobal ? "YES" : "NO");
  }
}

// ---- Ice warning buzzer pattern (owns buzzer ONLY when called) ----
void runIceWarningPattern() {
  // Only call this when iceRiskGlobal == true AND not in ALARM
  const unsigned long ON_MS  = 350;
  const unsigned long OFF_MS = 850;

  unsigned long now = millis();
  if (!iceBeepOn) {
    iceBeepOn = true;
    icePhaseStart = now;
    tone(BUZZER_PIN, 1400); // distinct ice tone
  } else {
    unsigned long elapsed = now - icePhaseStart;
    if (elapsed >= ON_MS) {
      beepOff();
    }
    if (elapsed >= ON_MS + OFF_MS) {
      iceBeepOn = false;   // start new cycle
    }
  }
}

void setup() {
  pinMode(TILT_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN,  INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);
  beepOff();
  enter(IDLE);
}

void loop() {
  readTilt();
  readButton();
  updateIceRisk();   // updates iceRiskGlobal once per second

  // Button cancels ALARM
  if (state == ALARM && buttonPressedNow()) {
    enter(IDLE);
  }

  // Fall detection state machine
  bool sideways = isSideways();

  switch (state) {
    case IDLE:
      if (sideways) enter(SIDE_PENDING);
      break;

    case SIDE_PENDING:
      if (!sideways) {
        enter(IDLE);
        break;
      }
      if (millis() - stateStart >= FALL_HOLD_MS) {
        enter(ALARM);
      }
      break;

    case ALARM:
      // continuous tone already set in enter(ALARM)
      break;
  }

  // -------- Buzzer arbitration --------
  // Only ONE of these owns the buzzer at a time:
  // 1) ALARM (highest priority)
  // 2) Ice warning (if risk)
  // 3) Proximity beeps

  if (state == ALARM) {
    // alarm tone already playing; do nothing
    return;
  }

  if (iceRiskGlobal) {
    // ice warning pattern owns buzzer
    runIceWarningPattern();
  } else {
    // obstacle beeps own buzzer
    runProximityBeep();
  }
}
