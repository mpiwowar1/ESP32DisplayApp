/**
 * @file handlebutton.h
 * @brief Simple short/long press detection for one button.
 */

#pragma once
#include "globals.h"

/** @brief Function pointer type for button callbacks. */
typedef void (*ButtonCallback)();

/**
 * @brief Check the button and fire the right callback.
 *
 * Call this every loop iteration. It tracks state internally.
 *
 * @param onShortPress Called on a quick tap (may be NULL).
 * @param onLongPress  Called after holding >= LONG_PRESS_TIME (may be NULL).
 */
void handleButton(ButtonCallback onShortPress, ButtonCallback onLongPress);