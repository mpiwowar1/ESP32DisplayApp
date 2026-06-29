/**
 * @file bootanimation.h
 * @brief Spiral vortex boot animation for the LED matrix.
 */

#pragma once
#include "globals.h"

/**
 * @brief Play a swirling spiral animation, then fade to black.
 *
 * Takes about 700ms for the spiral plus a short fade-out.
 */
void displayBootAnimation() {
    Serial.println("Boot animation...");

    const int   CX          = 32;
    const int   CY          = 32;
    const int   FRAMES      = 35;
    const float MAX_DIST    = 46.0f;
    const float RING_WIDTH  = 10.0f;
    const float SPIRAL_WRAP = 0.18f;
    const float SPIN_SPEED  = 0.30f;

    /* Main spiral animation */
    for (int frame = 0; frame < FRAMES; frame++) {
        float progress  = (float)frame / (FRAMES - 1);
        float ringFront = MAX_DIST * (1.0f - progress);

        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                float dx   = x - CX;
                float dy   = y - CY;
                float dist = sqrtf(dx * dx + dy * dy);
                float ang  = atan2f(dy, dx);

                float phase = ang + dist * SPIRAL_WRAP - frame * SPIN_SPEED;

                float ringDelta = dist - ringFront;
                float intensity;
                if (ringDelta > 0.0f) {
                    intensity = 0.0f;
                } else {
                    intensity = expf(ringDelta / RING_WIDTH);
                }

                float shimmer = 0.5f + 0.5f * sinf(phase * 3.0f);
                float bright  = intensity * (0.6f + 0.4f * shimmer);

                uint8_t r = (uint8_t)(bright * (50  + 40  * sinf(phase + 4.0f)));
                uint8_t g = (uint8_t)(bright * (120 + 60  * sinf(phase + 2.0f)));
                uint8_t b = (uint8_t)(bright * (160 + 60  * sinf(phase)));

                dma_display->drawPixelRGB888(x, y, r, g, b);
            }
        }

        delay(20);
    }

    /* Fade out to black */
    for (int fade = 8; fade >= 0; fade--) {
        float f = (float)fade / 8.0f;
        for (int x = 0; x < 64; x++) {
            for (int y = 0; y < 64; y++) {
                float dx = x - CX, dy = y - CY;
                float dist = sqrtf(dx * dx + dy * dy);
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
