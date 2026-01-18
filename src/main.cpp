/**
 * @file main.cpp
 * @brief Main program entry point for Smart Pill Box
 * @project Smart Pill Box with ESP32
 * @version 1.0
 * 
 * This is the main controller that coordinates all system components:
 * - Time management with RTC
 * - Dose scheduling and tracking
 * - User interface via OLED display
 * - Button input handling
 * - Alarm control
 * - Lid sensor monitoring
 * - WiFi web server for remote configuration
 * - Persistent storage
 */

#include <Arduino.h>
#include <Wire.h>

// Project modules
#include "config.h"
#include "TimeManager.h"
#include "DoseManager.h"
#include "UIManager.h"
#include "ButtonHandler.h"
#include "AlarmController.h"
#include "LidSensor.h"
#include "PillBoxWebServer.h"
#include "Storage.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
TimeManager timeManager;
DoseManager doseManager;
UIManager uiManager;
ButtonHandler buttonHandler;
AlarmController alarmController;
LidSensor lidSensor;
PillBoxWebServer webServer;
Storage storage;

// System state
SystemState systemState;

// Editing state variables
Time12H editingTime;
uint8_t editField = 0;
uint8_t editDay, editMonth;
uint16_t editYear;

// Timing variables
uint32_t lastTimeCheck = 0;
uint32_t lastDisplayUpdate = 0;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void initializeSystem();
void handleHomeScreen();
void handleMainMenu();
void handleDoseMenu();
void handleDoseList();
void handleDoseEdit(bool isNew);
void handleTimeEdit();
void handleDateEdit();
void handleAlarmToggle();
void handleWiFiToggle();
void handleAlertState();
void checkDoseTime();
void checkMidnightReset();
void updateDisplay();
void handleButtonsInMenu();
void goToHome();
void saveSystemState();

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("\n========================================");
    DEBUG_PRINTLN("  Smart Pill Box - Starting Up");
    DEBUG_PRINTLN("========================================\n");
    
    initializeSystem();
}

