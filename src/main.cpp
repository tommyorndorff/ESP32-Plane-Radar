/**
 * Plane Radar — WiFi setup, then radar UI on the round GC9A01 display.
 * BOOT short-tap: cycle display mode (radar → gauge → radar …)
 *                 or cycle radar range when in radar mode.
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "services/usgs_client.h"
#include "services/wifi_setup.h"
#include "ui/gauge_display.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/status_screens.h"

namespace {

enum class DisplayMode { kRadar, kGauge };

DisplayMode g_mode = DisplayMode::kRadar;
bool g_screen_visible = false;
unsigned long g_wifi_down_since = 0;
unsigned long g_last_reconnect_ms = 0;
unsigned long g_last_adsb_fetch_ms = 0;
unsigned long g_last_gauge_fetch_ms = 0;

bool wifiUp() { return WiFi.status() == WL_CONNECTED; }

void drawCurrentMode() {
  if (!wifiUp()) {
    g_screen_visible = false;
    return;
  }
  if (g_mode == DisplayMode::kRadar) {
    ui::radarDisplayDraw();
  } else {
    ui::gaugeDisplayDraw();
  }
  g_screen_visible = true;
}

void onBootTap() {
  if (g_mode == DisplayMode::kRadar) {
    // First tap in radar mode cycles range, same as before.
    ui::radar::rangeNext();
    char label[12];
    ui::radar::formatCurrentRing3Label(label, sizeof(label));
    Serial.printf("Range: %s (outer ~%.0f km)\n", label,
                  ui::radar::rangeCurrent().outer_km);
    if (g_screen_visible && wifiUp()) {
      ui::radarDisplayDraw();
    }
  } else {
    // In gauge mode, tap returns to radar.
    g_mode = DisplayMode::kRadar;
    Serial.println("Mode: radar");
    if (wifiUp()) {
      ui::radarDisplayDraw();
      g_screen_visible = true;
    }
  }
}

// Long-press held for kBootResetHoldMs → stays as-is (resets WiFi, handled in wifi_setup).
// Double-tap (two taps within ~500 ms) → switch to gauge mode from radar.
unsigned long g_last_tap_ms = 0;
constexpr unsigned long kDoubleTapWindowMs = 500UL;

void handleBootButton() {
  bootButtonPollLongPress();
  if (!bootButtonConsumeTap()) {
    return;
  }

  const unsigned long now = millis();
  const bool double_tap =
      g_mode == DisplayMode::kRadar &&
      g_last_tap_ms != 0 &&
      (now - g_last_tap_ms) < kDoubleTapWindowMs;
  g_last_tap_ms = now;

  if (double_tap) {
    g_mode = DisplayMode::kGauge;
    Serial.println("Mode: gauge");
    if (wifiUp()) {
      ui::gaugeDisplayDraw();
      g_screen_visible = true;
      // Trigger an immediate fetch so the display isn't empty.
      if (services::usgs::fetchUpdate()) {
        ui::gaugeDisplayRefresh();
      }
      g_last_gauge_fetch_ms = millis();
    }
  } else {
    onBootTap();
  }
}

void fetchAndDrawAircraft() {
  const float fetch_km = ui::radar::fetchRadiusKm();
  if (!services::adsb::fetchUpdate(services::location::lat(),
                                   services::location::lon(), fetch_km)) {
    handleBootButton();
    return;
  }
  ui::radarDisplayRefreshAircraft();
  handleBootButton();
}

void fetchAndRefreshGauge() {
  if (!services::usgs::fetchUpdate()) {
    return;
  }
  if (g_screen_visible && g_mode == DisplayMode::kGauge) {
    ui::gaugeDisplayRefresh();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Plane Radar");

  bootButtonInit();
  displayInit();
  if (wifiShowsSetupScreenOnBoot()) {
    statusScreenPortal();
  }
  services::location::init();
  ui::radar::rangeInit();

  if (wifiSetupConnect()) {
    drawCurrentMode();
  }
}

void loop() {
  handleBootButton();

  if (!wifiUp()) {
    if (g_screen_visible) {
      Serial.println("WiFi lost — will reconnect");
      g_screen_visible = false;
    }

    if (g_wifi_down_since == 0) {
      g_wifi_down_since = millis();
    }

    const unsigned long down_ms = millis() - g_wifi_down_since;
    if (down_ms >= config::kWifiDownGraceMs &&
        millis() - g_last_reconnect_ms >= config::kWifiReconnectIntervalMs) {
      g_last_reconnect_ms = millis();
      if (wifiReconnect()) {
        g_wifi_down_since = 0;
        drawCurrentMode();
      }
    }
  } else {
    g_wifi_down_since = 0;

    if (!g_screen_visible) {
      drawCurrentMode();
    } else if (g_mode == DisplayMode::kRadar) {
      if (millis() - g_last_adsb_fetch_ms >= config::kAdsbFetchIntervalMs) {
        g_last_adsb_fetch_ms = millis();
        fetchAndDrawAircraft();
      }
    } else {
      if (millis() - g_last_gauge_fetch_ms >= config::kGaugeFetchIntervalMs) {
        g_last_gauge_fetch_ms = millis();
        fetchAndRefreshGauge();
      }
    }
  }

  delay(10);
}
