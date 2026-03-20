#include "menu.h"


/*
 * ═══════════════════════════════════════════════════════════════
 *  menu.cpp  –  Scrolling menu with per-pixel-row dimming
 *              (flicker-reduced: background drawn once, only
 *               text bands refreshed during animation)
 * ═══════════════════════════════════════════════════════════════
 *
 *  Key fix vs the previous version:
 *
 *    OLD: pick one colour for the entire word based on word-centre Y
 *    NEW: for every lit pixel test its absolute Y on the panel,
 *         compute brightness from that exact row – identical to the
 *         Python draw_letter() loop:
 *
 *         dimming = 255 × (|32 − pixelY| / 32)²   (curved falloff)
 *         p       = 0   if pixelY is inside the selection box
 *                   255 − dimming  otherwise
 *
 *  Anti-flicker change:
 *    OLD: every animation frame calls clearScreen() + drawBackground()
 *         → white box flashes black on every frame.
 *    NEW: background is drawn ONCE on full redraws.  During animation
 *         only the 9-row band around each word is restored to its
 *         background colour, then the word is redrawn on top.
 *
 *  Each glyph is 6 × 8 px (5 data columns + 1 gap column).
 *  Pixel data is read from PROGMEM via menuFontPixel().
 */

// ── Layout constants (must match draw_bg / draw_frame in main.py) ────────────

static const int BOX_X      =  3;
static const int BOX_Y      = 22;
static const int BOX_W      = 58;
static const int BOX_H      = 17;
static const int CORNER_CUT =  8;

static const int PREV_Y     =  7;   // word above the selection box
static const int SEL_Y      = 27;   // word inside the selection box
static const int NEXT_Y     = 47;   // word below the selection box
static const int AFTER_Y    = 67;   // word that slides in from the bottom

// ── Animation ────────────────────────────────────────────────────────────────

static const int ANIM_FRAMES   = 20;  // frames per scroll
static const int ANIM_DELAY_MS = 40;  // ms per frame (~25 fps)
static const int ANIM_STEP_PX  =  1;  // px moved per frame (20 × 1 = 20 px total)

// ── Private: O(1) corner-cut checker ─────────────────────────────────────────
//
//  Returns true if (px, py) is one of the bevelled corner pixels drawn by
//  drawBackground().  Used by restoreBand() so it can reproduce the exact
//  corner pattern without re-running the full drawBackground() loop.

static bool isCutPixel(int px, int py) {
    // Top-edge diagonals  (row BOX_Y+1+i  →  i = py-BOX_Y-1)
    int dT = py - BOX_Y - 1;
    if (dT >= 0 && dT < CORNER_CUT) {
        if (px == BOX_X + 1 + dT || px == BOX_X + BOX_W - 2 - dT)
            return true;
    }
    // Bottom-edge diagonals  (row BOX_Y+BOX_H-2-i  →  i = BOX_Y+BOX_H-2-py)
    int dB = BOX_Y + BOX_H - 2 - py;
    if (dB >= 0 && dB < CORNER_CUT) {
        if (px == BOX_X + 1 + dB || px == BOX_X + BOX_W - 2 - dB)
            return true;
    }
    return false;
}

// ── Private: restore a horizontal band to its background colour ───────────────
//
//  Called during animation instead of clearScreen().
//  Rows [y0 .. y1] (inclusive) are filled:
//    • white (200,200,200) – inside the selection-box fill, not a cut pixel
//    • black (0,0,0)       – everything else
//
//  The white box and its bevelled corners are never touched between frames.