void initializeSystem() {
    // Initialize I2C
    Wire.begin(OLED_SDA, OLED_SCL);
    
    // Initialize storage first to load saved settings
    if (!storage.begin()) {
        DEBUG_PRINTLN("WARNING: Storage initialization failed");
    }
    
    // Load saved settings
    bool alarmEnabled, muteMode;
    storage.loadSettings(alarmEnabled, muteMode);
    systemState.alarmEnabled = alarmEnabled;
    systemState.muteMode = muteMode;
    
    // Initialize UI
    if (!uiManager.begin()) {
        DEBUG_PRINTLN("ERROR: Display initialization failed");
        while (1) delay(1000);  // Halt if display fails
    }
    uiManager.displayConfirmation("Starting...", false);
    
    // Initialize RTC
    if (!timeManager.begin()) {
        DEBUG_PRINTLN("ERROR: RTC initialization failed");
        uiManager.displayError("RTC Error!");
        delay(3000);
    }
    
    // Check if RTC lost power
    if (timeManager.lostPower()) {
        uiManager.displayError("Time lost!\nSet time in menu");
        delay(3000);
    }
    
    // Initialize dose manager and load saved doses
    doseManager.begin();
    Dose loadedDoses[MAX_DOSES];
    uint8_t loadedCount = storage.loadDoses(loadedDoses);
    for (uint8_t i = 0; i < loadedCount; i++) {
        doseManager.addDose(loadedDoses[i].time);
        if (!loadedDoses[i].enabled) {
            doseManager.setDoseEnabled(i, false);
        }
    }
    
    // Initialize other components
    buttonHandler.begin();
    alarmController.begin();
    alarmController.setEnabled(systemState.alarmEnabled);
    lidSensor.begin();
    
    // Initialize web server (but don't start it yet)
    webServer.begin(&timeManager, &doseManager, &alarmController, &storage);
    
    // Load last known day for midnight detection
    systemState.currentDay = storage.loadLastDay();
    
    // Play startup sound
    alarmController.playStartup();
    
    // Show startup complete
    uiManager.displaySuccess("Ready!");
    delay(1000);
    
    // Go to home screen
    systemState.currentMenu = MENU_HOME;
    systemState.lastActivity = millis();
    
    DEBUG_PRINTLN("System initialization complete");
    DEBUG_PRINTF("Doses configured: %d\n", doseManager.getDoseCount());
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    // Update all input handlers
    buttonHandler.update();
    lidSensor.update();
    alarmController.update();
    uiManager.update();
    
    // Check for any button press to wake screen
    if (buttonHandler.anyButtonPressed() && !uiManager.isOn()) {
        uiManager.turnOn();
        buttonHandler.clearEvents();  // Don't process this press as action
        systemState.lastActivity = millis();
        return;
    }
    
    // Periodically check for dose time
    if (millis() - lastTimeCheck >= TIME_CHECK_INTERVAL) {
        lastTimeCheck = millis();
        checkDoseTime();
        checkMidnightReset();
    }
    
    // Handle lid opening during alarm
    if (systemState.alarmActive && lidSensor.justOpened()) {
        DEBUG_PRINTLN("Lid opened during alarm - marking dose taken");
        
        if (systemState.activeDoseIndex >= 0) {
            doseManager.markDoseTaken(systemState.activeDoseIndex);
            storage.logLidOpening(timeManager.getUnixTime(), 
                                  systemState.activeDoseIndex, true);
        }
        
        alarmController.stopAlarm();
        alarmController.playConfirm();
        systemState.alarmActive = false;
        systemState.snoozeActive = false;
        systemState.activeDoseIndex = -1;
        systemState.currentMenu = MENU_HOME;
    }
    
    // Handle current menu state
    switch (systemState.currentMenu) {
        case MENU_HOME:
            handleHomeScreen();
            break;
            
        case MENU_MAIN:
            handleMainMenu();
            break;
            
        case MENU_EDIT_DOSES:
            handleDoseMenu();
            break;
            
        case MENU_ADD_DOSE:
            handleDoseEdit(true);
            break;
            
        case MENU_EDIT_DOSE:
            handleDoseEdit(false);
            break;
            
        case MENU_DELETE_DOSE:
            handleDoseList();
            break;
            
        case MENU_EDIT_TIME:
            handleTimeEdit();
            break;
            
        case MENU_EDIT_DATE:
            handleDateEdit();
            break;
            
        case MENU_ALARM_TOGGLE:
            handleAlarmToggle();
            break;
            
        case MENU_WIFI_TOGGLE:
            handleWiFiToggle();
            break;
            
        case MENU_ALERT:
            handleAlertState();
            break;
    }
    
    // Check screen timeout (not during alarm)
    if (!systemState.alarmActive && uiManager.checkTimeout()) {
        systemState.currentMenu = MENU_HOME;
    }
}

// ============================================================================
// STATE HANDLERS
// ============================================================================

void handleHomeScreen() {
    // Get current data
    Time12H currentTime = timeManager.getCurrentTime();
    int16_t minutesToNext = doseManager.getMinutesUntilNextDose(timeManager);
    uint8_t takenCount = doseManager.getDosesTakenCount();
    uint8_t totalCount = doseManager.getEnabledDosesCount();
    
    // Update display
    if (millis() - lastDisplayUpdate >= 1000) {
        lastDisplayUpdate = millis();
        uiManager.displayHome(currentTime, minutesToNext, takenCount, totalCount,
                             systemState.wifiEnabled, systemState.muteMode);
    }
    
    // Handle OK button - go to menu
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        systemState.currentMenu = MENU_MAIN;
        systemState.menuSelection = 0;
        uiManager.updateActivity();
    } else if (okEvent == BTN_LONG_PRESS) {
        // Toggle WiFi
        systemState.wifiEnabled = !systemState.wifiEnabled;
        if (systemState.wifiEnabled) {
            webServer.start();
            alarmController.playConfirm();
        } else {
            webServer.stop();
        }
    }
    
    // Handle NEXT button long press - toggle mute
    ButtonEvent nextEvent = buttonHandler.getNextEvent();
    if (nextEvent == BTN_LONG_PRESS) {
        systemState.muteMode = !systemState.muteMode;
        alarmController.setEnabled(!systemState.muteMode);
        storage.saveSettings(systemState.alarmEnabled, systemState.muteMode);
        alarmController.playConfirm();
    }
    
    // Handle BACK button long press - already on home
    buttonHandler.getBackEvent();  // Consume any event
}

