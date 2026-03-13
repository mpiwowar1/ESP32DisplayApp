#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include <SPIFFS.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// =========SETTING BRIGHTNESS =========
void PostBrightness(MatrixPanel_I2S_DMA *dma_display,uint8_t *brightness,WebServer *server);