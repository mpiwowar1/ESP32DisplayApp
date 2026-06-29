/**
 * @file handlebutton.cpp
 * @brief Debounced button handler with short and long press detection.
 */

#include "handlebutton.h"

/**
 * @brief Poll the button and call the right callback.
 *
 * Uses static variables to track state between calls.
 * Short press fires on release, long press fires while held.
 *
 * @param onShortPress Callback for a quick tap (can be NULL).
 * @param onLongPress  Callback when held >= LONG_PRESS_TIME (can be NULL).
 */
void handleButton(ButtonCallback onShortPress,
                  ButtonCallback onLongPress) {
    static bool lastButtonState   = HIGH;
    static bool buttonPressed     = false;
    static unsigned long pressStartTime = 0;
    static bool longPressHandled  = false;

    bool currentState = digitalRead(BUTTON_PIN);

    /* Button just pressed down */
    if (lastButtonState == HIGH && currentState == LOW) {
        buttonPressed    = true;
        pressStartTime   = millis();
        longPressHandled = false;
    }

    /* Button is being held */
    if (buttonPressed && currentState == LOW) {
        if (!longPressHandled &&
            millis() - pressStartTime >= LONG_PRESS_TIME) {
            longPressHandled = true;
            if (onLongPress) onLongPress();
        }
    }

    /* Button just released */
    if (lastButtonState == LOW && currentState == HIGH) {
        if (buttonPressed && !longPressHandled) {
            if (onShortPress) onShortPress();
        }
        buttonPressed = false;
    }

    lastButtonState = currentState;
}