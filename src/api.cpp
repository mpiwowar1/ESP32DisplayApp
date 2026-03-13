#include "api.h"

void PostBrightness() {
    Serial.println("PostBrightness called!");

    if (!server.hasArg("brightness")) {
        server.send(400, "text/plain", "Missing brightness");
        return;
    }

    String brightnesssave = server.arg("brightness");
    brightnesssave.trim();
    brightness = (uint8_t)brightnesssave.toInt();

    Serial.printf("Saving brightness: %d\n", brightness);

    File file = SPIFFS.open("/brightness.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open /brightness.txt for writing");
        server.send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    file.print("brightness=");
    file.println(brightness);
    file.close(); 

    Serial.println("Brightness saved successfully");
    server.send(200, "text/plain", "Brightness stored successfully");

    dma_display->setBrightness8(brightness);
}

void GetBrightness()
{
  String tempbrightness = "{";
  tempbrightness += "\"brightness\":\"" + String(brightness) + "\"}";
  
  server.send(200, "application/json", tempbrightness);
}

void PostCredentials() {
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

void GetWebsite() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void DeleteStored() {
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

void GetStatus() {
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

void GetSpeed(){
    String json = "{\"speed\":\"" + String(gifSpeed) + "\"}";
    server.send(200, "application/json", json);
}

void PostSpeed(){
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
}

void PostMedia(){
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

