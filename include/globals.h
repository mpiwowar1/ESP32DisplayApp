/**
 * @file globals.h
 * @brief Shared defines, pin mappings, and global variables.
 */

#pragma once
#include <AnimatedGIF.h>
#include <WebServer.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/** @brief Default AP name used during setup mode. */
extern const char* ssid;

/* --- HUB75 data pins --- */
#define R1_PIN  25
#define G1_PIN  26
#define B1_PIN  27
#define R2_PIN  14
#define G2_PIN  12
#define B2_PIN  13

/* --- HUB75 address / control pins --- */
#define A_PIN   19
#define B_PIN   18
#define C_PIN    5
#define D_PIN   17
#define E_PIN   16
#define LAT_PIN 22
#define OE_PIN   4
#define CLK_PIN 23

/** @brief GPIO for the physical button. */
#define BUTTON_PIN 15

/* --- Panel size --- */
#define PANEL_WIDTH   64
#define PANEL_HEIGHT  64
#define CHAIN_LENGTH   1

/* --- SPIFFS file paths --- */
#define STORED_FILE       "/current.dat"
#define TYPE_FILE         "/type.txt"
#define STORED_WIFI       "/cred.txt"
#define STORED_BRIGHTNESS "/brightness.txt"
#define STORED_SPEED      "/speed.txt"

/* --- Shared runtime objects --- */
extern MatrixPanel_I2S_DMA* dma_display;
extern WebServer server;
extern AnimatedGIF gif;
extern String currentType;
extern bool isPlayingGif;
extern uint8_t* gifData;
extern size_t gifSize;
extern uint8_t brightness;
extern float gifSpeed;

/** @brief How long the button must be held for a long press (ms). */
constexpr unsigned long LONG_PRESS_TIME = 1200;

/* --- Embedded web page served at "/" --- */
const char HTML_PAGE[] PROGMEM =
"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>LED Matrix</title><style>"
"body{font-family:Arial;background:#667eea;color:#fff;padding:20px;margin:0;text-align:center}"
".box{max-width:500px;margin:0 auto;background:rgba(255,255,255,0.1);padding:20px;border-radius:10px}"
"h1{margin:0 0 20px}"
".info{background:rgba(0,0,0,0.2);padding:15px;border-radius:8px;margin:15px 0;font-size:14px}"
".highlight{background:rgba(76,175,80,0.3);padding:10px;border-radius:5px;margin:10px 0}"
"code{background:rgba(0,0,0,0.3);padding:2px 6px;border-radius:3px}"
".btn{background:#764ba2;color:#fff;border:none;padding:12px 20px;border-radius:8px;margin:5px;cursor:pointer}"
".btn:hover{background:#8b5cb8}"
"</style></head><body><div class='box'>"
"<h1>LED Matrix 64x64</h1>"
"<div class='highlight'><strong>✓ Content persists after power off!</strong></div>"
"<div class='info'><strong>How to use:</strong><br>"
"1. Connect to this WiFi once<br>"
"2. Send image or GIF from your app<br>"
"3. It stays forever (even after unplugging!)<br>"
"4. Disconnect and enjoy<br></div>"
"<div class='info'><strong>API Endpoints:</strong><br>"
"POST <code>/upload</code> - Send image/GIF<br>"
"POST <code>/delete</code> - Clear display<br>"
"GET <code>/status</code> - Check what's stored<br></div>"
"<div class='info'><strong>Data formats:</strong><br>"
"• Images: RGB565 (8192 bytes) or RGB888 (12288 bytes)<br>"
"• GIFs: Standard GIF file (max ~100KB)<br></div>"
"<button class='btn' onclick='location.reload()'>Refresh</button>"
"<button class='btn' onclick='clearDisplay()'>Clear Display</button>"
"<script>function clearDisplay(){fetch('/delete',{method:'POST'}).then(()=>alert('Display cleared!'))}</script>"
"</div></body></html>";
