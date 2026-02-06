/**
 * @file PillBoxWebServer.cpp
 * @brief WiFi Access Point and Web Server implementation
 * @project Smart Pill Box with ESP32
 * @version 2.0 — State-machine based WiFi management
 *
 * Architecture:
 *   update() is called every loop() iteration and drives a non-blocking
 *   state machine:
 *
 *     WIFI_ST_OFF ──requestStart()──▶ WIFI_ST_STARTING
 *       step 0: WiFi.mode(WIFI_AP)          wait 100 ms
 *       step 1: WiFi.softAP(...)            wait 100 ms
 *       step 2: register routes + server.begin() (once)
 *              ──▶ WIFI_ST_RUNNING
 *
 *     WIFI_ST_RUNNING ──requestStop()──▶ WIFI_ST_STOPPING
 *       step 0: WiFi.softAPdisconnect(true) wait 100 ms
 *       step 1: WiFi.mode(WIFI_OFF)         wait 50 ms
 *              ──▶ WIFI_ST_OFF
 *
 *   server.begin() is called EXACTLY ONCE.  On subsequent start cycles
 *   the server is already listening; we only re-enable the AP radio so
 *   clients can connect again.
 *
 *   WiFi.mode(WIFI_OFF) is safe to call because the AsyncWebServer's
 *   internal TCP listener survives the mode change on ESP-IDF/Arduino —
 *   it re-binds when WiFi.mode(WIFI_AP) is called again.  This lets us
 *   fully power-down the radio when WiFi is toggled off.
 */

#include "PillBoxWebServer.h"
#include "TimeManager.h"
#include "DoseManager.h"
#include "AlarmController.h"
#include "Storage.h"

// Stabilization delays between sub-steps (milliseconds)
static const uint32_t STEP_DELAY_MS = 100;

// ============================================================================
// Constructor
// ============================================================================
PillBoxWebServer::PillBoxWebServer() : server(WEB_SERVER_PORT) {
    wifiState       = WIFI_ST_OFF;
    startupStep     = 0;
    shutdownStep    = 0;
    stepTimestamp    = 0;

    spiffsMounted   = false;
    routesConfigured = false;
    serverStarted   = false;

    timeManager     = nullptr;
    doseManager     = nullptr;
    alarmController = nullptr;
    storage         = nullptr;
    timeEditUnlocked = false;
    timeUnlockCallback = nullptr;
}

// ============================================================================
// begin() — called once from setup()
// ============================================================================
void PillBoxWebServer::begin(TimeManager* tm, DoseManager* dm,
                              AlarmController* ac, Storage* st) {
    timeManager     = tm;
    doseManager     = dm;
    alarmController = ac;
    storage         = st;

    // Mount SPIFFS exactly once
    if (!spiffsMounted) {
        if (SPIFFS.begin(true)) {
            spiffsMounted = true;
            DEBUG_PRINTLN("SPIFFS mounted successfully");
        } else {
            DEBUG_PRINTLN("ERROR: SPIFFS mount failed");
        }
    }

    DEBUG_PRINTLN("PillBoxWebServer initialized");
}

// ============================================================================
// State machine tick — call from loop()
// ============================================================================
void PillBoxWebServer::update() {
    switch (wifiState) {
        case WIFI_ST_STARTING:
            handleStarting();
            break;
        case WIFI_ST_STOPPING:
            handleStopping();
            break;
        default:
            break;  // RUNNING / OFF — nothing to tick
    }
}

// ============================================================================
// requestStart / requestStop — only set target state, never block
// ============================================================================
void PillBoxWebServer::requestStart() {
    if (wifiState != WIFI_ST_OFF) {
        return;  // Already running or in transition
    }
    startupStep   = 0;
    stepTimestamp  = millis();
    wifiState     = WIFI_ST_STARTING;
    DEBUG_PRINTLN("WiFi start requested");
}

void PillBoxWebServer::requestStop() {
    if (wifiState != WIFI_ST_RUNNING) {
        return;  // Not running or already stopping
    }
    shutdownStep  = 0;
    stepTimestamp  = millis();
    wifiState     = WIFI_ST_STOPPING;
    timeEditUnlocked = false;
    DEBUG_PRINTLN("WiFi stop requested");
}

