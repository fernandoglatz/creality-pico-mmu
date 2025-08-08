#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Servo.h>

#define LED_PIN 5
#define BUZZER_PIN 6

#define MMU_DIR_PIN 7
#define MMU_STEP_PIN 8
#define MMU_ENABLE_PIN 9

#define MMU_SERVO_PIN 10
#define CUTTER_SERVO_PIN 3

#define BAUD_RATE 9600

#define MMU_SLOW_PULSE_DELAY 50
#define MMU_ACCEL_DECEL_SKIP_STEPS 200UL
#define MMU_MOTOR_STEPS 200
#define MMU_DEFAULT_RPM 500
#define MMU_MICROSTEPS 64
#define MMU_MIN_RPM 50
#define MMU_DIRECTION HIGH
const unsigned long MMU_SENSOR_CHECK_INTERVAL = (unsigned long)MMU_MOTOR_STEPS * (unsigned long)MMU_MICROSTEPS * (unsigned long)10;

#define NUM_LEDS 16  // 2 bars of 8 LEDs each
#define NUMBER_OF_FILAMENTS 8
#define FILAMENT_RELEASE_OFFSET 2

#define FILAMENT_HUB_SENSOR_PIN 2

#define ACTION_BUTTON_PIN 0
#define CREALITY_FILAMENT_SENSOR_PIN 1

#define FILAMENT_ONE_SENSOR_PIN 15
#define FILAMENT_TWO_SENSOR_PIN 14
#define FILAMENT_THREE_SENSOR_PIN 13
#define FILAMENT_FOUR_SENSOR_PIN 12
#define FILAMENT_FIVE_SENSOR_PIN 11
#define FILAMENT_SIX_SENSOR_PIN 10
#define FILAMENT_SEVEN_SENSOR_PIN 9
#define FILAMENT_EIGHT_SENSOR_PIN 8

#define ALIVE_MESSAGE_INTERVAL 5000

#define NOTE_A4 440
#define NOTE_A5 880
#define NOTE_B5 988
#define NOTE_C5 523
#define NOTE_C6 1047
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392

const int STARTUP_MELODY[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
const int STARTUP_NOTE_DURATIONS[] = {200, 200, 200, 500};

const int ERROR_MELODY[] = {NOTE_A4, NOTE_A4, NOTE_A4};
const int ERROR_NOTE_DURATIONS[] = {200, 200, 600};

const int FILAMENT_INSERTED_MELODY[] = {NOTE_C5, NOTE_E5, NOTE_G5};
const int FILAMENT_INSERTED_NOTE_DURATIONS[] = {150, 150, 300};

const int FILAMENT_REMOVED_MELODY[] = {NOTE_G5, NOTE_E5, NOTE_C5};
const int FILAMENT_REMOVED_NOTE_DURATIONS[] = {150, 150, 300};

const int MARIO_VICTORY_MELODY[] = {
    NOTE_E5, NOTE_G5, NOTE_C6, NOTE_B5,
    NOTE_A5, NOTE_F5, NOTE_D5, NOTE_E5};

const int MARIO_VICTORY_NOTE_DURATIONS[] = {
    150, 150, 300, 300,
    300, 300, 300, 600};

const int FILAMENT_LEDS[] = {0, 3, 5, 7, 8, 11, 13, 15};
const int FILAMENT_SENSOR_PINS[] = {FILAMENT_ONE_SENSOR_PIN, FILAMENT_TWO_SENSOR_PIN, FILAMENT_THREE_SENSOR_PIN,
                                    FILAMENT_FOUR_SENSOR_PIN, FILAMENT_FIVE_SENSOR_PIN, FILAMENT_SIX_SENSOR_PIN,
                                    FILAMENT_SEVEN_SENSOR_PIN, FILAMENT_EIGHT_SENSOR_PIN};

long ledStates[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool filamentStates[] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN);
Adafruit_MCP23X17 mcp;
Servo mmuServo;
Servo cutterServo;

int lastColorIndex = -1;
int lastFilamentLED = -1;
int lastMMUPosition = 0;
int activeFilament = -1;

unsigned long previousAliveMessageMillis = 0;
unsigned long previousStartupBlinkMillis = 0;
bool startupBlinkState = false;

bool started = false;
bool hubState = HIGH;
bool lastHubState = HIGH;
bool hubStateStucked = false;
bool autoExtruding = false;

const long BLACK_COLOR = pixels.Color(0, 0, 0);
const long RED_COLOR = pixels.Color(255, 0, 0);
const long GREEN_COLOR = pixels.Color(0, 255, 0);
const long BLUE_COLOR = pixels.Color(0, 0, 255);
const long YELLOW_COLOR = pixels.Color(255, 255, 0);
const long WHITE_COLOR = pixels.Color(255, 255, 255);
const long CYAN_COLOR = pixels.Color(0, 150, 255);
const long MAGENTA_COLOR = pixels.Color(255, 0, 255);
const long ORANGE_COLOR = pixels.Color(255, 128, 0);

// config from machine
// default, change it in printer config
int filamentPositions[] = {170, 148, 126, 104, 80, 56, 32, 10};
long extrudeMilimeters = 23;
long retractMilimeters = 60;
long minRetractMilimeters = 70;
long milimetersToStuck = 50;
double milimetersPerRotation = 18.28571429;

void logInfo(const String& message, const String& extra) {
    Serial.print("[");
    Serial.print(millis());

    Serial.print("] INFO - ");
    Serial.print(message);
    Serial.println(extra);
}

void logWarn(const String& message, const String& extra) {
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] WARN - ");
    Serial.print(message);
    Serial.println(extra);
}

