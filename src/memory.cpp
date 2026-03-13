#include "memory.h"

void displayStoredImage() {
  if (!SPIFFS.exists(STORED_FILE)) {
    Serial.println("No image file found");
    return;
  }

  File file = SPIFFS.open(STORED_FILE, "r");
  if (!file) {
    Serial.println("Failed to open image file");
    return;
  }

  size_t size = file.size();
  Serial.printf("Loading image: %d bytes\n", size);

  dma_display->clearScreen();

  if (size == 8192) {
    // RGB565 format
    for (int y = 0; y < 64; y++) {
      for (int x = 0; x < 64; x++) {
        uint8_t byte1 = file.read();
        uint8_t byte2 = file.read();
        uint16_t color = (byte1 << 8) | byte2;
        dma_display->drawPixel(x, y, color);
      }
    }
  } else if (size == 12288) {
    // RGB888 format
    for (int y = 0; y < 64; y++) {
      for (int x = 0; x < 64; x++) {
        uint8_t r = file.read();
        uint8_t g = file.read();
        uint8_t b = file.read();
        dma_display->drawPixelRGB888(x, y, r, g, b);
      }
    }
  }

  file.close();
  Serial.println("✓ Image displayed from storage");
}

float ReadGifSpeed() {
    if (!SPIFFS.exists(STORED_SPEED)) return 1.0f;
    File file = SPIFFS.open(STORED_SPEED, "r");
    if (!file) return 1.0f;
    String line = file.readStringUntil('\n');
    file.close();
    line.trim();
    if (!line.startsWith("speed=")) return 1.0f;
    return line.substring(6).toFloat();
}

uint8_t ReadBrightness()
{
  if (!SPIFFS.exists(STORED_BRIGHTNESS)) {
    Serial.println("Brightness not saved defaulting to 90");
    return 90;
  }

  File file = SPIFFS.open(STORED_BRIGHTNESS, "r");
  if (!file) {
    Serial.println("Brightness file cannot be opened");
    return 90;
  }

  String brightnessread = file.readStringUntil('\n');
  file.close();

  brightnessread.trim();

  if (!brightnessread.startsWith("brightness=")) {
    Serial.println("Faulty brightness file");
    return 90;
  }

  brightnessread = brightnessread.substring(11);


  uint8_t tempbrightness = brightnessread.toInt();

  Serial.print("Returning brightness: ");
  Serial.println(brightnessread);
  return tempbrightness;
}

void playStoredGif() {
  if (!SPIFFS.exists(STORED_FILE)) {
    Serial.println("No GIF file found");
    return;
  }

  File file = SPIFFS.open(STORED_FILE, "r");
  if (!file) {
    Serial.println("Failed to open GIF file");
    return;
  }

  gifSize = file.size();
  Serial.printf("Loading GIF: %d bytes\n", gifSize);

  // Free previous GIF data
  if (gifData) {
    free(gifData);
  }

  // Allocate memory for GIF
  gifData = (uint8_t*)malloc(gifSize);
  if (!gifData) {
    Serial.println("Failed to allocate memory for GIF");
    file.close();
    return;
  }

  // Read GIF into memory
  file.read(gifData, gifSize);
  file.close();

  // Open and play GIF
  if (gif.open(gifData, gifSize, GIFDraw)) {
    Serial.println("✓ GIF playing from storage");
    isPlayingGif = true;
  } else {
    Serial.println("Failed to open GIF");
    free(gifData);
    gifData = nullptr;
  }
}

void loadAndDisplayStored() {
  // Check if type file exists
  if (!SPIFFS.exists(TYPE_FILE)) {
    Serial.println("No stored content found");
    return;
  }

  // Read type
  File typeFile = SPIFFS.open(TYPE_FILE, "r");
  if (typeFile) {
    currentType = typeFile.readString();
    currentType.trim();
    typeFile.close();
    Serial.printf("Found stored content: %s\n", currentType.c_str());
  }

  // Load and display based on type
  if (currentType == "image") {
    displayStoredImage();
  } else if (currentType == "gif") {
    playStoredGif();
  }
}
