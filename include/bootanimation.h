#pragma once
#include "globals.h"

void displayBootAnimation() {
    Serial.println("Boot animation...");

    const int   CX          = 32;
    const int   CY          = 32;
    const int   FRAMES      = 35;       // total frames – keep it snappy
    const float MAX_DIST    = 46.0f;    // rough radius to screen corner
    const float RING_WIDTH  = 10.0f;    // how thick the leading ring is
    const float SPIRAL_WRAP = 0.18f;    // how tightly the arms curl
    const float SPIN_SPEED  = 0.30f;    // rotation per frame (radians)

    for (int frame = 0; frame < FRAMES; frame++) {
        float progress  = (float)frame / (FRAMES - 1);          // 0 → 1
        float ringFront = MAX_DIST * (1.0f - progress);         // shrinks edge→centre

        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                float dx   = x - CX;
                float dy   = y - CY;
                float dist = sqrtf(dx * dx + dy * dy);
                float ang  = atan2f(dy, dx);

                // Spiral phase: pixels further out are rotated more,
                // plus the whole vortex spins as frames advance.
                float phase = ang + dist * SPIRAL_WRAP - frame * SPIN_SPEED;

                // Intensity: bright at the leading ring, fades behind it.
                float ringDelta = dist - ringFront;             // >0 = outside ring (not yet lit)
                float intensity;
                if (ringDelta > 0.0f) {
                    intensity = 0.0f;                           // ahead of the wave = dark
                } else {
                    intensity = expf(ringDelta / RING_WIDTH);   // exponential fade behind
                }

                // Add a soft swirl shimmer on top of the base intensity.
                float shimmer = 0.5f + 0.5f * sinf(phase * 3.0f);
                float bright  = intensity * (0.6f + 0.4f * shimmer);

                // Toned, cool palette: teal → blue → purple hues.
                // Keeping max channel values low (~180) avoids eye-searing brightness.
                uint8_t r = (uint8_t)(bright * (50  + 40  * sinf(phase + 4.0f)));
                uint8_t g = (uint8_t)(bright * (120 + 60  * sinf(phase + 2.0f)));
                uint8_t b = (uint8_t)(bright * (160 + 60  * sinf(phase)));

                dma_display->drawPixelRGB888(x, y, r, g, b);
            }
        }

        delay(20);  // ~50 fps cap; total ~700 ms for the full animation
    }

    // Fade out quickly rather than hard-cutting to black
    for (int fade = 8; fade >= 0; fade--) {
        float f = (float)fade / 8.0f;
        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                float dx = x - CX, dy = y - CY;
                float dist = sqrtf(dx*dx + dy*dy);
                float ang  = atan2f(dy, dx);
                float phase = ang + dist * SPIRAL_WRAP;
                uint8_t r = (uint8_t)(f * (50  + 40  * sinf(phase + 4.0f)));
                uint8_t g = (uint8_t)(f * (120 + 60  * sinf(phase + 2.0f)));
                uint8_t b = (uint8_t)(f * (160 + 60  * sinf(phase)));
                dma_display->drawPixelRGB888(x, y, r, g, b);
            }
        }
        delay(18);
    }

    dma_display->clearScreen();
}
