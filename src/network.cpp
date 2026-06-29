/**
 * @file network.cpp
 * @brief WiFi connection (STA and AP modes) and web server setup.
 */

#include "network.h"

/**
 * @brief Register all HTTP routes and start listening on port 80.
 */
void setupWebServer() {
    server.on("/",              HTTP_GET,  GetWebsite);
    server.on("/status",        HTTP_GET,  GetStatus);
    server.on("/delete",        HTTP_POST, DeleteStored);
    server.on("/cred",          HTTP_POST, PostCredentials);
    server.on("/postbrightness", HTTP_POST, PostBrightness);
    server.on("/getbrightness", HTTP_GET,  GetBrightness);
    server.on("/getspeed",      HTTP_GET,  GetSpeed);
    server.on("/postspeed",     HTTP_POST, PostSpeed);

    server.on("/upload", HTTP_POST,
        []() { server.send(200, "text/plain", "Saved! Will persist after power off"); },
        PostMedia);

    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("Web server started");
}

/**
 * @brief Start the ESP32 as a WiFi access point (AP mode).
 *
 * Fully resets the WiFi stack first to avoid leftover state.
 */
void setupWiFi() {
    Serial.println("Setting up WiFi Access Point...");

    isPlayingGif = false;
    gif.close();
    dma_display->clearScreen();
    delay(300);

    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(1000);

    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);

    bool result = WiFi.softAP(ssid);
    if (!result) {
        Serial.println("AP failed to start!");
        return;
    }

    delay(5000);

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

/**
 * @brief Try connecting to the WiFi network saved in /wifi.txt.
 *
 * Reads SSID and password from flash, attempts to connect for
 * up to 15 seconds.
 *
 * @return true if connected, false otherwise.
 */
bool ConnectToStoredWiFi() {
    if (!SPIFFS.exists("/wifi.txt")) {
        Serial.println("No saved WiFi credentials");
        return false;
    }

    File file = SPIFFS.open("/wifi.txt", "r");
    if (!file) {
        Serial.println("Cannot open WiFi credentials file");
        return false;
    }

    String ssidLine = file.readStringUntil('\n');
    String passLine = file.readStringUntil('\n');
    file.close();

    ssidLine.trim();
    passLine.trim();

    if (!ssidLine.startsWith("ssid=") || !passLine.startsWith("password=")) {
        Serial.println("Bad WiFi credentials format");
        return false;
    }

    String ssid     = ssidLine.substring(5);
    String password = passLine.substring(9);

    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.disconnect(true, true);
    delay(300);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    delay(300);

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        if (!MDNS.begin("matrix")) {
            Serial.println("mDNS setup failed");
        }
        return true;
    } else {
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);
        delay(200);
        Serial.println("\nWiFi connection failed");
        return false;
    }
}

/**
 * @brief Log WiFi system events to serial for debugging.
 * @param event The WiFi event type from the ESP-IDF.
 */
void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] %d\n", event);

    switch (event) {
        case SYSTEM_EVENT_AP_START:
            Serial.println("AP Started");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected");
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
