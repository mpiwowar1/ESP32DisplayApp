/**
 * @file menu.cpp
 * @brief Scrolling menu with animated transitions and hold-to-confirm.
 *
 * Each glyph is 6x8 pixels (5 data columns + 1 gap).
 * Text outside the selection box is dimmed based on distance from centre.
 * During scroll animation, only the text bands are redrawn to avoid flicker.
 */

#include "menu.h"

/* --- Selection box layout --- */
static const int BOX_X      =  3;
static const int BOX_Y      = 22;
static const int BOX_W      = 58;
static const int BOX_H      = 17;
static const int CORNER_CUT =  8;

/* --- Vertical positions of the four visible labels --- */
static const int PREV_Y     =  7;
static const int SEL_Y      = 27;
static const int NEXT_Y     = 47;
static const int AFTER_Y    = 67;

/* --- Scroll animation settings --- */
static const int ANIM_FRAMES   = 20;
static const int ANIM_DELAY_MS = 40;
static const int ANIM_STEP_PX  =  1;

/**
 * @brief Check if a pixel is part of the bevelled corner cutout.
 * @param px  X coordinate on the panel.
 * @param py  Y coordinate on the panel.
 * @return true if this pixel should be black (corner cut).
 */
static bool isCutPixel(int px, int py) {
    int dT = py - BOX_Y - 1;
    if (dT >= 0 && dT < CORNER_CUT) {
        if (px == BOX_X + 1 + dT || px == BOX_X + BOX_W - 2 - dT)
            return true;
    }
    int dB = BOX_Y + BOX_H - 2 - py;
    if (dB >= 0 && dB < CORNER_CUT) {
        if (px == BOX_X + 1 + dB || px == BOX_X + BOX_W - 2 - dB)
            return true;
    }
    return false;
}

/**
 * @brief Repaint a horizontal strip to its background colour.
 *
 * Used during animation instead of clearing the whole screen.
 * Rows inside the selection box get white, everything else gets black.
 *
 * @param y0  First row to restore (inclusive).
 * @param y1  Last row to restore (inclusive).
 */
static void restoreBand(int y0, int y1) {
    uint16_t white = dma_display->color565(200, 200, 200);
    uint16_t black = dma_display->color565(0, 0, 0);

    for (int py = y0; py <= y1; py++) {
        if (py < 0 || py >= PANEL_HEIGHT) continue;

        bool rowInBox = (py >= BOX_Y && py < BOX_Y + BOX_H);

        if (!rowInBox) {
            dma_display->drawFastHLine(0, py, PANEL_WIDTH, black);
        } else {
            if (BOX_X > 0)
                dma_display->drawFastHLine(0, py, BOX_X, black);

            dma_display->drawFastHLine(BOX_X, py, BOX_W, white);

            int rx = BOX_X + BOX_W;
            if (rx < PANEL_WIDTH)
                dma_display->drawFastHLine(rx, py, PANEL_WIDTH - rx, black);

            /* Re-cut the corner pixels */
            int dT = py - BOX_Y - 1;
            if (dT >= 0 && dT < CORNER_CUT) {
                dma_display->drawPixelRGB888(BOX_X + 1 + dT,         py, 0, 0, 0);
                dma_display->drawPixelRGB888(BOX_X + BOX_W - 2 - dT, py, 0, 0, 0);
            }
            int dB = BOX_Y + BOX_H - 2 - py;
            if (dB >= 0 && dB < CORNER_CUT) {
                dma_display->drawPixelRGB888(BOX_X + 1 + dB,         py, 0, 0, 0);
                dma_display->drawPixelRGB888(BOX_X + BOX_W - 2 - dB, py, 0, 0, 0);
            }
        }
    }
}

/**
 * @brief Draw the selection box (light grey with bevelled corners).
 */
static void drawBackground() {
    for (int i = 0; i < BOX_W; i++)
        for (int j = 0; j < BOX_H; j++)
            dma_display->drawPixelRGB888(BOX_X + i, BOX_Y + j, 200, 200, 200);

    for (int i = 0; i < CORNER_CUT; i++) {
        int xL = BOX_X + 1 + i;
        int xR = BOX_X + BOX_W - 2 - i;
        int yT = BOX_Y + 1 + i;
        int yB = BOX_Y + BOX_H - 2 - i;
        dma_display->drawPixelRGB888(xL, yT, 0, 0, 0);
        dma_display->drawPixelRGB888(xL, yB, 0, 0, 0);
        dma_display->drawPixelRGB888(xR, yT, 0, 0, 0);
        dma_display->drawPixelRGB888(xR, yB, 0, 0, 0);
    }
}