// ============================================================================
// Non-blocking startup handler
// ============================================================================
void PillBoxWebServer::handleStarting() {
    uint32_t elapsed = millis() - stepTimestamp;

    switch (startupStep) {
        case 0:
            // Step 0: Set WiFi mode
            WiFi.mode(WIFI_AP);
            yield();
            startupStep   = 1;
            stepTimestamp  = millis();
            DEBUG_PRINTLN("WiFi: mode set to AP");
            break;

        case 1:
            // Wait for mode switch to stabilize
            if (elapsed < STEP_DELAY_MS) return;

            // Step 1: Start soft AP
            if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD,
                             WIFI_AP_CHANNEL, 0, WIFI_MAX_CONNECTIONS)) {
                DEBUG_PRINTLN("ERROR: WiFi.softAP failed — aborting");
                WiFi.mode(WIFI_OFF);
                wifiState = WIFI_ST_OFF;
                return;
            }
            yield();
            startupStep   = 2;
            stepTimestamp  = millis();
            DEBUG_PRINTF("WiFi: AP started, SSID=%s\n", WIFI_AP_SSID);
            break;

        case 2:
            // Wait for AP to be fully ready
            if (elapsed < STEP_DELAY_MS) return;

            // Step 2: Register routes + start server (once only)
            if (!routesConfigured) {
                setupRoutes();
                routesConfigured = true;
                DEBUG_PRINTLN("WiFi: routes registered");
            }
            if (!serverStarted) {
                server.begin();
                serverStarted = true;
                DEBUG_PRINTLN("WiFi: HTTP server started");
            }

            DEBUG_PRINTF("WiFi: RUNNING — IP %s\n",
                         WiFi.softAPIP().toString().c_str());
            wifiState = WIFI_ST_RUNNING;
            break;
    }
}

// ============================================================================
// Non-blocking shutdown handler
// ============================================================================
void PillBoxWebServer::handleStopping() {
    uint32_t elapsed = millis() - stepTimestamp;

    switch (shutdownStep) {
        case 0:
            // Step 0: Disconnect all clients + disable AP
            WiFi.softAPdisconnect(true);
            yield();
            shutdownStep  = 1;
            stepTimestamp  = millis();
            DEBUG_PRINTLN("WiFi: AP disconnected");
            break;

        case 1:
            // Wait for disconnect to finish
            if (elapsed < STEP_DELAY_MS) return;

            // Step 1: Turn radio off to save power
            WiFi.mode(WIFI_OFF);
            yield();
            shutdownStep  = 2;
            stepTimestamp  = millis();
            DEBUG_PRINTLN("WiFi: radio OFF");
            break;

        case 2:
            // Final settle
            if (elapsed < 50) return;

            wifiState = WIFI_ST_OFF;
            DEBUG_PRINTLN("WiFi: state → OFF");
            break;
    }
}

// ============================================================================
// Accessors
// ============================================================================
String PillBoxWebServer::getIPAddress() const {
    if (wifiState == WIFI_ST_RUNNING) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

uint8_t PillBoxWebServer::getConnectedClients() const {
    if (wifiState == WIFI_ST_RUNNING) {
        return WiFi.softAPgetStationNum();
    }
    return 0;
}

void PillBoxWebServer::setTimeUnlockCallback(void (*callback)(bool)) {
    timeUnlockCallback = callback;
}

void PillBoxWebServer::setupRoutes() {
    // Default headers for efficient delivery
    DefaultHeaders::Instance().addHeader("Connection", "close");
    
    // Serve index.html — direct SPIFFS response (non-blocking)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/index.html", "text/html");
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    });
    
    // Serve static files (CSS, JS, icons) with long-term caching
    server.serveStatic("/", SPIFFS, "/").setCacheControl("max-age=86400");
    
    // Handle CORS preflight — immediate 204 response
    server.on("/*", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(204);
        addCorsHeaders(response);
        request->send(response);
    });
    
    // GET /api/status
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    
    // GET /api/doses
    server.on("/api/doses", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetDoses(request);
    });
    
    // POST /api/time
    server.on("/api/time", HTTP_POST, 
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSetTime(request, data, len);
        }
    );
    
    // POST /api/date
    server.on("/api/date", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSetDate(request, data, len);
        }
    );
    
    // POST /api/doses
    server.on("/api/doses", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSetDoses(request, data, len);
        }
    );
    
    // POST /api/dose (add single dose)
    server.on("/api/dose", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleAddDose(request, data, len);
        }
    );
    
    // DELETE /api/dose
    server.on("/api/dose", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        handleDeleteDose(request);
    });
    
    // POST /api/alarm
    server.on("/api/alarm", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleSetAlarm(request, data, len);
        }
    );
    
    // POST /api/unlock-time
    server.on("/api/unlock-time", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        NULL,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            handleUnlockTime(request, data, len);
        }
    );
    
    // GET /api/logs
    server.on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetLogs(request);
    });
    
    // 404 handler
    server.onNotFound([this](AsyncWebServerRequest* request) {
        sendError(request, 404, "Not Found");
    });
}

