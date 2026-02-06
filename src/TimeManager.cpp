/**
 * @file TimeManager.cpp
 * @brief RTC time management implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "TimeManager.h"

bool TimeManager::begin() {
    if (!rtc.begin()) {
        DEBUG_PRINTLN("ERROR: RTC not found!");
        return false;
    }
    
    if (rtc.lostPower()) {
        DEBUG_PRINTLN("WARNING: RTC lost power, setting default time");
        // Set to a default time: 12:00:00 PM, January 1, 2024
        rtc.adjust(DateTime(2024, 1, 1, 12, 0, 0));
    }
    
    lastCacheUpdate = 0;
    updateCache();
    
    DEBUG_PRINTLN("TimeManager initialized successfully");
    return true;
}

void TimeManager::updateCache() {
    uint32_t now = millis();
    if (now - lastCacheUpdate >= TIME_CHECK_INTERVAL) {
        cachedDateTime = rtc.now();
        lastCacheUpdate = now;
    }
}

Time12H TimeManager::getCurrentTime() {
    updateCache();
    Time12H result = convert24to12(cachedDateTime.hour());
    result.minute = cachedDateTime.minute();
    return result;
}

uint8_t TimeManager::getCurrentHour24() {
    updateCache();
    return cachedDateTime.hour();
}

void TimeManager::setTime(Time12H time) {
    if (!isValidTime(time)) {
        DEBUG_PRINTLN("ERROR: Invalid time values");
        return;
    }
    
    uint8_t hour24 = convert12to24(time);
    DateTime current = rtc.now();
    
    rtc.adjust(DateTime(
        current.year(),
        current.month(),
        current.day(),
        hour24,
        time.minute,
        0
    ));
    
    lastCacheUpdate = 0; // Force cache update
    updateCache();
    
    DEBUG_PRINTF("Time set to: %02d:%02d %s\n", 
                 time.hour, time.minute, time.isPM ? "PM" : "AM");
}

void TimeManager::setTime24(uint8_t hour, uint8_t minute, uint8_t second) {
    if (hour > 23 || minute > 59 || second > 59) {
        DEBUG_PRINTLN("ERROR: Invalid time values");
        return;
    }
    
    DateTime current = rtc.now();
    rtc.adjust(DateTime(
        current.year(),
        current.month(),
        current.day(),
        hour,
        minute,
        second
    ));
    
    lastCacheUpdate = 0;
    updateCache();
}

void TimeManager::setDate(uint8_t day, uint8_t month, uint16_t year) {
    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 2000 || year > 2099) {
        DEBUG_PRINTLN("ERROR: Invalid date values");
        return;
    }
    
    DateTime current = rtc.now();
    rtc.adjust(DateTime(
        year,
        month,
        day,
        current.hour(),
        current.minute(),
        current.second()
    ));
    
    lastCacheUpdate = 0;
    updateCache();
    
    DEBUG_PRINTF("Date set to: %02d/%02d/%04d\n", day, month, year);
}

void TimeManager::getDate(uint8_t& day, uint8_t& month, uint16_t& year) {
    updateCache();
    day = cachedDateTime.day();
    month = cachedDateTime.month();
    year = cachedDateTime.year();
}

uint8_t TimeManager::getDayOfWeek() {
    updateCache();
    return cachedDateTime.dayOfTheWeek();
}

uint32_t TimeManager::getUnixTime() {
    updateCache();
    return cachedDateTime.unixtime();
}

bool TimeManager::isTimeMatch(Time12H t1, Time12H t2) {
    return t1.hour == t2.hour && 
           t1.minute == t2.minute && 
           t1.isPM == t2.isPM;
}

Time12H TimeManager::convert24to12(uint8_t hour24) {
    Time12H result;
    
    if (hour24 == 0) {
        result.hour = 12;
        result.isPM = false;
    } else if (hour24 < 12) {
        result.hour = hour24;
        result.isPM = false;
    } else if (hour24 == 12) {
        result.hour = 12;
        result.isPM = true;
    } else {
        result.hour = hour24 - 12;
        result.isPM = true;
    }
    
    return result;
}

uint8_t TimeManager::convert12to24(Time12H time) {
    if (time.isPM) {
        if (time.hour == 12) {
            return 12;
        }
        return time.hour + 12;
    } else {
        if (time.hour == 12) {
            return 0;
        }
        return time.hour;
    }
}

uint16_t TimeManager::minutesUntil(Time12H target) {
    updateCache();
    
    // Convert both times to minutes since midnight
    uint16_t currentMinutes = cachedDateTime.hour() * 60 + cachedDateTime.minute();
    
    uint8_t targetHour24 = convert12to24(target);
    uint16_t targetMinutes = targetHour24 * 60 + target.minute;
    
    if (targetMinutes > currentMinutes) {
        return targetMinutes - currentMinutes;
    } else if (targetMinutes < currentMinutes) {
        // Target is tomorrow
        return (24 * 60 - currentMinutes) + targetMinutes;
    }
    
    return 0; // Times are equal
}

bool TimeManager::isValidTime(Time12H time) {
    return time.hour >= 1 && time.hour <= 12 && time.minute <= 59;
}

bool TimeManager::lostPower() {
    return rtc.lostPower();
}

void TimeManager::formatTime(Time12H time, char* buffer) {
    sprintf(buffer, "%2d:%02d %s", time.hour, time.minute, time.isPM ? "PM" : "AM");
}

void TimeManager::formatDate(char* buffer) {
    updateCache();
    sprintf(buffer, "%02d/%02d/%04d", 
            cachedDateTime.day(), 
            cachedDateTime.month(), 
            cachedDateTime.year());
}