void logError(const String& message, const String& extra) {
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] ERROR - ");
    Serial.print(message);
    Serial.println(extra);
}

void responseOk() {
    Serial.println("OK");
}

void responseError() {
    Serial.println("ERROR");
}

void responseAlive() {
    Serial.println("ALIVE");
}

void changeLED(int index, long color) {
    int position = FILAMENT_LEDS[index];

    pixels.setPixelColor(position, color);
    pixels.show();
}

void blinkLED(int index, long color) {
    for (int i = 0; i < 5; i++) {
        changeLED(index, color);
        delay(200);

        changeLED(index, BLACK_COLOR);
        delay(200);
    }

    changeLED(index, color);
}

void disableLEDs() {
    for (int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, BLACK_COLOR);
        pixels.show();
    }
}

void startupLEDs() {
    for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
        changeLED(i, CYAN_COLOR);
        delay(100);
    }
}

void finishStartupLEDs() {
    for (int i = NUMBER_OF_FILAMENTS - 1; i >= 0; i--) {
        changeLED(i, BLACK_COLOR);
        delay(100);
    }
}

void blinkStartupLEDs() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousStartupBlinkMillis >= 500) {
        previousStartupBlinkMillis = currentMillis;
        startupBlinkState = !startupBlinkState;

        for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
            changeLED(i, BLACK_COLOR);
            if (startupBlinkState) {
                changeLED(i, ORANGE_COLOR);
            } else {
                changeLED(i, BLACK_COLOR);
            }
        }
    }
}

void blinkErrorLEDs() {
    while (true) {
        for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
            changeLED(i, RED_COLOR);
        }
        delay(500);

        for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
            changeLED(i, BLACK_COLOR);
        }
        delay(500);
    }
}

void saveLEDStates() {
    for (int i = 0; i < NUM_LEDS; i++) {
        ledStates[i] = pixels.getPixelColor(i);
    }
}

void restoreLEDStates() {
    for (int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, ledStates[i]);
    }
    pixels.show();
}

void changeMusicLED(int index) {
    long color;
    int currentColorIndex;

    do {
        currentColorIndex = random(0, 7);
    } while (currentColorIndex == lastColorIndex);

    lastColorIndex = currentColorIndex;

    switch (currentColorIndex) {
        case 0:
            color = RED_COLOR;
            break;
        case 1:
            color = GREEN_COLOR;
            break;
        case 2:
            color = BLUE_COLOR;
            break;
        case 3:
            color = YELLOW_COLOR;
            break;
        case 4:
            color = WHITE_COLOR;
            break;
        case 5:
            color = CYAN_COLOR;
            break;
        case 6:
            color = MAGENTA_COLOR;
            break;
        case 7:
            color = ORANGE_COLOR;
            break;
    }

    changeLED(index, color);
}

void buttonClickSound() {
    int clickDuration = 50;

    tone(BUZZER_PIN, NOTE_C6, clickDuration);
    delay(clickDuration * 1.3);
    noTone(BUZZER_PIN);
}

