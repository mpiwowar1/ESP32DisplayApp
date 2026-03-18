#include "handlebutton.h"

void handleButton(ButtonCallback onShortPress,
                  ButtonCallback onLongPress) {
  static bool lastButtonState = HIGH;
  static bool buttonPressed = false;
  static unsigned long pressStartTime = 0;
  static bool longPressHandled = false;

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
      if (onLongPress) onLongPress();
    }
  }

  // Button released
  if (lastButtonState == LOW && currentState == HIGH) {
    if (buttonPressed && !longPressHandled) {
      if (onShortPress) onShortPress();
    }
    buttonPressed = false;
  }

  lastButtonState = currentState;
}