/**
 * @brief Draw a single character with per-pixel brightness dimming.
 *
 * Pixels inside the selection box are black (text on white background).
 * Pixels outside fade to dark based on distance from screen centre.
 *
 * @param screenX  Left column of the glyph.
 * @param screenY  Top row of the glyph.
 * @param c        ASCII character to draw.
 */
static void drawLetter(int screenX, int screenY, char c) {
    for (int col = 0; col < 6; col++) {
        for (int row = 0; row < 8; row++) {
            if (!menuFontPixel(c, col, row)) continue;

            int px = screenX + col;
            int py = screenY + row;

            if (px < 0 || px >= PANEL_WIDTH || py < 0 || py >= PANEL_HEIGHT) continue;

            uint8_t bright;

            if (py > 21 && py < 39) {
                bright = 0;  /* Inside box: black text */
            } else {
                float dist    = fabsf(32.0f - (float)py);
                float ratio   = dist / 32.0f;
                float dimming = 255.0f * ratio;
                float p       = 255.0f - dimming;
                if (p < 0.0f) p = 0.0f;
                bright = (uint8_t)p;
            }

            dma_display->drawPixelRGB888(px, py, bright, bright, bright);
        }
    }
}

/**
 * @brief Draw a word centred horizontally at the given Y position.
 * @param word     Null-terminated string to draw.
 * @param screenY  Top row for the 8-pixel-tall text.
 */
static void drawWord(const char* word, int screenY) {
    if (!word) return;
    int len = (int)strlen(word);
    int startX = (PANEL_WIDTH - len * 6) / 2;

    for (int i = 0; i < len; i++) {
        drawLetter(startX + i * 6, screenY, word[i]);
    }
}

/**
 * @brief Render all four visible menu labels.
 *
 * @param items     Menu item array.
 * @param count     Number of items.
 * @param snapIdx   Currently selected index.
 * @param offset    Vertical pixel offset for animation.
 * @param animating If true, only refresh text bands (no full clear).
 */
static void drawFrame(MenuItem* items, int count, int snapIdx, int offset,
                      bool animating = false) {
    auto mod = [&](int i) -> int { return ((i % count) + count) % count; };

    if (!animating) {
        dma_display->clearScreen();
        drawBackground();
    } else {
        restoreBand(PREV_Y  + offset - 1, PREV_Y  + offset + 9);
        restoreBand(SEL_Y   + offset - 1, SEL_Y   + offset + 9);
        restoreBand(NEXT_Y  + offset - 1, NEXT_Y  + offset + 9);
        restoreBand(AFTER_Y + offset - 1, AFTER_Y + offset + 9);
    }

    drawWord(items[mod(snapIdx - 1)].label, PREV_Y  + offset);
    drawWord(items[mod(snapIdx    )].label, SEL_Y   + offset);
    drawWord(items[mod(snapIdx + 1)].label, NEXT_Y  + offset);
    drawWord(items[mod(snapIdx + 2)].label, AFTER_Y + offset);
}

/* --- Hold-to-confirm progress circle --- */
static const int HOLD_CX = PANEL_WIDTH - 7;
static const int HOLD_CY = 6;
static const int HOLD_R  = 5;

/**
 * @brief Draw a pie-chart progress circle in the top-right corner.
 *
 * Shows how long the button has been held. A dim grey disc is the
 * background; a white arc fills clockwise as t goes from 0 to 1.
 *
 * @param t  Progress from 0.0 (empty) to 1.0 (full).
 */
static void drawHoldFrame(float t) {
    float sweep = t * 2.0f * (float)M_PI;

    for (int py = HOLD_CY - HOLD_R - 1; py <= HOLD_CY + HOLD_R + 1; py++) {
        for (int px = HOLD_CX - HOLD_R - 1; px <= HOLD_CX + HOLD_R + 1; px++) {
            if (px < 0 || px >= PANEL_WIDTH || py < 0 || py >= PANEL_HEIGHT) continue;

            float dx   = (float)(px - HOLD_CX);
            float dy   = (float)(py - HOLD_CY);
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > (float)HOLD_R + 0.5f) {
                dma_display->drawPixelRGB888(px, py, 0, 0, 0);
            } else {
                float angle = atan2f(dx, -dy);
                if (angle < 0.0f) angle += 2.0f * (float)M_PI;

                if (angle <= sweep)
                    dma_display->drawPixelRGB888(px, py, 255, 255, 255);
                else
                    dma_display->drawPixelRGB888(px, py, 50, 50, 50);
            }
        }
    }
}

