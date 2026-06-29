/**
 * @file network.h
 * @brief WiFi connection, AP mode, and web server setup.
 */

#pragma once
#include "globals.h"
#include "memory.h"
#include "api.h"
#include <ESPmDNS.h>

/** @brief Register all HTTP routes and start the web server. */
void setupWebServer();

/** @brief Start the ESP32 as a WiFi access point for setup. */
void setupWiFi();

/** @brief Try to connect to the WiFi saved in /wifi.txt. Returns true on success. */
bool ConnectToStoredWiFi();

/** @brief Log WiFi events (connect, disconnect, etc.) to serial. */
void WiFiEvent(WiFiEvent_t event);