void playMIDI(const int* melody, const int* noteDurations, int notes, bool ledEnabled) {
    if (ledEnabled) {
        saveLEDStates();
        disableLEDs();
    }

    for (int thisNote = 0; thisNote < notes; thisNote++) {
        int noteDuration = noteDurations[thisNote];
        int note = melody[thisNote];

        int filamentLED;

        do {
            filamentLED = random(0, NUMBER_OF_FILAMENTS - 1);
        } while (filamentLED == lastFilamentLED);

        lastFilamentLED = filamentLED;

        if (note == 0) {
            noTone(BUZZER_PIN);
        } else {
            tone(BUZZER_PIN, note, noteDuration);

            if (ledEnabled) {
                changeMusicLED(filamentLED);
            }
        }

        delay(noteDuration * 1.3);

        if (ledEnabled) {
            changeLED(filamentLED, BLACK_COLOR);
        }
    }

    noTone(BUZZER_PIN);

    if (ledEnabled) {
        restoreLEDStates();
    }
}

void startupMIDI(bool ledEnabled) {
    int notes = sizeof(STARTUP_MELODY) / sizeof(STARTUP_MELODY[0]);
    playMIDI(STARTUP_MELODY, STARTUP_NOTE_DURATIONS, notes, ledEnabled);
}

void errorMIDI(bool ledEnabled) {
    int notes = sizeof(ERROR_MELODY) / sizeof(ERROR_MELODY[0]);
    playMIDI(ERROR_MELODY, ERROR_NOTE_DURATIONS, notes, ledEnabled);
}

void filamentInsertedMIDI(bool ledEnabled) {
    int notes = sizeof(FILAMENT_INSERTED_MELODY) / sizeof(FILAMENT_INSERTED_MELODY[0]);
    playMIDI(FILAMENT_INSERTED_MELODY, FILAMENT_INSERTED_NOTE_DURATIONS, notes, ledEnabled);
}

void filamentRemovedMIDI(bool ledEnabled) {
    int notes = sizeof(FILAMENT_REMOVED_MELODY) / sizeof(FILAMENT_REMOVED_MELODY[0]);
    playMIDI(FILAMENT_REMOVED_MELODY, FILAMENT_REMOVED_NOTE_DURATIONS, notes, ledEnabled);
}

void marioVictoryMIDI(bool ledEnabled) {
    int notes = sizeof(MARIO_VICTORY_MELODY) / sizeof(MARIO_VICTORY_MELODY[0]);
    playMIDI(MARIO_VICTORY_MELODY, MARIO_VICTORY_NOTE_DURATIONS, notes, ledEnabled);
}

bool playMIDI(int position) {
    switch (position) {
        case 0:
            startupMIDI(true);
            return true;

        case 1:
            errorMIDI(true);
            return true;

        case 2:
            filamentInsertedMIDI(true);
            return true;

        case 3:
            filamentRemovedMIDI(true);
            return true;

        case 4:
            marioVictoryMIDI(true);
            return true;

        default:
            logError(F("Unknown MIDI "), String(position));
    }

    return false;
}

void setMissingFilament() {
    logInfo(F("Setting missing filament, pausing print"), "");
    mcp.digitalWrite(CREALITY_FILAMENT_SENSOR_PIN, HIGH);
}

void unsetMissingFilament() {
    mcp.digitalWrite(CREALITY_FILAMENT_SENSOR_PIN, LOW);
}

void changeHubState() {
    // Read raw pin state directly
    hubState = (PIND & (1 << FILAMENT_HUB_SENSOR_PIN));
    hubStateStucked = false;
}

long getDegreesFromMilimeters(long milimeters) {
    return milimeters * 360 / milimetersPerRotation;
}

long getStepsFromDegrees(long degrees) {
    return degrees * (unsigned long)MMU_MICROSTEPS * (unsigned long)MMU_MOTOR_STEPS / 360UL;
}

long getStepsFromMilimeters(long milimeters) {
    long degrees = getDegreesFromMilimeters(milimeters);
    return getStepsFromDegrees(degrees);
}

long getMilimetersFromSteps(long steps) {
    double degrees = steps * 360 / (unsigned long)MMU_MICROSTEPS / (unsigned long)MMU_MOTOR_STEPS;
    return degrees * milimetersPerRotation / 360;
}

