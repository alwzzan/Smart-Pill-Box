/**
 * @file TimeManager.h
 * @brief RTC time management with 12-hour format support
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>
#include "config.h"

class TimeManager {
public:
    /**
     * @brief Initialize the RTC module
     * @return true if RTC is found and running
     */
    bool begin();
    
    /**
     * @brief Get current time in 12-hour format
     * @return Time12H structure with current time
     */
    Time12H getCurrentTime();
    
    /**
     * @brief Get current time in 24-hour format
     * @return Hour in 24-hour format (0-23)
     */
    uint8_t getCurrentHour24();
    
    /**
     * @brief Set time using 12-hour format
     * @param time Time12H structure with new time
     */
    void setTime(Time12H time);
    
    /**
     * @brief Set time using 24-hour format
     * @param hour Hour (0-23)
     * @param minute Minute (0-59)
     * @param second Second (0-59)
     */
    void setTime24(uint8_t hour, uint8_t minute, uint8_t second = 0);
    
    /**
     * @brief Set date
     * @param day Day of month (1-31)
     * @param month Month (1-12)
     * @param year Year (2000-2099)
     */
    void setDate(uint8_t day, uint8_t month, uint16_t year);
    
    /**
     * @brief Get current date
     * @param day Output day
     * @param month Output month
     * @param year Output year
     */
    void getDate(uint8_t& day, uint8_t& month, uint16_t& year);
    
    /**
     * @brief Get day of week (0=Sunday, 6=Saturday)
     * @return Day of week
     */
    uint8_t getDayOfWeek();
    
    /**
     * @brief Get current Unix timestamp
     * @return Unix timestamp
     */
    uint32_t getUnixTime();
    
    /**
     * @brief Compare two times for equality (ignoring seconds)
     * @param t1 First time
     * @param t2 Second time
     * @return true if times match
     */
    static bool isTimeMatch(Time12H t1, Time12H t2);
    
    /**
     * @brief Convert 24-hour format to 12-hour format
     * @param hour24 Hour in 24-hour format (0-23)
     * @return Time12H with hour and AM/PM
     */
    static Time12H convert24to12(uint8_t hour24);
    
    /**
     * @brief Convert 12-hour format to 24-hour format
     * @param time Time12H structure
     * @return Hour in 24-hour format (0-23)
     */
    static uint8_t convert12to24(Time12H time);
    
    /**
     * @brief Calculate minutes until a target time
     * @param target Target time
     * @return Minutes until target (0-1439)
     */
    uint16_t minutesUntil(Time12H target);
    
    /**
     * @brief Validate time values
     * @param time Time to validate
     * @return true if valid (hour 1-12, minute 0-59)
     */
    static bool isValidTime(Time12H time);
    
    /**
     * @brief Check if RTC lost power (time may be invalid)
     * @return true if power was lost
     */
    bool lostPower();
    
    /**
     * @brief Get formatted time string (HH:MM AM/PM)
     * @param time Time to format
     * @param buffer Output buffer (min 12 chars)
     */
    static void formatTime(Time12H time, char* buffer);
    
    /**
     * @brief Get formatted date string (DD/MM/YYYY)
     * @param buffer Output buffer (min 11 chars)
     */
    void formatDate(char* buffer);

private:
    RTC_DS3231 rtc;
    DateTime cachedDateTime;
    uint32_t lastCacheUpdate;
    
    /**
     * @brief Update cached time (call periodically)
     */
    void updateCache();
};

#endif // TIME_MANAGER_H
