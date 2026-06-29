/**
 * @file drawing.cpp
 * @brief GIF frame drawing callback for the AnimatedGIF library.
 */

#include "drawing.h"

/**
 * @brief Draw one scanline of a GIF frame onto the LED matrix.
 *
 * Handles transparency and the "restore to background" disposal mode.
 *
 * @param pDraw Frame data from the GIF decoder.
 */
void GIFDraw(GIFDRAW* pDraw) {
    uint8_t*  s;
    uint16_t* d;
    uint16_t* usPalette;
    int x, y;

    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y;

    if (y >= PANEL_HEIGHT || pDraw->iX >= PANEL_WIDTH)
        return;

    s = pDraw->pPixels;

    /* Disposal method 2: fill transparent pixels with background colour */
    if (pDraw->ucDisposalMethod == 2) {
        for (x = 0; x < pDraw->iWidth; x++) {
            if (s[x] == pDraw->ucTransparent) {
                s[x] = pDraw->ucBackground;
            }
        }
        pDraw->ucHasTransparency = 0;
    }

    /* Draw pixels, skipping transparent ones if needed */
    if (pDraw->ucHasTransparency) {
        uint8_t c, ucTransparent = pDraw->ucTransparent;
        for (x = 0; x < pDraw->iWidth; x++) {
            c = s[x];
            if (c != ucTransparent) {
                uint16_t color = usPalette[c];
                int px = pDraw->iX + x;
                if (px < PANEL_WIDTH && y < PANEL_HEIGHT) {
                    dma_display->drawPixel(px, y, color);
                }
            }
        }
    } else {
        for (x = 0; x < pDraw->iWidth; x++) {
            uint16_t color = usPalette[s[x]];
            int px = pDraw->iX + x;
            if (px < PANEL_WIDTH && y < PANEL_HEIGHT) {
                dma_display->drawPixel(px, y, color);
            }
        }
    }
}
