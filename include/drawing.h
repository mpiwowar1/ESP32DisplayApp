/**
 * @file drawing.h
 * @brief GIF frame rendering callback for the LED matrix.
 */

#pragma once
#include "globals.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>

/**
 * @brief Callback passed to AnimatedGIF to draw each frame line.
 * @param pDraw Frame data provided by the GIF decoder.
 */
void GIFDraw(GIFDRAW* pDraw);