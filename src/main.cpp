/*
 * ESP32 LED Matrix - Persistent Storage (AP Mode)
 * 
 * Features:
 * - Creates its own WiFi (LED_Matrix_64x64)
 * - Saves images/GIFs to flash storage (survives power off!)
 * - Auto-plays last content on boot
 * - Phone sends once, it stays forever
 * - Supports both static images and animated GIFs
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "api.h"
#include "globals.h"
#include "memory.h"
#include "drawing.h"
#include "network.h"






// ========== FUNCTION PROTOTYPES ==========
void setupSPIFFS();
void displayBootAnimation();
void HandleLongButtonPress();
void HandleShortButtonPress();



// ========== SETUP ==========
void setup() {
  Serial.begin(115200);

  delay(500);

  esp_log_level_set("wifi", ESP_LOG_VERBOSE);
  esp_log_level_set("dhcps", ESP_LOG_VERBOSE);
  
  WiFi.onEvent(WiFiEvent);



  Serial.println("\n\n=====================================");
  Serial.println("LED Matrix - Persistent Storage");
  Serial.println("=====================================\n");

  // Initialize SPIFFS (flash storage)
  setupSPIFFS();

  //Setting Up button
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // internal pull-up


  // Configure matrix
  HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, CHAIN_LENGTH);
  mxconfig.gpio.r1 = R1_PIN;
  mxconfig.gpio.g1 = G1_PIN;
  mxconfig.gpio.b1 = B1_PIN;
  mxconfig.gpio.r2 = R2_PIN;
  mxconfig.gpio.g2 = G2_PIN;
  mxconfig.gpio.b2 = B2_PIN;
  mxconfig.gpio.a = A_PIN;
  mxconfig.gpio.b = B_PIN;
  mxconfig.gpio.c = C_PIN;
  mxconfig.gpio.d = D_PIN;
  mxconfig.gpio.e = E_PIN;
  mxconfig.gpio.lat = LAT_PIN;
  mxconfig.gpio.oe = OE_PIN;
  mxconfig.gpio.clk = CLK_PIN;


  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  brightness = ReadBrightness();

  dma_display->setBrightness8(brightness);
  gifSpeed = ReadGifSpeed();

  dma_display->clearScreen();

  Serial.println("✓ Matrix initialized");

  // Show boot animation
  displayBootAnimation();


  isPlayingGif = false;
  gif.close();

  dma_display->clearScreen();
  delay(200);


  if (ConnectToStoredWiFi()) {
      setupWebServer();
  }


  dma_display->clearScreen();
  dma_display->setTextColor(dma_display->color565(0, 255, 0));
  dma_display->setTextSize(1);
  dma_display->setCursor(2, 2);
  if(WiFi.status() != WL_CONNECTED){
    dma_display->print("WIFI NOT");
  }
  else{
    dma_display->print("WIFI");
  }
  dma_display->setCursor(2, 12);
  dma_display->print("CONNECTED");
  delay(2000);
  // Load and display stored content (if any)
  dma_display->clearScreen();
  loadAndDisplayStored();

  Serial.println("\n=====================================");
  Serial.println("Ready! Content persists after power off");
  Serial.println("=====================================\n");
}

// ========== MAIN LOOP ==========
void loop() {
  server.handleClient();


  static bool lastButtonState = HIGH;
  static bool buttonPressed = false;
  static unsigned long pressStartTime = 0;
  static bool longPressHandled = false;

  bool currentState = digitalRead(BUTTON_PIN);
  // Button just pressed
  if (lastButtonState == HIGH && currentState == LOW) {
    buttonPressed = true;
    pressStartTime = millis();
    longPressHandled = false;
  }

  // Button held down
  if (buttonPressed && currentState == LOW) {
    if (!longPressHandled &&
        millis() - pressStartTime >= LONG_PRESS_TIME) {
      longPressHandled = true;
      HandleLongButtonPress();
    }
  }

  // Button released
  if (lastButtonState == LOW && currentState == HIGH) {
    if (buttonPressed && !longPressHandled) {
      HandleShortButtonPress();
    }
    buttonPressed = false;
  }

  lastButtonState = currentState;


  if (isPlayingGif && gifData != nullptr) {
    if (gifSpeed <= 0.01f) {
        delay(50);
    } else {
        int frameDelayMs = 0;
        gif.playFrame(true, &frameDelayMs);
        if (gifSpeed < 1.0f && frameDelayMs > 0) {
            int extra = (int)(frameDelayMs * (1.0f / gifSpeed - 1.0f));
            delay(extra);
        }
    }
}

  delay(1);
}


// ======Handling button press(SETUP MODE) ===========
void HandleLongButtonPress() {
  Serial.println("Button pressed: entering SETUP mode");
  dma_display->clearScreen();
  dma_display->setTextColor(dma_display->color565(0, 255, 0));
  dma_display->setTextSize(1);
  dma_display->setCursor(2, 2);
  dma_display->print("STARTING");
  dma_display->setCursor(2, 12);
  dma_display->print("SETUP MODE");

  // Wait for button release before proceeding
  delay(50);
  while (digitalRead(BUTTON_PIN) == LOW) { delay(10); }
  delay(50);

  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(1000);


  isPlayingGif = false;
  gif.close();

  dma_display->clearScreen();
  delay(200);


  setupWiFi();
  setupWebServer();

  // Draw SETUP MODE indicator after setupWiFi() is done
  dma_display->clearScreen();
  dma_display->setTextColor(dma_display->color565(0, 255, 0));
  dma_display->setTextSize(1);
  dma_display->setCursor(2, 2);
  dma_display->print("SETUP MODE");
  dma_display->setCursor(2, 12);
  dma_display->print(WiFi.softAPSSID());
  dma_display->setCursor(7, 22);
  dma_display->print(WiFi.softAPIP().toString());
  dma_display->setCursor(16, 32);
  dma_display->print("Hold to erase Memory");

  Serial.println("SETUP mode active. Press button again to exit.");

  // Serve clients until button is pressed again
      bool lastButtonState  = HIGH;   // NOT static
    bool buttonPressed    = false;
    unsigned long pressStartTime = 0;
    bool longPressHandled = false;
  while (true) {
    server.handleClient();
    delay(1);

  bool currentState = digitalRead(BUTTON_PIN);

  // Button just pressed
  if (lastButtonState == HIGH && currentState == LOW) {
    buttonPressed = true;
    pressStartTime = millis();
    longPressHandled = false;
  }

  // Button held down
  if (buttonPressed && currentState == LOW) {
    if (!longPressHandled &&
        millis() - pressStartTime >= LONG_PRESS_TIME) {
      longPressHandled = true;
      dma_display->clearScreen();
      dma_display->setTextColor(dma_display->color565(0, 255, 0));
      dma_display->setTextSize(1);
      dma_display->setCursor(2, 2);
      dma_display->print("FORMATING...");
      SPIFFS.format();
      dma_display->clearScreen();
      dma_display->setTextColor(dma_display->color565(0, 255, 0));
      dma_display->setTextSize(1);
      dma_display->setCursor(2, 2);
      dma_display->print("MEMORY(SPIFFS)");
      dma_display->setCursor(2, 12);
      dma_display->print("FORMATED");
      delay(2000);


      dma_display->clearScreen();
      dma_display->setTextColor(dma_display->color565(0, 255, 0));
      dma_display->setTextSize(1);
      dma_display->setCursor(2, 2);
      dma_display->print("SETUP MODE");
      dma_display->setCursor(2, 12);
      dma_display->print(WiFi.softAPSSID());
      dma_display->setCursor(7, 22);
      dma_display->print(WiFi.softAPIP().toString());
      dma_display->setCursor(16, 32);
      dma_display->print("Hold to erase Memory");

    }
  }

  // Button released
  if (lastButtonState == LOW && currentState == HIGH) {
    if (buttonPressed && !longPressHandled) {
      break;
    }
    buttonPressed = false;
  }

  lastButtonState = currentState;

  }

  Serial.println("Exiting SETUP mode...");
  dma_display->clearScreen();
  dma_display->setTextColor(dma_display->color565(0, 255, 0));
  dma_display->setTextSize(1);
  dma_display->setCursor(2, 2);
  dma_display->print("EXITING &");
  dma_display->setCursor(2, 12);
  dma_display->print("CONNECTING");
  dma_display->setCursor(2, 22);
  dma_display->print("TO WIFI");

  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(500);

  if (ConnectToStoredWiFi()) {
    setupWebServer();
    Serial.println("STA mode restored.");
    dma_display->clearScreen();
    dma_display->setTextColor(dma_display->color565(0, 255, 0));
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->print("CONNECTED");
  } else {
    Serial.println("Could not connect — check saved credentials.");
    dma_display->clearScreen();
    dma_display->setTextColor(dma_display->color565(255, 0, 0));
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->print("WiFi FAIL");
    dma_display->setCursor(2, 14);
    dma_display->print("Check cred");
  }
  delay(2000);
  dma_display->clearScreen();
  loadAndDisplayStored();
}
// ========= RECONNECT WIFI IF NOT CONNECTED PLUS INFO ===========
void HandleShortButtonPress(){
  dma_display->clearScreen();
  dma_display->setTextColor(dma_display->color565(255, 0, 0));
  dma_display->setTextSize(1);
  dma_display->setCursor(2, 2);
  dma_display->print("WiFi CheckUp");

  if(WiFi.status() != WL_CONNECTED){
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(500);

  if (ConnectToStoredWiFi()) {
    setupWebServer();
    Serial.println("STA mode connected");
  } else {
    Serial.println("Could not connect");
    dma_display->clearScreen();
    dma_display->setTextColor(dma_display->color565(255, 0, 0));
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->print("WiFi FAILED");
  }
}
if(WiFi.status() == WL_CONNECTED){
    dma_display->clearScreen();
    dma_display->setTextColor(dma_display->color565(255, 0, 0));
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->print("Connected");
    dma_display->setCursor(2, 12);
    dma_display->print(WiFi.SSID());
    dma_display->setCursor(2, 22);
    dma_display->print(WiFi.localIP().toString());
}

while (true) {
  server.handleClient();
  delay(1);

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      while (digitalRead(BUTTON_PIN) == LOW) { delay(10); }
      delay(50);
      break;
    }
  }
}
dma_display->clearScreen();
loadAndDisplayStored();

}

// ========== SPIFFS SETUP ==========
void setupSPIFFS() {
  Serial.println("Initializing flash storage (SPIFFS)...");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("✗ SPIFFS mount failed!");
    return;
  }

  Serial.println("✓ SPIFFS mounted");
  
  // Show storage info
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  Serial.printf("  Total: %d bytes\n", totalBytes);
  Serial.printf("  Used: %d bytes\n", usedBytes);
  Serial.printf("  Free: %d bytes\n", totalBytes - usedBytes);
}

// ========== BOOT ANIMATION ==========
void displayBootAnimation() {
    Serial.println("Boot animation...");

    const int   CX          = 32;
    const int   CY          = 32;
    const int   FRAMES      = 35;       // total frames – keep it snappy
    const float MAX_DIST    = 46.0f;    // rough radius to screen corner
    const float RING_WIDTH  = 10.0f;    // how thick the leading ring is
    const float SPIRAL_WRAP = 0.18f;    // how tightly the arms curl
    const float SPIN_SPEED  = 0.30f;    // rotation per frame (radians)

    for (int frame = 0; frame < FRAMES; frame++) {
        float progress  = (float)frame / (FRAMES - 1);          // 0 → 1
        float ringFront = MAX_DIST * (1.0f - progress);         // shrinks edge→centre

        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                float dx   = x - CX;
                float dy   = y - CY;
                float dist = sqrtf(dx * dx + dy * dy);
                float ang  = atan2f(dy, dx);

                // Spiral phase: pixels further out are rotated more,
                // plus the whole vortex spins as frames advance.
                float phase = ang + dist * SPIRAL_WRAP - frame * SPIN_SPEED;

                // Intensity: bright at the leading ring, fades behind it.
                float ringDelta = dist - ringFront;             // >0 = outside ring (not yet lit)
                float intensity;
                if (ringDelta > 0.0f) {
                    intensity = 0.0f;                           // ahead of the wave = dark
                } else {
                    intensity = expf(ringDelta / RING_WIDTH);   // exponential fade behind
                }

                // Add a soft swirl shimmer on top of the base intensity.
                float shimmer = 0.5f + 0.5f * sinf(phase * 3.0f);
                float bright  = intensity * (0.6f + 0.4f * shimmer);

                // Toned, cool palette: teal → blue → purple hues.
                // Keeping max channel values low (~180) avoids eye-searing brightness.
                uint8_t r = (uint8_t)(bright * (50  + 40  * sinf(phase + 4.0f)));
                uint8_t g = (uint8_t)(bright * (120 + 60  * sinf(phase + 2.0f)));
                uint8_t b = (uint8_t)(bright * (160 + 60  * sinf(phase)));

                dma_display->drawPixelRGB888(x, y, r, g, b);
            }
        }

        delay(20);  // ~50 fps cap; total ~700 ms for the full animation
    }

    // Fade out quickly rather than hard-cutting to black
    for (int fade = 8; fade >= 0; fade--) {
        float f = (float)fade / 8.0f;
        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                float dx = x - CX, dy = y - CY;
                float dist = sqrtf(dx*dx + dy*dy);
                float ang  = atan2f(dy, dx);
                float phase = ang + dist * SPIRAL_WRAP;
                uint8_t r = (uint8_t)(f * (50  + 40  * sinf(phase + 4.0f)));
                uint8_t g = (uint8_t)(f * (120 + 60  * sinf(phase + 2.0f)));
                uint8_t b = (uint8_t)(f * (160 + 60  * sinf(phase)));
                dma_display->drawPixelRGB888(x, y, r, g, b);
            }
        }
        delay(18);
    }

    dma_display->clearScreen();
}