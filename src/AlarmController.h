/**
 * @file AlarmController.h
 * @brief Buzzer control and alarm pattern management
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef ALARM_CONTROLLER_H
#define ALARM_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

// Alarm pattern types
enum AlarmPattern {
    PATTERN_GENTLE,     // Soft intermittent beeps
    PATTERN_STANDARD,   // Regular beeping
    PATTERN_URGENT,     // Fast continuous beeping
    PATTERN_CONFIRM     // Single confirmation beep
};

class AlarmController {
public:
    /**
     * @brief Initialize the buzzer
     */
    void begin();
    
    /**
     * @brief Start the alarm with a pattern
     * @param pattern Alarm pattern to use
     */
    void startAlarm(AlarmPattern pattern = PATTERN_STANDARD);
    
    /**
     * @brief Stop the alarm
     */
    void stopAlarm();
    
    /**
     * @brief Snooze the alarm for a duration
     * @param seconds Snooze duration in seconds
     */
    void snooze(uint16_t seconds = SNOOZE_DURATION);
    
    /**
     * @brief Update alarm (call in loop for patterns)
     */
    void update();
    
    /**
     * @brief Check if alarm is currently active
     * @return true if alarm is sounding
     */
    bool isActive() const { return active; }
    
    /**
     * @brief Check if snooze is active
     * @return true if in snooze mode
     */
    bool isSnoozed() const { return snoozed; }
    
    /**
     * @brief Get remaining snooze time
     * @return Seconds remaining in snooze
     */
    uint16_t getSnoozeRemaining() const;
    
    /**
     * @brief Play a single beep
     * @param frequency Tone frequency in Hz
     * @param duration Duration in milliseconds
     */
    void beep(uint16_t frequency = BUZZER_FREQUENCY, uint16_t duration = 100);
    
    /**
     * @brief Play confirmation sound
     */
    void playConfirm();
    
    /**
     * @brief Play error sound
     */
    void playError();
    
    /**
     * @brief Play startup sound
     */
    void playStartup();
    
    /**
     * @brief Set alarm volume (if using PWM)
     * @param volume Volume level 0-255
     */
    void setVolume(uint8_t volume);
    
    /**
     * @brief Enable or disable buzzer output
     * @param enabled Buzzer enabled state
     */
    void setEnabled(bool enabled) { buzzerEnabled = enabled; }
    
    /**
     * @brief Check if buzzer is enabled
     * @return true if enabled
     */
    bool isEnabled() const { return buzzerEnabled; }

private:
    bool active;
    bool snoozed;
    bool buzzerEnabled;
    bool buzzerOn;
    uint8_t volume;
    AlarmPattern currentPattern;
    uint32_t snoozeEndTime;
    uint32_t lastToggle;
    uint8_t patternStep;
    
    // Pattern timing arrays (on/off pairs in ms)
    static const uint16_t GENTLE_PATTERN[];
    static const uint16_t STANDARD_PATTERN[];
    static const uint16_t URGENT_PATTERN[];
    
    /**
     * @brief Get current pattern timing
     * @return Pointer to current pattern array
     */
    const uint16_t* getCurrentPattern() const;
    
    /**
     * @brief Get pattern length
     * @return Number of steps in current pattern
     */
    uint8_t getPatternLength() const;
    
    /**
     * @brief Turn buzzer on
     */
    void buzzerOutput(bool on);
};

#endif // ALARM_CONTROLLER_H