void PillBoxWebServer::handleGetStatus(AsyncWebServerRequest* request) {
    StaticJsonDocument<384> doc;
    
    // Current time
    Time12H currentTime = timeManager->getCurrentTime();
    doc["time"]["hour"] = currentTime.hour;
    doc["time"]["minute"] = currentTime.minute;
    doc["time"]["isPM"] = currentTime.isPM;
    
    // Current date
    uint8_t day, month;
    uint16_t year;
    timeManager->getDate(day, month, year);
    doc["date"]["day"] = day;
    doc["date"]["month"] = month;
    doc["date"]["year"] = year;
    
    // Doses
    doc["doseCount"] = doseManager->getDoseCount();
    doc["dosesTaken"] = doseManager->getDosesTakenCount();
    
    // Next dose
    int16_t minutesToNext = doseManager->getMinutesUntilNextDose(*timeManager);
    doc["minutesToNextDose"] = minutesToNext;
    
    // Alarm status
    doc["alarmEnabled"] = alarmController->isEnabled();
    doc["alarmActive"] = alarmController->isActive();
    doc["snoozed"] = alarmController->isSnoozed();
    
    // Time edit unlock status
    doc["timeEditUnlocked"] = timeEditUnlocked;
    
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, 200, response);
}

void PillBoxWebServer::handleGetDoses(AsyncWebServerRequest* request) {
    StaticJsonDocument<768> doc;
    JsonArray doses = doc.createNestedArray("doses");
    
    for (uint8_t i = 0; i < doseManager->getDoseCount(); i++) {
        Dose* dose = doseManager->getDose(i);
        if (dose) {
            JsonObject doseObj = doses.createNestedObject();
            doseObj["id"] = i;
            doseObj["hour"] = dose->time.hour;
            doseObj["minute"] = dose->time.minute;
            doseObj["isPM"] = dose->time.isPM;
            doseObj["enabled"] = dose->enabled;
            doseObj["taken"] = dose->taken;
        }
    }
    
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, 200, response);
}

void PillBoxWebServer::handleSetTime(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    if (!timeEditUnlocked) {
        sendError(request, 403, "Time editing is locked");
        return;
    }
    
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendError(request, 400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("hour") || !doc.containsKey("minute") || !doc.containsKey("isPM")) {
        sendError(request, 400, "Missing required fields");
        return;
    }
    
    Time12H newTime;
    newTime.hour = doc["hour"];
    newTime.minute = doc["minute"];
    newTime.isPM = doc["isPM"];
    
    if (!TimeManager::isValidTime(newTime)) {
        sendError(request, 400, "Invalid time values");
        return;
    }
    
    timeManager->setTime(newTime);
    
    sendJsonResponse(request, 200, "{\"success\":true}");
}

void PillBoxWebServer::handleSetDate(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    if (!timeEditUnlocked) {
        sendError(request, 403, "Time editing is locked");
        return;
    }
    
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendError(request, 400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("day") || !doc.containsKey("month") || !doc.containsKey("year")) {
        sendError(request, 400, "Missing required fields");
        return;
    }
    
    uint8_t day = doc["day"];
    uint8_t month = doc["month"];
    uint16_t year = doc["year"];
    
    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 2000 || year > 2099) {
        sendError(request, 400, "Invalid date values");
        return;
    }
    
    timeManager->setDate(day, month, year);
    
    sendJsonResponse(request, 200, "{\"success\":true}");
}