static void restoreBand(int y0, int y1) {
    uint16_t white = dma_display->color565(200, 200, 200);
    uint16_t black = dma_display->color565(0, 0, 0);

    for (int py = y0; py <= y1; py++) {
        if (py < 0 || py >= PANEL_HEIGHT) continue;

        bool rowInBox = (py >= BOX_Y && py < BOX_Y + BOX_H);

        if (!rowInBox) {
            // Entire row is black – one call
            dma_display->drawFastHLine(0, py, PANEL_WIDTH, black);
        } else {
            // Left margin
            if (BOX_X > 0)
                dma_display->drawFastHLine(0, py, BOX_X, black);
            // Box fill (white)
            dma_display->drawFastHLine(BOX_X, py, BOX_W, white);
            // Right margin
            int rx = BOX_X + BOX_W;
            if (rx < PANEL_WIDTH)
                dma_display->drawFastHLine(rx, py, PANEL_WIDTH - rx, black);
            // Punch corner-cut pixels back to black (at most 2 per row)
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

// ── Private: selection-box background ───────────────────────────────────────

static void drawBackground() {
    // Solid light-grey fill
    for (int i = 0; i < BOX_W; i++)
        for (int j = 0; j < BOX_H; j++)
            dma_display->drawPixelRGB888(BOX_X + i, BOX_Y + j, 200, 200, 200);

    // Diagonal corner cutouts (same pixel formula as draw_bg() in main.py)
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

// ── Private: pixel-accurate letter renderer ──────────────────────────────────

/**
 * Draw one character, computing brightness independently for every pixel row.
 *
 * Python equivalent (draw_letter):
 *
 *   for i in range(6):           <- col
 *     for j in range(8):         <- row
 *       if font[letter][j][i]:
 *         dimming = int(255 * (abs(32-(y+j))/32)**2)
 *         p = 0 if y+j>21 and y+j<39 else 255-dimming
 *         matrix.draw_pixel(x+i, y+j, p, p, p)
 *
 * @param screenX   Left pixel column of the glyph on the panel.
 * @param screenY   Top  pixel row    of the glyph on the panel.
 * @param c         ASCII character to draw.
 */
static void drawLetter(int screenX, int screenY, char c) {
    for (int col = 0; col < 6; col++) {          // 5 data + 1 gap
        for (int row = 0; row < 8; row++) {       // 8 pixel rows

            if (!menuFontPixel(c, col, row)) continue;  // pixel off -> skip

            int px = screenX + col;
            int py = screenY + row;

            // Clip to panel bounds
            if (px < 0 || px >= PANEL_WIDTH || py < 0 || py >= PANEL_HEIGHT) continue;

            // ── Brightness for this exact pixel row ───────────────────────
            //
            // Inside the selection box  ->  black text on the white background
            //   Python: p = 0  when  y+j > 21 AND y+j < 39
            //
            // Outside the box  ->  curved greyscale dimming centred on row 32
            //   Python: dimming = 255 * (|32 - py| / 32)^2
            //           p       = 255 - dimming

            uint8_t brightness;

            if (py > 21 && py < 39) {
                // Inside box: black so it reads against the grey background
                brightness = 0;
            } else {
                float dist    = fabsf(32.0f - (float)py);   // 0 at centre, 32 at edges
                float ratio   = dist / 32.0f;               // 0.0 ... 1.0
                float dimming = 255.0f * ratio;             // linear falloff
                float p       = 255.0f - dimming;
                if (p < 0.0f) p = 0.0f;
                brightness = (uint8_t)p;
            }

            dma_display->drawPixelRGB888(px, py, brightness, brightness, brightness);
        }
    }
}

// ── Private: word renderer (centred) ────────────────────────────────────────

/**
 * Draw a null-terminated word centred horizontally.
 * Each character is 6 px wide (5 data + 1 gap).
 *
 * @param word     String to render.
 * @param screenY  Top pixel row of the 8-px-tall glyph on the panel.
 */
static void drawWord(const char* word, int screenY) {
    if (!word) return;
    int len = (int)strlen(word);
    int startX = (PANEL_WIDTH - len * 6) / 2;   // centre horizontally

    for (int i = 0; i < len; i++) {
        drawLetter(startX + i * 6, screenY, word[i]);
    }
}

// ── Private: full frame renderer ────────────────────────────────────────────

/**
 * Render 4 words, with two possible strategies:
 *
 * animating = false  (full redraw, used for idle state)
 *   • clearScreen() + drawBackground() – wipes everything, repaints box
 *   • drawWord() × 4
 *
 * animating = true   (band-only update, called every animation frame)
 *   • restoreBand() × 4 – erases only the 9-row strip around each word
 *                         (8 glyph rows + 1 trailing row for the pixel
 *                         that scrolled out at the bottom last frame)
 *   • drawWord() × 4
 *   The box, corners, and untouched rows are NEVER redrawn → no flicker.
 *
 * @param items    Menu item array.
 * @param count    Number of items.
 * @param snapIdx  Index that was "current" when the animation started.
 * @param offset   Vertical pixel offset: 0 = idle, negative = scrolling up.
 * @param animating  true during scroll animation, false for full redraws.
 */
static void drawFrame(MenuItem* items, int count, int snapIdx, int offset,
                      bool animating = false) {

    // Safe modulo (handles negative indices)
    auto mod = [&](int i) -> int { return ((i % count) + count) % count; };

    if (!animating) {
        // Full redraw: clear everything then repaint the box
        dma_display->clearScreen();
        drawBackground();
    } else {
        // Animation: erase only the 11-row band around each word.
        // -1 / +9 (one extra row each side) eliminates any 1-pixel off-by-one
        // artifact left by the previous frame as words scroll upward.
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

// ── Private: hold-to-confirm animation ───────────────────────────────────────
//
//  Repaints only the selection-box region so the words outside are untouched.
//
//  @param t        Progress 0.0 → 1.0 over LONG_PRESS_TIME.
//                  Triangle wave: box goes black → white (0..0.5)
//                                 then white → black (0.5..1.0).
//  @param word     The currently selected label (drawn centred in the box).

// ── Private: hold-to-confirm animation ───────────────────────────────────────
//
//  Draws a small pie-chart progress circle in the top-right corner.
//  A dim grey disc shows the full track; a white arc fills clockwise from
//  the top as t goes 0.0 → 1.0.  Only the circle's bounding box is touched.

static const int HOLD_CX = PANEL_WIDTH - 7;   // centre x  (top-right corner)
static const int HOLD_CY = 6;                  // centre y
static const int HOLD_R  = 5;                  // radius in pixels

static void drawHoldFrame(float t) {
    float sweep = t * 2.0f * (float)M_PI;   // filled arc, radians clockwise

    for (int py = HOLD_CY - HOLD_R - 1; py <= HOLD_CY + HOLD_R + 1; py++) {
        for (int px = HOLD_CX - HOLD_R - 1; px <= HOLD_CX + HOLD_R + 1; px++) {
            if (px < 0 || px >= PANEL_WIDTH || py < 0 || py >= PANEL_HEIGHT) continue;

            float dx   = (float)(px - HOLD_CX);
            float dy   = (float)(py - HOLD_CY);
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > (float)HOLD_R + 0.5f) {
                dma_display->drawPixelRGB888(px, py, 0, 0, 0);   // outside – black
            } else {
                // atan2(dx, -dy): 0 at top, increases clockwise
                float angle = atan2f(dx, -dy);
                if (angle < 0.0f) angle += 2.0f * (float)M_PI;

                if (angle <= sweep)
                    dma_display->drawPixelRGB888(px, py, 255, 255, 255);  // filled arc
                else
                    dma_display->drawPixelRGB888(px, py, 50,  50,  50);   // dim track
            }
        }
    }
}

// ── Public API ───────────────────────────────────────────────────────────────

void showMenu(MenuItem* items, int count) {
    if (!items || count <= 0) return;

    // Pause any playing GIF (caller resumes it inside onSelect if needed)
    isPlayingGif = false;

    int  currentIdx  = 0;
    bool lastState   = HIGH;
    bool btnPressed  = false;
    unsigned long pressStart = 0;
    bool longHandled = false;

    // Initial full draw: clearScreen + box + words
    drawFrame(items, count, currentIdx, 0, /*animating=*/false);

    while (true) {
        server.handleClient();

        bool btnState = (bool)digitalRead(BUTTON_PIN);

        // Falling edge - button just pressed
        if (lastState == HIGH && btnState == LOW) {
            btnPressed  = true;
            pressStart  = millis();
            longHandled = false;
        }

        // Button held - draw circular progress, confirm when full
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

        // Rising edge - button released
        if (lastState == LOW && btnState == HIGH) {
            if (btnPressed && !longHandled) {
                // Erase the partial circle before scrolling
                drawHoldFrame(0.0f);

                // Scroll animation.
                // Start at f=1: frame 0 (offset=0) is already on screen,
                // so the box is never redrawn during animation.
                int snapIdx = currentIdx;
                for (int f = 1; f <= ANIM_FRAMES; f++) {
                    drawFrame(items, count, snapIdx,
                              -(f * ANIM_STEP_PX), /*animating=*/true);
                    delay(ANIM_DELAY_MS);
                }

                currentIdx = (currentIdx + 1) % count;
                // Full redraw for a clean idle state
                drawFrame(items, count, currentIdx, 0, /*animating=*/false);
            }
            btnPressed = false;
        }

        lastState = btnState;
        delay(5);
    }
}



// ======Handling button press(SETUP MODE) ===========
void doSetup() {
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
void doWifi(){
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