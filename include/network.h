#include "globals.h"
#include "memory.h"
#include "api.h"

// ========== WEB SERVER SETUP ==========
void setupWebServer();

// ========== WIFI SETUP ==========
void setupWiFi();

// ====== CONNECT TO WIFI ======
bool ConnectToStoredWiFi();

// ====== HANDLE WIFI EVENTS ======
void WiFiEvent(WiFiEvent_t event);