# Plane Radar

<img width="800" height="450" alt="plane-radar" src="https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419" />

**3D printed case (STL + assembly):** [MakerWorld](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083) · **Firmware:** [Releases](https://github.com/MatixYo/ESP32-Plane-Radar/releases)

Firmware for an **ESP32-C3 Super Mini** and a **1.28″ round GC9A01** display (240×240). Shows a circular **ADS-B radar** around your configured location, with **WiFiManager** for first-time setup.

## What it does

1. **Wi‑Fi setup** (if needed) — captive portal on AP **`PlaneRadar-Setup`**
2. **Radar** — live aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid
3. **Stream gauge** — real-time CFS, stage height, and water temp from [USGS NWIS](https://waterservices.usgs.gov/) with a 24 h sparkline arc

After Wi‑Fi is saved, the device reconnects automatically; both screens update on their own timers.

## Getting started (beginners)

This is the fastest way to get the device working — no coding or software installation required.

### What you need

- **ESP32-C3 Super Mini** board
- **1.28″ round GC9A01 display** (240×240)
- USB-C cable that supports data (not charge-only)
- A computer with **Google Chrome or Microsoft Edge**

Wire the display to the ESP32 before powering it on — see the [Wiring](#wiring-gc9a01--esp32-c3-super-mini) table below.

### Step 1 — Download the firmware

Go to the [Releases](https://github.com/MatixYo/ESP32-Plane-Radar/releases) page and download the latest `plane-radar-vX.Y.Z.bin` file.

### Step 2 — Put the board in download mode

1. Plug the ESP32-C3 into your computer via USB.
2. Hold the **BOOT** button (labeled `BOOT` on the board).
3. While holding BOOT, press and release the **RESET** button (labeled `RST`).
4. Release BOOT. The board is now in download mode.

### Step 3 — Flash the firmware in your browser

1. Open **[esptool-js](https://espressif.github.io/esptool-js/)** in Chrome or Edge.
2. Click **Connect**, select your ESP32-C3 from the list, and click **Connect**.
3. Set **Flash Address** to `0x0`.
4. Click **Choose File** and select the `.bin` file you downloaded.
5. Click **Program**. Wait for it to finish (about 30 seconds).
6. Press the **RESET** button on the board to reboot into the new firmware.

### Step 4 — First-time Wi-Fi setup

On first boot the display shows a yellow setup screen.

1. On your phone or laptop, connect to the Wi-Fi network **`PlaneRadar-Setup`**.
2. A captive portal should open automatically. If not, go to **`http://192.168.4.1`** in your browser.
3. Enter your home Wi-Fi name and password, then fill in your **latitude and longitude** (search your address on [Google Maps](https://maps.google.com/), right-click the pin, and copy the coordinates).
4. Tap **Save**. The board reboots and starts showing aircraft on the radar.

That's it. After the first setup the device connects automatically every time it powers on.

---

## Controls (BOOT, GPIO 9, active LOW)

| Action | Effect |
|--------|--------|
| **Single tap** (radar mode) | Cycle range preset (5 → 10 → 15 → 25 km); saved to flash |
| **Double-tap** (radar mode) | Switch to stream gauge screen |
| **Single tap** (gauge mode) | Return to radar |
| **Hold 3 s** | Clear Wi‑Fi, location, and units; reboot into setup portal |

During setup you can also hold BOOT at power-on to force a credential reset (same as the long press).

## Wi‑Fi setup portal

1. Connect to **`PlaneRadar-Setup`**
2. Open **`http://plane-radar.local`** (preferred) or **`http://192.168.4.1`** — both are shown on the yellow setup screen; captive portal may open automatically
3. Set home Wi‑Fi, then save

mDNS hostname is configured in `config.h` as `kPortalHostname` (`plane-radar` → **plane-radar.local** on the setup AP). Some phones resolve `.local` slowly; use the IP if needed.

**Custom fields** (stored in NVS):

| Field | Purpose |
|-------|---------|
| **Latitude / Longitude** | Radar center and ADS-B query position (defaults in `config.h` until set) |
| **Display distances in miles** | Ring scale label in **mi** instead of **km** (e.g. `6mi` vs `10km`) |

After a reset, the device reboots and shows the setup screen immediately (no “Connecting” loop on stale credentials).

## Radar display

### Grid

- Dark blue background, subdued green rings and crosshairs
- White **N / S / E / W** at the bezel; range label on the **east** spoke (ring 3 = ¾ of outer radius)
- White center dot

Layout and colors: `include/ui/radar_theme.h`.

### Range presets

| Ring 3 label | Outer radius (aircraft scale) |
|------------|-------------------------------|
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Preset and miles/km choice persist across reboot (`planeradar` NVS namespace).

### Aircraft

- **Inside the outer ring** — red heading triangle, magenta speed vector (clipped at the ring), callsign / type / altitude tags
- **Outside the ring** (still within ADS-B fetch) — small **red dot on the screen rim** at the correct bearing (direction cue; not distance-accurate past the ring)
- **Tags** — placed toward the **center**: west (left) → tag on the **right** of the symbol; east (right) → tag on the **left**

As range decreases (or aircraft approach), targets move inward; beyond-ring dots become full symbols when they cross the outer ring.

### ADS-B

- Source: `https://opendata.adsb.fi/api/v3/`
- Fetch radius: `ui::radar::fetchRadiusKm()` — scales with the active preset to roughly the screen edge (so rim dots have data)
- Poll interval: `kAdsbFetchIntervalMs` (5 s) in `config.h`
- Ground aircraft hidden by default (`kAdsbShowGroundAircraft`)

## Stream gauge screen

- **Display**: large CFS in the center, stage height (ft) and water temperature (°F) below, a 270° sparkline arc around the bezel showing the last 24 h of flow (colored dim-blue → bright-cyan by relative level)
- **Source**: [USGS NWIS Instantaneous Values](https://waterservices.usgs.gov/) — free, no API key
- **Poll interval**: every 2 minutes (`kGaugeFetchIntervalMs`); USGS updates every 15 minutes
- **Find your gauge ID**: visit [waterdata.usgs.gov/nwis/rt](https://waterdata.usgs.gov/nwis/rt), locate your nearest stream gauge on the map, and copy the site number (e.g. `01646500`)
- **Set it**: change `kUsgsGaugeId` in `include/config.h`

## Configuration

Edit **`include/config.h`** for hardware and behavior:

| Area | Keys / notes |
|------|----------------|
| Portal | `kPortalApName`, `kPortalIp`, `kPortalHostname` / `kPortalHostUrl` (mDNS; needs `-DWM_MDNS` in `platformio.ini`) |
| Wi‑Fi timing | connect attempts, reconnect grace, portal timeout (`0` = no timeout) |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| Display SPI | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |
| Stream gauge | `kUsgsGaugeId`, `kGaugeFetchIntervalMs`, `kGaugeHistoryCount` |

Range presets: `include/ui/radar_range.h` (`kRangePresets`).

## Project layout

```
include/
  config.h
  hardware/
    lgfx_config.hpp
    display.h
    display_font.h
  ui/
    radar_theme.h
    radar_range.h
    radar_display.h
    gauge_display.h
    status_screens.h
  services/
    wifi_setup.h
    radar_location.h
    adsb_client.h
    usgs_client.h
data/
  ui_font.vlw              — embedded smooth UI font (Noto Sans Bold)
lib/
  radar_math/              — header-only pure math/parsing (host-testable)
src/
  main.cpp
  hardware/
  ui/
  services/
test/
  test_radar_math/         — Unity native tests (pio test -e native)
```

## Wiring (GC9A01 ↔ ESP32-C3 Super Mini)

| Display | ESP32-C3 |
|---------|----------|
| VCC | 3V3 |
| GND | GND |
| RST | GPIO **0** |
| CS | GPIO **1** |
| DC | GPIO **10** |
| SDA (MOSI) | GPIO **3** |
| SCL (SCLK) | GPIO **4** |
| BOOT (user) | GPIO **9** |

## Build

```bash
pio run -t upload
pio device monitor
```

- PlatformIO env: **`supermini`**
- Serial: **115200** baud
- USB CDC on boot enabled in `platformio.ini` for the Super Mini

### Web-flashable release image

Single `.bin` for [esptool-js](https://espressif.github.io/esptool-js/) and similar tools (ESP32-C3, 4 MB, flash at **0x0**):

```bash
chmod +x scripts/merge-firmware.sh   # once
./scripts/merge-firmware.sh
```

Writes `release/plane-radar-merged.bin`. Skip rebuild if firmware is already built:

```bash
./scripts/merge-firmware.sh --no-build
```

Or via PlatformIO only (output: `.pio/build/supermini/firmware-merged.bin`):

```bash
pio run -e supermini
pio run -t merge -e supermini
```

Put the board in download mode (hold **BOOT**, tap **RESET**), then flash with Chrome/Edge over USB.

### CI and releases (GitHub Actions)

| Workflow | When | Output |
|----------|------|--------|
| [Build](.github/workflows/build.yml) | Push / PR to `main` | Artifact `plane-radar-supermini` (merged + split `.bin` files, ~90 days) |
| [Release](.github/workflows/release.yml) | Git tag `v*` (e.g. `v1.0.0`) | GitHub Release asset `plane-radar-v1.0.0.bin` + `.sha256` |

To ship a version users can download:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The release workflow builds firmware in CI and attaches the merged image to the release. Download from **Releases** on GitHub, then flash at **0x0** (ESP32-C3, 4 MB).

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
