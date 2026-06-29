/**
 * @file globals.cpp
 * @brief Definitions for the shared global variables declared in globals.h.
 */

#include "globals.h"

const char* ssid = "SetUPMatrix";

MatrixPanel_I2S_DMA* dma_display = nullptr;
WebServer server(80);
AnimatedGIF gif;
String currentType   = "none";
bool isPlayingGif    = false;
uint8_t* gifData     = nullptr;
size_t gifSize       = 0;
uint8_t brightness   = 90;
float gifSpeed       = 1.0f;
