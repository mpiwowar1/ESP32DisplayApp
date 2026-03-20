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
#include "handlebutton.h"
#include "bootanimation.h"
#include "menu.h"






// ========== FUNCTION PROTOTYPES ==========
void setupSPIFFS();
void displayBootAnimation();



void doClose() { loadAndDisplayStored(); }


MenuItem mainMenu[] = {
    { "CLOSE", doClose },
    { "WIFI",  doWifi  },
    { "SETUP", doSetup },
};


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


  handleButton(doWifi,[](){
    showMenu(mainMenu,3);
  });

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
