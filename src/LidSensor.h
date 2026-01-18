/**
 * @file LidSensor.h
 * @brief Reed switch lid sensor monitoring
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef LID_SENSOR_H
#define LID_SENSOR_H

#include <Arduino.h>
#include "config.h"

class LidSensor {
public:
    /**
     * @brief Initialize the lid sensor
     */
    void begin();
    
    /**
     * @brief Update sensor state (call every loop)
     */
    void update();
    
    /**
     * @brief Check if lid is currently open
     * @return true if lid is open
     */
    bool isOpen() const { return lidOpen; }
    
    /**
     * @brief Check if lid was just opened (edge detection)
     * @return true if lid just opened (only returns true once per opening)
     */
    bool justOpened();
    
    /**
     * @brief Check if lid was just closed (edge detection)
     * @return true if lid just closed (only returns true once per closing)
     */
    bool justClosed();
    
    /**
     * @brief Get time since lid was last opened
     * @return Milliseconds since last opening
     */
    uint32_t timeSinceLastOpen() const;
    
    /**
     * @brief Get number of openings today
     * @return Opening count
     */
    uint16_t getOpeningsToday() const { return openingsToday; }
    
    /**
     * @brief Reset daily opening counter
     */
    void resetDailyCount() { openingsToday = 0; }
    
    /**
     * @brief Check if sensor is working (for diagnostics)
     * @return true if sensor readings are valid
     */
    bool isSensorWorking() const { return sensorWorking; }

private:
    bool lidOpen;
    bool lastState;
    bool currentReading;
    bool justOpenedFlag;
    bool justClosedFlag;
    bool sensorWorking;
    uint32_t lastDebounceTime;
    uint32_t stateChangeTime;
    uint32_t lastOpenTime;
    uint16_t openingsToday;
    
    /**
     * @brief Debounce the sensor reading
     * @return Debounced state
     */
    bool debounce();
};

#endif // LID_SENSOR_H
