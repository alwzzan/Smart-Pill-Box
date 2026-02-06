/**
 * @file PillBoxWebServer.h
 * @brief WiFi Access Point and Web Server for remote configuration
 * @project Smart Pill Box with ESP32
 * @version 2.0 — State-machine based WiFi management
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

/**
 * @brief WiFi state machine states
 */
enum WiFiState {
    WIFI_ST_OFF = 0,      // WiFi is completely off
    WIFI_ST_STARTING,     // WiFi AP is being initialized (non-blocking)
    WIFI_ST_RUNNING,      // WiFi AP is up and server is accepting connections
    WIFI_ST_STOPPING      // WiFi AP is shutting down (non-blocking)
};

class PillBoxWebServer {
public:
    PillBoxWebServer();
    
    /**
     * @brief Initialize dependencies (call once in setup)
     */
    void begin(TimeManager* timeManager, DoseManager* doseManager, 
               AlarmController* alarmController, Storage* storage);
    
    /**
     * @brief Non-blocking state machine tick (call every loop iteration)
     */
    void update();
    
    /**
     * @brief Request WiFi start (non-blocking, triggers state transition)
     */
    void requestStart();
    
    /**
     * @brief Request WiFi stop (non-blocking, triggers state transition)
     */
    void requestStop();
    
    /**
     * @brief Check if WiFi AP + server are fully running
     */
    bool isRunning() const { return wifiState == WIFI_ST_RUNNING; }
    
    /**
     * @brief Check if WiFi is completely off
     */
    bool isOff() const { return wifiState == WIFI_ST_OFF; }
    
    /**
     * @brief Check if WiFi is in a transition state
     */
    bool isBusy() const { return wifiState == WIFI_ST_STARTING || wifiState == WIFI_ST_STOPPING; }
    
    /**
     * @brief Get current WiFi state
     */
    WiFiState getState() const { return wifiState; }
    
    /**
     * @brief Get IP address of the access point
     */
    String getIPAddress() const;
    
    /**
     * @brief Get number of connected clients
     */
    uint8_t getConnectedClients() const;
    
    /**
     * @brief Set callback for time unlock request
     */
    void setTimeUnlockCallback(void (*callback)(bool));

private:
    // ---- State machine ----
    WiFiState wifiState;
    uint8_t startupStep;        // Sub-step within STARTING state
    uint8_t shutdownStep;       // Sub-step within STOPPING state
    uint32_t stepTimestamp;     // millis() at last sub-step transition
    
    // ---- Initialization guards ----
    bool spiffsMounted;
    bool routesConfigured;
    bool serverStarted;         // server.begin() called exactly once
    
    // ---- Server & dependencies ----
    AsyncWebServer server;
    TimeManager* timeManager;
    DoseManager* doseManager;
    AlarmController* alarmController;
    Storage* storage;
    bool timeEditUnlocked;
    void (*timeUnlockCallback)(bool);
    
    // ---- State handlers ----
    void handleStarting();
    void handleStopping();
    
    // ---- Route setup (once) ----
    void setupRoutes();
    
    // ---- HTTP handlers ----
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleSetTime(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleSetDate(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleGetDoses(AsyncWebServerRequest* request);
    void handleSetDoses(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleAddDose(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleDeleteDose(AsyncWebServerRequest* request);
    void handleSetAlarm(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleUnlockTime(AsyncWebServerRequest* request, uint8_t* data, size_t len);
    void handleGetLogs(AsyncWebServerRequest* request);
    
    // ---- Response helpers ----
    void addCorsHeaders(AsyncWebServerResponse* response);
    void sendJsonResponse(AsyncWebServerRequest* request, int code, const String& json);
    void sendError(AsyncWebServerRequest* request, int code, const String& message);
};

#endif // PILLBOX_WEBSERVER_H