/**
 * @brief Show the scrolling menu and block until user picks an item.
 *
 * Short press = scroll to next item. Long press = confirm selection.
 * The web server keeps running while the menu is open.
 *
 * @param items  Array of MenuItem structs.
 * @param count  Number of items in the array.
 */
void showMenu(MenuItem* items, int count) {
    if (!items || count <= 0) return;

    isPlayingGif = false;

    int  currentIdx  = 0;
    bool lastState   = HIGH;
    bool btnPressed  = false;
    unsigned long pressStart = 0;
    bool longHandled = false;

    drawFrame(items, count, currentIdx, 0, false);

    while (true) {
        server.handleClient();

        bool btnState = (bool)digitalRead(BUTTON_PIN);

        /* Button just pressed */
        if (lastState == HIGH && btnState == LOW) {
            btnPressed  = true;
            pressStart  = millis();
            longHandled = false;
        }

        /* Button held - show progress circle */
        if (btnPressed && btnState == LOW && !longHandled) {
            unsigned long held = millis() - pressStart;
            float t = (float)held / (float)LONG_PRESS_TIME;
            if (t > 1.0f) t = 1.0f;

            drawHoldFrame(t);

            if (held >= LONG_PRESS_TIME) {
                longHandled = true;
                dma_display->clearScreen();
                if (items[currentIdx].onSelect)
                    items[currentIdx].onSelect();
                return;
            }
        }

        /* Button released */
        if (lastState == LOW && btnState == HIGH) {
            if (btnPressed && !longHandled) {
                drawHoldFrame(0.0f);

                /* Scroll animation */
                int snapIdx = currentIdx;
                for (int f = 1; f <= ANIM_FRAMES; f++) {
                    drawFrame(items, count, snapIdx, -(f * ANIM_STEP_PX), true);
                    delay(ANIM_DELAY_MS);
                }

                currentIdx = (currentIdx + 1) % count;
                drawFrame(items, count, currentIdx, 0, false);
            }
            btnPressed = false;
        }

        lastState = btnState;
        delay(5);
    }
}

/**
 * @brief Enter setup mode: start AP, show config info, allow memory format.
 *
 * Short press exits setup. Long press formats SPIFFS.
 * After exiting, tries to reconnect to stored WiFi.
 */
void doSetup() {
    Serial.println("Entering SETUP mode");
    dma_display->clearScreen();
    dma_display->setTextColor(dma_display->color565(0, 255, 0));
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->print("STARTING");
    dma_display->setCursor(2, 12);
    dma_display->print("SETUP MODE");

    /* Wait for button release */
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

    /* Show AP info on the display */
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

    Serial.println("SETUP mode active. Press button to exit.");

    bool lastButtonState     = HIGH;
    bool buttonPressed       = false;
    unsigned long pressStartTime = 0;
    bool longPressHandled    = false;

    while (true) {
        server.handleClient();
        delay(1);

        bool currentState = digitalRead(BUTTON_PIN);

        /* Button just pressed */
        if (lastButtonState == HIGH && currentState == LOW) {
            buttonPressed    = true;
            pressStartTime   = millis();
            longPressHandled = false;
        }

        /* Button held - format memory on long press */
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

                /* Redraw setup screen */
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

        /* Button released - short press exits setup */
        if (lastButtonState == LOW && currentState == HIGH) {
            if (buttonPressed && !longPressHandled) {
                break;
            }
            buttonPressed = false;
        }

        lastButtonState = currentState;
    }

    /* Exit setup and reconnect to stored WiFi */
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
        Serial.println("Could not connect - check saved credentials.");
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

/**
 * @brief Check WiFi, reconnect if needed, show status on display.
 *
 * Blocks until the user presses the button to go back.
 */
void doWifi() {
    dma_display->clearScreen();
    dma_display->setTextColor(dma_display->color565(255, 0, 0));
    dma_display->setTextSize(1);
    dma_display->setCursor(2, 2);
    dma_display->print("WiFi CheckUp");

    if (WiFi.status() != WL_CONNECTED) {
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

    if (WiFi.status() == WL_CONNECTED) {
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

    /* Wait for button press to exit */
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