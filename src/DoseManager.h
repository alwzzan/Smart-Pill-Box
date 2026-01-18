/**
 * @file DoseManager.h
 * @brief Dose scheduling and tracking management
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef DOSE_MANAGER_H
#define DOSE_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "TimeManager.h"

// Forward declaration
class Storage;

class DoseManager {
public:
    /**
     * @brief Initialize dose manager
     */
    void begin();
    
    /**
     * @brief Add a new dose at specified time
     * @param time Time for the dose
     * @return true if dose added successfully
     */
    bool addDose(Time12H time);
    
    /**
     * @brief Remove a dose by index
     * @param index Index of dose to remove
     * @return true if removed successfully
     */
    bool removeDose(uint8_t index);
    
    /**
     * @brief Update a dose's time
     * @param index Index of dose to update
     * @param time New time for the dose
     * @return true if updated successfully
     */
    bool updateDose(uint8_t index, Time12H time);
    
    /**
     * @brief Enable or disable a dose
     * @param index Dose index
     * @param enabled Enable state
     */
    void setDoseEnabled(uint8_t index, bool enabled);
    
    /**
     * @brief Check if current time matches any dose time
     * @param currentTime Current time to check
     * @return Index of matching dose, or -1 if no match
     */
    int8_t checkDoseTime(Time12H currentTime);
    
    /**
     * @brief Mark a dose as taken
     * @param index Dose index
     */
    void markDoseTaken(uint8_t index);
    
    /**
     * @brief Check if a dose has been taken
     * @param index Dose index
     * @return true if taken
     */
    bool isDoseTaken(uint8_t index);
    
    /**
     * @brief Reset all daily taken status (call at midnight)
     */
    void resetDailyStatus();
    
    /**
     * @brief Get next upcoming dose
     * @param currentTime Current time
     * @return Index of next dose, or -1 if none
     */
    int8_t getNextDose(Time12H currentTime);
    
    /**
     * @brief Get minutes until next dose
     * @param timeManager TimeManager reference for calculation
     * @return Minutes until next dose, or -1 if none
     */
    int16_t getMinutesUntilNextDose(TimeManager& timeManager);
    
    /**
     * @brief Get dose count
     * @return Number of configured doses
     */
    uint8_t getDoseCount() const { return doseCount; }
    
    /**
     * @brief Get dose by index
     * @param index Dose index
     * @return Pointer to dose, or nullptr if invalid index
     */
    Dose* getDose(uint8_t index);
    
    /**
     * @brief Get all doses array
     * @return Pointer to doses array
     */
    Dose* getDoses() { return doses; }
    
    /**
     * @brief Sort doses chronologically
     */
    void sortDoses();
    
    /**
     * @brief Check if a time slot is available (considering spacing)
     * @param time Time to check
     * @param excludeIndex Index to exclude from check (-1 for none)
     * @return true if time slot is available
     */
    bool isTimeSlotAvailable(Time12H time, int8_t excludeIndex = -1);
    
    /**
     * @brief Save doses to persistent storage
     * @param storage Storage manager reference
     */
    void saveToStorage(Storage& storage);
    
    /**
     * @brief Load doses from persistent storage
     * @param storage Storage manager reference
     */
    void loadFromStorage(Storage& storage);
    
    /**
     * @brief Clear all doses
     */
    void clearAllDoses();
    
    /**
     * @brief Get count of doses taken today
     * @return Number of doses taken
     */
    uint8_t getDosesTakenCount();
    
    /**
     * @brief Get count of enabled doses
     * @return Number of enabled doses
     */
    uint8_t getEnabledDosesCount();

private:
    Dose doses[MAX_DOSES];
    uint8_t doseCount;
    
    /**
     * @brief Convert Time12H to minutes since midnight
     * @param time Time to convert
     * @return Minutes since midnight (0-1439)
     */
    uint16_t timeToMinutes(Time12H time);
    
    /**
     * @brief Check if two times are within MIN_DOSE_SPACING
     * @param t1 First time
     * @param t2 Second time
     * @return true if times are too close
     */
    bool timesTooClose(Time12H t1, Time12H t2);
};

#endif // DOSE_MANAGER_H
