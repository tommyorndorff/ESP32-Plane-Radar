# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash

```bash
# Build firmware
pio run -e supermini

# Build and flash to device
pio run -t upload

# Open serial monitor (115200 baud)
pio device monitor

# Build merged web-flash image (outputs .pio/build/supermini/firmware-merged.bin)
pio run -e supermini
pio run -t merge -e supermini

# Build and produce release/plane-radar-merged.bin
./scripts/merge-firmware.sh

# Skip rebuild if firmware already built
./scripts/merge-firmware.sh --no-build
```

There are no unit tests — this is embedded firmware; validation is done by flashing and observing the device.

## Architecture

The firmware follows a layered namespace structure:

- **`config` namespace** (`include/config.h`) — compile-time constants for all hardware pins, timing, and behavior tuning. This is the single place to change hardware configuration.

- **`hardware/` layer** — LovyanGFX display driver (`lgfx_config.hpp` defines the `LGFX` class for GC9A01 over SPI; `display.h`/`display.cpp` expose `displayInit()`). Note: `bootButtonInit()` and all BOOT button ISR/polling code live in `src/services/wifi_setup.cpp`, not in the hardware layer.

- **`services/` layer** — network services with namespaces:
  - `services::adsb` — fetches aircraft from `opendata.adsb.fi/api/v3/`; stores up to 64 `Aircraft` structs in a static buffer
  - `services::location` — reads/writes radar center lat/lon from NVS (`planeradar` namespace)
  - `wifi_setup` — wraps WiFiManager for captive portal setup

- **`ui/` layer** — display rendering:
  - `ui::radar` — range preset management; persists to NVS; `fetchRadiusKm()` scales fetch area to the screen edge
  - `ui::radarDisplayDraw()` — draws the full sonar grid; caches it to a sprite for fast refresh
  - `ui::radarDisplayRefreshAircraft()` — blits cached grid then redraws only aircraft (avoids full clear)
  - `status_screens` — yellow/black status overlays (portal, connecting, etc.)

- **`main.cpp`** — Arduino `setup()`/`loop()` wiring only; no logic beyond the state machine for WiFi recovery and ADS-B poll timing.

### Key design points

- The radar grid is rendered once into a sprite; aircraft refreshes blit the sprite then draw on top — this is why `radarDisplayRefreshAircraft()` is separate from `radarDisplayDraw()`.
- Three NVS namespaces are used deliberately to avoid handle conflicts: `planeradar` (range preset index, miles/km preference), `radar` (lat/lon), and `wifi` (force-portal flag set before reboot on credential reset).
- `ui::radar::fetchRadiusKm()` intentionally fetches beyond the visible outer ring so that aircraft just outside the ring still appear as rim dots.
- WiFiManager custom parameters (lat, lon, units) are read back in `services::location::init()` and `ui::radar::rangeInit()` after the portal saves them.
- Aircraft are drawn far-first (insertion sort by distance²) so nearer aircraft render on top of farther ones.
- The GC9A01 panel uses BGR order (`kDisplayRgbOrder = true`). `initPalette()` in `radar_display.cpp` compensates by swapping R↔B when calling `color565()` for aircraft colors — changing any color needs to account for this swap.
- `data/ui_font.vlw` is a pre-built binary (Noto Sans Bold 15, generated from TFT_eSPI smooth-font tooling). There is no script in this repo to regenerate it. The display code gracefully falls back to GFX bitmap fonts (`FreeSansBold12pt7b` / `FreeSansBold9pt7b`) if the VLW font fails to load.
- The ADS-B HTTPS client uses `client.setInsecure()` — TLS is used for transport encryption but certificate verification is skipped.
- The merged `.bin` (bootloader + partition table + firmware at correct offsets) is what users flash via esptool-js at address `0x0`.

## CI & Releases

- **Build workflow** triggers on push/PR to `main`; artifacts expire in ~90 days.
- **Release workflow** triggers on `v*` tags; produces `plane-radar-vX.Y.Z.bin` as a GitHub Release asset.

```bash
git tag v1.0.0
git push origin v1.0.0
```
