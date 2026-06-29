/**
 * @file menu.h
 * @brief Scrolling selection menu for the 64x64 LED matrix.
 *
 * Short press scrolls to the next item, long press confirms.
 * The menu blocks until a selection is made.
 */

#pragma once
#include "globals.h"
#include "font.h"
#include <math.h>
#include "network.h"

/** @brief Callback type for when a menu item is selected. */
typedef void (*MenuItemCallback)();

/** @brief One entry in the menu list. */
struct MenuItem {
    const char*      label;     ///< Text shown on the display (capitals work best).
    MenuItemCallback onSelect;  ///< Function called when this item is confirmed.
};

/**
 * @brief Show a scrolling menu and wait for the user to pick an item.
 *
 * @param items  Array of MenuItem structs.
 * @param count  How many items are in the array.
 */
void showMenu(MenuItem* items, int count);

/** @brief Enter setup mode: start AP, serve config page, format memory. */
void doSetup();

/** @brief Check WiFi status, reconnect if needed, show info on display. */
void doWifi();