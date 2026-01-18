/**
 * @file ButtonHandler.cpp
 * @brief Button input handling implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "ButtonHandler.h"

void ButtonHandler::begin() {
    // Initialize button pins with internal pull-up
    uint8_t pins[BUTTON_COUNT] = {BTN_OK, BTN_NEXT, BTN_BACK};
    
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].pin = pins[i];
        buttons[i].lastReading = HIGH;
        buttons[i].currentState = HIGH;
        buttons[i].wasPressed = false;
        buttons[i].lastDebounceTime = 0;
        buttons[i].pressStartTime = 0;
        buttons[i].longPressTriggered = false;
        buttons[i].pendingEvent = BTN_NONE;
        
        pinMode(pins[i], INPUT_PULLUP);
    }
    
    lastActivity = millis();
    
    DEBUG_PRINTLN("ButtonHandler initialized");
}

void ButtonHandler::update() {
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        processButton(i);
    }
}

void ButtonHandler::processButton(uint8_t index) {
    if (index >= BUTTON_COUNT) return;
    
    ButtonState& btn = buttons[index];
    bool reading = digitalRead(btn.pin);
    
    // Check for state change (for debouncing)
    if (reading != btn.lastReading) {
        btn.lastDebounceTime = millis();
    }
    
    // If reading has been stable for debounce period
    if ((millis() - btn.lastDebounceTime) > DEBOUNCE_DELAY) {
        // State has actually changed
        if (reading != btn.currentState) {
            btn.currentState = reading;
            
            if (btn.currentState == LOW) {
                // Button pressed
                btn.pressStartTime = millis();
                btn.longPressTriggered = false;
                btn.wasPressed = true;
                lastActivity = millis();
                DEBUG_PRINTF("Button %d pressed\n", index);
            } else {
                // Button released
                if (btn.wasPressed && !btn.longPressTriggered) {
                    // Short press completed
                    btn.pendingEvent = BTN_SHORT_PRESS;
                    DEBUG_PRINTF("Button %d short press\n", index);
                }
                btn.wasPressed = false;
            }
        }
        
        // Check for long press while button is held
        if (btn.currentState == LOW && btn.wasPressed && !btn.longPressTriggered) {
            if ((millis() - btn.pressStartTime) >= LONG_PRESS_DURATION) {
                btn.longPressTriggered = true;
                btn.pendingEvent = BTN_LONG_PRESS;
                DEBUG_PRINTF("Button %d long press\n", index);
            }
        }
    }
    
    btn.lastReading = reading;
}

ButtonEvent ButtonHandler::getOkEvent() {
    return consumeEvent(BUTTON_OK);
}

ButtonEvent ButtonHandler::getNextEvent() {
    return consumeEvent(BUTTON_NEXT);
}

ButtonEvent ButtonHandler::getBackEvent() {
    return consumeEvent(BUTTON_BACK);
}

ButtonEvent ButtonHandler::consumeEvent(uint8_t index) {
    if (index >= BUTTON_COUNT) return BTN_NONE;
    
    ButtonEvent event = buttons[index].pendingEvent;
    buttons[index].pendingEvent = BTN_NONE;
    return event;
}

bool ButtonHandler::anyButtonPressed() {
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        if (buttons[i].currentState == LOW) {
            return true;
        }
    }
    return false;
}

bool ButtonHandler::isPressed(uint8_t buttonIndex) {
    if (buttonIndex >= BUTTON_COUNT) return false;
    return buttons[buttonIndex].currentState == LOW;
}

void ButtonHandler::clearEvents() {
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        buttons[i].pendingEvent = BTN_NONE;
    }
}

uint32_t ButtonHandler::timeSinceLastActivity() {
    return millis() - lastActivity;
}
