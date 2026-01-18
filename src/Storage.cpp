/**
 * @file Storage.cpp
 * @brief Persistent storage implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "Storage.h"
#include "DoseManager.h"

// Storage keys
static const char* KEY_VERSION = "version";
static const char* KEY_DOSE_COUNT = "doseCount";
static const char* KEY_DOSES = "doses";
static const char* KEY_ALARM_EN = "alarmEn";
static const char* KEY_MUTE_MODE = "muteMode";
static const char* KEY_LAST_DAY = "lastDay";
static const char* KEY_LOG_COUNT = "logCount";
static const char* KEY_LOGS = "logs";
static const char* KEY_CRC = "crc";

bool Storage::begin() {
    initialized = prefs.begin(STORAGE_NAMESPACE, false);
    
    if (!initialized) {
        DEBUG_PRINTLN("ERROR: Failed to initialize Preferences");
        return false;
    }
    
    // Check version and migrate if necessary
    uint8_t storedVersion = prefs.getUChar(KEY_VERSION, 0);
    
    if (storedVersion == 0) {
        // First run - initialize with defaults
        prefs.putUChar(KEY_VERSION, STORAGE_VERSION);
        prefs.putUChar(KEY_DOSE_COUNT, 0);
        prefs.putBool(KEY_ALARM_EN, true);
        prefs.putBool(KEY_MUTE_MODE, false);
        prefs.putUChar(KEY_LAST_DAY, 0);
        prefs.putUShort(KEY_LOG_COUNT, 0);
        DEBUG_PRINTLN("Storage initialized with defaults");
    } else if (storedVersion < STORAGE_VERSION) {
        migrateData(storedVersion);
    }
    
    DEBUG_PRINTF("Storage initialized. Version: %d\n", STORAGE_VERSION);
    return true;
}

void Storage::saveDoses(Dose* doses, uint8_t count) {
    if (!initialized) return;
    
    // Save count
    prefs.putUChar(KEY_DOSE_COUNT, count);
    
    // Serialize doses to byte array
    // Format: [hour, minute, isPM, enabled] for each dose
    uint8_t buffer[MAX_DOSES * 4];
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t offset = i * 4;
        buffer[offset] = doses[i].time.hour;
        buffer[offset + 1] = doses[i].time.minute;
        buffer[offset + 2] = (doses[i].time.isPM ? 1 : 0);
        buffer[offset + 3] = (doses[i].enabled ? 1 : 0);
    }
    
    prefs.putBytes(KEY_DOSES, buffer, count * 4);
    
    // Save CRC
    uint8_t crc = calculateCRC(buffer, count * 4);
    prefs.putUChar(KEY_CRC, crc);
    
    DEBUG_PRINTF("Saved %d doses to storage\n", count);
}

uint8_t Storage::loadDoses(Dose* doses) {
    if (!initialized) return 0;
    
    uint8_t count = prefs.getUChar(KEY_DOSE_COUNT, 0);
    
    if (count == 0 || count > MAX_DOSES) {
        return 0;
    }
    
    // Load dose data
    uint8_t buffer[MAX_DOSES * 4];
    size_t bytesRead = prefs.getBytes(KEY_DOSES, buffer, count * 4);
    
    if (bytesRead != count * 4) {
        DEBUG_PRINTLN("ERROR: Dose data corrupted");
        return 0;
    }
    
    // Verify CRC
    uint8_t storedCrc = prefs.getUChar(KEY_CRC, 0);
    uint8_t calculatedCrc = calculateCRC(buffer, count * 4);
    
    if (storedCrc != calculatedCrc) {
        DEBUG_PRINTLN("ERROR: Dose data CRC mismatch");
        return 0;
    }
    
    // Deserialize doses
    for (uint8_t i = 0; i < count; i++) {
        uint8_t offset = i * 4;
        doses[i].time.hour = buffer[offset];
        doses[i].time.minute = buffer[offset + 1];
        doses[i].time.isPM = (buffer[offset + 2] == 1);
        doses[i].enabled = (buffer[offset + 3] == 1);
        doses[i].taken = false;  // Reset taken status on load
        doses[i].id = i;
    }
    
    DEBUG_PRINTF("Loaded %d doses from storage\n", count);
    return count;
}

void Storage::saveSettings(bool alarmEnabled, bool muteMode) {
    if (!initialized) return;
    
    prefs.putBool(KEY_ALARM_EN, alarmEnabled);
    prefs.putBool(KEY_MUTE_MODE, muteMode);
    
    DEBUG_PRINTLN("Settings saved");
}

void Storage::loadSettings(bool& alarmEnabled, bool& muteMode) {
    if (!initialized) {
        alarmEnabled = true;
        muteMode = false;
        return;
    }
    
    alarmEnabled = prefs.getBool(KEY_ALARM_EN, true);
    muteMode = prefs.getBool(KEY_MUTE_MODE, false);
    
    DEBUG_PRINTF("Settings loaded: alarm=%d, mute=%d\n", alarmEnabled, muteMode);
}

void Storage::logLidOpening(uint32_t timestamp, int8_t doseIndex, bool wasOnTime) {
    if (!initialized) return;
    
    uint16_t logCount = prefs.getUShort(KEY_LOG_COUNT, 0);
    
    // Implement circular buffer for logs
    uint8_t logIndex = logCount % MAX_LOG_ENTRIES;
    
    // Create log entry
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.doseIndex = (doseIndex >= 0) ? doseIndex : 255;
    entry.wasOnTime = wasOnTime;
    
    // Create key for this log entry
    char logKey[16];
    sprintf(logKey, "log%d", logIndex);
    
    // Store as bytes
    prefs.putBytes(logKey, &entry, sizeof(LogEntry));
    
    // Update count
    logCount++;
    prefs.putUShort(KEY_LOG_COUNT, logCount);
    
    DEBUG_PRINTF("Logged lid opening at %lu\n", timestamp);
}

uint8_t Storage::getLogs(LogEntry* logs, uint8_t maxEntries) {
    if (!initialized) return 0;
    
    uint16_t totalLogs = prefs.getUShort(KEY_LOG_COUNT, 0);
    
    if (totalLogs == 0) return 0;
    
    uint8_t entriesToRead = min((uint16_t)maxEntries, min(totalLogs, (uint16_t)MAX_LOG_ENTRIES));
    
    // Calculate starting index for circular buffer
    uint16_t startIndex = 0;
    if (totalLogs > MAX_LOG_ENTRIES) {
        startIndex = totalLogs % MAX_LOG_ENTRIES;
    }
    
    for (uint8_t i = 0; i < entriesToRead; i++) {
        uint8_t logIndex = (startIndex + i) % MAX_LOG_ENTRIES;
        
        char logKey[16];
        sprintf(logKey, "log%d", logIndex);
        
        prefs.getBytes(logKey, &logs[i], sizeof(LogEntry));
    }
    
    return entriesToRead;
}

uint16_t Storage::getLogCount() {
    if (!initialized) return 0;
    return prefs.getUShort(KEY_LOG_COUNT, 0);
}

void Storage::clearLogs() {
    if (!initialized) return;
    
    prefs.putUShort(KEY_LOG_COUNT, 0);
    
    // Clear individual log entries
    for (uint8_t i = 0; i < MAX_LOG_ENTRIES; i++) {
        char logKey[16];
        sprintf(logKey, "log%d", i);
        prefs.remove(logKey);
    }
    
    DEBUG_PRINTLN("Logs cleared");
}

void Storage::saveLastDay(uint8_t day) {
    if (!initialized) return;
    prefs.putUChar(KEY_LAST_DAY, day);
}

uint8_t Storage::loadLastDay() {
    if (!initialized) return 0;
    return prefs.getUChar(KEY_LAST_DAY, 0);
}

bool Storage::verifyIntegrity() {
    if (!initialized) return false;
    
    uint8_t count = prefs.getUChar(KEY_DOSE_COUNT, 0);
    
    if (count == 0) {
        return true;  // No data to verify
    }
    
    // Load and verify CRC
    uint8_t buffer[MAX_DOSES * 4];
    size_t bytesRead = prefs.getBytes(KEY_DOSES, buffer, count * 4);
    
    if (bytesRead != count * 4) {
        return false;
    }
    
    uint8_t storedCrc = prefs.getUChar(KEY_CRC, 0);
    uint8_t calculatedCrc = calculateCRC(buffer, count * 4);
    
    return (storedCrc == calculatedCrc);
}

void Storage::factoryReset() {
    if (!initialized) return;
    
    prefs.clear();
    
    // Reinitialize with defaults
    prefs.putUChar(KEY_VERSION, STORAGE_VERSION);
    prefs.putUChar(KEY_DOSE_COUNT, 0);
    prefs.putBool(KEY_ALARM_EN, true);
    prefs.putBool(KEY_MUTE_MODE, false);
    prefs.putUChar(KEY_LAST_DAY, 0);
    prefs.putUShort(KEY_LOG_COUNT, 0);
    
    DEBUG_PRINTLN("Factory reset complete");
}

uint8_t Storage::getVersion() {
    if (!initialized) return 0;
    return prefs.getUChar(KEY_VERSION, 0);
}

size_t Storage::getFreeSpace() {
    if (!initialized) return 0;
    return prefs.freeEntries();
}

uint8_t Storage::calculateCRC(uint8_t* data, size_t length) {
    uint8_t crc = 0;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

void Storage::migrateData(uint8_t oldVersion) {
    DEBUG_PRINTF("Migrating storage from version %d to %d\n", oldVersion, STORAGE_VERSION);
    
    // Add migration logic here for future versions
    // For now, just update the version
    
    prefs.putUChar(KEY_VERSION, STORAGE_VERSION);
}
