/**
 * @file config.h
 * @brief Pin definitions and system constants for Smart Pill Box
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// DISPLAY CONFIGURATION (I2C)
// ============================================================================
#define OLED_SDA            21
#define OLED_SCL            22
#define OLED_ADDR           0x3C
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64

// ============================================================================
// RTC CONFIGURATION (I2C - shares same bus as OLED)
// ============================================================================
#define RTC_SDA             21
#define RTC_SCL             22

// ============================================================================
// BUTTON CONFIGURATION (Active LOW with internal pull-up)
// ============================================================================
#define BTN_OK              25
#define BTN_NEXT            26
#define BTN_BACK            27

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================
#define REED_SWITCH         32      // Lid magnetic sensor (Active LOW)

// ============================================================================
// BUZZER CONFIGURATION
// ============================================================================
#define BUZZER_PIN          33
#define BUZZER_FREQUENCY    2000    // Hz for tone generation
#define BUZZER_CHANNEL      0       // LEDC channel for PWM

// ============================================================================
// TIMING CONSTANTS
// ============================================================================
#define DEBOUNCE_DELAY          50      // Button debounce (ms)
#define LONG_PRESS_DURATION     3000    // Long press threshold (ms)
#define SCREEN_TIMEOUT          180000  // Screen off after 3 minutes (ms)
#define SNOOZE_DURATION         300     // Snooze duration (seconds)
#define LID_DEBOUNCE_DURATION   500     // Lid must be open for 500ms
#define TIME_CHECK_INTERVAL     1000    // Check time every 1 second (ms)
#define ALARM_CHECK_TOLERANCE   5       // ±5 seconds tolerance for alarm

// ============================================================================
// DOSE CONFIGURATION
// ============================================================================
#define MAX_DOSES               10      // Maximum doses per day
#define MIN_DOSE_SPACING        15      // Minimum minutes between doses

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
#define WIFI_AP_SSID            "SmartPillBox"
#define WIFI_AP_PASSWORD        "pillbox123"
#define WIFI_AP_CHANNEL         1
#define WIFI_MAX_CONNECTIONS    4
#define WEB_SERVER_PORT         80

// ============================================================================
// STORAGE CONFIGURATION
// ============================================================================
#define STORAGE_NAMESPACE       "pillbox"
#define STORAGE_VERSION         1
#define MAX_LOG_ENTRIES         100     // Maximum lid opening logs

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================
#define DEBUG_ENABLED           1
#if DEBUG_ENABLED
    #define DEBUG_PRINT(x)      Serial.print(x)
    #define DEBUG_PRINTLN(x)    Serial.println(x)
    #define DEBUG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * @brief 12-hour time format structure
 */
struct Time12H {
    uint8_t hour;       // 1-12
    uint8_t minute;     // 0-59
    bool isPM;          // false = AM, true = PM
    
    // Default constructor
    Time12H() : hour(12), minute(0), isPM(false) {}
    
    // Parameterized constructor
    Time12H(uint8_t h, uint8_t m, bool pm) : hour(h), minute(m), isPM(pm) {}
    
    // Comparison operators
    bool operator==(const Time12H& other) const {
        return hour == other.hour && minute == other.minute && isPM == other.isPM;
    }
    
    bool operator!=(const Time12H& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Dose schedule structure
 */
struct Dose {
    Time12H time;
    bool enabled;
    bool taken;         // Reset daily at midnight
    uint8_t id;
    
    Dose() : enabled(false), taken(false), id(0) {}
};

/**
 * @brief Menu state enumeration
 */
enum MenuState {
    MENU_HOME = 0,
    MENU_MAIN,
    MENU_EDIT_DOSES,
    MENU_ADD_DOSE,
    MENU_EDIT_DOSE,
    MENU_DELETE_DOSE,
    MENU_EDIT_TIME,
    MENU_EDIT_DATE,
    MENU_ALARM_TOGGLE,
    MENU_WIFI_TOGGLE,
    MENU_ALERT
};

/**
 * @brief Button event enumeration
 */
enum ButtonEvent {
    BTN_NONE = 0,
    BTN_SHORT_PRESS,
    BTN_LONG_PRESS
};

/**
 * @brief System state structure
 */
struct SystemState {
    bool alarmEnabled;
    bool wifiEnabled;
    bool muteMode;
    bool screenOn;
    bool alarmActive;
    bool snoozeActive;
    uint32_t snoozeUntil;       // Unix timestamp
    MenuState currentMenu;
    uint8_t menuSelection;
    uint8_t editIndex;
    Dose doses[MAX_DOSES];
    uint8_t doseCount;
    int8_t activeDoseIndex;     // Currently alerting dose (-1 if none)
    uint32_t lastActivity;
    uint8_t currentDay;         // For midnight reset detection
    
    SystemState() : 
        alarmEnabled(true),
        wifiEnabled(false),
        muteMode(false),
        screenOn(true),
        alarmActive(false),
        snoozeActive(false),
        snoozeUntil(0),
        currentMenu(MENU_HOME),
        menuSelection(0),
        editIndex(0),
        doseCount(0),
        activeDoseIndex(-1),
        lastActivity(0),
        currentDay(0) {}
};

// ============================================================================
// MENU STRINGS
// ============================================================================
const char* const MENU_ITEMS[] = {
    "Edit Doses",
    "Set Time",
    "Set Date",
    "Alarm",
    "WiFi"
};
#define MENU_ITEMS_COUNT 5

const char* const DOSE_MENU_ITEMS[] = {
    "Add Dose",
    "Edit Dose",
    "Delete Dose",
    "Back"
};
#define DOSE_MENU_ITEMS_COUNT 4

#endif // CONFIG_H
