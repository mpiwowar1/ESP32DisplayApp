/**
 * @file api.h
 * @brief HTTP endpoint handlers for the web server.
 */

#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include <SPIFFS.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "globals.h"
#include "memory.h"

/** @brief POST /postbrightness - save and apply brightness. */
void PostBrightness();

/** @brief GET /getbrightness - return current brightness as JSON. */
void GetBrightness();

/** @brief POST /cred - save new WiFi SSID and password to flash. */
void PostCredentials();

/** @brief GET / - serve the built-in HTML control page. */
void GetWebsite();

/** @brief POST /delete - erase stored media and clear the display. */
void DeleteStored();

/** @brief GET /status - return storage info as JSON. */
void GetStatus();

/** @brief GET /getspeed - return current GIF speed as JSON. */
void GetSpeed();

/** @brief POST /postspeed - save and apply a new GIF playback speed. */
void PostSpeed();

/** @brief POST /upload - receive an image or GIF file in chunks. */
void PostMedia();