void handleMainMenu() {
    uiManager.displayMainMenu(systemState.menuSelection);
    
    // OK button - select item
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        switch (systemState.menuSelection) {
            case 0:  // Edit Doses
                systemState.currentMenu = MENU_EDIT_DOSES;
                systemState.menuSelection = 0;
                break;
            case 1:  // Set Time
                systemState.currentMenu = MENU_EDIT_TIME;
                editingTime = timeManager.getCurrentTime();
                editField = 0;
                break;
            case 2:  // Set Date
                systemState.currentMenu = MENU_EDIT_DATE;
                timeManager.getDate(editDay, editMonth, editYear);
                editField = 0;
                break;
            case 3:  // Alarm
                systemState.currentMenu = MENU_ALARM_TOGGLE;
                break;
            case 4:  // WiFi
                systemState.currentMenu = MENU_WIFI_TOGGLE;
                break;
        }
        uiManager.updateActivity();
    }
    
    // NEXT button - navigate
    ButtonEvent nextEvent = buttonHandler.getNextEvent();
    if (nextEvent == BTN_SHORT_PRESS) {
        systemState.menuSelection = (systemState.menuSelection + 1) % MENU_ITEMS_COUNT;
        uiManager.updateActivity();
    } else if (nextEvent == BTN_LONG_PRESS) {
        // Toggle mute
        systemState.muteMode = !systemState.muteMode;
        alarmController.setEnabled(!systemState.muteMode);
        storage.saveSettings(systemState.alarmEnabled, systemState.muteMode);
        alarmController.playConfirm();
    }
    
    // BACK button - return to home
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS || backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
}