void setCutterServoPosition(int position) {
    cutterServo.attach(CUTTER_SERVO_PIN);
    cutterServo.write(position);
    delay(1000);
    cutterServo.detach();
}

void setMMUServoPosition(int position) {
    lastMMUPosition = position;

    mmuServo.attach(MMU_SERVO_PIN);
    mmuServo.write(position);
    delay(1000);
    mmuServo.detach();
}

void testLED(int index) {
    logInfo(F("Testing LED "), String(index + 1));

    blinkLED(index, RED_COLOR);
    blinkLED(index, GREEN_COLOR);
    blinkLED(index, BLUE_COLOR);
    blinkLED(index, YELLOW_COLOR);
    blinkLED(index, CYAN_COLOR);
    blinkLED(index, MAGENTA_COLOR);
    blinkLED(index, ORANGE_COLOR);
    blinkLED(index, WHITE_COLOR);
}

void safeTestLED(int index) {
    logInfo(F("Testing LEDs"), "");

    saveLEDStates();
    testLED(index);
    restoreLEDStates();
}

void testLEDs() {
    logInfo(F("Testing LEDs"), "");

    saveLEDStates();

    for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
        testLED(i);
    }

    restoreLEDStates();
}

bool setFilament(int index) {
    activeFilament = index;

    for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
        bool filamentState = filamentStates[i];

        if (filamentState == LOW) {
            changeLED(i, CYAN_COLOR);
        } else {
            changeLED(i, BLACK_COLOR);
        }
    }
    changeLED(activeFilament, ORANGE_COLOR);

    bool filamentState = filamentStates[activeFilament];
    int position = filamentPositions[activeFilament];

    setMMUServoPosition(position);

    if (filamentState == LOW) {
        unsetMissingFilament();

        if (hubStateStucked) {
            changeLED(activeFilament, YELLOW_COLOR);
        } else {
            changeLED(activeFilament, GREEN_COLOR);
        }

    } else {
        setMissingFilament();
        changeLED(activeFilament, RED_COLOR);

        errorMIDI(false);
        blinkLED(activeFilament, RED_COLOR);
        return false;
    }

    return true;
}

void filamentRelease() {
    saveLEDStates();

    if (lastMMUPosition > 90) {
        changeLED(7, ORANGE_COLOR);
        setMMUServoPosition(filamentPositions[7]);

    } else {
        changeLED(0, ORANGE_COLOR);
        setMMUServoPosition(filamentPositions[0]);
    }

    restoreLEDStates();
}

bool swapFinish() {
    if (hubStateStucked) {
        setMissingFilament();
        errorMIDI(false);
        return false;
    }

    return true;
}

unsigned long rotateMmu(long degrees, int rpm, bool accelerationEnabled, bool decelerationEnabled, bool resetOnSensor) {
    if (degrees == 0) {
        return 0;
    }

    digitalWrite(MMU_ENABLE_PIN, LOW);

    if (degrees < 0) {
        digitalWrite(MMU_DIR_PIN, !MMU_DIRECTION);
        degrees *= -1;
    } else {
        digitalWrite(MMU_DIR_PIN, MMU_DIRECTION);
    }

    if (rpm == 0) {
        rpm = MMU_DEFAULT_RPM;
    } else if (rpm < MMU_MIN_RPM) {
        rpm = MMU_MIN_RPM;
    }

    unsigned long totalSteps;
    unsigned long steps = getStepsFromDegrees(degrees);
    unsigned long decelerationSteps = steps - (steps / 100UL);

    unsigned long targetPulsePeriod = 60000000UL / ((unsigned long)rpm * (unsigned long)MMU_MICROSTEPS * (unsigned long)MMU_MOTOR_STEPS);
    unsigned int targetDelay = targetPulsePeriod / 2UL;
    unsigned int currentDelay = MMU_SLOW_PULSE_DELAY;
    unsigned long skipStepCount = 0;
    bool acelerated = false;
    bool lastHubState = hubState;

    if (!accelerationEnabled) {
        currentDelay = targetDelay;
    }

    for (unsigned long i = 0; i < steps; i++) {
        if (skipStepCount > MMU_ACCEL_DECEL_SKIP_STEPS) {
            if (decelerationEnabled && currentDelay != MMU_SLOW_PULSE_DELAY && i > decelerationSteps) {
                skipStepCount = 0;
                currentDelay += 1;

                if (currentDelay > MMU_SLOW_PULSE_DELAY) {
                    currentDelay = MMU_SLOW_PULSE_DELAY;
                }

            } else if (accelerationEnabled && !acelerated && currentDelay != targetDelay) {
                skipStepCount = 0;
                currentDelay -= 1;

                if (currentDelay < targetDelay) {
                    currentDelay = targetDelay;
                    acelerated = true;
                }
            }
        }

        skipStepCount++;
        totalSteps++;

        if (hubState != lastHubState && resetOnSensor) {
            logInfo(F("Resetting on filament sensor"), "");

            i = 0;
            skipStepCount = 0;
            currentDelay = targetDelay;
            lastHubState = hubState;
        }

        digitalWrite(MMU_STEP_PIN, HIGH);
        delayMicroseconds(currentDelay);
        digitalWrite(MMU_STEP_PIN, LOW);
        delayMicroseconds(currentDelay);
    }

    digitalWrite(MMU_ENABLE_PIN, HIGH);

    return totalSteps;
}

