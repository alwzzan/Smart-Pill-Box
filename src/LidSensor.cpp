/**
 * @file LidSensor.cpp
 * @brief Reed switch lid sensor implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "LidSensor.h"

void LidSensor::begin() {
    // Configure reed switch pin with internal pull-up
    pinMode(REED_SWITCH, INPUT_PULLUP);
    
    // Initialize state
    lidOpen = false;
    lastState = HIGH;  // Reed switch is normally closed (HIGH when closed)
    currentReading = HIGH;
    justOpenedFlag = false;
    justClosedFlag = false;
    sensorWorking = true;
    lastDebounceTime = 0;
    stateChangeTime = 0;
    lastOpenTime = 0;
    openingsToday = 0;
    
    // Initial reading
    currentReading = digitalRead(REED_SWITCH);
    lastState = currentReading;
    lidOpen = (currentReading == HIGH);  // HIGH = no magnet = lid open
    
    DEBUG_PRINTF("LidSensor initialized. Lid is %s\n", lidOpen ? "OPEN" : "CLOSED");
}

void LidSensor::update() {
    // Clear edge detection flags
    justOpenedFlag = false;
    justClosedFlag = false;
    
    // Read current state
    bool reading = digitalRead(REED_SWITCH);
    
    // Check for state change
    if (reading != currentReading) {
        currentReading = reading;
        lastDebounceTime = millis();
        stateChangeTime = millis();
    }
    
    // Check if reading has been stable for debounce period
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        // Additional check: ensure state is stable for longer duration
        // to prevent false triggers from vibration
        if ((millis() - stateChangeTime) >= LID_DEBOUNCE_DURATION) {
            bool newState = (currentReading == HIGH);  // HIGH = lid open
            
            if (newState != lidOpen) {
                lidOpen = newState;
                
                if (lidOpen) {
                    // Lid just opened
                    justOpenedFlag = true;
                    lastOpenTime = millis();
                    openingsToday++;
                    DEBUG_PRINTF("Lid OPENED. Total openings today: %d\n", openingsToday);
                } else {
                    // Lid just closed
                    justClosedFlag = true;
                    DEBUG_PRINTLN("Lid CLOSED");
                }
            }
        }
    }
}

bool LidSensor::justOpened() {
    if (justOpenedFlag) {
        justOpenedFlag = false;
        return true;
    }
    return false;
}

bool LidSensor::justClosed() {
    if (justClosedFlag) {
        justClosedFlag = false;
        return true;
    }
    return false;
}

uint32_t LidSensor::timeSinceLastOpen() const {
    if (lastOpenTime == 0) {
        return 0xFFFFFFFF;  // Never opened
    }
    return millis() - lastOpenTime;
}

bool LidSensor::debounce() {
    static bool lastReading = HIGH;
    static uint32_t lastChange = 0;
    
    bool reading = digitalRead(REED_SWITCH);
    
    if (reading != lastReading) {
        lastChange = millis();
        lastReading = reading;
    }
    
    if ((millis() - lastChange) > DEBOUNCE_DELAY) {
        return reading;
    }
    
    return currentReading;  // Return last stable state
}
