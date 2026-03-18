#pragma once
#include "globals.h"

typedef void (*ButtonCallback)();

void handleButton(ButtonCallback onShortPress,ButtonCallback onLongPress);