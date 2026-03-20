#pragma once
#include "globals.h"   // pulls in dma_display, BUTTON_PIN, LONG_PRESS_TIME, etc.
#include "font.h"   // PROGMEM 6×8 font + menuFontPixel()
#include <math.h>
#include "network.h"

/*
 * ═══════════════════════════════════════════════════════════════
 *  menu.h  –  Scrolling selection menu for the 64×64 LED matrix
 * ═══════════════════════════════════════════════════════════════
 *
 *  Behaviour (mirrors the Python simulator in main.py):
 *
 *   ┌──────────────────────────────────────────┐
 *   │            (dim)  CLOSE                  │  ← previous item
 *   │  ╔══════════════════════════════════╗    │
 *   │  ║           WIFI                  ║    │  ← selected item (white box)
 *   │  ╚══════════════════════════════════╝    │
 *   │            (dim)  SETUP                  │  ← next item
 *   └──────────────────────────────────────────┘
 *
 *  Controls (uses the existing BUTTON_PIN / LONG_PRESS_TIME globals):
 *
 *   Short press  →  scroll UP to the next item  (animated, 20 frames)
 *   Long  press  →  confirm selection           (calls onSelect callback)
 *
 *  Usage example (add to main.cpp):
 *
 *    #include "menu.h"
 *
 *    void doClose() { loadAndDisplayStored(); }
 *    void doWifi()  { HandleShortButtonPress(); }
 *    void doSetup() { HandleLongButtonPress();  }
 *
 *    MenuItem mainMenu[] = {
 *        { "CLOSE", doClose },
 *        { "WIFI",  doWifi  },
 *        { "SETUP", doSetup },
 *    };
 *
 *    // Call from loop() or a button handler:
 *    showMenu(mainMenu, 3);
 *
 *  showMenu() blocks until the user confirms a selection, then returns.
 *  While it is running it still calls server.handleClient() so the web
 *  server stays responsive.
 */

// ── Data types ───────────────────────────────────────────────────────────────

/** Callback signature: called when the user long-presses on an item. */
typedef void (*MenuItemCallback)();

/** One row in the menu. */
struct MenuItem {
    const char*      label;     ///< Text shown on the display (capitals look best)
    MenuItemCallback onSelect;  ///< Function called when this item is confirmed
};

// ── Public API ───────────────────────────────────────────────────────────────

/**
 * Show a scrolling menu and block until the user selects an item.
 *
 * @param items   Array of MenuItem structs.
 * @param count   Number of elements in the array.
 *
 * The currently displayed GIF (if any) is paused while the menu is open and
 * NOT automatically resumed – do that inside your onSelect callback if needed.
 */
void showMenu(MenuItem* items, int count);



// ======Handling button press(SETUP MODE) ===========
void doSetup();

// ========= RECONNECT WIFI IF NOT CONNECTED PLUS INFO ===========
void doWifi();