/**
 * @file memory.h
 * @brief Reading and displaying images/GIFs stored in SPIFFS.
 */

#pragma once
#include <SPIFFS.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "globals.h"
#include "drawing.h"

/** @brief Read a stored image from flash and draw it on the matrix. */
void displayStoredImage();

/** @brief Read the saved GIF speed from flash. Returns 1.0 if missing. */
float ReadGifSpeed();

/** @brief Read saved brightness from flash. Returns 90 if missing. */
uint8_t ReadBrightness();

/** @brief Load a GIF from flash into RAM and start playing it. */
void playStoredGif();

/** @brief Check what content type is stored and display it. */
void loadAndDisplayStored();

/** @brief Mount SPIFFS and print storage info to serial. */
void setupSPIFFS();