void rotateMmuToSensor(int targetState, long milimeters, long milimetersToStuck, int direction, int rpm) {
    if (milimeters == 0) {
        return;
    }

    if (hubState == targetState) {
        hubStateStucked = true;
        changeLED(activeFilament, YELLOW_COLOR);
        logWarn("Hub sensor stucked or missing", (""));
    }

    digitalWrite(MMU_ENABLE_PIN, LOW);
    digitalWrite(MMU_DIR_PIN, direction);

    if (rpm == 0) {
        rpm = MMU_DEFAULT_RPM;
    } else if (rpm < MMU_MIN_RPM) {
        rpm = MMU_MIN_RPM;
    }

    unsigned long stepsToStuck = getStepsFromMilimeters(milimetersToStuck);
    unsigned long minRetractSteps = getStepsFromMilimeters(minRetractMilimeters);

    if (direction != MMU_DIRECTION) {
        milimeters *= -1;  // retract
    }

    unsigned long targetPulsePeriod = 60000000UL / ((unsigned long)rpm * (unsigned long)MMU_MICROSTEPS * (unsigned long)MMU_MOTOR_STEPS);
    unsigned int targetDelay = targetPulsePeriod / 2UL;
    unsigned int currentDelay = MMU_SLOW_PULSE_DELAY;
    unsigned long skipStepCount = 0;
    unsigned long checkIntervalCount = 0;
    unsigned long steps = 0;

    while (hubState != targetState || hubStateStucked || (direction != MMU_DIRECTION && steps < minRetractSteps)) {
        if (skipStepCount > MMU_ACCEL_DECEL_SKIP_STEPS && currentDelay != targetDelay) {
            skipStepCount = 0;
            currentDelay -= 1;

            if (currentDelay < targetDelay) {
                currentDelay = targetDelay;
            }
        }

        if (direction != MMU_DIRECTION && steps > stepsToStuck && steps > minRetractSteps) {
            hubStateStucked = true;

            changeLED(activeFilament, YELLOW_COLOR);

            logWarn("Hub sensor stucked or missing on retract", (""));
            break;

        } else if (direction == MMU_DIRECTION && steps > stepsToStuck && !autoExtruding) {
            hubStateStucked = true;

            changeLED(activeFilament, YELLOW_COLOR);

            logWarn("Hub sensor stucked or missing on extrude", (""));
            break;
        }

        if (checkIntervalCount > MMU_SENSOR_CHECK_INTERVAL) {
            checkIntervalCount = 0;

            if (activeFilament != -1) {
                int filamentPin = FILAMENT_SENSOR_PINS[activeFilament];
                bool filamentState = mcp.digitalRead(filamentPin);

                if (filamentState == HIGH) {
                    digitalWrite(MMU_ENABLE_PIN, HIGH);
                    logInfo("Filament T" + String(activeFilament) + " removed", "");

                    setMissingFilament();

                    changeLED(activeFilament, RED_COLOR);
                    errorMIDI(false);
                    blinkLED(activeFilament, RED_COLOR);

                    return;
                }
            }
        }

        digitalWrite(MMU_STEP_PIN, HIGH);
        delayMicroseconds(currentDelay);
        digitalWrite(MMU_STEP_PIN, LOW);
        delayMicroseconds(currentDelay);

        checkIntervalCount++;
        skipStepCount++;
        steps++;
    }

    if (!hubStateStucked) {
        long degrees = getDegreesFromMilimeters(milimeters);
        bool resetOnSensor = direction != MMU_DIRECTION;  // reset on retract
        steps += rotateMmu(degrees, rpm, false, true, resetOnSensor);
    }

    long stepsMilimeters = getMilimetersFromSteps(steps);

    if (direction == MMU_DIRECTION) {
        logInfo(F("Extruded milimeters: "), String(stepsMilimeters));

    } else if (direction != MMU_DIRECTION) {
        logInfo(F("Retracted milimeters: "), String(stepsMilimeters));
    }

    digitalWrite(MMU_ENABLE_PIN, HIGH);
}

