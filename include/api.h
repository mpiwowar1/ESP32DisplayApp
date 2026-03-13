#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include <SPIFFS.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "globals.h"
#include "memory.h"

// =========SETTING BRIGHTNESS =========
void PostBrightness();

// =========GETTING BRIGHTNESS =========
void GetBrightness();

// ========== HANDLE CHANGING WIFI CREDENTIALS ==========
void PostCredentials();

// ========== HANDLES WEBSITE ==========
void GetWebsite();

// ========== HANDLE DELETE ==========
void DeleteStored();

// ========== HANDLE STATUS ==========
void GetStatus();

// =========GETTING Speed =========
void GetSpeed();

// ========== HANDLE CHANGING GifSpeed ==========
void PostSpeed();

// ========== HANDLE CHANGING MEDIA(GIF/IMAGE) ==========
void PostMedia();

