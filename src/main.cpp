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


// ========== WIFI CONFIGURATION ==========
const char* ssid = "SetUPMatrix";


// ========== PIN CONFIGURATION ==========
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 19
#define B_PIN 18
#define C_PIN 5
#define D_PIN 17
#define E_PIN 16
#define LAT_PIN 22
#define OE_PIN 4
#define CLK_PIN 23
#define BUTTON_PIN 15


#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define CHAIN_LENGTH 1

// ========== STORAGE FILES ==========
#define STORED_FILE "/current.dat"   // Stores the actual image/GIF data
#define TYPE_FILE "/type.txt"         // Stores "image" or "gif"
#define STORED_WIFI "/cred.txt"
#define STORED_BRIGHTNESS "/brightness.txt"
#define STORED_SPEED "/speed.txt"


// ========== GLOBAL OBJECTS ==========
MatrixPanel_I2S_DMA *dma_display = nullptr;
WebServer server(80);
AnimatedGIF gif;

String currentType = "none";
bool isPlayingGif = false;
uint8_t* gifData = nullptr;
size_t gifSize = 0;
uint8_t brightness = 90;
const unsigned long LONG_PRESS_TIME = 1200;  // ms
float gifSpeed = 1.0f;




// ========== FUNCTION PROTOTYPES ==========
bool ConnectToStoredWiFi();
void setupWiFi();
void setupSPIFFS();
void setupWebServer();
void handleRoot();
void handleDelete();
void handleStatus();
void HandleWifi();
void loadAndDisplayStored();
void displayStoredImage();
void playStoredGif();
void GIFDraw(GIFDRAW *pDraw);
void displayBootAnimation();
void HandleLongButtonPress();
void HandleShortButtonPress();
void PostBrightness();
void GetBrightness();
uint8_t ReadBrightness();
float ReadGifSpeed();


void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] %d\n", event);

  switch(event) {
    case SYSTEM_EVENT_AP_START:
      Serial.println("AP Started");
      break;

    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Client connected (auth OK)");
      break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;

    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("IP assigned to client");
      break;

    default:
      break;
  }
}


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

// =========GETTING BRIGHTNESS =========
void GetBrightness()
{
  String tempbrightness = "{";
  tempbrightness += "\"brightness\":\"" + String(brightness) + "\"}";
  
  server.send(200, "application/json", tempbrightness);
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

// ====== CONNECT TO WIFI ======
bool ConnectToStoredWiFi() {
  if (!SPIFFS.exists("/wifi.txt")) {
    Serial.println("Brak zapisanych danych WiFi");
    return false;
  }

  File file = SPIFFS.open("/wifi.txt", "r");
  if (!file) {
    Serial.println("Nie mozna otworzyc pliku WiFi");
    return false;
  }

  String ssidLine = file.readStringUntil('\n');
  String passLine = file.readStringUntil('\n');
  file.close();

  ssidLine.trim();
  passLine.trim();

  if (!ssidLine.startsWith("ssid=") || !passLine.startsWith("password=")) {
    Serial.println("Nieprawidlowy format pliku WiFi");
    return false;
  }

  String ssid = ssidLine.substring(5);
  String password = passLine.substring(9);

  Serial.print("Laczenie z WiFi:");
  Serial.println(ssid);
  Serial.print(";");
  Serial.println(password);

  WiFi.disconnect(true, true);
  delay(300);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);     // VERY important
  delay(300);

  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nPolaczono z WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);           
    delay(200);
    Serial.println("\nNie udalo sie polaczyc z WiFi");
    return false;
  }
}


// ========== WIFI SETUP ==========
void setupWiFi() {
  Serial.println("Setting up WiFi Access Point...");

  isPlayingGif = false;
  gif.close();
  dma_display->clearScreen();
  delay(300);
  // FULL reset of WiFi stack
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true, true);  // erase old config
  WiFi.mode(WIFI_OFF);
  delay(1000);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);   // CRITICAL

  bool result = WiFi.softAP(ssid);

  if (!result) {
    Serial.println("AP failed to start!");
    return;
  }

  delay(5000);  // Give DHCP time to stabilize

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}