void handleDoseMenu() {
    uiManager.displayDoseMenu(systemState.menuSelection);
    
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        switch (systemState.menuSelection) {
            case 0:  // Add Dose
                if (doseManager.getDoseCount() >= MAX_DOSES) {
                    uiManager.displayError("Max doses reached");
                    delay(1500);
                } else {
                    systemState.currentMenu = MENU_ADD_DOSE;
                    editingTime = Time12H(12, 0, false);
                    editField = 0;
                }
                break;
            case 1:  // Edit Dose
                if (doseManager.getDoseCount() == 0) {
                    uiManager.displayError("No doses to edit");
                    delay(1500);
                } else {
                    systemState.currentMenu = MENU_EDIT_DOSE;
                    systemState.editIndex = 0;
                }
                break;
            case 2:  // Delete Dose
                if (doseManager.getDoseCount() == 0) {
                    uiManager.displayError("No doses to delete");
                    delay(1500);
                } else {
                    systemState.currentMenu = MENU_DELETE_DOSE;
                    systemState.editIndex = 0;
                }
                break;
            case 3:  // Back
                systemState.currentMenu = MENU_MAIN;
                systemState.menuSelection = 0;
                break;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent nextEvent = buttonHandler.getNextEvent();
    if (nextEvent == BTN_SHORT_PRESS) {
        systemState.menuSelection = (systemState.menuSelection + 1) % DOSE_MENU_ITEMS_COUNT;
        uiManager.updateActivity();
    }
    
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS) {
        systemState.currentMenu = MENU_MAIN;
        systemState.menuSelection = 0;
    } else if (backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
}

void handleDoseList() {
    // Display dose list for selection (delete mode)
    uiManager.displayDoseList(doseManager.getDoses(), doseManager.getDoseCount(), 
                              systemState.editIndex);
    
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        // Delete selected dose
        if (doseManager.removeDose(systemState.editIndex)) {
            doseManager.saveToStorage(storage);
            alarmController.playConfirm();
        }
        
        if (doseManager.getDoseCount() == 0) {
            systemState.currentMenu = MENU_EDIT_DOSES;
            systemState.menuSelection = 0;
        } else if (systemState.editIndex >= doseManager.getDoseCount()) {
            systemState.editIndex = doseManager.getDoseCount() - 1;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent nextEvent = buttonHandler.getNextEvent();
    if (nextEvent == BTN_SHORT_PRESS) {
        if (doseManager.getDoseCount() > 0) {
            systemState.editIndex = (systemState.editIndex + 1) % doseManager.getDoseCount();
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS) {
        systemState.currentMenu = MENU_EDIT_DOSES;
        systemState.menuSelection = 2;
    } else if (backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
}

void handleDoseEdit(bool isNew) {
    if (!isNew && systemState.currentMenu == MENU_EDIT_DOSE) {
        // First time entering edit mode - show list to select
        static bool inListMode = true;
        
        if (inListMode) {
            uiManager.displayDoseList(doseManager.getDoses(), doseManager.getDoseCount(),
                                     systemState.editIndex);
            
            ButtonEvent okEvent = buttonHandler.getOkEvent();
            if (okEvent == BTN_SHORT_PRESS) {
                Dose* selectedDose = doseManager.getDose(systemState.editIndex);
                if (selectedDose) {
                    editingTime = selectedDose->time;
                    editField = 0;
                    inListMode = false;
                }
            }
            
            ButtonEvent nextEvent = buttonHandler.getNextEvent();
            if (nextEvent == BTN_SHORT_PRESS) {
                systemState.editIndex = (systemState.editIndex + 1) % doseManager.getDoseCount();
            }
            
            ButtonEvent backEvent = buttonHandler.getBackEvent();
            if (backEvent == BTN_SHORT_PRESS) {
                inListMode = true;
                systemState.currentMenu = MENU_EDIT_DOSES;
                systemState.menuSelection = 1;
            }
            return;
        }
    }
    
    uiManager.displayDoseEdit(editingTime, editField, isNew);
    
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        editField++;
        if (editField > 2) {
            // Save the dose
            if (isNew) {
                if (doseManager.addDose(editingTime)) {
                    doseManager.saveToStorage(storage);
                    alarmController.playConfirm();
                    systemState.currentMenu = MENU_EDIT_DOSES;
                    systemState.menuSelection = 0;
                } else {
                    alarmController.playError();
                    uiManager.displayError("Time conflict!");
                    delay(1500);
                }
            } else {
                if (doseManager.updateDose(systemState.editIndex, editingTime)) {
                    doseManager.saveToStorage(storage);
                    alarmController.playConfirm();
                    systemState.currentMenu = MENU_EDIT_DOSES;
                    systemState.menuSelection = 1;
                } else {
                    alarmController.playError();
                    uiManager.displayError("Time conflict!");
                    delay(1500);
                }
            }
            editField = 0;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent nextEvent = buttonHandler.getNextEvent();
    if (nextEvent == BTN_SHORT_PRESS) {
        // Increment current field
        switch (editField) {
            case 0:  // Hour
                editingTime.hour = (editingTime.hour % 12) + 1;
                break;
            case 1:  // Minute
                editingTime.minute = (editingTime.minute + 1) % 60;
                break;
            case 2:  // AM/PM
                editingTime.isPM = !editingTime.isPM;
                break;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS) {
        if (editField > 0) {
            editField--;
        } else {
            systemState.currentMenu = MENU_EDIT_DOSES;
            systemState.menuSelection = isNew ? 0 : 1;
        }
    } else if (backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
}

void handleTimeEdit() {
    uiManager.displayTimeEdit(editingTime, editField);
    
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        editField++;
        if (editField > 2) {
            // Save time
            timeManager.setTime(editingTime);
            alarmController.playConfirm();
            systemState.currentMenu = MENU_MAIN;
            systemState.menuSelection = 1;
            editField = 0;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent nextEvent = buttonHandler.getNextEvent();
    if (nextEvent == BTN_SHORT_PRESS) {
        switch (editField) {
            case 0:  // Hour
                editingTime.hour = (editingTime.hour % 12) + 1;
                break;
            case 1:  // Minute
                editingTime.minute = (editingTime.minute + 1) % 60;
                break;
            case 2:  // AM/PM
                editingTime.isPM = !editingTime.isPM;
                break;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS) {
        if (editField > 0) {
            editField--;
        } else {
            systemState.currentMenu = MENU_MAIN;
            systemState.menuSelection = 1;
        }
    } else if (backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
}

void handleDateEdit() {
    uiManager.displayDateEdit(editDay, editMonth, editYear, editField);
    
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        editField++;
        if (editField > 2) {
            // Save date
            timeManager.setDate(editDay, editMonth, editYear);
            alarmController.playConfirm();
            systemState.currentMenu = MENU_MAIN;
            systemState.menuSelection = 2;
            editField = 0;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent nextEvent = buttonHandler.getNextEvent();
    if (nextEvent == BTN_SHORT_PRESS) {
        switch (editField) {
            case 0:  // Day
                editDay = (editDay % 31) + 1;
                break;
            case 1:  // Month
                editMonth = (editMonth % 12) + 1;
                break;
            case 2:  // Year
                editYear++;
                if (editYear > 2099) editYear = 2024;
                break;
        }
        uiManager.updateActivity();
    }
    
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS) {
        if (editField > 0) {
            editField--;
        } else {
            systemState.currentMenu = MENU_MAIN;
            systemState.menuSelection = 2;
        }
    } else if (backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
}

void handleAlarmToggle() {
    uiManager.displayAlarmToggle(systemState.alarmEnabled);
    
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        systemState.alarmEnabled = !systemState.alarmEnabled;
        alarmController.setEnabled(systemState.alarmEnabled);
        storage.saveSettings(systemState.alarmEnabled, systemState.muteMode);
        alarmController.playConfirm();
        uiManager.updateActivity();
    }
    
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS) {
        systemState.currentMenu = MENU_MAIN;
        systemState.menuSelection = 3;
    } else if (backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
    
    buttonHandler.getNextEvent();  // Consume
}

void handleWiFiToggle() {
    String ip = systemState.wifiEnabled ? webServer.getIPAddress() : "";
    uiManager.displayWiFiToggle(systemState.wifiEnabled, ip.c_str());
    
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_SHORT_PRESS) {
        systemState.wifiEnabled = !systemState.wifiEnabled;
        if (systemState.wifiEnabled) {
            webServer.start();
        } else {
            webServer.stop();
        }
        alarmController.playConfirm();
        uiManager.updateActivity();
    }
    
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS) {
        systemState.currentMenu = MENU_MAIN;
        systemState.menuSelection = 4;
    } else if (backEvent == BTN_LONG_PRESS) {
        goToHome();
    }
    
    buttonHandler.getNextEvent();  // Consume
}

void handleAlertState() {
    Dose* activeDose = doseManager.getDose(systemState.activeDoseIndex);
    
    if (systemState.snoozeActive) {
        uint16_t remaining = alarmController.getSnoozeRemaining();
        uiManager.displaySnooze(remaining);
        
        if (remaining == 0) {
            systemState.snoozeActive = false;
        }
    } else if (activeDose) {
        uiManager.displayAlert(systemState.activeDoseIndex + 1, activeDose->time);
    }
    
    // BACK button - snooze
    ButtonEvent backEvent = buttonHandler.getBackEvent();
    if (backEvent == BTN_SHORT_PRESS && !systemState.snoozeActive) {
        alarmController.snooze(SNOOZE_DURATION);
        systemState.snoozeActive = true;
        alarmController.playConfirm();
    }
    
    // OK button - acknowledged without taking (not recommended)
    ButtonEvent okEvent = buttonHandler.getOkEvent();
    if (okEvent == BTN_LONG_PRESS) {
        // Long press to dismiss without opening lid
        alarmController.stopAlarm();
        systemState.alarmActive = false;
        systemState.snoozeActive = false;
        systemState.currentMenu = MENU_HOME;
    }
    
    buttonHandler.getNextEvent();  // Consume
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void checkDoseTime() {
    if (!systemState.alarmEnabled || systemState.muteMode) {
        return;
    }
    
    if (systemState.alarmActive || systemState.snoozeActive) {
        return;
    }
    
    Time12H currentTime = timeManager.getCurrentTime();
    int8_t doseIndex = doseManager.checkDoseTime(currentTime);
    
    if (doseIndex >= 0) {
        DEBUG_PRINTF("Dose %d is due!\n", doseIndex);
        
        systemState.alarmActive = true;
        systemState.activeDoseIndex = doseIndex;
        systemState.currentMenu = MENU_ALERT;
        
        alarmController.startAlarm(PATTERN_STANDARD);
        uiManager.turnOn();
    }
}

void checkMidnightReset() {
    uint8_t day, month;
    uint16_t year;
    timeManager.getDate(day, month, year);
    
    if (day != systemState.currentDay && systemState.currentDay != 0) {
        DEBUG_PRINTLN("New day detected - resetting dose status");
        doseManager.resetDailyStatus();
        lidSensor.resetDailyCount();
        storage.saveLastDay(day);
    }
    
    systemState.currentDay = day;
}

void goToHome() {
    systemState.currentMenu = MENU_HOME;
    systemState.menuSelection = 0;
    uiManager.updateActivity();
}

void saveSystemState() {
    storage.saveSettings(systemState.alarmEnabled, systemState.muteMode);
    doseManager.saveToStorage(storage);
}