void extrude(long milimeters, int rpm) {
    long totalMilimetersToStuck = milimetersToStuck + retractMilimeters + milimeters;
    rotateMmuToSensor(LOW, milimeters, totalMilimetersToStuck, MMU_DIRECTION, rpm);
}

void retract(long milimeters, int rpm) {
    long totalMilimetersToStuck = extrudeMilimeters + milimeters;
    rotateMmuToSensor(HIGH, milimeters, totalMilimetersToStuck, !MMU_DIRECTION, rpm);
}

void readHubState() {
    if (hubState != lastHubState) {
        logInfo(F("Hub state changed to "), String(hubState));
        lastHubState = hubState;

        if (activeFilament > -1 && hubState == LOW && filamentStates[activeFilament] == LOW) {
            changeLED(activeFilament, GREEN_COLOR);
            unsetMissingFilament();
        }
    }
}

void readSensors(bool soundEnabled) {
    for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
        int pin = FILAMENT_SENSOR_PINS[i];
        bool state = mcp.digitalRead(pin);
        if (state != filamentStates[i]) {
            filamentStates[i] = state;

            if (state == LOW) {
                logInfo("Filament T" + String(i) + " inserted", "");

                if (i == activeFilament) {
                    unsetMissingFilament();

                    if (hubStateStucked) {
                        changeLED(activeFilament, YELLOW_COLOR);
                    } else {
                        changeLED(activeFilament, GREEN_COLOR);
                    }

                } else {
                    changeLED(i, CYAN_COLOR);
                }

                if (soundEnabled) {
                    filamentInsertedMIDI(false);
                }

            } else {
                logInfo("Filament T" + String(i) + " removed", "");

                if (i == activeFilament) {
                    setMissingFilament();

                    changeLED(i, RED_COLOR);

                    if (soundEnabled) {
                        errorMIDI(false);
                    }

                    blinkLED(i, RED_COLOR);

                } else {
                    changeLED(i, BLACK_COLOR);

                    if (soundEnabled) {
                        filamentRemovedMIDI(false);
                    }
                }
            }
        }
    }
}

unsigned long actionButtonPressedTime = 0;

void readActionButtonPressed() {
    bool state = mcp.digitalRead(ACTION_BUTTON_PIN);

    if (state == LOW && actionButtonPressedTime == 0) {
        actionButtonPressedTime = millis();
    }

    if (state == HIGH && actionButtonPressedTime > 0) {
        unsigned long buttonPressedDuration = millis() - actionButtonPressedTime;
        actionButtonPressedTime = 0;

        buttonClickSound();

        if (buttonPressedDuration > 1000) {
            logInfo(F("Action button pressed long"), "");

            if (activeFilament > -1 && filamentStates[activeFilament] == LOW && hubState == HIGH) {
                autoExtruding = true;
                int mmuPosition = filamentPositions[activeFilament];

                changeLED(activeFilament, ORANGE_COLOR);
                setMMUServoPosition(mmuPosition);
                extrude(extrudeMilimeters, MMU_DEFAULT_RPM);
                filamentRelease();
                autoExtruding = false;
            }
        } else {
            logInfo(F("Action button pressed short"), "");
            filamentRelease();
        }
    }
}

