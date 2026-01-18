/**
 * @file PillBoxWebServer.h
 * @brief WiFi Access Point and Web Server for remote configuration
 * @project Smart Pill Box with ESP32
 * @version 1.0
 */

#ifndef PILLBOX_WEBSERVER_H
#define PILLBOX_WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "config.h"

// Forward declarations
class TimeManager;
class DoseManager;
class AlarmController;
class Storage;

class PillBoxWebServer {
public:
    /**
     * @brief Constructor
     */
    PillBoxWebServer();
    
    /**
     * @brief Initialize web server
     * @param timeManager Reference to TimeManager
     * @param doseManager Reference to DoseManager
     * @param alarmController Reference to AlarmController
     * @param storage Reference to Storage
     */
    void begin(TimeManager* timeManager, DoseManager* doseManager, 
               AlarmController* alarmController, Storage* storage);
    
    /**
     * @brief Start WiFi Access Point and web server
     * @return true if started successfully
     */
    bool start();
    
    /**
     * @brief Stop WiFi and web server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool isRunning() const { return running; }
    
    /**
     * @brief Get IP address of the access point
     * @return IP address as string
     */
    String getIPAddress() const;
    
    /**
     * @brief Get number of connected clients
     * @return Number of connected clients
     */
    uint8_t getConnectedClients() const;
    
    /**
     * @brief Set callback for time unlock request
     * @param callback Function to call when unlock requested
     */
    void setTimeUnlockCallback(void (*callback)(bool));

private:
    AsyncWebServer server;
    TimeManager* timeManager;
    DoseManager* doseManager;
    AlarmController* alarmController;
    Storage* storage;
    bool running;
    bool timeEditUnlocked;
    void (*timeUnlockCallback)(bool);
    
    /**
     * @brief Setup API routes
     */
    void setupRoutes();
    
    /**
     * @brief Handle GET /api/status
     */
    void handleGetStatus(AsyncWebServerRequest* request);
    
    /**
     * @brief Handle POST /api/time
     */
    void handleSetTime(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    
    /**
     * @brief Handle POST /api/date
     */
    void handleSetDate(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    
    /**
     * @brief Handle GET /api/doses
     */
    void handleGetDoses(AsyncWebServerRequest* request);
    
    /**
     * @brief Handle POST /api/doses
     */
    void handleSetDoses(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    
    /**
     * @brief Handle POST /api/dose
     */
    void handleAddDose(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    
    /**
     * @brief Handle DELETE /api/dose
     */
    void handleDeleteDose(AsyncWebServerRequest* request);
    
    /**
     * @brief Handle POST /api/alarm
     */
    void handleSetAlarm(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    
    /**
     * @brief Handle POST /api/unlock-time
     */
    void handleUnlockTime(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    
    /**
     * @brief Handle GET /api/logs
     */
    void handleGetLogs(AsyncWebServerRequest* request);
    
    /**
     * @brief Add CORS headers to response
     */
    void addCorsHeaders(AsyncWebServerResponse* response);
    
    /**
     * @brief Send JSON response
     */
    void sendJsonResponse(AsyncWebServerRequest* request, int code, const String& json);
    
    /**
     * @brief Send error response
     */
    void sendError(AsyncWebServerRequest* request, int code, const String& message);
};

#endif // PILLBOX_WEBSERVER_H
