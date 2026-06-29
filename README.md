# ESP32 LED Matrix Display (64×64)

A firmware for the ESP32 that drives a 64×64 HUB75 LED matrix panel. Upload images and GIFs over WiFi from your phone or computer — they get saved to flash and survive power cycles.

---

## Features

- **Persistent storage** — uploaded content is saved to SPIFFS (on-chip flash). Unplug the board, plug it back in, and the last image or GIF is still there.
- **WiFi control** — connect the ESP32 to your home WiFi network and upload content from any device on the same network.
- **Setup mode (AP)** — if no WiFi is configured, the ESP32 creates its own access point (`SetUPMatrix`) so you can connect directly and set things up.
- **Static images** — supports raw RGB565 (8192 bytes) and RGB888 (12288 bytes) formats for a 64×64 pixel image.
- **Animated GIFs** — upload standard GIF files (up to ~100KB) and they play in a loop on the display.
- **Adjustable brightness** — change display brightness over the API. The setting is saved to flash.
- **Adjustable GIF speed** — slow down GIF playback from 0% to 100% speed. Also saved to flash.
- **On-device menu** — a physical button opens a scrolling menu on the display itself, letting you check WiFi status, enter setup mode, or format memory without needing a computer.
- **Boot animation** — a swirling spiral vortex animation plays every time the board powers on.
- **mDNS** — once connected to WiFi, the board is reachable at `http://matrix.local` (on supported devices).
- **Built-in web page** — navigate to the board's IP in a browser to see a simple control page with status info and a clear button.

---

## Hardware

### Parts needed

| Part | Details |
|---|---|
| **ESP32 dev board** | Any ESP32-WROOM-32 based board (e.g. ESP32 DevKit v1) |
| **HUB75 LED matrix panel** | 64×64 pixels, 1/32 scan, with E pin support |
| **5V power supply** | At least 4A recommended for a 64×64 panel at full brightness |
| **Push button** | Momentary, normally open |
| **Wires** | For connecting the button and panel |

### Wiring

#### HUB75 Panel → ESP32

| Signal | ESP32 GPIO |
|--------|-----------|
| R1     | 25        |
| G1     | 26        |
| B1     | 27        |
| R2     | 14        |
| G2     | 12        |
| B2     | 13        |
| A      | 19        |
| B      | 18        |
| C      | 5         |
| D      | 17        |
| E      | 16        |
| LAT    | 22        |
| OE     | 4         |
| CLK    | 23        |

#### Button

| Pin | ESP32 GPIO |
|-----|-----------|
| One leg | GPIO 15 |
| Other leg | GND |

The button uses the internal pull-up resistor, so no external resistor is needed.

---

## Building & Flashing

This project uses [PlatformIO](https://platformio.org/).

### With PlatformIO CLI

```bash
# Build
pio run

# Upload to the board
pio run --target upload

# Open serial monitor
pio device monitor --baud 115200
```
### Dependencies

These are pulled automatically by PlatformIO from `platformio.ini`:

- [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA) — DMA-driven HUB75 panel driver
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) v1.12.4+ — graphics primitives (text, shapes)
- [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF) v2.1.0+ — GIF decoder

---

## How It Works

### Boot sequence

1. Mount SPIFFS (formats on first boot).
2. Set up the LED matrix with DMA.
3. Load saved brightness and GIF speed from flash.
4. Play the boot animation.
5. Try to connect to saved WiFi credentials.
6. Show connection status on the display for 2 seconds.
7. Load and display the last uploaded content.
8. Enter the main loop.

### Main loop

- Serves HTTP requests from the web server.
- Polls the physical button for short/long presses.
- Advances GIF animation frames (if a GIF is playing).

### Physical button

| Action | Result |
|--------|--------|
| **Long press** (hold 1.2s) | Opens the on-screen menu |
| **Short press** (in menu) | Scroll to next menu item |
| **Long press** (in menu) | Confirm selected item |

### On-screen menu

The menu shows three options:

| Item | What it does |
|------|-------------|
| **CLOSE** | Close the menu and go back to displaying stored content |
| **WI-FI** | Show WiFi connection info (SSID, IP). Reconnects if disconnected |
| **SETUP** | Enter setup mode (starts AP, allows credential setup and memory formatting) |

In **setup mode**:
- The ESP32 creates its own WiFi network (`SetUPMatrix`).
- The display shows the AP name and IP address.
- You can connect to it and use the API to upload content or set WiFi credentials.
- **Short press** exits setup and reconnects to the stored WiFi.
- **Long press** formats the entire SPIFFS memory (erases everything).

---

## API Reference

All endpoints are served on port 80. When connected to your home WiFi, use the board's IP or `http://matrix.local`.

