#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

const uint8_t pauseButtonPin = 2;
const uint8_t resetButtonPin = 3;

const unsigned long debounceDelayMs = 30;

bool running = false;
unsigned long baseElapsedMs = 0;
unsigned long runStartMs = 0;

bool pauseRawState = LOW;
bool pauseStableState = LOW;
unsigned long pauseLastChangeMs = 0;

bool resetRawState = LOW;
bool resetStableState = LOW;
unsigned long resetLastChangeMs = 0;

int lastHours = -1;
int lastMinutes = -1;
int lastSeconds = -1;
int lastMilliseconds = -1;

unsigned long getElapsedTimeMs() {
  if (running) {
    return baseElapsedMs + (millis() - runStartMs);
  }

  return baseElapsedMs;
}

void printTwoDigits(uint8_t col, uint8_t row, unsigned int value) {
  lcd.setCursor(col, row);

  if (value < 10) {
    lcd.print('0');
  }

  lcd.print(value);
}

void printThreeDigits(uint8_t col, uint8_t row, unsigned int value) {
  lcd.setCursor(col, row);

  if (value < 100) {
    lcd.print('0');
  }

  if (value < 10) {
    lcd.print('0');
  }

  lcd.print(value);
}

void updateTimeDisplay(unsigned long elapsedMs) {
  const unsigned int hours = (elapsedMs / 3600000UL) % 100;
  const unsigned int minutes = (elapsedMs / 60000UL) % 60;
  const unsigned int seconds = (elapsedMs / 1000UL) % 60;
  const unsigned int milliseconds = elapsedMs % 1000;

  if ((int)hours != lastHours) {
    printTwoDigits(2, 1, hours);
    lastHours = hours;
  }

  if ((int)minutes != lastMinutes) {
    printTwoDigits(5, 1, minutes);
    lastMinutes = minutes;
  }

  if ((int)seconds != lastSeconds) {
    printTwoDigits(8, 1, seconds);
    lastSeconds = seconds;
  }

  if ((int)milliseconds != lastMilliseconds) {
    printThreeDigits(11, 1, milliseconds);
    lastMilliseconds = milliseconds;
  }
}

void togglePauseResume() {
  if (running) {
    baseElapsedMs = getElapsedTimeMs();
    running = false;
  } else {
    runStartMs = millis();
    running = true;
  }
}

void resetTimer() {
  baseElapsedMs = 0;

  if (running) {
    runStartMs = millis();
  }

  lastHours = -1;
  lastMinutes = -1;
  lastSeconds = -1;
  lastMilliseconds = -1;

  updateTimeDisplay(0);
}

void handlePauseButton() {
  const bool reading = digitalRead(pauseButtonPin);
  const unsigned long nowMs = millis();

  if (reading != pauseRawState) {
    pauseRawState = reading;
    pauseLastChangeMs = nowMs;
  }

  if ((nowMs - pauseLastChangeMs) >= debounceDelayMs && pauseStableState != pauseRawState) {
    pauseStableState = pauseRawState;

    if (pauseStableState == HIGH) {
      togglePauseResume();
    }
  }
}

void handleResetButton() {
  const bool reading = digitalRead(resetButtonPin);
  const unsigned long nowMs = millis();

  if (reading != resetRawState) {
    resetRawState = reading;
    resetLastChangeMs = nowMs;
  }

  if ((nowMs - resetLastChangeMs) >= debounceDelayMs && resetStableState != resetRawState) {
    resetStableState = resetRawState;

    if (resetStableState == HIGH) {
      resetTimer();
    }
  }
}

void setup() {
  pinMode(pauseButtonPin, INPUT);
  pinMode(resetButtonPin, INPUT);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(3, 0);
  lcd.print("CRONOMETRO");

  lcd.setCursor(2, 1);
  lcd.print("00:00:00.000");

  resetTimer();
}

void loop() {
  handlePauseButton();
  handleResetButton();
  updateTimeDisplay(getElapsedTimeMs());
}