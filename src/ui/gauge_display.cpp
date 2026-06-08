#include "ui/gauge_display.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <cmath>
#include <cstdio>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "services/usgs_client.h"

namespace fonts = lgfx::v1::fonts;

namespace {

// Layout
constexpr int kCx = config::kDisplayWidth / 2;
constexpr int kCy = config::kDisplayHeight / 2;

// Sparkline arc: 270° sweep starting from the bottom-left, going clockwise.
// Leaves a gap at the bottom so the arc doesn't cover the label area.
constexpr float kArcStartDeg = 135.0f;   // bottom-left
constexpr float kArcSweepDeg = 270.0f;   // clockwise to bottom-right
constexpr int   kArcRadius   = 112;
constexpr int   kArcThick    = 5;

// Colors (RGB, applied via tft.color565)
constexpr uint8_t kBgR = 4,   kBgG = 10,  kBgB = 28;    // same navy as radar
constexpr uint8_t kArcLoR = 16, kArcLoG = 60, kArcLoB = 120;  // dim blue (no data / low)
constexpr uint8_t kArcHiR = 0,  kArcHiG = 180, kArcHiB = 255; // bright cyan (high flow)
constexpr uint8_t kCfsR = 255, kCfsG = 255, kCfsB = 255;
constexpr uint8_t kLabelR = 140, kLabelG = 200, kLabelB = 255;
constexpr uint8_t kUnitR  = 80,  kUnitG  = 130, kUnitB  = 200;
constexpr uint8_t kNoDataR = 80, kNoDataG = 80, kNoDataB = 80;

constexpr float kDegToRad = 0.01745329252f;

uint16_t colorBg()     { return tft.color565(kBgR,    kBgG,    kBgB); }
uint16_t colorCfs()    { return tft.color565(kCfsR,   kCfsG,   kCfsB); }
uint16_t colorLabel()  { return tft.color565(kLabelR, kLabelG, kLabelB); }
uint16_t colorUnit()   { return tft.color565(kUnitR,  kUnitG,  kUnitB); }
uint16_t colorNoData() { return tft.color565(kNoDataR, kNoDataG, kNoDataB); }

// Lerp between dim-blue and bright-cyan based on 0–1 fraction of sparkline max.
uint16_t arcColor(float frac) {
  if (frac < 0.0f) frac = 0.0f;
  if (frac > 1.0f) frac = 1.0f;
  const uint8_t r = static_cast<uint8_t>(kArcLoR + frac * (kArcHiR - kArcLoR));
  const uint8_t g = static_cast<uint8_t>(kArcLoG + frac * (kArcHiG - kArcLoG));
  const uint8_t b = static_cast<uint8_t>(kArcLoB + frac * (kArcHiB - kArcLoB));
  return tft.color565(r, g, b);
}

void drawArcSegment(float angle_deg, float frac) {
  const float rad = angle_deg * kDegToRad;
  const float sin_a = sinf(rad);
  const float cos_a = cosf(rad);

  const uint16_t col = arcColor(frac);
  for (int t = 0; t < kArcThick; ++t) {
    const int r = kArcRadius - t;
    const int x = kCx + static_cast<int>(lroundf(sin_a * r));
    const int y = kCy - static_cast<int>(lroundf(cos_a * r));
    tft.drawPixel(x, y, col);
  }
}

void drawSparklineArc() {
  const size_t n = services::usgs::historyCount();

  // Find min/max CFS across history for normalisation.
  float hi = 0.0f;
  float lo = 1e9f;
  const services::usgs::GaugeReading* hist = services::usgs::history();
  for (size_t i = 0; i < n; ++i) {
    if (hist[i].cfs < 0.0f) continue;
    if (hist[i].cfs > hi) hi = hist[i].cfs;
    if (hist[i].cfs < lo) lo = hist[i].cfs;
  }
  const float range = (hi > lo) ? (hi - lo) : 1.0f;

  // Draw: each history reading maps to one angular segment of the arc.
  // We spread n points across kArcSweepDeg.
  const float deg_per_step = (n > 1) ? kArcSweepDeg / static_cast<float>(n - 1) : 0.0f;
  for (size_t i = 0; i < n; ++i) {
    const float angle = kArcStartDeg + static_cast<float>(i) * deg_per_step;
    // Convert from "0=up, CW" compass to standard math: add 90
    // Actually our draw convention: angle 0 = up (north), positive = CW.
    // drawArcSegment uses sin/cos with north-up convention already.
    const float frac = (hist[i].cfs >= 0.0f) ? (hist[i].cfs - lo) / range : 0.0f;
    drawArcSegment(angle, frac);
  }

  // If no history yet, draw a dim placeholder ring.
  if (n == 0) {
    for (int deg = 0; deg < static_cast<int>(kArcSweepDeg); ++deg) {
      drawArcSegment(kArcStartDeg + static_cast<float>(deg), 0.0f);
    }
  }
}

void applyLargeStyle() {
  if (displayFontIsSmooth()) {
    displayFontSetSmoothSize(tft, 1.4f);
  } else {
    displayFontSetBitmap(tft, &fonts::FreeSansBold18pt7b);
  }
}

void applyMediumStyle() {
  if (displayFontIsSmooth()) {
    displayFontSetSmoothSize(tft, 0.9f);
  } else {
    displayFontSetBitmap(tft, &fonts::FreeSansBold12pt7b);
  }
}

void applySmallStyle() {
  if (displayFontIsSmooth()) {
    displayFontSetSmoothSize(tft, 0.72f);
  } else {
    displayFontSetBitmap(tft, &fonts::FreeSans9pt7b);
  }
}

void drawValues() {
  const services::usgs::GaugeReading& r = services::usgs::latest();
  const uint16_t bg = colorBg();

  tft.setTextDatum(textdatum_t::middle_center);

  // CFS — large, white, center of display.
  applyLargeStyle();
  const int cfs_h = tft.fontHeight();

  char cfs_buf[16];
  if (r.cfs >= 0.0f) {
    if (r.cfs >= 10000.0f) {
      snprintf(cfs_buf, sizeof(cfs_buf), "%.0fk", r.cfs / 1000.0f);
    } else {
      snprintf(cfs_buf, sizeof(cfs_buf), "%.0f", r.cfs);
    }
    tft.setTextColor(colorCfs(), bg);
  } else {
    strncpy(cfs_buf, "---", sizeof(cfs_buf));
    tft.setTextColor(colorNoData(), bg);
  }
  tft.drawString(cfs_buf, kCx, kCy - cfs_h / 4);

  // "CFS" unit label just below the number.
  applySmallStyle();
  tft.setTextColor(colorUnit(), bg);
  tft.drawString("CFS", kCx, kCy + cfs_h / 2);

  // Stage and water temp on the lower half.
  applyMediumStyle();
  const int med_h = tft.fontHeight();
  const int detail_y = kCy + cfs_h + med_h;

  char stage_buf[16];
  if (r.stage_ft >= 0.0f) {
    snprintf(stage_buf, sizeof(stage_buf), "%.2f ft", r.stage_ft);
    tft.setTextColor(colorLabel(), bg);
  } else {
    strncpy(stage_buf, "-- ft", sizeof(stage_buf));
    tft.setTextColor(colorNoData(), bg);
  }
  tft.drawString(stage_buf, kCx, detail_y);

  applySmallStyle();
  char temp_buf[16];
  if (r.temp_c >= 0.0f) {
    const float temp_f = r.temp_c * 9.0f / 5.0f + 32.0f;
    snprintf(temp_buf, sizeof(temp_buf), "%.1f\xB0""F", temp_f);
    tft.setTextColor(colorLabel(), bg);
  } else {
    strncpy(temp_buf, "-- \xB0""F", sizeof(temp_buf));
    tft.setTextColor(colorNoData(), bg);
  }
  tft.drawString(temp_buf, kCx, detail_y + med_h + 4);
}

void drawBackground() {
  tft.fillScreen(colorBg());
  drawSparklineArc();
}

}  // namespace

namespace ui {

void gaugeDisplayDraw() {
  displayFontEnsureLoaded(tft);
  drawBackground();
  drawValues();
  tft.setTextDatum(textdatum_t::top_left);
}

void gaugeDisplayRefresh() {
  // Clear value area (center circle), redraw values and arc.
  tft.fillCircle(kCx, kCy, kArcRadius - kArcThick - 1, colorBg());
  drawSparklineArc();
  drawValues();
  tft.setTextDatum(textdatum_t::top_left);
}

}  // namespace ui
