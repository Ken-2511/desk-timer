// desk-timer: countdown timer with a 6-bit binary LED display.
//
// The LEDs stay off unless a button was pressed in the last 3 seconds.
// D1 (left) is the highest bit and D6 (right) the lowest.
//
// IDLE      SW2 steps through 1, 5, 10, 25, 45, 60 minutes, shown in binary.
//           SW1 starts the timer.
// RUNNING   The LEDs count down from 63 to 0 in equal steps.
//           SW1 shows the count, SW2 cancels the timer.
// FINISHED  Beep-beep-beep, 1 s pause, repeated, with the LEDs flashing.
//           Any button stops it; it also stops by itself after 10 minutes.
//
// While the LEDs are off, the first SW2 press only lights them, so the
// selection is never changed (or the timer cancelled) without seeing it.

// ---------- Pins ----------
const int LED_PINS[6] = {7, 6, 5, 4, 3, 2};  // D1..D6, highest bit first
const int START_BUTTON = 8;                  // SW1, reads LOW when pressed
const int RESET_BUTTON = 9;                  // SW2, reads LOW when pressed
const int BUZZER = 10;

// ---------- Settings ----------
const int LENGTHS_MIN[6] = {1, 5, 10, 25, 45, 60};
const unsigned long DISPLAY_ON_MS = 3000;
const unsigned long DEBOUNCE_MS = 10;
const unsigned long ALARM_BEEP_MS = 1000 / 6;   // each beep and each gap
const unsigned long ALARM_PAUSE_MS = 1000;      // silence after 3 beeps
const unsigned long ALARM_TIMEOUT_MS = 10UL * 60 * 1000;
const int BEEP_HZ = 4000;

// ---------- State ----------
enum State { IDLE, RUNNING, FINISHED };
enum Button { NONE, START, RESET };

State state = IDLE;
int lengthIndex = 3;  // 25 min at power-up
unsigned long timerStartMs = 0;
unsigned long timerLengthMs = 0;
unsigned long alarmStartMs = 0;
unsigned long lastPressMs = 0;
bool buzzerOn = false;

void setup() {
  for (int i = 0; i < 6; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  lastPressMs = millis();  // show the selection for 3 s at power-up
}

void loop() {
  bool displayWasOn = isDisplayOn();
  Button button = readButton();

  switch (state) {
    case IDLE:
      if (button == START) {
        startTimer();
      } else if (button == RESET && displayWasOn) {
        lengthIndex = (lengthIndex + 1) % 6;
      }
      showIfDisplayOn(LENGTHS_MIN[lengthIndex]);
      break;

    case RUNNING: {
      unsigned long elapsedMs = millis() - timerStartMs;
      if (button == RESET && displayWasOn) {
        state = IDLE;
      } else if (elapsedMs >= timerLengthMs) {
        startAlarm();
      } else {
        showIfDisplayOn(countdownValue(elapsedMs));
      }
      break;
    }

    case FINISHED:
      if (button != NONE || alarmIsOver()) {
        stopAlarm();
        state = IDLE;
      } else {
        playAlarm();
      }
      break;
  }
}

// ---------- Timer ----------

void startTimer() {
  timerLengthMs = LENGTHS_MIN[lengthIndex] * 60UL * 1000;
  timerStartMs = millis();
  state = RUNNING;
}

// Splits the timer into 63 equal steps: 63 at the start, 1 in the last step.
// It would reach 0 exactly when the timer ends, so the LEDs never all go dark.
int countdownValue(unsigned long elapsedMs) {
  return 63 - elapsedMs * 63 / timerLengthMs;
}

// ---------- LEDs ----------

bool isDisplayOn() {
  return millis() - lastPressMs < DISPLAY_ON_MS;
}

// Shows a number from 0 to 63 in binary. 0 turns all LEDs off.
void showNumber(int value) {
  for (int i = 0; i < 6; i++) {
    int bit = (value >> (5 - i)) & 1;
    digitalWrite(LED_PINS[i], bit ? HIGH : LOW);
  }
}

void showIfDisplayOn(int value) {
  showNumber(isDisplayOn() ? value : 0);
}

// ---------- Alarm ----------

void startAlarm() {
  alarmStartMs = millis();
  state = FINISHED;
}

// One cycle: beep, gap, beep, gap, beep, then a 1 s pause.
// The LEDs light up together with each beep.
void playAlarm() {
  const unsigned long cycleMs = 5 * ALARM_BEEP_MS + ALARM_PAUSE_MS;
  unsigned long slot = ((millis() - alarmStartMs) % cycleMs) / ALARM_BEEP_MS;
  bool beepNow = slot == 0 || slot == 2 || slot == 4;

  if (beepNow && !buzzerOn) {
    tone(BUZZER, BEEP_HZ);
  } else if (!beepNow && buzzerOn) {
    noTone(BUZZER);
  }
  buzzerOn = beepNow;

  showNumber(beepNow ? 63 : 0);
}

bool alarmIsOver() {
  return millis() - alarmStartMs >= ALARM_TIMEOUT_MS;
}

void stopAlarm() {
  noTone(BUZZER);
  buzzerOn = false;
}

// ---------- Buttons ----------

// Returns which button was pressed, or NONE.
// After a press it waits for the release, then gives a short click beep.
// The program pauses while a button is held, but the timer keeps
// counting because it is based on millis().
Button readButton() {
  Button button;
  int pin;
  if (isPressed(START_BUTTON)) {
    button = START;
    pin = START_BUTTON;
  } else if (isPressed(RESET_BUTTON)) {
    button = RESET;
    pin = RESET_BUTTON;
  } else {
    return NONE;
  }

  stopAlarm();  // silence the alarm as soon as a button goes down

  while (digitalRead(pin) == LOW) {
    // wait for release
  }
  delay(DEBOUNCE_MS);  // ignore bounce on release

  tone(BUZZER, BEEP_HZ, 30);
  lastPressMs = millis();
  return button;
}

// True if the pin still reads LOW after the debounce time.
bool isPressed(int pin) {
  if (digitalRead(pin) == HIGH) {
    return false;
  }
  delay(DEBOUNCE_MS);
  return digitalRead(pin) == LOW;
}