// ========== WEB SERVER SETUP ==========
void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/delete", HTTP_POST, handleDelete);
  server.on("/cred",HTTP_POST,HandleWifi);
  server.on("/postbrightness",HTTP_POST,[](){
    PostBrightness(dma_display,&brightness,&server);
  });
  server.on("/getbrightness",HTTP_GET,GetBrightness);


  server.on("/getspeed", HTTP_GET, []() {
    String json = "{\"speed\":\"" + String(gifSpeed) + "\"}";
    server.send(200, "application/json", json);
  });

  server.on("/postspeed", HTTP_POST, []() {
    if (!server.hasArg("speed")) {
        server.send(400, "text/plain", "Missing speed");
        return;
    }
    String val = server.arg("speed");
    val.trim();
    gifSpeed = val.toFloat();
    gifSpeed = max(0.0f, min(1.0f, gifSpeed));

    File file = SPIFFS.open(STORED_SPEED, FILE_WRITE);
    if (file) {
        file.print("speed=");
        file.println(gifSpeed);
        file.close();
    }
    server.send(200, "text/plain", "Speed saved");
  });


  // Handle upload with proper body parsing
  server.on("/upload", HTTP_POST, 
    []() {
      // This is called after upload completes
      server.send(200, "text/plain", "Saved! Will persist after power off");
    },
    []() {
      // This is called during upload for each chunk
      HTTPUpload& upload = server.upload();
      static File uploadFile;
      static size_t totalSize = 0;
      static bool isFirstChunk = true;
      
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("\n📥 Upload starting: %s\n", upload.filename.c_str());
        totalSize = 0;
        isFirstChunk = true;
        
        // Open file for writing
        uploadFile = SPIFFS.open(STORED_FILE, "w");
        if (!uploadFile) {
          Serial.println("✗ Failed to open file for writing");
        }
      }
      else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
          // Write chunk to file
          uploadFile.write(upload.buf, upload.currentSize);
          totalSize += upload.currentSize;
          
          if (isFirstChunk) {
            // Check file type from first chunk
            if (upload.buf[0] == 'G' && upload.buf[1] == 'I' && upload.buf[2] == 'F') {
              Serial.println("Type: Animated GIF");
              File typeFile = SPIFFS.open(TYPE_FILE, "w");
              if (typeFile) {
                typeFile.print("gif");
                typeFile.close();
              }
              currentType = "gif";
            } else {
              Serial.println("Type: Static image");
              File typeFile = SPIFFS.open(TYPE_FILE, "w");
              if (typeFile) {
                typeFile.print("image");
                typeFile.close();
              }
              currentType = "image";
            }
            isFirstChunk = false;
          }
          
          Serial.printf("  Received: %d bytes (total: %d)\n", upload.currentSize, totalSize);
        }
      }



    else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
        uploadFile.close();
        Serial.printf("Upload complete: %d bytes\n", totalSize);

        // Stop any playing GIF before showing new content —
        // otherwise the GIF loop overwrites the new image immediately.
        isPlayingGif = false;
        gif.close();
        if (gifData) {
            free(gifData);
            gifData = nullptr;
        }

        dma_display->clearScreen();
        delay(50);  // small pause so the clear actually renders
        loadAndDisplayStored();
    }
}



    }
  );
  
  server.begin();
  Serial.println("✓ Web server started");
}

// ========== WEB PAGE ==========
const char HTML_PAGE[] PROGMEM = 
"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>LED Matrix</title><style>"
"body{font-family:Arial;background:#667eea;color:#fff;padding:20px;margin:0;text-align:center}"
".box{max-width:500px;margin:0 auto;background:rgba(255,255,255,0.1);padding:20px;border-radius:10px}"
"h1{margin:0 0 20px}"
".info{background:rgba(0,0,0,0.2);padding:15px;border-radius:8px;margin:15px 0;font-size:14px}"
".highlight{background:rgba(76,175,80,0.3);padding:10px;border-radius:5px;margin:10px 0}"
"code{background:rgba(0,0,0,0.3);padding:2px 6px;border-radius:3px}"
".btn{background:#764ba2;color:#fff;border:none;padding:12px 20px;border-radius:8px;margin:5px;cursor:pointer}"
".btn:hover{background:#8b5cb8}"
"</style></head><body><div class='box'>"
"<h1>LED Matrix 64x64</h1>"
"<div class='highlight'><strong>✓ Content persists after power off!</strong></div>"
"<div class='info'><strong>How to use:</strong><br>"
"1. Connect to this WiFi once<br>"
"2. Send image or GIF from your app<br>"
"3. It stays forever (even after unplugging!)<br>"
"4. Disconnect and enjoy<br></div>"
"<div class='info'><strong>API Endpoints:</strong><br>"
"POST <code>/upload</code> - Send image/GIF<br>"
"POST <code>/delete</code> - Clear display<br>"
"GET <code>/status</code> - Check what's stored<br></div>"
"<div class='info'><strong>Data formats:</strong><br>"
"• Images: RGB565 (8192 bytes) or RGB888 (12288 bytes)<br>"
"• GIFs: Standard GIF file (max ~100KB)<br></div>"
"<button class='btn' onclick='location.reload()'>Refresh</button>"
"<button class='btn' onclick='clearDisplay()'>Clear Display</button>"
"<script>function clearDisplay(){fetch('/delete',{method:'POST'}).then(()=>alert('Display cleared!'))}</script>"
"</div></body></html>";