void PillBoxWebServer::handleSetDoses(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendError(request, 400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("doses") || !doc["doses"].is<JsonArray>()) {
        sendError(request, 400, "Missing doses array");
        return;
    }
    
    // Clear existing doses
    doseManager->clearAllDoses();
    
    JsonArray doses = doc["doses"].as<JsonArray>();
    for (JsonObject doseObj : doses) {
        Time12H time;
        time.hour = doseObj["hour"];
        time.minute = doseObj["minute"];
        time.isPM = doseObj["isPM"];
        
        if (TimeManager::isValidTime(time)) {
            doseManager->addDose(time);
        }
    }
    
    // Save to storage
    if (storage) {
        doseManager->saveToStorage(*storage);
    }
    
    sendJsonResponse(request, 200, "{\"success\":true}");
}

void PillBoxWebServer::handleAddDose(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendError(request, 400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("hour") || !doc.containsKey("minute") || !doc.containsKey("isPM")) {
        sendError(request, 400, "Missing required fields");
        return;
    }
    
    Time12H time;
    time.hour = doc["hour"];
    time.minute = doc["minute"];
    time.isPM = doc["isPM"];
    
    if (!TimeManager::isValidTime(time)) {
        sendError(request, 400, "Invalid time values");
        return;
    }
    
    if (!doseManager->addDose(time)) {
        sendError(request, 400, "Cannot add dose (max reached or time conflict)");
        return;
    }
    
    // Save to storage
    if (storage) {
        doseManager->saveToStorage(*storage);
    }
    
    sendJsonResponse(request, 200, "{\"success\":true}");
}

void PillBoxWebServer::handleDeleteDose(AsyncWebServerRequest* request) {
    if (!request->hasParam("id")) {
        sendError(request, 400, "Missing id parameter");
        return;
    }
    
    uint8_t id = request->getParam("id")->value().toInt();
    
    if (!doseManager->removeDose(id)) {
        sendError(request, 400, "Invalid dose id");
        return;
    }
    
    // Save to storage
    if (storage) {
        doseManager->saveToStorage(*storage);
    }
    
    sendJsonResponse(request, 200, "{\"success\":true}");
}

void PillBoxWebServer::handleSetAlarm(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendError(request, 400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("enabled")) {
        sendError(request, 400, "Missing enabled field");
        return;
    }
    
    bool enabled = doc["enabled"];
    alarmController->setEnabled(enabled);
    
    sendJsonResponse(request, 200, "{\"success\":true}");
}

void PillBoxWebServer::handleUnlockTime(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        sendError(request, 400, "Invalid JSON");
        return;
    }
    
    if (!doc.containsKey("unlock")) {
        sendError(request, 400, "Missing unlock field");
        return;
    }
    
    timeEditUnlocked = doc["unlock"];
    
    if (timeUnlockCallback) {
        timeUnlockCallback(timeEditUnlocked);
    }
    
    DEBUG_PRINTF("Time editing %s\n", timeEditUnlocked ? "UNLOCKED" : "LOCKED");
    
    sendJsonResponse(request, 200, "{\"success\":true}");
}

void PillBoxWebServer::handleGetLogs(AsyncWebServerRequest* request) {
    // TODO: Implement log retrieval from storage
    StaticJsonDocument<256> doc;
    JsonArray logs = doc.createNestedArray("logs");
    
    // Placeholder - would be populated from storage
    doc["totalOpenings"] = 0;
    
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, 200, response);
}

void PillBoxWebServer::addCorsHeaders(AsyncWebServerResponse* response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void PillBoxWebServer::sendJsonResponse(AsyncWebServerRequest* request, int code, const String& json) {
    AsyncWebServerResponse* response = request->beginResponse(code, "application/json", json);
    addCorsHeaders(response);
    request->send(response);
}

void PillBoxWebServer::sendError(AsyncWebServerRequest* request, int code, const String& message) {
    StaticJsonDocument<128> doc;
    doc["error"] = message;
    
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, code, response);
}
