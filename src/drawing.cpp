#include "drawing.h"


// ========== GIF DRAW CALLBACK ==========
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;
  
  if (y >= PANEL_HEIGHT || pDraw->iX >= PANEL_WIDTH) 
    return;
    
  s = pDraw->pPixels;
  
  if (pDraw->ucDisposalMethod == 2) {
    for (x = 0; x < pDraw->iWidth; x++) {
      if (s[x] == pDraw->ucTransparent) {
        s[x] = pDraw->ucBackground;
      }
    }
    pDraw->ucHasTransparency = 0;
  }
  
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
