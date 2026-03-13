#pragma once
#include <SPIFFS.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "globals.h"
#include "drawing.h"

// ========== DISPLAY STORED IMAGE ==========
void displayStoredImage();

// ========== RETURN GIF SPEED ==========
float ReadGifSpeed();

// ==============READING BRIGHTNESS FROM FILE ==============
uint8_t ReadBrightness();

// ========== PLAY STORED GIF ==========
void playStoredGif();

// ========== LOAD AND DISPLAY STORED CONTENT ==========
void loadAndDisplayStored();