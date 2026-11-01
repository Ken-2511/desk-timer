// desk-timer smoke test: LED chaser (跑马灯) plus button and buzzer checks.
//
// - LEDs D1-D6 chase left to right, bouncing back at each end.
// - SW1 (start/pause, D8) pauses or resumes the chaser.
// - SW2 (reset, D9) jumps back to the leftmost LED.
// - The buzzer (D10) beeps once at power-up and on every button press.

const uint8_t LED_PINS[] = {7, 6, 5, 4, 3, 2};  // D1..D6, left to right
const uint8_t LED_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

const uint8_t BTN_START = 8;   // SW1, active LOW
const uint8_t BTN_RESET = 9;   // SW2, active LOW
const uint8_t BUZZER = 10;

const unsigned int BEEP_HZ = 4000;
const unsigned long STEP_MS = 120;
const unsigned long DEBOUNCE_MS = 30;

int8_t position = 0;
int8_t direction = 1;
bool paused = false;
unsigned long lastStep = 0;

struct Button {
  uint8_t pin;
  bool stableState;      // true = released (HIGH)
  bool lastReading;
  unsigned long changedAt;
};

Button startBtn = {BTN_START, true, true, 0};
Button resetBtn = {BTN_RESET, true, true, 0};

// Returns true once per press, after debouncing.
bool pressed(Button &b) {
  bool reading = digitalRead(b.pin);
  if (reading != b.lastReading) {
    b.lastReading = reading;
    b.changedAt = millis();
  }
  if (millis() - b.changedAt > DEBOUNCE_MS && reading != b.stableState) {
    b.stableState = reading;
    return reading == LOW;
  }
  return false;
}

void beep(unsigned long ms) {
  tone(BUZZER, BEEP_HZ, ms);
}

void showLed(int8_t index) {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], i == index ? HIGH : LOW);
  }
}

void setup() {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  // Power-up check: all LEDs on with a beep, then start chasing.
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }
  beep(200);
  delay(500);
  showLed(position);
  lastStep = millis();
}

void loop() {
  if (pressed(startBtn)) {
    paused = !paused;
    beep(50);
  }
  if (pressed(resetBtn)) {
    position = 0;
    direction = 1;
    showLed(position);
    lastStep = millis();
    beep(50);
  }

  if (!paused && millis() - lastStep >= STEP_MS) {
    lastStep += STEP_MS;
    if (position + direction < 0 || position + direction >= LED_COUNT) {
      direction = -direction;
    }
    position += direction;
    showLed(position);
  }
}
