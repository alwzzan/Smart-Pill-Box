/**
 * @file PillBoxWebServer.cpp
 * @brief WiFi Access Point and Web Server implementation
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#include "PillBoxWebServer.h"
#include "TimeManager.h"
#include "DoseManager.h"
#include "AlarmController.h"
#include "Storage.h"

PillBoxWebServer::PillBoxWebServer() : server(WEB_SERVER_PORT) {
    timeManager = nullptr;
    doseManager = nullptr;
    alarmController = nullptr;
    storage = nullptr;
    running = false;
    timeEditUnlocked = false;
    timeUnlockCallback = nullptr;
}

void PillBoxWebServer::begin(TimeManager* tm, DoseManager* dm, 
                              AlarmController* ac, Storage* st) {
    timeManager = tm;
    doseManager = dm;
    alarmController = ac;
    storage = st;
    
    // Initialize SPIFFS for serving HTML files
    if (!SPIFFS.begin(true)) {
        DEBUG_PRINTLN("ERROR: SPIFFS mount failed");
    }
    
    DEBUG_PRINTLN("PillBoxWebServer initialized");
}

bool PillBoxWebServer::start() {
    if (running) {
        return true;
    }
    
    // Configure WiFi Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_MAX_CONNECTIONS);
    
    IPAddress IP = WiFi.softAPIP();
    DEBUG_PRINTF("WiFi AP started. SSID: %s\n", WIFI_AP_SSID);
    DEBUG_PRINTF("IP Address: %s\n", IP.toString().c_str());
    
    // Setup routes
    setupRoutes();
    
    // Start server
    server.begin();
    running = true;
    
    DEBUG_PRINTLN("Web server started");
    return true;
}

void PillBoxWebServer::stop() {
    if (!running) {
        return;
    }
    
    server.end();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    
    running = false;
    timeEditUnlocked = false;
    
    DEBUG_PRINTLN("Web server stopped");
}

String PillBoxWebServer::getIPAddress() const {
    if (running) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

uint8_t PillBoxWebServer::getConnectedClients() const {
    if (running) {
        return WiFi.softAPgetStationNum();
    }
    return 0;
}

void PillBoxWebServer::setTimeUnlockCallback(void (*callback)(bool)) {
    timeUnlockCallback = callback;
}

void PillBoxWebServer::setupRoutes() {
    // Serve index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });
    
    // Serve static files
    server.serveStatic("/", SPIFFS, "/");
    
    // Handle CORS preflight
    server.on("/*", HTTP_OPTIONS, [this](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200);
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
    StaticJsonDocument<512> doc;
    
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
    StaticJsonDocument<1024> doc;
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
    StaticJsonDocument<1024> doc;
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
