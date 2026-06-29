/**
 * @file api.cpp
 * @brief HTTP endpoint handler implementations.
 */

#include "api.h"

/** @brief Save brightness to flash and apply it to the display. */
void PostBrightness() {
    Serial.println("PostBrightness called");

    if (!server.hasArg("brightness")) {
        server.send(400, "text/plain", "Missing brightness");
        return;
    }

    String val = server.arg("brightness");
    val.trim();
    brightness = (uint8_t)val.toInt();

    Serial.printf("Saving brightness: %d\n", brightness);

    File file = SPIFFS.open("/brightness.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open /brightness.txt");
        server.send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    file.print("brightness=");
    file.println(brightness);
    file.close();

    Serial.println("Brightness saved");
    server.send(200, "text/plain", "Brightness stored successfully");

    dma_display->setBrightness8(brightness);
}

/** @brief Return the current brightness as a JSON object. */
void GetBrightness() {
    String json = "{\"brightness\":\"" + String(brightness) + "\"}";
    server.send(200, "application/json", json);
}

/** @brief Read SSID and password from the request and save them to flash. */
void PostCredentials() {
    Serial.println("PostCredentials called");

    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        Serial.println("Missing SSID or password");
        server.send(400, "text/plain", "Missing SSID or password");
        return;
    }

    String ssidcred = server.arg("ssid");
    String passwordcred = server.arg("password");
    ssidcred.trim();
    passwordcred.trim();

    Serial.printf("Saving SSID: '%s', Password: '%s'\n",
                  ssidcred.c_str(), passwordcred.c_str());

    File file = SPIFFS.open("/wifi.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open /wifi.txt");
        server.send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    file.print("ssid=");
    file.println(ssidcred);
    file.print("password=");
    file.println(passwordcred);
    file.close();

    Serial.println("WiFi credentials saved");
    server.send(200, "text/plain", "WiFi credentials stored successfully");
}

/** @brief Serve the built-in HTML page from PROGMEM. */
void GetWebsite() {
    server.send_P(200, "text/html", HTML_PAGE);
}

/** @brief Delete stored media, stop any GIF, and clear the display. */
void DeleteStored() {
    if (SPIFFS.exists(STORED_FILE)) SPIFFS.remove(STORED_FILE);
    if (SPIFFS.exists(TYPE_FILE))   SPIFFS.remove(TYPE_FILE);

    isPlayingGif = false;
    if (gifData) {
        free(gifData);
        gifData = nullptr;
    }

    dma_display->clearScreen();
    currentType = "none";

    server.send(200, "text/plain", "Cleared");
    Serial.println("Storage cleared");
}

/** @brief Build and send a JSON object with current storage status. */
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

/** @brief Return the current GIF playback speed as JSON. */
void GetSpeed() {
    String json = "{\"speed\":\"" + String(gifSpeed) + "\"}";
    server.send(200, "application/json", json);
}

/** @brief Save a new GIF speed value (0.0 to 1.0) to flash. */
void PostSpeed() {
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

/**
 * @brief Handle chunked file upload (image or GIF).
 *
 * Called multiple times per upload: once for START, once per data WRITE
 * chunk, and once for END. Detects GIF vs image by checking the first
 * three bytes of the file ("GIF" magic).
 */
void PostMedia() {
    HTTPUpload& upload = server.upload();
    static File uploadFile;
    static size_t totalSize = 0;
    static bool isFirstChunk = true;

    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("\nUpload starting: %s\n", upload.filename.c_str());
        totalSize = 0;
        isFirstChunk = true;
        uploadFile = SPIFFS.open(STORED_FILE, "w");
        if (!uploadFile) {
            Serial.println("Failed to open file for writing");
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
            totalSize += upload.currentSize;

            if (isFirstChunk) {
                /* Detect file type from magic bytes */
                if (upload.buf[0] == 'G' && upload.buf[1] == 'I' && upload.buf[2] == 'F') {
                    Serial.println("Type: GIF");
                    File typeFile = SPIFFS.open(TYPE_FILE, "w");
                    if (typeFile) { typeFile.print("gif"); typeFile.close(); }
                    currentType = "gif";
                } else {
                    Serial.println("Type: image");
                    File typeFile = SPIFFS.open(TYPE_FILE, "w");
                    if (typeFile) { typeFile.print("image"); typeFile.close(); }
                    currentType = "image";
                }
                isFirstChunk = false;
            }

            Serial.printf("  Received: %d bytes (total: %d)\n",
                          upload.currentSize, totalSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            Serial.printf("Upload complete: %d bytes\n", totalSize);

            /* Stop any playing GIF before showing new content */
            isPlayingGif = false;
            gif.close();
            if (gifData) {
                free(gifData);
                gifData = nullptr;
            }

            dma_display->clearScreen();
            delay(50);
            loadAndDisplayStored();
        }
    }
}
