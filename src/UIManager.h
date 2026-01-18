/**
 * @file UIManager.h
 * @brief OLED display management and menu system
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Forward declarations
class TimeManager;
class DoseManager;

class UIManager {
public:
    /**
     * @brief Initialize the OLED display
     * @return true if display initialized successfully
     */
    bool begin();
    
    /**
     * @brief Display home screen with time and next dose info
     * @param time Current time
     * @param minutesToNextDose Minutes until next dose (-1 if none)
     * @param dosesTaken Number of doses taken today
     * @param totalDoses Total doses scheduled
     * @param wifiOn WiFi status
     * @param muteOn Mute status
     */
    void displayHome(Time12H time, int16_t minutesToNextDose, 
                     uint8_t dosesTaken, uint8_t totalDoses,
                     bool wifiOn, bool muteOn);
    
    /**
     * @brief Display main menu
     * @param selection Current selection index
     */
    void displayMainMenu(uint8_t selection);
    
    /**
     * @brief Display dose management menu
     * @param selection Current selection index
     */
    void displayDoseMenu(uint8_t selection);
    
    /**
     * @brief Display list of all doses
     * @param doses Array of doses
     * @param count Number of doses
     * @param selection Current selection index
     */
    void displayDoseList(Dose* doses, uint8_t count, uint8_t selection);
    
    /**
     * @brief Display dose edit screen
     * @param time Current time being edited
     * @param editField 0=hour, 1=minute, 2=AM/PM
     * @param isNew true if adding new dose
     */
    void displayDoseEdit(Time12H time, uint8_t editField, bool isNew);
    
    /**
     * @brief Display time edit screen
     * @param time Current time being edited
     * @param editField 0=hour, 1=minute, 2=AM/PM
     */
    void displayTimeEdit(Time12H time, uint8_t editField);
    
    /**
     * @brief Display date edit screen
     * @param day Day value
     * @param month Month value
     * @param year Year value
     * @param editField 0=day, 1=month, 2=year
     */
    void displayDateEdit(uint8_t day, uint8_t month, uint16_t year, uint8_t editField);
    
    /**
     * @brief Display alarm toggle screen
     * @param enabled Current alarm state
     */
    void displayAlarmToggle(bool enabled);
    
    /**
     * @brief Display WiFi toggle screen
     * @param enabled Current WiFi state
     * @param ipAddress IP address if connected
     */
    void displayWiFiToggle(bool enabled, const char* ipAddress = nullptr);
    
    /**
     * @brief Display medication alert screen (animated)
     * @param doseNumber Dose number (1-based)
     * @param doseTime Time of the dose
     */
    void displayAlert(uint8_t doseNumber, Time12H doseTime);
    
    /**
     * @brief Display snooze active screen
     * @param remainingSeconds Seconds until alarm resumes
     */
    void displaySnooze(uint16_t remainingSeconds);
    
    /**
     * @brief Display confirmation dialog
     * @param message Message to display
     * @param confirm true to show "OK to confirm"
     */
    void displayConfirmation(const char* message, bool confirm = true);
    
    /**
     * @brief Display error message
     * @param message Error message
     */
    void displayError(const char* message);
    
    /**
     * @brief Display success message
     * @param message Success message
     */
    void displaySuccess(const char* message);
    
    /**
     * @brief Turn off the display
     */
    void turnOff();
    
    /**
     * @brief Turn on the display
     */
    void turnOn();
    
    /**
     * @brief Check if display is on
     * @return true if display is on
     */
    bool isOn() const { return displayOn; }
    
    /**
     * @brief Update activity timestamp (resets screen timeout)
     */
    void updateActivity();
    
    /**
     * @brief Check and handle screen timeout
     * @return true if screen was turned off
     */
    bool checkTimeout();
    
    /**
     * @brief Update display (call in loop for animations)
     */
    void update();
    
    /**
     * @brief Set display brightness
     * @param brightness 0-255
     */
    void setBrightness(uint8_t brightness);

private:
    Adafruit_SSD1306 display;
    bool displayOn;
    uint32_t lastActivity;
    uint8_t animationFrame;
    uint32_t lastAnimationUpdate;
    
    /**
     * @brief Draw status bar with icons
     * @param wifiOn WiFi status
     * @param muteOn Mute status
     * @param alarmOn Alarm enabled status
     */
    void drawStatusBar(bool wifiOn, bool muteOn, bool alarmOn = true);
    
    /**
     * @brief Draw a menu item
     * @param y Y position
     * @param text Item text
     * @param selected Is this item selected
     */
    void drawMenuItem(int16_t y, const char* text, bool selected);
    
    /**
     * @brief Draw a large time display
     * @param time Time to display
     * @param y Y position
     * @param large Use large font
     */
    void drawTime(Time12H time, int16_t y, bool large = true);
    
    /**
     * @brief Draw progress bar
     * @param x X position
     * @param y Y position
     * @param width Bar width
     * @param height Bar height
     * @param progress Progress 0-100
     */
    void drawProgressBar(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t progress);
    
    /**
     * @brief Draw pill icon
     * @param x X position
     * @param y Y position
     */
    void drawPillIcon(int16_t x, int16_t y);
    
    /**
     * @brief Draw bell/alarm icon
     * @param x X position
     * @param y Y position
     * @param ringing Animate as ringing
     */
    void drawBellIcon(int16_t x, int16_t y, bool ringing = false);
    
    /**
     * @brief Center text horizontally
     * @param text Text to center
     * @param y Y position
     */
    void drawCenteredText(const char* text, int16_t y);
};

#endif // UI_MANAGER_H
