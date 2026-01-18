/**
 * @file AlarmController.cpp
 * @brief Buzzer control implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "AlarmController.h"

// Pattern definitions (on/off durations in ms)
// Gentle: slow, soft beeps
const uint16_t AlarmController::GENTLE_PATTERN[] = {200, 1500, 200, 1500, 200, 3000};
// Standard: regular beeping
const uint16_t AlarmController::STANDARD_PATTERN[] = {500, 500, 500, 500, 500, 1000};
// Urgent: fast beeping
const uint16_t AlarmController::URGENT_PATTERN[] = {200, 200, 200, 200, 200, 200};

void AlarmController::begin() {
    // Configure buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Setup LEDC for tone generation
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQUENCY, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWrite(BUZZER_CHANNEL, 0);
    
    active = false;
    snoozed = false;
    buzzerEnabled = true;
    buzzerOn = false;
    volume = 128; // 50% default volume
    currentPattern = PATTERN_STANDARD;
    snoozeEndTime = 0;
    lastToggle = 0;
    patternStep = 0;
    
    DEBUG_PRINTLN("AlarmController initialized");
}

void AlarmController::startAlarm(AlarmPattern pattern) {
    if (!buzzerEnabled) {
        DEBUG_PRINTLN("Alarm blocked - buzzer disabled");
        return;
    }
    
    active = true;
    snoozed = false;
    currentPattern = pattern;
    patternStep = 0;
    lastToggle = millis();
    buzzerOutput(true);
    
    DEBUG_PRINTF("Alarm started with pattern %d\n", pattern);
}

void AlarmController::stopAlarm() {
    active = false;
    snoozed = false;
    buzzerOutput(false);
    patternStep = 0;
    
    DEBUG_PRINTLN("Alarm stopped");
}

void AlarmController::snooze(uint16_t seconds) {
    if (!active) return;
    
    snoozed = true;
    snoozeEndTime = millis() + (seconds * 1000UL);
    buzzerOutput(false);
    
    DEBUG_PRINTF("Alarm snoozed for %d seconds\n", seconds);
}

void AlarmController::update() {
    // Check if snooze has ended
    if (snoozed && active) {
        if (millis() >= snoozeEndTime) {
            snoozed = false;
            patternStep = 0;
            lastToggle = millis();
            buzzerOutput(true);
            DEBUG_PRINTLN("Snooze ended, alarm resumed");
        }
        return;
    }
    
    // Update pattern if alarm is active
    if (active && !snoozed) {
        const uint16_t* pattern = getCurrentPattern();
        uint8_t patternLen = getPatternLength();
        
        if (millis() - lastToggle >= pattern[patternStep]) {
            patternStep++;
            if (patternStep >= patternLen) {
                patternStep = 0;
            }
            
            // Even steps are ON, odd steps are OFF
            buzzerOutput(patternStep % 2 == 0);
            lastToggle = millis();
        }
    }
}

uint16_t AlarmController::getSnoozeRemaining() const {
    if (!snoozed || millis() >= snoozeEndTime) {
        return 0;
    }
    return (snoozeEndTime - millis()) / 1000;
}

void AlarmController::beep(uint16_t frequency, uint16_t duration) {
    if (!buzzerEnabled) return;
    
    ledcWriteTone(BUZZER_CHANNEL, frequency);
    ledcWrite(BUZZER_CHANNEL, volume);
    delay(duration);
    ledcWrite(BUZZER_CHANNEL, 0);
}

void AlarmController::playConfirm() {
    if (!buzzerEnabled) return;
    
    // Two rising tones
    ledcWriteTone(BUZZER_CHANNEL, 1000);
    ledcWrite(BUZZER_CHANNEL, volume);
    delay(80);
    ledcWriteTone(BUZZER_CHANNEL, 1500);
    delay(80);
    ledcWrite(BUZZER_CHANNEL, 0);
}

void AlarmController::playError() {
    if (!buzzerEnabled) return;
    
    // Two falling tones
    ledcWriteTone(BUZZER_CHANNEL, 800);
    ledcWrite(BUZZER_CHANNEL, volume);
    delay(100);
    ledcWriteTone(BUZZER_CHANNEL, 400);
    delay(150);
    ledcWrite(BUZZER_CHANNEL, 0);
}

void AlarmController::playStartup() {
    if (!buzzerEnabled) return;
    
    // Three ascending tones
    uint16_t tones[] = {523, 659, 784}; // C5, E5, G5
    
    for (int i = 0; i < 3; i++) {
        ledcWriteTone(BUZZER_CHANNEL, tones[i]);
        ledcWrite(BUZZER_CHANNEL, volume / 2);
        delay(100);
        ledcWrite(BUZZER_CHANNEL, 0);
        delay(50);
    }
}

void AlarmController::setVolume(uint8_t vol) {
    volume = vol;
    if (buzzerOn) {
        ledcWrite(BUZZER_CHANNEL, volume);
    }
}

void AlarmController::buzzerOutput(bool on) {
    buzzerOn = on;
    
    if (on && buzzerEnabled) {
        ledcWriteTone(BUZZER_CHANNEL, BUZZER_FREQUENCY);
        ledcWrite(BUZZER_CHANNEL, volume);
    } else {
        ledcWrite(BUZZER_CHANNEL, 0);
    }
}

const uint16_t* AlarmController::getCurrentPattern() const {
    switch (currentPattern) {
        case PATTERN_GENTLE:
            return GENTLE_PATTERN;
        case PATTERN_URGENT:
            return URGENT_PATTERN;
        case PATTERN_STANDARD:
        default:
            return STANDARD_PATTERN;
    }
}

uint8_t AlarmController::getPatternLength() const {
    switch (currentPattern) {
        case PATTERN_GENTLE:
            return sizeof(GENTLE_PATTERN) / sizeof(uint16_t);
        case PATTERN_URGENT:
            return sizeof(URGENT_PATTERN) / sizeof(uint16_t);
        case PATTERN_STANDARD:
        default:
            return sizeof(STANDARD_PATTERN) / sizeof(uint16_t);
    }
}
