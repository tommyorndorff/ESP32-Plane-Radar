#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Pure, dependency-free math and parsing utilities.
// No Arduino/ESP32/LovyanGFX headers — compiles on native host for testing.

namespace radar_math {

constexpr float kKmPerDeg = 111.0f;
constexpr float kKmPerMile = 1.609344f;
constexpr float kDegToRad = 0.01745329252f;

// Aircraft position relative to radar center, in km.
// Applies cos(lat) correction so east/west distances are accurate at non-equatorial latitudes.
inline void offsetKm(double center_lat, double center_lon,
                     float lat, float lon,
                     float* dx_km, float* dy_km, float* dist_km) {
  const float cos_lat = cosf(static_cast<float>(center_lat) * kDegToRad);
  *dx_km = static_cast<float>(lon - center_lon) * kKmPerDeg * cos_lat;
  *dy_km = static_cast<float>(lat - center_lat) * kKmPerDeg;
  *dist_km = sqrtf((*dx_km) * (*dx_km) + (*dy_km) * (*dy_km));
}

// WiFiManager checkbox: submits value= attribute ("T") when checked, "" when unchecked.
inline bool portalCheckboxChecked(const char* value) {
  if (value == nullptr || value[0] == '\0') {
    return false;
  }
  if (value[1] == '\0') {
    return value[0] == 'T' || value[0] == 't';
  }
  return strcmp(value, "on") == 0;
}

inline bool validLatLon(double lat, double lon) {
  return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

inline bool parseCoord(const char* text, double* out) {
  if (text == nullptr || text[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  const double v = strtod(text, &end);
  if (end == text || (end != nullptr && *end != '\0')) {
    return false;
  }
  *out = v;
  return true;
}

inline void formatRing3Label(char* buf, size_t len, float ring3_km, bool use_miles) {
  if (use_miles) {
    const int mi = static_cast<int>(lroundf(ring3_km / kKmPerMile));
    snprintf(buf, len, "%dmi", mi);
  } else {
    const int km = static_cast<int>(lroundf(ring3_km));
    snprintf(buf, len, "%dkm", km);
  }
}

}  // namespace radar_math
