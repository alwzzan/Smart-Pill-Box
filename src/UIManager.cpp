/**
 * @file UIManager.cpp
 * @brief OLED display management implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "UIManager.h"
#include "TimeManager.h"
// Pill icon bitmap (16x16)
static const uint8_t PROGMEM pillIcon[] = {
    0x00, 0x00, 0x03, 0xC0, 0x0F, 0xF0, 0x1F, 0xF8,
    0x3F, 0xFC, 0x3F, 0xFC, 0x7F, 0xFE, 0x7F, 0xFE,
    0x7F, 0xFE, 0x7F, 0xFE, 0x3F, 0xFC, 0x3F, 0xFC,
    0x1F, 0xF8, 0x0F, 0xF0, 0x03, 0xC0, 0x00, 0x00
};

// Bell icon bitmap (16x16)
static const uint8_t PROGMEM bellIcon[] = {
    0x01, 0x80, 0x01, 0x80, 0x07, 0xE0, 0x0F, 0xF0,
    0x0F, 0xF0, 0x0F, 0xF0, 0x1F, 0xF8, 0x1F, 0xF8,
    0x1F, 0xF8, 0x3F, 0xFC, 0x3F, 0xFC, 0x7F, 0xFE,
    0x00, 0x00, 0x03, 0xC0, 0x03, 0xC0, 0x00, 0x00
};

// WiFi icon bitmap (16x12)
static const uint8_t PROGMEM wifiIcon[] = {
    0x07, 0xE0, 0x1F, 0xF8, 0x78, 0x1E, 0xE0, 0x07,
    0x0F, 0xF0, 0x3C, 0x3C, 0x10, 0x08, 0x07, 0xE0,
    0x0C, 0x30, 0x00, 0x00, 0x03, 0xC0, 0x03, 0xC0
};

// Mute icon bitmap (12x12)
static const uint8_t PROGMEM muteIcon[] = {
    0x00, 0x60, 0x01, 0xE0, 0x07, 0x60, 0x1D, 0xE0,
    0x35, 0xE0, 0x6D, 0xE0, 0x35, 0xE0, 0x1D, 0xE0,
    0x07, 0x60, 0x01, 0xE0, 0x00, 0x60, 0x00, 0x00
};

bool UIManager::begin() {
    display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        DEBUG_PRINTLN("ERROR: OLED allocation failed");
        return false;
    }
    
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Smart Pill Box");
    display.println("Initializing...");
    display.display();
    
    displayOn = true;
    lastActivity = millis();
    animationFrame = 0;
    lastAnimationUpdate = 0;
    
    DEBUG_PRINTLN("UIManager initialized successfully");
    return true;
}

void UIManager::displayHome(Time12H time, int16_t minutesToNextDose,
                            uint8_t dosesTaken, uint8_t totalDoses,
                            bool wifiOn, bool muteOn) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Status bar at top
    drawStatusBar(wifiOn, muteOn);
    
    // Large time display
    drawTime(time, 14, true);
    
    // Separator line
    display.drawFastHLine(0, 38, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Next dose info
    display.setTextSize(1);
    if (minutesToNextDose >= 0) {
        char buffer[32];
        if (minutesToNextDose == 0) {
            sprintf(buffer, "DOSE NOW!");
        } else if (minutesToNextDose < 60) {
            sprintf(buffer, "Next: %d min", minutesToNextDose);
        } else {
            uint8_t hours = minutesToNextDose / 60;
            uint8_t mins = minutesToNextDose % 60;
            sprintf(buffer, "Next: %dh %dm", hours, mins);
        }
        drawCenteredText(buffer, 42);
    } else {
        drawCenteredText("No doses scheduled", 42);
    }
    
    // Progress at bottom
    char progressStr[16];
    sprintf(progressStr, "Today: %d/%d", dosesTaken, totalDoses);
    drawCenteredText(progressStr, 54);
    
    display.display();
}

void UIManager::displayMainMenu(uint8_t selection) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    drawCenteredText("MENU", 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Menu items
    uint8_t startIndex = 0;
    uint8_t visibleItems = 4;
    
    if (selection >= visibleItems) {
        startIndex = selection - visibleItems + 1;
    }
    
    for (uint8_t i = 0; i < visibleItems && (startIndex + i) < MENU_ITEMS_COUNT; i++) {
        uint8_t itemIndex = startIndex + i;
        drawMenuItem(14 + i * 12, MENU_ITEMS[itemIndex], itemIndex == selection);
    }
    
    // Scroll indicator
    if (MENU_ITEMS_COUNT > visibleItems) {
        if (startIndex > 0) {
            display.fillTriangle(124, 14, 120, 18, 127, 18, SSD1306_WHITE);
        }
        if (startIndex + visibleItems < MENU_ITEMS_COUNT) {
            display.fillTriangle(124, 60, 120, 56, 127, 56, SSD1306_WHITE);
        }
    }
    
    display.display();
}

void UIManager::displayDoseMenu(uint8_t selection) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    drawCenteredText("DOSE SETTINGS", 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Menu items
    for (uint8_t i = 0; i < DOSE_MENU_ITEMS_COUNT; i++) {
        drawMenuItem(14 + i * 12, DOSE_MENU_ITEMS[i], i == selection);
    }
    
    display.display();
}

void UIManager::displayDoseList(Dose* doses, uint8_t count, uint8_t selection) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    char title[20];
    sprintf(title, "DOSES (%d)", count);
    drawCenteredText(title, 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    if (count == 0) {
        drawCenteredText("No doses configured", 30);
        display.display();
        return;
    }
    
    // List doses
    uint8_t startIndex = 0;
    uint8_t visibleItems = 4;
    
    if (selection >= visibleItems) {
        startIndex = selection - visibleItems + 1;
    }
    
    for (uint8_t i = 0; i < visibleItems && (startIndex + i) < count; i++) {
        uint8_t itemIndex = startIndex + i;
        char timeStr[16];
        TimeManager::formatTime(doses[itemIndex].time, timeStr);
        
        // Add status indicator
        char line[24];
        if (doses[itemIndex].taken) {
            sprintf(line, "%s [Done]", timeStr);
        } else if (!doses[itemIndex].enabled) {
            sprintf(line, "%s [Off]", timeStr);
        } else {
            strcpy(line, timeStr);
        }
        
        drawMenuItem(14 + i * 12, line, itemIndex == selection);
    }
    
    display.display();
}

void UIManager::displayDoseEdit(Time12H time, uint8_t editField, bool isNew) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    drawCenteredText(isNew ? "ADD DOSE" : "EDIT DOSE", 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Time display
    display.setTextSize(2);
    char hourStr[4], minStr[4], ampmStr[4];
    sprintf(hourStr, "%2d", time.hour);
    sprintf(minStr, "%02d", time.minute);
    strcpy(ampmStr, time.isPM ? "PM" : "AM");
    
    int16_t totalWidth = 12 * 2 + 12 + 12 * 2 + 6 + 12 * 2; // Approximate width
    int16_t startX = (SCREEN_WIDTH - 84) / 2;
    
    // Hour
    if (editField == 0) {
        display.fillRect(startX - 2, 22, 28, 20, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(startX, 24);
    display.print(hourStr);
    display.setTextColor(SSD1306_WHITE);
    
    // Colon
    display.setCursor(startX + 24, 24);
    display.print(":");
    
    // Minute
    if (editField == 1) {
        display.fillRect(startX + 34, 22, 28, 20, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(startX + 36, 24);
    display.print(minStr);
    display.setTextColor(SSD1306_WHITE);
    
    // AM/PM
    display.setTextSize(1);
    if (editField == 2) {
        display.fillRect(startX + 68, 28, 20, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(startX + 70, 30);
    display.print(ampmStr);
    display.setTextColor(SSD1306_WHITE);
    
    // Instructions
    display.setTextSize(1);
    drawCenteredText("NEXT:Change OK:Save", 54);
    
    display.display();
}

void UIManager::displayTimeEdit(Time12H time, uint8_t editField) {
    displayDoseEdit(time, editField, false);
    
    // Override title
    display.fillRect(0, 0, SCREEN_WIDTH, 10, SSD1306_BLACK);
    display.setTextSize(1);
    drawCenteredText("SET TIME", 0);
    display.display();
}

void UIManager::displayDateEdit(uint8_t day, uint8_t month, uint16_t year, uint8_t editField) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    drawCenteredText("SET DATE", 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Date display
    display.setTextSize(2);
    char dayStr[4], monthStr[4], yearStr[6];
    sprintf(dayStr, "%02d", day);
    sprintf(monthStr, "%02d", month);
    sprintf(yearStr, "%04d", year);
    
    int16_t startX = 8;
    
    // Day
    if (editField == 0) {
        display.fillRect(startX - 2, 22, 28, 20, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(startX, 24);
    display.print(dayStr);
    display.setTextColor(SSD1306_WHITE);
    
    // Separator
    display.setCursor(startX + 24, 24);
    display.print("/");
    
    // Month
    if (editField == 1) {
        display.fillRect(startX + 34, 22, 28, 20, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(startX + 36, 24);
    display.print(monthStr);
    display.setTextColor(SSD1306_WHITE);
    
    // Separator
    display.setCursor(startX + 60, 24);
    display.print("/");
    
    // Year
    if (editField == 2) {
        display.fillRect(startX + 70, 22, 52, 20, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(startX + 72, 24);
    display.print(yearStr);
    display.setTextColor(SSD1306_WHITE);
    
    // Instructions
    display.setTextSize(1);
    drawCenteredText("NEXT:Change OK:Save", 54);
    
    display.display();
}

void UIManager::displayAlarmToggle(bool enabled) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    drawCenteredText("ALARM SETTINGS", 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Icon
    drawBellIcon(56, 18, false);
    
    // Status
    display.setTextSize(2);
    if (enabled) {
        drawCenteredText("ON", 40);
    } else {
        drawCenteredText("OFF", 40);
    }
    
    // Instructions
    display.setTextSize(1);
    drawCenteredText("OK:Toggle BACK:Exit", 54);
    
    display.display();
}

void UIManager::displayWiFiToggle(bool enabled, const char* ipAddress) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    drawCenteredText("WIFI SETTINGS", 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Icon
    display.drawBitmap(56, 14, wifiIcon, 16, 12, SSD1306_WHITE);
    
    // Status
    display.setTextSize(2);
    if (enabled) {
        drawCenteredText("ON", 30);
        display.setTextSize(1);
        if (ipAddress) {
            drawCenteredText(ipAddress, 46);
        }
    } else {
        drawCenteredText("OFF", 34);
    }
    
    // Instructions
    display.setTextSize(1);
    drawCenteredText("OK:Toggle BACK:Exit", 54);
    
    display.display();
}

void UIManager::displayAlert(uint8_t doseNumber, Time12H doseTime) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Animated border
    if (animationFrame % 2 == 0) {
        display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        display.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, SSD1306_WHITE);
    }
    
    // Bell icon (animated)
    int16_t bellOffset = (animationFrame % 4 < 2) ? -2 : 2;
    drawBellIcon(56 + bellOffset, 8, true);
    
    // Alert text
    display.setTextSize(2);
    drawCenteredText("TAKE", 26);
    drawCenteredText("MEDICINE", 44);
    
    // Dose time
    display.setTextSize(1);
    char timeStr[16];
    TimeManager::formatTime(doseTime, timeStr);
    drawCenteredText(timeStr, 56);
    
    display.display();
}

void UIManager::displaySnooze(uint16_t remainingSeconds) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    drawCenteredText("SNOOZED", 0);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Remaining time
    display.setTextSize(2);
    uint8_t mins = remainingSeconds / 60;
    uint8_t secs = remainingSeconds % 60;
    char timeStr[10];
    sprintf(timeStr, "%d:%02d", mins, secs);
    drawCenteredText(timeStr, 24);
    
    // Progress bar
    uint8_t progress = ((SNOOZE_DURATION - remainingSeconds) * 100) / SNOOZE_DURATION;
    drawProgressBar(10, 46, SCREEN_WIDTH - 20, 8, progress);
    
    // Instructions
    display.setTextSize(1);
    drawCenteredText("Open lid to take dose", 56);
    
    display.display();
}

void UIManager::displayConfirmation(const char* message, bool confirm) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    display.setTextSize(1);
    drawCenteredText(message, 20);
    
    if (confirm) {
        drawCenteredText("OK to confirm", 40);
        drawCenteredText("BACK to cancel", 50);
    }
    
    display.display();
}

void UIManager::displayError(const char* message) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    display.setTextSize(1);
    drawCenteredText("ERROR", 10);
    display.drawFastHLine(20, 20, SCREEN_WIDTH - 40, SSD1306_WHITE);
    
    drawCenteredText(message, 30);
    drawCenteredText("Press any button", 50);
    
    display.display();
}

void UIManager::displaySuccess(const char* message) {
    if (!displayOn) return;
    
    display.clearDisplay();
    
    display.setTextSize(1);
    drawCenteredText("SUCCESS", 10);
    display.drawFastHLine(20, 20, SCREEN_WIDTH - 40, SSD1306_WHITE);
    
    drawCenteredText(message, 35);
    
    display.display();
}

void UIManager::turnOff() {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    displayOn = false;
    DEBUG_PRINTLN("Display turned off");
}

void UIManager::turnOn() {
    display.ssd1306_command(SSD1306_DISPLAYON);
    displayOn = true;
    lastActivity = millis();
    DEBUG_PRINTLN("Display turned on");
}

void UIManager::updateActivity() {
    lastActivity = millis();
    if (!displayOn) {
        turnOn();
    }
}

bool UIManager::checkTimeout() {
    if (displayOn && (millis() - lastActivity >= SCREEN_TIMEOUT)) {
        turnOff();
        return true;
    }
    return false;
}

void UIManager::update() {
    // Update animation frame
    if (millis() - lastAnimationUpdate >= 250) {
        animationFrame++;
        lastAnimationUpdate = millis();
    }
}

void UIManager::setBrightness(uint8_t brightness) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(brightness);
}

void UIManager::drawStatusBar(bool wifiOn, bool muteOn, bool alarmOn) {
    // WiFi icon
    if (wifiOn) {
        display.drawBitmap(0, 0, wifiIcon, 16, 12, SSD1306_WHITE);
    }
    
    // Mute icon
    if (muteOn) {
        display.drawBitmap(SCREEN_WIDTH - 14, 0, muteIcon, 12, 12, SSD1306_WHITE);
    }
    
    // Alarm indicator
    if (!alarmOn) {
        display.setTextSize(1);
        display.setCursor(SCREEN_WIDTH - 28, 2);
        display.print("Zz");
    }
}

void UIManager::drawMenuItem(int16_t y, const char* text, bool selected) {
    if (selected) {
        display.fillRect(0, y - 1, SCREEN_WIDTH, 11, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    } else {
        display.setTextColor(SSD1306_WHITE);
    }
    
    display.setTextSize(1);
    display.setCursor(4, y);
    display.print(text);
    
    if (selected) {
        display.setCursor(SCREEN_WIDTH - 8, y);
        display.print("<");
    }
    
    display.setTextColor(SSD1306_WHITE);
}

void UIManager::drawTime(Time12H time, int16_t y, bool large) {
    char timeStr[12];
    TimeManager::formatTime(time, timeStr);
    
    if (large) {
        display.setTextSize(2);
    } else {
        display.setTextSize(1);
    }
    
    drawCenteredText(timeStr, y);
}

void UIManager::drawProgressBar(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t progress) {
    display.drawRect(x, y, width, height, SSD1306_WHITE);
    
    int16_t fillWidth = ((width - 4) * progress) / 100;
    if (fillWidth > 0) {
        display.fillRect(x + 2, y + 2, fillWidth, height - 4, SSD1306_WHITE);
    }
}

void UIManager::drawPillIcon(int16_t x, int16_t y) {
    display.drawBitmap(x, y, pillIcon, 16, 16, SSD1306_WHITE);
}

void UIManager::drawBellIcon(int16_t x, int16_t y, bool ringing) {
    display.drawBitmap(x, y, bellIcon, 16, 16, SSD1306_WHITE);
    
    if (ringing) {
        // Add ringing lines
        display.drawLine(x - 4, y + 4, x - 2, y + 6, SSD1306_WHITE);
        display.drawLine(x + 18, y + 4, x + 20, y + 6, SSD1306_WHITE);
    }
}

void UIManager::drawCenteredText(const char* text, int16_t y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, y);
    display.print(text);
}
