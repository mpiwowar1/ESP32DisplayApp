#include "api.h"

void PostBrightness(MatrixPanel_I2S_DMA *dma_display,uint8_t *brightness,WebServer *server) {
    Serial.println("PostBrightness called!");

    if (!server->hasArg("brightness")) {
        server->send(400, "text/plain", "Missing brightness");
        return;
    }

    String brightnesssave = server->arg("brightness");
    brightnesssave.trim();
    *brightness = (uint8_t)brightnesssave.toInt();

    Serial.printf("Saving brightness: %d\n", brightness);

    File file = SPIFFS.open("/brightness.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open /brightness.txt for writing");
        server->send(500, "text/plain", "Failed to open file for writing");
        return;
    }

    file.print("brightness=");
    file.println(*brightness);
    file.close(); 

    Serial.println("Brightness saved successfully");
    server->send(200, "text/plain", "Brightness stored successfully");

    dma_display->setBrightness8(*brightness);
}