void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

// ========== HANDLE DELETE ==========
void handleDelete() {
  if (SPIFFS.exists(STORED_FILE)) {
    SPIFFS.remove(STORED_FILE);
  }
  if (SPIFFS.exists(TYPE_FILE)) {
    SPIFFS.remove(TYPE_FILE);
  }

  isPlayingGif = false;
  if (gifData) {
    free(gifData);
    gifData = nullptr;
  }

  dma_display->clearScreen();
  currentType = "none";

  server.send(200, "text/plain", "Cleared");
  Serial.println("🗑️ Storage cleared");
}

// ========== HANDLE STATUS ==========
void handleStatus() {
  String status = "{";
  status += "\"type\":\"" + currentType + "\",";
  status += "\"stored\":" + String(SPIFFS.exists(STORED_FILE) ? "true" : "false") + ",";
  
  if (SPIFFS.exists(STORED_FILE)) {
    File file = SPIFFS.open(STORED_FILE, "r");
    if (file) {
      status += "\"size\":" + String(file.size()) + ",";
      file.close();
    }
  }
  
  status += "\"storage_used\":" + String(SPIFFS.usedBytes()) + ",";
  status += "\"storage_total\":" + String(SPIFFS.totalBytes());
  status += "}";
  
  server.send(200, "application/json", status);
}

// ========== HANDLE CHANGING WIFI CREDENTIALS ==========

void HandleWifi() {
    Serial.println("HandleWifi called!");

    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        Serial.println("Missing SSID or password");
        server.send(400, "text/plain", "Missing SSID or password");
        return;
    }

    // Copy strings properly
    String ssidcred = server.arg("ssid");
    String passwordcred = server.arg("password");
    ssidcred.trim();
    passwordcred.trim();

    Serial.printf("Saving SSID: '%s', Password: '%s'\n", ssidcred.c_str(), passwordcred.c_str());

    // Open file in write mode (overwrites existing)
    File file = SPIFFS.open("/wifi.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open /wifi.txt for writing");
        server.send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    file.print("ssid=");
    file.println(ssidcred);
    file.print("password=");
    file.println(passwordcred);
    file.close();

    Serial.println("WiFi credentials saved successfully");
    server.send(200, "text/plain", "WiFi credentials stored successfully");
}



// ========== LOAD AND DISPLAY STORED CONTENT ==========
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

// ========== DISPLAY STORED IMAGE ==========
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

// ========== PLAY STORED GIF ==========
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

// ========== GIF DRAW CALLBACK ==========
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;
  
  if (y >= PANEL_HEIGHT || pDraw->iX >= PANEL_WIDTH) 
    return;
    
  s = pDraw->pPixels;
  
  if (pDraw->ucDisposalMethod == 2) {
    for (x = 0; x < pDraw->iWidth; x++) {
      if (s[x] == pDraw->ucTransparent) {
        s[x] = pDraw->ucBackground;
      }
    }
    pDraw->ucHasTransparency = 0;
  }
  
  if (pDraw->ucHasTransparency) {
    uint8_t c, ucTransparent = pDraw->ucTransparent;
    for (x = 0; x < pDraw->iWidth; x++) {
      c = s[x];
      if (c != ucTransparent) {
        uint16_t color = usPalette[c];
        int px = pDraw->iX + x;
        if (px < PANEL_WIDTH && y < PANEL_HEIGHT) {
          dma_display->drawPixel(px, y, color);
        }
      }
    }
  } else {
    for (x = 0; x < pDraw->iWidth; x++) {
      uint16_t color = usPalette[s[x]];
      int px = pDraw->iX + x;
      if (px < PANEL_WIDTH && y < PANEL_HEIGHT) {
        dma_display->drawPixel(px, y, color);
      }
    }
  }
}

// ==============READING BRIGHTNESS FROM FILE ==============
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