void processSerialInput() {
    String input = Serial.readStringUntil('\n');
    input.toUpperCase();
    input.trim();

    if (input == F("START")) {
        logInfo(F("Starting up..."), "");

        disableLEDs();
        startupLEDs();
        startupMIDI(true);
        finishStartupLEDs();

        changeHubState();
        readSensors(false);

        started = true;

        logInfo(F("Started"), "");

        responseOk();

    } else if (input.startsWith(F("SYNC"))) {
        logInfo(F("Syncing config..."), "");

        int newPositions[NUMBER_OF_FILAMENTS];

        const char* inputStr = input.c_str();

        const char* posStr = strstr(inputStr, "FILAMENT_POSITIONS");
        if (posStr) {
            sscanf(posStr, "FILAMENT_POSITIONS %d %d %d %d %d %d %d %d",
                   &newPositions[0], &newPositions[1], &newPositions[2], &newPositions[3],
                   &newPositions[4], &newPositions[5], &newPositions[6], &newPositions[7]);
        }

        const char* extStr = strstr(inputStr, "EXTRUDE_MM");
        if (extStr) {
            sscanf(extStr, "EXTRUDE_MM %ld", &extrudeMilimeters);
        }

        const char* rtrStr = strstr(inputStr, "RETRACT_MM");
        if (rtrStr) {
            sscanf(rtrStr, "RETRACT_MM %ld", &retractMilimeters);
        }

        const char* minRetrStr = strstr(inputStr, "MIN_RETRACT_MM");
        if (minRetrStr) {
            sscanf(minRetrStr, "MIN_RETRACT_MM %ld", &minRetractMilimeters);
        }

        const char* mmPerRotStr = strstr(inputStr, "MM_PER_ROTATION");
        if (mmPerRotStr) {
            sscanf(mmPerRotStr, "MM_PER_ROTATION %lf", &milimetersPerRotation);
        }

        const char* mmToStkStr = strstr(inputStr, "MM_TO_STUCK");
        if (mmToStkStr) {
            sscanf(mmToStkStr, "MM_TO_STUCK %ld", &milimetersToStuck);
        }

        logInfo(F("New positions: "), "");
        for (int i = 0; i < NUMBER_OF_FILAMENTS; i++) {
            logInfo(String(i + 1) + " => " + String(filamentPositions[i]), "");
        }

        logInfo(F("New extrude mm: "), String(extrudeMilimeters));
        logInfo(F("New retract mm: "), String(retractMilimeters));
        logInfo(F("New min retract mm: "), String(minRetractMilimeters));
        logInfo(F("New mm per rotation: "), String(milimetersPerRotation));
        logInfo(F("New mm to stuck: "), String(milimetersToStuck));
        logInfo(F("Config synced"), "");
        responseOk();

    } else if (input.startsWith(F("FILAMENT_RELEASE"))) {
        logInfo(F("Releasing filament"), "");

        responseOk();  // async
        filamentRelease();

        logInfo(F("Filament released"), "");

    } else if (input.startsWith(F("FILAMENT"))) {
        int index = input.substring(input.indexOf(' ') + 1).toInt();
        logInfo(F("Setting filament T"), String(index));

        bool result = setFilament(index);

        if (result) {
            logInfo(F("Filament set"), "");
            responseOk();
        } else {
            logError(F("Failed to set filament T"), String(index));
            responseError();
        }
    } else if (input.startsWith(F("EXTRUDE"))) {
        long milimeters = 0;
        int rpm = 0;

        sscanf(input.c_str(), "EXTRUDE %ld %d", &milimeters, &rpm);

        logInfo(F("Extruding..."), "");
        extrude(milimeters, rpm);
        logInfo(F("Extruded"), "");
        responseOk();

    } else if (input.startsWith(F("RETRACT"))) {
        long milimeters = 0;
        int rpm = 0;

        sscanf(input.c_str(), "RETRACT %ld %d", &milimeters, &rpm);

        logInfo(F("Retracting..."), "");

        responseOk();  // async
        delay(100);

        retract(milimeters, rpm);
        logInfo(F("Retracted"), "");

    } else if (input.startsWith(F("SWAP_FINISH"))) {
        logInfo(F("Swap finishing..."), "");
        bool finished = swapFinish();

        if (finished) {
            logInfo(F("Swap finished"), "");
            responseOk();
        } else {
            logInfo(F("Swap not finished"), "");
            responseError();
        }

    } else if (input.startsWith(F("CUTTER_POSITION"))) {
        int position = input.substring(input.indexOf(' ') + 1).toInt();

        logInfo(F("Setting cutter position to "), String(position));
        setCutterServoPosition(position);
        logInfo(F("Cutter position set to "), String(position));

        responseOk();

    } else if (input.startsWith(F("MMU_POSITION"))) {
        int position = input.substring(input.indexOf(' ') + 1).toInt();

        logInfo(F("Setting MMU position to "), String(position));
        setMMUServoPosition(position);
        logInfo(F("MMU position set to "), String(position));
        responseOk();

    } else if (input.startsWith(F("MMU_ROTATE"))) {
        long degrees = 0;
        int rpm = 0;

        sscanf(input.c_str(), "MMU_ROTATE %ld %d", &degrees, &rpm);

        logInfo(F("Rotating MMU "), String(degrees));
        logInfo(F("RPM "), String(rpm));
        rotateMmu(degrees, rpm, true, true, false);
        logInfo(F("MMU rotated "), String(degrees));
        responseOk();

    } else if (input.startsWith(F("MIDI"))) {
        int position = input.substring(input.indexOf(' ') + 1).toInt();
        logInfo(F("Playing MIDI "), String(position));
        bool played = playMIDI(position);
        if (played) {
            logInfo(F("MIDI played"), "");
            responseOk();
        } else {
            logError(F("Failed to play MIDI "), String(position));
            responseError();
        }

    } else if (input.startsWith(F("TEST_LEDS"))) {
        logInfo(F("Testing LEDs..."), "");

        testLEDs();
        logInfo(F("LEDs tested"), "");
        responseOk();

    } else if (input.startsWith(F("TEST_LED"))) {
        int index = input.substring(input.indexOf(' ') + 1).toInt();
        logInfo(F("Testing LED "), String(index));

        safeTestLED(index - 1);

        logInfo(F("LED tested"), "");
        responseOk();

    } else if (input.startsWith(F("STRESS"))) {
        logInfo(F("Stressing... "), "");
        for (int i = 0; i < 10; i++) {
            setMMUServoPosition(0);
            setMMUServoPosition(180);
        }
        responseOk();

    } else {
        logError(F("Unknown command "), input);
        responseError();
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    logInfo(F("Starting..."), "");

    randomSeed(analogRead(0));

    pixels.begin();

    pinMode(FILAMENT_HUB_SENSOR_PIN, INPUT_PULLUP);
    hubState = digitalRead(FILAMENT_HUB_SENSOR_PIN);
    lastHubState = hubState;
    attachInterrupt(digitalPinToInterrupt(FILAMENT_HUB_SENSOR_PIN), changeHubState, CHANGE);

    pinMode(MMU_DIR_PIN, OUTPUT);
    pinMode(MMU_STEP_PIN, OUTPUT);
    pinMode(MMU_ENABLE_PIN, OUTPUT);
    digitalWrite(MMU_ENABLE_PIN, HIGH);

    setCutterServoPosition(0);
    setMMUServoPosition(0);

    if (!mcp.begin_I2C()) {
        logError(F("Failed to initialize MCP23017"), "");
        blinkErrorLEDs();
    }

    mcp.pinMode(CREALITY_FILAMENT_SENSOR_PIN, OUTPUT);

    mcp.pinMode(ACTION_BUTTON_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_ONE_SENSOR_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_TWO_SENSOR_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_THREE_SENSOR_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_FOUR_SENSOR_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_FIVE_SENSOR_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_SIX_SENSOR_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_SEVEN_SENSOR_PIN, INPUT_PULLUP);
    mcp.pinMode(FILAMENT_EIGHT_SENSOR_PIN, INPUT_PULLUP);

    mmuServo.detach();
    cutterServo.detach();

    Serial.println("READY");
}

void loop() {
    if (Serial.available() > 0) {
        processSerialInput();
    }

    if (started) {
        readSensors(true);
        readHubState();
        readActionButtonPressed();

        unsigned long currentMillis = millis();
        if (currentMillis - previousAliveMessageMillis > ALIVE_MESSAGE_INTERVAL) {
            responseAlive();
            previousAliveMessageMillis = currentMillis;
        }
    } else {
        blinkStartupLEDs();
    }
}