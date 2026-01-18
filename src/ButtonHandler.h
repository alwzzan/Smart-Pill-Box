/**
 * @file ButtonHandler.h
 * @brief Button input handling with debouncing and long press detection
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "config.h"

// Button indices
#define BUTTON_OK       0
#define BUTTON_NEXT     1
#define BUTTON_BACK     2
#define BUTTON_COUNT    3

class ButtonHandler {
public:
    /**
     * @brief Initialize button pins
     */
    void begin();
    
    /**
     * @brief Update button states (call every loop)
     */
    void update();
    
    /**
     * @brief Check for button event on OK button
     * @return Button event type
     */
    ButtonEvent getOkEvent();
    
    /**
     * @brief Check for button event on NEXT button
     * @return Button event type
     */
    ButtonEvent getNextEvent();
    
    /**
     * @brief Check for button event on BACK button
     * @return Button event type
     */
    ButtonEvent getBackEvent();
    
    /**
     * @brief Check if any button is currently pressed
     * @return true if any button is pressed
     */
    bool anyButtonPressed();
    
    /**
     * @brief Check if specific button is currently pressed
     * @param buttonIndex Button index (BUTTON_OK, BUTTON_NEXT, BUTTON_BACK)
     * @return true if button is pressed
     */
    bool isPressed(uint8_t buttonIndex);
    
    /**
     * @brief Clear all pending events
     */
    void clearEvents();
    
    /**
     * @brief Get time since last button activity
     * @return Milliseconds since last button press
     */
    uint32_t timeSinceLastActivity();

private:
    struct ButtonState {
        uint8_t pin;
        bool lastReading;
        bool currentState;
        bool wasPressed;
        uint32_t lastDebounceTime;
        uint32_t pressStartTime;
        bool longPressTriggered;
        ButtonEvent pendingEvent;
    };
    
    ButtonState buttons[BUTTON_COUNT];
    uint32_t lastActivity;
    
    /**
     * @brief Process individual button
     * @param index Button index
     */
    void processButton(uint8_t index);
    
    /**
     * @brief Get event from button and clear it
     * @param index Button index
     * @return Pending event
     */
    ButtonEvent consumeEvent(uint8_t index);
};

#endif // BUTTON_HANDLER_H