### `GET /`

Returns the built-in HTML control page.

### `GET /status`

Returns storage info as JSON.

```json
{
  "type": "gif",
  "stored": true,
  "size": 45320,
  "storage_used": 53248,
  "storage_total": 1507328
}
```

| Field | Description |
|-------|-------------|
| `type` | `"image"`, `"gif"`, or `"none"` |
| `stored` | Whether a file exists in storage |
| `size` | Size of the stored file in bytes |
| `storage_used` | Total SPIFFS bytes used |
| `storage_total` | Total SPIFFS capacity |

### `POST /upload`

Upload an image or GIF file. Send the file as a multipart form upload.

```bash
curl -X POST -F "file=@mygif.gif" http://matrix.local/upload
```

**Supported formats:**
- **RGB565 raw** — exactly 8192 bytes (2 bytes per pixel, 64×64), big-endian.
- **RGB888 raw** — exactly 12288 bytes (3 bytes per pixel, 64×64).
- **GIF** — standard GIF file, recommended max ~100KB (limited by ESP32 RAM).

The file type is auto-detected from the first 3 bytes (`GIF` magic = GIF, anything else = image).

### `POST /delete`

Clear the display and delete the stored content.

```bash
curl -X POST http://matrix.local/delete
```

### `POST /cred`

Save new WiFi credentials. The ESP32 will use these on the next boot.

```bash
curl -X POST -d "ssid=MyNetwork&password=MyPassword" http://matrix.local/cred
```

### `GET /getbrightness`

Returns current brightness as JSON.

```json
{ "brightness": "90" }
```

### `POST /postbrightness`

Set display brightness (0–255).

```bash
curl -X POST -d "brightness=120" http://matrix.local/postbrightness
```

### `GET /getspeed`

Returns current GIF playback speed as JSON.

```json
{ "speed": "0.50" }
```

### `POST /postspeed`

Set GIF playback speed (0.0 = paused, 1.0 = full speed).

```bash
curl -X POST -d "speed=0.5" http://matrix.local/postspeed
```

---

## Project Structure

```
ESP32DisplayApp/
├── platformio.ini          # Build config, dependencies, board settings
├── README.md
├── include/
│   ├── api.h               # HTTP endpoint declarations
│   ├── bootanimation.h     # Boot-up spiral animation (runs from header)
│   ├── drawing.h           # GIF frame render callback
│   ├── font.h              # 6×8 bitmap font in PROGMEM
│   ├── globals.h           # Pin defines, constants, global variables, HTML page
│   ├── handlebutton.h      # Button callback types
│   ├── memory.h            # Flash read/write declarations
│   ├── menu.h              # Menu data types and declarations
│   └── network.h           # WiFi and server setup declarations
├── src/
│   ├── main.cpp            # setup() and loop()
│   ├── api.cpp             # HTTP endpoint implementations
│   ├── drawing.cpp         # GIF line-by-line rendering
│   ├── globals.cpp         # Global variable definitions
│   ├── handlebutton.cpp    # Short/long press detection
│   ├── memory.cpp          # SPIFFS read/write for images, GIFs, settings
│   ├── menu.cpp            # Scrolling menu, setup mode, WiFi check
│   └── network.cpp         # WiFi AP/STA setup, web server routes
├── lib/                    # (empty, dependencies come from platformio.ini)
└── test/                   # (empty)
```

### Flash storage files (SPIFFS)

| File | Contents |
|------|----------|
| `/current.dat` | The uploaded image or GIF binary data |
| `/type.txt` | Content type: `"image"` or `"gif"` |
| `/wifi.txt` | Saved WiFi credentials (`ssid=...` and `password=...`) |
| `/brightness.txt` | Saved brightness value (`brightness=90`) |
| `/speed.txt` | Saved GIF speed value (`speed=1.00`) |

---

## Partition Table

Uses `huge_app.csv` to give the firmware maximum flash space. This is set in `platformio.ini`:

```ini
board_build.partitions = huge_app.csv
```

This leaves roughly 1.5MB for SPIFFS storage.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Display stays black | Check the 5V power supply. HUB75 panels need a lot of current. |
| GIF won't play | Make sure the file is under ~100KB. The ESP32 loads the entire GIF into RAM. |
| Can't connect to WiFi | Enter setup mode (long press → SETUP), connect to the AP, and re-send credentials via `POST /cred`. |
| Upload fails | Check SPIFFS space via `GET /status`. You may need to format memory (long press in setup mode). |
| mDNS not working | `matrix.local` only works on devices that support mDNS (most phones and computers do, some routers don't). Use the IP address instead. |
| Panel shows wrong colors | Double-check the HUB75 wiring. One swapped pin will mess up the colors. |

---
