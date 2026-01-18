/**
 * @file Storage.h
 * @brief Persistent storage management using ESP32 Preferences
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// Forward declarations
class DoseManager;

// Log entry structure
struct LogEntry {
    uint32_t timestamp;     // Unix timestamp
    uint8_t doseIndex;      // Which dose was taken
    bool wasOnTime;         // Was it taken on time
};

class Storage {
public:
    /**
     * @brief Initialize storage system
     * @return true if initialized successfully
     */
    bool begin();
    
    /**
     * @brief Save doses to storage
     * @param doses Array of doses
     * @param count Number of doses
     */
    void saveDoses(Dose* doses, uint8_t count);
    
    /**
     * @brief Load doses from storage
     * @param doses Array to load into
     * @return Number of doses loaded
     */
    uint8_t loadDoses(Dose* doses);
    
    /**
     * @brief Save system settings
     * @param alarmEnabled Alarm enabled state
     * @param muteMode Mute mode state
     */
    void saveSettings(bool alarmEnabled, bool muteMode);
    
    /**
     * @brief Load system settings
     * @param alarmEnabled Output alarm enabled state
     * @param muteMode Output mute mode state
     */
    void loadSettings(bool& alarmEnabled, bool& muteMode);
    
    /**
     * @brief Log a lid opening event
     * @param timestamp Unix timestamp of event
     * @param doseIndex Index of related dose (-1 if none)
     * @param wasOnTime Was the opening on time for the dose
     */
    void logLidOpening(uint32_t timestamp, int8_t doseIndex = -1, bool wasOnTime = false);
    
    /**
     * @brief Get log entries
     * @param logs Array to fill with log entries
     * @param maxEntries Maximum entries to retrieve
     * @return Number of entries retrieved
     */
    uint8_t getLogs(LogEntry* logs, uint8_t maxEntries);
    
    /**
     * @brief Get total number of log entries
     * @return Number of logged events
     */
    uint16_t getLogCount();
    
    /**
     * @brief Clear all logs
     */
    void clearLogs();
    
    /**
     * @brief Save last known date (for midnight detection)
     * @param day Day of month
     */
    void saveLastDay(uint8_t day);
    
    /**
     * @brief Load last known date
     * @return Last saved day of month
     */
    uint8_t loadLastDay();
    
    /**
     * @brief Check data integrity
     * @return true if data is valid
     */
    bool verifyIntegrity();
    
    /**
     * @brief Reset all storage to defaults
     */
    void factoryReset();
    
    /**
     * @brief Get storage version
     * @return Storage format version
     */
    uint8_t getVersion();
    
    /**
     * @brief Get free storage space
     * @return Free bytes available
     */
    size_t getFreeSpace();

private:
    Preferences prefs;
    bool initialized;
    
    /**
     * @brief Calculate CRC for data integrity
     * @param data Data to checksum
     * @param length Data length
     * @return CRC value
     */
    uint8_t calculateCRC(uint8_t* data, size_t length);
    
    /**
     * @brief Migrate data from old versions
     * @param oldVersion Previous storage version
     */
    void migrateData(uint8_t oldVersion);
};

#endif // STORAGE_H
