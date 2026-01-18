/**
 * @file DoseManager.cpp
 * @brief Dose scheduling and tracking implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "DoseManager.h"
#include "Storage.h"

void DoseManager::begin() {
    doseCount = 0;
    for (uint8_t i = 0; i < MAX_DOSES; i++) {
        doses[i] = Dose();
        doses[i].id = i;
    }
    DEBUG_PRINTLN("DoseManager initialized");
}

bool DoseManager::addDose(Time12H time) {
    if (doseCount >= MAX_DOSES) {
        DEBUG_PRINTLN("ERROR: Maximum doses reached");
        return false;
    }
    
    if (!TimeManager::isValidTime(time)) {
        DEBUG_PRINTLN("ERROR: Invalid time for dose");
        return false;
    }
    
    if (!isTimeSlotAvailable(time)) {
        DEBUG_PRINTLN("ERROR: Time slot not available (too close to existing dose)");
        return false;
    }
    
    doses[doseCount].time = time;
    doses[doseCount].enabled = true;
    doses[doseCount].taken = false;
    doses[doseCount].id = doseCount;
    doseCount++;
    
    sortDoses();
    
    char timeStr[12];
    TimeManager::formatTime(time, timeStr);
    DEBUG_PRINTF("Dose added at %s. Total doses: %d\n", timeStr, doseCount);
    
    return true;
}

bool DoseManager::removeDose(uint8_t index) {
    if (index >= doseCount) {
        DEBUG_PRINTLN("ERROR: Invalid dose index");
        return false;
    }
    
    // Shift remaining doses
    for (uint8_t i = index; i < doseCount - 1; i++) {
        doses[i] = doses[i + 1];
        doses[i].id = i;
    }
    
    doseCount--;
    doses[doseCount] = Dose();
    doses[doseCount].id = doseCount;
    
    DEBUG_PRINTF("Dose removed. Remaining doses: %d\n", doseCount);
    return true;
}

bool DoseManager::updateDose(uint8_t index, Time12H time) {
    if (index >= doseCount) {
        DEBUG_PRINTLN("ERROR: Invalid dose index");
        return false;
    }
    
    if (!TimeManager::isValidTime(time)) {
        DEBUG_PRINTLN("ERROR: Invalid time for dose");
        return false;
    }
    
    if (!isTimeSlotAvailable(time, index)) {
        DEBUG_PRINTLN("ERROR: Time slot not available");
        return false;
    }
    
    doses[index].time = time;
    sortDoses();
    
    char timeStr[12];
    TimeManager::formatTime(time, timeStr);
    DEBUG_PRINTF("Dose %d updated to %s\n", index, timeStr);
    
    return true;
}

void DoseManager::setDoseEnabled(uint8_t index, bool enabled) {
    if (index < doseCount) {
        doses[index].enabled = enabled;
    }
}

int8_t DoseManager::checkDoseTime(Time12H currentTime) {
    for (uint8_t i = 0; i < doseCount; i++) {
        if (doses[i].enabled && !doses[i].taken) {
            if (TimeManager::isTimeMatch(currentTime, doses[i].time)) {
                return i;
            }
        }
    }
    return -1;
}

void DoseManager::markDoseTaken(uint8_t index) {
    if (index < doseCount) {
        doses[index].taken = true;
        DEBUG_PRINTF("Dose %d marked as taken\n", index);
    }
}

bool DoseManager::isDoseTaken(uint8_t index) {
    if (index < doseCount) {
        return doses[index].taken;
    }
    return false;
}

void DoseManager::resetDailyStatus() {
    for (uint8_t i = 0; i < doseCount; i++) {
        doses[i].taken = false;
    }
    DEBUG_PRINTLN("Daily dose status reset");
}

int8_t DoseManager::getNextDose(Time12H currentTime) {
    uint16_t currentMinutes = timeToMinutes(currentTime);
    int8_t nextDose = -1;
    uint16_t minDiff = 24 * 60 + 1; // More than a day
    
    for (uint8_t i = 0; i < doseCount; i++) {
        if (doses[i].enabled && !doses[i].taken) {
            uint16_t doseMinutes = timeToMinutes(doses[i].time);
            uint16_t diff;
            
            if (doseMinutes > currentMinutes) {
                diff = doseMinutes - currentMinutes;
            } else if (doseMinutes < currentMinutes) {
                diff = (24 * 60 - currentMinutes) + doseMinutes;
            } else {
                // Current time matches dose time
                return i;
            }
            
            if (diff < minDiff) {
                minDiff = diff;
                nextDose = i;
            }
        }
    }
    
    return nextDose;
}

int16_t DoseManager::getMinutesUntilNextDose(TimeManager& timeManager) {
    Time12H currentTime = timeManager.getCurrentTime();
    int8_t nextDoseIndex = getNextDose(currentTime);
    
    if (nextDoseIndex < 0) {
        return -1;
    }
    
    return timeManager.minutesUntil(doses[nextDoseIndex].time);
}

Dose* DoseManager::getDose(uint8_t index) {
    if (index < doseCount) {
        return &doses[index];
    }
    return nullptr;
}

void DoseManager::sortDoses() {
    // Simple bubble sort (adequate for small arrays)
    for (uint8_t i = 0; i < doseCount - 1; i++) {
        for (uint8_t j = 0; j < doseCount - i - 1; j++) {
            uint16_t time1 = timeToMinutes(doses[j].time);
            uint16_t time2 = timeToMinutes(doses[j + 1].time);
            
            if (time1 > time2) {
                Dose temp = doses[j];
                doses[j] = doses[j + 1];
                doses[j + 1] = temp;
            }
        }
    }
    
    // Update IDs after sort
    for (uint8_t i = 0; i < doseCount; i++) {
        doses[i].id = i;
    }
}

bool DoseManager::isTimeSlotAvailable(Time12H time, int8_t excludeIndex) {
    uint16_t newMinutes = timeToMinutes(time);
    
    for (uint8_t i = 0; i < doseCount; i++) {
        if (i == excludeIndex) continue;
        
        uint16_t existingMinutes = timeToMinutes(doses[i].time);
        
        // Calculate absolute difference (considering day wrap)
        uint16_t diff = abs((int16_t)newMinutes - (int16_t)existingMinutes);
        if (diff > 12 * 60) {
            diff = 24 * 60 - diff;
        }
        
        if (diff < MIN_DOSE_SPACING) {
            return false;
        }
    }
    
    return true;
}

void DoseManager::clearAllDoses() {
    doseCount = 0;
    for (uint8_t i = 0; i < MAX_DOSES; i++) {
        doses[i] = Dose();
        doses[i].id = i;
    }
    DEBUG_PRINTLN("All doses cleared");
}

uint8_t DoseManager::getDosesTakenCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < doseCount; i++) {
        if (doses[i].taken) count++;
    }
    return count;
}

uint8_t DoseManager::getEnabledDosesCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < doseCount; i++) {
        if (doses[i].enabled) count++;
    }
    return count;
}

uint16_t DoseManager::timeToMinutes(Time12H time) {
    uint8_t hour24 = TimeManager::convert12to24(time);
    return hour24 * 60 + time.minute;
}

bool DoseManager::timesTooClose(Time12H t1, Time12H t2) {
    uint16_t min1 = timeToMinutes(t1);
    uint16_t min2 = timeToMinutes(t2);
    
    uint16_t diff = abs((int16_t)min1 - (int16_t)min2);
    if (diff > 12 * 60) {
        diff = 24 * 60 - diff;
    }
    
    return diff < MIN_DOSE_SPACING;
}

void DoseManager::saveToStorage(Storage& storage) {
    // Implementation in Storage class
}

void DoseManager::loadFromStorage(Storage& storage) {
    // Implementation in Storage class
}
