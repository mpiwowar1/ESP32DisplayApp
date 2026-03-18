#include "network.h"

void setupWebServer() {
  server.on("/", HTTP_GET, GetWebsite);
  server.on("/status", HTTP_GET, GetStatus);
  server.on("/delete", HTTP_POST, DeleteStored);
  server.on("/cred",HTTP_POST,PostCredentials);
  server.on("/postbrightness",HTTP_POST,PostBrightness);
  server.on("/getbrightness",HTTP_GET,GetBrightness);
  server.on("/getspeed", HTTP_GET,GetSpeed);
  server.on("/postspeed", HTTP_POST,PostSpeed);


  // Handle upload with proper body parsing
  server.on("/upload", HTTP_POST, 
    []() {
      server.send(200, "text/plain", "Saved! Will persist after power off");
    },PostMedia);
  
  server.begin();
  Serial.println("✓ Web server started");
}

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



