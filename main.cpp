/*
  Arduino Mega 2560 & 7x 555 Timer Passive Sequencer
  - Button on pin 29 triggers 7 timers on pins 22-28.
  - Measures the order and ON-time of each timer.
  - Blinks 7 LEDs on pins 30-36 according to the results.
*/

#include <Arduino.h>

// Pin Definitions
const int NUM_TIMERS = 7;
const int BUTTON_PIN = 29;
const int TIMER_PINS[NUM_TIMERS] = {22, 23, 24, 25, 26, 27, 28};
const int LED_PINS[NUM_TIMERS] = {30, 31, 32, 33, 34, 35, 36};

// Global variables to track state
bool sequenceIsActive = false;
unsigned long sequenceStartTime = 0;
unsigned long timerHighStartTimes[NUM_TIMERS] = {0};
unsigned long timerOnDurations[NUM_TIMERS] = {0};
bool currentTimerState[NUM_TIMERS] = {false};
int highSignalOrder[NUM_TIMERS] = {0};
int highTimersCount = 0;

// LED blinking variables
unsigned long ledLastToggleTimes[NUM_TIMERS] = {0};
bool ledStates[NUM_TIMERS] = {false};

// Function Declarations
void startNewSequence();
void monitorTimerPins();
void printFinalResults();
void blinkLedsAsynchronously();
String getOrdinalSuffix(int n);


void setup() {
  Serial.begin(9600);
  Serial.println("Arduino 7x 555 Timer Sequencer Initialized.");

  // Setup pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  for (int i = 0; i < NUM_TIMERS; i++) {
    pinMode(TIMER_PINS[i], INPUT);
  }

  for (int i = 0; i < NUM_TIMERS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
}

void loop() {
  // 1. Check for button press
  if (!sequenceIsActive && digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      startNewSequence();
    }
  }

  // 2. Monitor timers if running
  if (sequenceIsActive) {
    monitorTimerPins();
  }

  // 3. Blink LEDs when finished
  if (!sequenceIsActive && highTimersCount == NUM_TIMERS) {
    blinkLedsAsynchronously();
  }
}

// Resets all variables for a new run
void startNewSequence() {
  Serial.println("\n--- Starting New Sequence ---");
  sequenceIsActive = true;
  sequenceStartTime = millis();
  highTimersCount = 0;

  for (int i = 0; i < NUM_TIMERS; i++) {
    timerHighStartTimes[i] = 0;
    timerOnDurations[i] = 0;
    currentTimerState[i] = false;
    highSignalOrder[i] = 0;
    ledStates[i] = false;
    ledLastToggleTimes[i] = 0;
    digitalWrite(LED_PINS[i], LOW);
  }
}

// Checks timer pins for changes
void monitorTimerPins() {
  unsigned long currentTime = millis();

  for (int i = 0; i < NUM_TIMERS; i++) {
    int pin = TIMER_PINS[i];
    bool isPinHigh = (digitalRead(pin) == HIGH);

    // Timer turns ON (Rising edge)
    if (isPinHigh && !currentTimerState[i]) {
      currentTimerState[i] = true;
      timerHighStartTimes[i] = currentTime;

      if (highTimersCount < NUM_TIMERS) {
        highSignalOrder[highTimersCount] = pin;
        highTimersCount++;
        
        unsigned long elapsedTime = currentTime - sequenceStartTime;
        Serial.print("Timer on pin ");
        Serial.print(pin);
        Serial.print(" fired ");
        Serial.print(highTimersCount);
        Serial.print(getOrdinalSuffix(highTimersCount));
        Serial.print(" at ");
        Serial.print(elapsedTime);
        Serial.println(" ms");
      }
    }
    // Timer turns OFF (Falling edge)
    else if (!isPinHigh && currentTimerState[i]) {
      currentTimerState[i] = false;
      timerOnDurations[i] = currentTime - timerHighStartTimes[i];
      
      Serial.print("  -> Timer on pin ");
      Serial.print(pin);
      Serial.print(" OFF. Duration: ");
      Serial.print(timerOnDurations[i]);
      Serial.println(" ms");
    }
  }

  // Check if all timers are done
  bool allTimersFinished = true;
  for(int i = 0; i < NUM_TIMERS; i++) {
      if(timerOnDurations[i] == 0) {
          allTimersFinished = false;
          break;
      }
  }

  if (highTimersCount == NUM_TIMERS && allTimersFinished) {
    sequenceIsActive = false;
    Serial.println("--- Sequence Complete ---");
    printFinalResults();
  }
}

// Prints the results to the serial monitor
void printFinalResults() {
  Serial.println("\n--- Final Results ---");
  Serial.println("Firing Order:");
  for (int i = 0; i < NUM_TIMERS; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(": Pin ");
    Serial.println(highSignalOrder[i]);
  }

  Serial.println("\nBlink Rates (ms):");
  for (int i = 0; i < NUM_TIMERS; i++) {
    int orderedPin = highSignalOrder[i];
    unsigned long duration = 0;
    
    for(int j=0; j < NUM_TIMERS; j++){
      if(TIMER_PINS[j] == orderedPin){
        duration = timerOnDurations[j];
        break;
      }
    }
    
    Serial.print("  LED ");
    Serial.print(i + 1);
    Serial.print(" (Pin ");
    Serial.print(LED_PINS[i]);
    Serial.print(") Rate: ");
    Serial.print(duration);
    Serial.println(" ms");
  }
  Serial.println("\nStarting LED blinking...");
}

// Blinks the LEDs without using delay()
void blinkLedsAsynchronously() {
  unsigned long currentTime = millis();

  for (int i = 0; i < NUM_TIMERS; i++) {
    int orderedPin = highSignalOrder[i];
    unsigned long blinkDuration = 0;

    for (int j = 0; j < NUM_TIMERS; j++) {
      if (TIMER_PINS[j] == orderedPin) {
        blinkDuration = timerOnDurations[j];
        break;
      }
    }

    if (blinkDuration == 0) continue;

    if (currentTime - ledLastToggleTimes[i] >= blinkDuration) {
      ledLastToggleTimes[i] = currentTime;
      ledStates[i] = !ledStates[i];
      digitalWrite(LED_PINS[i], ledStates[i]);
    }
  }
}

// Helper to get "st", "nd", "rd", "th"
String getOrdinalSuffix(int n) {
  if (n % 100 >= 11 && n % 100 <= 13) {
    return "th";
  }
  switch (n % 10) {
    case 1: return "st";
    case 2: return "nd";
    case 3: return "rd";
    default: return "th";
  }
}
